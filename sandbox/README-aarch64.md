# Entering the aarch64 SDK (`sdk/`) with qemu-user

Two equivalent entry scripts, both tested:

| script | needs | style |
|--------|-------|-------|
| `bwrap-aarch64.sh` | `bwrap` + `unshare` | declarative: bwrap describes the sandbox |
| `unshare-aarch64.sh` | `unshare` only | imperative: hand-written mount plumbing |

Also here: `bench-cpu.sh` (qemu CPU-model benchmark, see Performance),
`cross-build-plan.md` (design notes for running the compiler natively - not
implemented) and `HANDOFF.md` (state, environment facts, gotchas, open
threads - read this first if you are continuing the work).

```sh
./bwrap-aarch64.sh                        # interactive bash inside the SDK
./bwrap-aarch64.sh uname -m               # -> aarch64
./bwrap-aarch64.sh -- bash -c 'zypper lr'
echo 'uname -m' | ./bwrap-aarch64.sh      # stdin goes to `bash -l`
```

Inside you are `root` in an **sdk aarch64** rootfs, `sdk/` is `/`
(read-write; files you create are owned by your host uid), `/proc`, `/dev`,
`/dev/pts` (fresh instance), `/dev/shm` are set up, `mount(8)` works, and
every `exec()` of an aarch64 binary is translated by `qemu-aarch64-static`.

## Knobs (same for both scripts)

| var | default | meaning |
|-----|---------|---------|
| `SDK`  | `./sdk`                     | rootfs to enter |
| `QEMU` | `./qemu-aarch64-static`     | emulator binary |
| `CONF` | `./qemu-aarch64-static.conf`| binfmt registration line |
| `NET=1` | off                      | keep host network (for `zypper`/`rpm`) + bind host `resolv.conf` |
| `RO=1`  | off                      | mount the SDK read-only |
| `WORK=<dir>` | unset               | bind a host dir to `/work` (rw) |
| `WORKTP=<dir>` | `/work` (bwrap only)| where `WORK` is mounted; `/mnt` already exists in the SDK |
| `QEMU_CPU=<model>` | qemu default (`max`) | exported into the sandbox; `cortex-a72` is 20-30% faster for scalar code |
| `CHDIR=<dir>` | `/root` (bwrap only)| initial cwd inside |
| `CMD='<cmd>'` | unset (bwrap only)  | run through `/bin/sh -c` |

```sh
WORK=~/proj ./bwrap-aarch64.sh bash -c 'cd /work && make'
NET=1 ./bwrap-aarch64.sh zypper in -y gcc
RO=1 ./bwrap-aarch64.sh /usr/bin/pkg-config --list-all
```

## How it works

1. **`unshare --user --map-root-user --mount`** — a private user+mount
   namespace where we are root with `CAP_SYS_ADMIN`, no real root needed.
2. **binfmt_misc** — mount it and write `qemu-aarch64-static.conf` into
   `register`, with the interpreter field rewritten from
   `/usr/bin/qemu-aarch64-static` (does not exist on the host) to the real
   path of our qemu binary.
   * `P` = the kernel opens the interpreter at registration time, so qemu
     does not have to be reachable from inside the sandbox;
   * entries registered in a *parent* user namespace stay usable in the
     *child* user namespace bwrap creates.
3. **the sandbox** — bwrap pivots `sdk/` to `/` (`unshare-aarch64.sh` does
   `mount --bind` + `pivot_root` + `umount -l` by hand), fresh `/proc`,
   minimal `/dev`, uid/gid 0.

## Things that bite

