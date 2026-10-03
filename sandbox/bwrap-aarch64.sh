#!/bin/bash
#
# bwrap into the aarch64 SDK rootfs (sdk/) using qemu-user + bubblewrap.
#
#   ./bwrap-aarch64.sh                          # interactive bash in the SDK
#   ./bwrap-aarch64.sh ls -l /usr/bin           # run a command in the SDK
#   ./bwrap-aarch64.sh -- bash -c 'zypper lr'   # "--" is accepted and ignored
#
# Environment knobs (all optional):
#
#   SDK=<dir>     SDK rootfs to enter        (default: <this dir>/sdk)
#   QEMU=<file>   qemu-aarch64-static        (default: <this dir>/qemu-aarch64-static)
#   CONF=<file>   binfmt registration line   (default: <this dir>/qemu-aarch64-static.conf)
#   NET=1         keep the host network instead of --unshare-net (for zypper/rpm)
#   RO=1          mount the SDK read-only
#   WORK=<dir>    bind a host directory to /work (read-write)
#   WORKTP=<dir>  where to mount WORK inside (default: /work; /mnt already
#                 exists in the SDK, so it creates nothing in sdk/)
#   CHDIR=<dir>   initial directory inside the SDK (default: /root)
#   CMD='<cmd>'   command line to run through /bin/sh -c
#
# How it works
#
#   1. `unshare --user --map-root-user --mount` gives us CAP_SYS_ADMIN and a
#      private mount namespace without being real root.
#   2. Inside it binfmt_misc is mounted and qemu-aarch64-static is registered
#      from qemu-aarch64-static.conf; the interpreter field of the conf is
#      rewritten to the real path of the binary. The "P" flag makes the kernel
#      open the interpreter at registration time, so qemu does not have to
#      exist inside the sandbox. Entries registered in a parent user namespace
#      stay visible in the child user namespace created by bwrap, which is
#      what makes exec() of aarch64 binaries - shebangs, child processes,
#      compilers, shells - work inside the sandbox.
#   3. bwrap pivots sdk/ to /, with a fresh /proc and /dev, uid/gid 0 and
#      CAP_SYS_ADMIN (so `mount` works inside too), and execs the command,
#      now transparently translated by qemu.
#
# Fallback without binfmt (one binary, no child processes):
#
#   ./qemu-aarch64-static -L sdk sdk/bin/uname -m
#
set -euo pipefail

SELF=$(readlink -f "$0")
DIR=$(dirname "$SELF")

SDK=${SDK:-$DIR/sdk}
QEMU=${QEMU:-$DIR/qemu-aarch64-static}
CONF=${CONF:-$DIR/qemu-aarch64-static.conf}
NET=${NET:-0}
RO=${RO:-0}
WORK=${WORK:-}
WORKTP=${WORKTP:-/work}
CHDIR=${CHDIR:-/root}
CMD=${CMD:-}

# ---------------------------------------------------------------- stage 2 ----
# Here we are root in a private user+mount namespace: register qemu, bwrap.
if [[ ${1:-} == @binfmt ]]; then
	declare -a usercmd=("${@:2}")

	reg=/proc/sys/fs/binfmt_misc/register
	# The host may already have binfmt_misc mounted READ-ONLY (systemd does
	# exactly that), in which case "-e register" is true but writing fails.
	# So only skip the mount when the existing register is really writable,
	# otherwise mount our own read-write instance on top - we are root in a
	# private mount namespace, the host mount table is not affected.
	if [[ ! -w $reg ]]; then
		if ! mount -t binfmt_misc none /proc/sys/fs/binfmt_misc; then
			echo "bwrap: cannot mount binfmt_misc; current state:" >&2
			grep binfmt /proc/mounts >&2 || true
			exit 1
		fi
	fi
	if [[ ! -w $reg ]]; then
		echo "bwrap: $reg is still not writable; current state:" >&2
		grep binfmt /proc/mounts >&2 || true
		exit 1
	fi

	# register the conf, pointing the interpreter at the real qemu binary
	awk -F: -v OFS=: -v q="$QEMU" '{ $7 = q; print }' "$CONF" \
		> /proc/sys/fs/binfmt_misc/register

	bwrap=(
		bwrap
		--clearenv
		--unshare-user --unshare-pid --unshare-uts --hostname aarch64-sdk
		--uid 0 --gid 0
		--cap-add CAP_SYS_ADMIN
		--die-with-parent
		--chdir "$CHDIR"
		--setenv HOME /root
		--setenv SHELL /bin/bash
		--setenv USER root --setenv LOGNAME root
		--setenv PATH /usr/bin:/bin:/usr/sbin:/sbin
		--setenv TERM "${TERM:-linux}"
		--setenv QEMU_LD_PREFIX /
	)
	[[ $NET == 1 ]] || bwrap+=(--unshare-net)

	if [[ $RO == 1 ]]; then bwrap+=(--ro-bind "$SDK" /); else bwrap+=(--bind "$SDK" /); fi
	bwrap+=(--dev /dev --proc /proc)

	if [[ -n $WORK ]]; then
		mkdir -p "$WORK"
		bwrap+=(--bind "$WORK" "$WORKTP")
	fi
	if [[ $NET == 1 && -r /etc/resolv.conf ]]; then
		bwrap+=(--ro-bind /etc/resolv.conf /etc/resolv.conf)
	fi

	# bwrap re-creates the current working directory inside the new root
	# (i.e. in sdk/), so move out of the way first: nothing gets created.
	cd /

	if [[ -n $CMD ]]; then
		exec "${bwrap[@]}" /bin/sh -c "$CMD"
	elif ((${#usercmd[@]} == 0)); then
		exec "${bwrap[@]}" /bin/bash -l
	else
		exec "${bwrap[@]}" "${usercmd[@]}"
	fi
fi

# ---------------------------------------------------------------- stage 1 ----
[[ -d $SDK ]]  || { echo "bwrap: no SDK rootfs at $SDK" >&2; exit 1; }
[[ -x $QEMU ]] || { echo "bwrap: no qemu binary at $QEMU" >&2; exit 1; }
[[ -r $CONF ]] || { echo "bwrap: no binfmt conf at $CONF" >&2; exit 1; }
command -v bwrap   >/dev/null || { echo "bwrap: bwrap not found" >&2; exit 1; }
command -v unshare >/dev/null || { echo "bwrap: unshare not found" >&2; exit 1; }

# drop "--" if the caller used it as a separator
[[ ${1:-} == -- ]] && shift

# re-exec ourselves as root in a private user+mount namespace
exec unshare --user --map-root-user --mount "$SELF" @binfmt "$@"