* **binfmt_misc must be mounted inside the new root.** In
  `unshare-aarch64.sh` it is mounted at `$sdk/proc/sys/fs/binfmt_misc`
  *before* `pivot_root`. If you mount the plain host `/proc/sys/fs/binfmt_misc`
  instead, the lazy umount of the old root kills the binfmt_misc superblock
  and with it the qemu registration — every exec then fails with
  `Exec format error`. (bwrap is immune: it unmounts the old root in a
  *child* mount namespace, the parent's binfmt_misc mount stays alive.)
* **`mknod` is EPERM in a user namespace here**, so a minimal `/dev` has to be
  built by bind-mounting `/dev/null`, `/dev/zero`, `/dev/tty`, ... from the
  host (that is what bwrap's `--dev` does internally). Binding all of
  `/dev` instead would expose every host device.
* **bwrap re-creates the host cwd inside the new root**, i.e. it would litter
  `sdk/z/env/...`; `bwrap-aarch64.sh` does `cd /` first.
* `WORK` with the default target `/work` makes bwrap create the empty
  mountpoint `sdk/work/`. `WORKTP=/mnt` (already exists) leaves no footprint.
* `NET=1` bind-mounts the host `resolv.conf`, creating `sdk/etc/resolv.conf`.
  This host currently has no route, so `NET=1` still cannot reach the internet.
* `/sys` is not mounted, `/etc/machine-id` is empty in this SDK.
* Emulation is TCG — builds are slow.

## bwrap vs unshare-only

`unshare` alone is enough (util-linux >= 2.40 even has `--load-interp`,
`--root`, `--wd`, `--kill-child`), and `unshare-aarch64.sh` is ~25% shorter.
What bwrap still buys you:

* `--dev` — private minimal `/dev` + a **new devpts instance** (`ptmxmode=666`),
  instead of hand-rolled binds;
* `--clearenv` + explicit `--setenv` instead of `env -i ...`;
* `--ro-bind` / `--bind` for the SDK and `WORK` as plain options, so nothing
  has to be mounted before the pivot (which is why `RO=1` needs the
  `put_old` dir to be created before the read-only remount in the unshare
  version);
* `--die-with-parent`, `--unshare-uts --hostname`, `--cap-add`;
* no `pivot_root`/`umount -l` recipe, no `--propagation private` footgun
  (forget it and your mounts propagate to the host mount table).

Also note `unshare --load-interp` takes the **registration line itself**, not
a file — passing a filename writes the filename into `register` and fails
with `EINVAL`.

## Performance

Measured on this host (8-core x86_64, `qemu-aarch64-static` 11.1.1, TCG) with
`./bench-cpu.sh`; native column is the same workload running as x86_64:

| workload | native | default (`-cpu max`) | `QEMU_CPU=cortex-a72` | slowdown |
|---|---|---|---|---|
| python int loop 4M   | 0.36 s | 7.25 s | 5.64 s | 20x -> 16x |
| python float loop 3M |  -     | 6.87 s | 4.96 s | 21x -> 15x |
| bash arith loop 400k | 0.93 s | 16.80 s | 14.17 s | 18x -> 15x |
| python startup x50   |  -     | 11.03 s (220 ms each) | 10.18 s | - |
| `/bin/true` x100     | 0.03 s (0.3 ms) | 1.40 s (14 ms) | 1.39 s | ~45x |
| sha256 96 MB         | 0.19 s | 2.36 s | 2.34 s | 12x |
| gzip 24 MB           | 0.015 s | 0.45 s | 0.46 s | 30x |
| openssl aes-256-cbc  |  -     | 1.067 s | 1.066 s | identical |

* Rule of thumb: **15-30x slower** than native, and **~14 ms per `exec()`**
  (vs 0.3 ms native). Anything that spawns many processes - `configure`,
  `make`, `rpmbuild`, meson/ninja, `zypper` scripts - is dominated by qemu
  startup, not by TCG. Batch work into fewer, bigger processes.
* **`QEMU_CPU=max,pauth=off,sve=off` does not help this SDK** (all deltas
  within noise). Reasons:
  * the SDK is not built with `-mbranch-protection`: `objdump -d` finds
    **0** `pacia/autia/xpaci` in `bash`, `libc`, `ld-linux`, `libstdc++`,
    `python3.8`, `cmake`, `rpm`, and no `NT_GNU_PROPERTY` note - so
    `pauth=off` has nothing to skip;
  * `sve=off` really does change what the guest sees
    (`AT_HWCAP: b01feffffffb -> b01fefbffffb`), but no SDK binary contains
    SVE instructions and the glibc hot paths here are not SVE-bound.
* **What does help: a narrower CPU model.** `cortex-a72`, `cortex-a53` or
  `neoverse-n1` give **20-30%** on scalar/FP interpreter code and ~8% on
  python startup; sha256/gzip/openssl are unchanged (they run through the
  same SHA/AES extension paths). The default `max` advertises the whole
  ARMv9 kitchen sink (SVE/SVE2/SME/MTE/PAuth/BTI/BF16/MOPS/...) and TCG pays
  for it even on ordinary code; `max,sve=off,sme=off` recovers only ~5%, and
  `max-arm-cpu` exposes just `sve`, `sme`, `pauth`, `pmu` as properties, so
  the rest cannot be switched off individually.
* **Safety of a narrow model:** the SDK is baseline ARMv8-A - no LSE atomics,
  no FP16, no SVE, no PAC in libc/ld/libstdc++/python/cmake/rpm - so a v8.0
  core is architecturally sufficient. Verified under `cortex-a72` and
  `neoverse-n1`: `cmake`, `rpm`, `zypper`, `pkg-config`, `python3` with
  `ssl/sqlite3/hashlib/zlib/json/ctypes`, `openssl speed`, `ldd` all pass.
* **Do not use `sve=off` alone.** With SME still enabled (`max,sve=off`) the
  guest SIGSEGVs and can hang (SME requires SVE). Either keep both on or use
  a real core model.
* Other knobs in this build: `QEMU_TB_SIZE` (`-tb-size`),
  `QEMU_RESERVED_VA` (`-R`), `-one-insn-per-tb`. `-accel` is **not** accepted
  by linux-user, so MTTCG cannot be toggled - it is already on, so parallel
  builds (`-j`) scale across processes.

```sh
QEMU_CPU=cortex-a72 ./unshare-aarch64.sh            # whole sandbox
./unshare-aarch64.sh bash -c 'export QEMU_CPU=cortex-a72; make -j8'   # children only
```

## Building inside the sandbox (design notes, not implemented)

Because compilation is the CPU-bound part of the 15-30x penalty, the biggest
remaining win is to **not compile under emulation**: make `gcc`/`g++`/`ld`
resolve to a native x86_64 cross-compiler and keep qemu only for steps that
must run aarch64 code (target-side code generators, test suites).

The enabling fact is already true here: binfmt registers **only aarch64**, so
any x86_64 ELF placed inside the sandbox runs natively (host `mount`, `umount`
and the host loader are already run this way during setup). Hijacking is then
just a matter of layer: variables (`CC/CXX`, cmake toolchain file, meson
cross-file, rpm `%__cc`) -> `PATH` shim -> `mount --bind` over
`/usr/bin/gcc` -> `LD_PRELOAD`/ptrace.

Current blockers are factual, not mechanical: the SDK ships **no compiler and
no `make`** and has no `/usr/include/c++`, while the host has `clang`
(cross-capable) but a single-target `gcc` and no `zig`/`ccache`/`distcc`.

See [cross-build-plan.md](cross-build-plan.md) for the inventory, the three
placement options (native toolchain inside the sandbox / host `--sysroot=$SDK`
/ hybrid), the validation checklist and the five questions that must be
answered before implementing any of it.

## Fallback without binfmt (one binary, no child processes)

```sh
./qemu-aarch64-static -L sdk sdk/bin/uname -m
```

Anything that forks and execs (`bash -c 'ls'`, `make`, `gcc`) fails with
`Exec format error` — that needs the binfmt route.

## Troubleshooting

| symptom | cause / fix |
|---------|-------------|
| `Exec format error` on exec | binfmt entry missing or dead — see "Things that bite" #1 |
| `register is not writable` | systemd mounts the host `binfmt_misc` **read-only**, so the file exists but cannot be written. The script now tests `-w` (not `-e`) and mounts its own read-write instance on top inside the private mount namespace; `unshare-aarch64.sh` was never affected because it always mounts its own instance at `$sdk/proc/sys/fs/binfmt_misc` |
| `cannot register ... in binfmt_misc` | `unshare` blocked (seccomp, policy denying userns) — stop and ask the operator |
| `unshare: mount /proc failed: Operation not permitted` | mounting procfs needs a new pid namespace; pass `--pid --fork` (or `--map-root-user`) |
| `mknod: Operation not permitted` | expected in a userns; bind-mount the device files instead |
