# AGENTS.md — working notes for this repo

Human-facing docs (options table, controls, build deps, architecture prose):
**README.md**. This file only adds what an agent needs to not break things.

## Verify before claiming done

```sh
make            # must stay warning-free
make test       # headless selftest; exit 0 = pass  <- the gate
```

There is no unit-test framework: `./imp2d --selftest` *is* the test suite
(physics invariants + `glReadPixels` pixel checks). Adding a feature usually
means adding a `check(...)` there.

To look at actual pixels (the only way to see rendering here):

```sh
./imp2d --mode off --size 800x600 --frames 120 --dump-ppm shot.ppm --quiet
magick convert shot.ppm shot.png     # then open the image
```

`make clean` removes objects, `imp2d`, `frame.ppm` and `selftest.ppm`.

Separately, `./imp2d --stress` (physics-only, no EGL/GL, ~6 s) settles a
2000-disk gravity pile and prints stability metrics (mean/max penetration,
residual & jitter velocity, |v_com| drift, depth profile, KE decay). Its
gates are regression bounds *tuned to the impulse solver* (see the comment
in `stress()`); change solver behaviour and the metric lines, not the
gates, are the comparison.

## The solver: impulse, and only impulse

`sim_step()` is the whole solver: integrate (+gravity) → `walls()` →
`grid_build()` → `ITER` relaxation passes of `collide_pairs()` (one-sided
normal impulse, Coulomb tangential impulse, Baumgarte positional correction
with `CORR_SLOP`) → `walls()` again. The pair impulses conserve momentum
exactly; only walls and gravity change it, and there is no damping of any
kind.

There is no `--solver` / `--compliance` flag, no `Sim.solver` field and no
xpbd code in this tree. Position-based dynamics lives in `../sim2d`, together
with its own hard-won notes (iterating the velocity sweeps, never deriving
velocity from position, sparse-gated restitution). Do **not** port it back
here: `imp2d` exists as the impulse baseline to compare that against — and
xpbd reads worse on this scene, which is few large balls on a billiard table,
not a dense jam of small disks.

What the impulse solver is *expected* to do, so nobody "fixes" it by
accident (`--stress`, 2000 disks r 2..8, 30 s, seed 7):

- the pile stays **alive**, not asleep: mean|v| ≈ 35 px/s, max|v| ≈ 111 px/s,
  jitter ≈ 124 px/s, mean-pen 0.94 px, 1.3 % KE residual. A gravity jam never
  fully settles here; that is the solver's signature, and `../sim2d` exists
  precisely because xpbd does settle (~61 px/s jitter, 0.48 px mean-pen).
- an elastic table (`--rest 1.0`, no gravity, no friction) keeps **100 %** of
  its energy, and a head-on hit conserves `p` exactly — both are selftest
  gates, so adding any drag or fudge factor fails `make test`.

## Environment quirks

- **The sandbox has no reachable X11 server.** `DISPLAY` is usually set (e.g.
  `:0.0`) but nothing is listening behind it: `/tmp/.X11-unix` is empty, TCP
  `127.0.0.1:6000` is refused and `XAUTHORITY` is unset, so the windowed path
  dies with `cannot connect to the X server (DISPLAY=...)`. A set `DISPLAY`
  therefore proves nothing — probe it, don't assume:

  ```sh
  timeout 5 ./imp2d --frames 3 --quiet   # exit 1 + "cannot connect" = no display
  ```

  Do **not** burn time trying to conjure a display: `X`/`Xorg`/`Xvfb`/`xvfb-run`
  are installed but starting one does not work here (the read-only `/tmp` stops
  it from creating its socket). Verify everything through `--mode off` instead;
  interactive keys (e.g. `n`) cannot be smoke-tested — cover them with a
  selftest check. The app already prints a `--mode off` hint on this failure,
  which is the intended behaviour; don't "fix" it.
- **`/tmp` is read-only.** Write scratch files in the repo (`*.ppm` is
  gitignored).
- `pkg-config --libs egl glesv2` does **not** emit `-lxcb`; ad-hoc compiles of
  the sources need `-lEGL -lGLESv2 -lxcb -lm` explicitly (the Makefile does).
- The Makefile has no `-MMD` dependency tracking; header edits may need
  `make clean`.

## Coupled invariants (these have bitten before)

- **Background is white** (`glClearColor` in `glr_init`). The selftest pixel
  checks assume it: background `> 200`, hover/ghost/aim-line colours are
  *dark* (hover darkens balls by `0.65×`, it does not brighten them). Change
  the clear colour → update the checks in `main.c`.
- **Grid overlay** (`n`, `draw_grid` in `gl_draw.c`): samples the physics grid
  state, cell size = `2 * max_r`, lines at multiples of `cell`. The selftest
  derives its sample row from `sim.cell` — don't hard-code a pixel.
- **`grid_alloc()` must leave `head[]` at `-1`.** The renderer reads the grid
  before the first `grid_build()` (frame 0, or while paused); uninitialised
  `head` draws garbage cell outlines.
- **`Renderer.vbo` is shared** between the ball pass and the line batcher
  (`lines_reserve` / `lines_push` / `lines_flush`). Always `lines_flush` before
  queueing something else, and reset `line_n` (flush does it).
- `glr_draw()` takes `show_grid` as its last argument; all call sites are in
  `main.c` (main loop + 3 in the selftest).
- **Ball vertex layout is 10 floats** (`corner2, center2, radius1, color3,
  alpha1, ang1` in `gl_draw.c::push_ball`); the FS draws antipodal spin
  "billiard dots" rotated by `ang` (rotated in the *vertex* shader — the FS
  is mediump). A **negative `ang` disables the marker** (used by the ghost
  ball); the dots are dark on white, which selftest 6c pixel-checks.
- **Spin state lives in `Ball`** (`ang, omega, I, inv_I`); `sim_kinetic()`
  includes the rotational `0.5·I·ω²` — every energy invariant must use it.
  `sim_add_random(..., spin)` only draws the spin RNG when `spin > 0`, so
  frictionless runs stay byte-identical to the pre-spin stream for a seed;
  `Sim.friction` defaults to 0 (frictionless) in `sim_init` — main sets it
  from `--mu`, selftest scenarios opt in explicitly.

## Constraints

- Plain C11, no new dependencies: keep it `egl glesv2 xcb` only — no SDL, no
  GLU, no Xlib in application code, no textures/fonts.
- `sim.c` / `sim.h` stay free of X and GL includes (physics is testable
  headless and must stay deterministic for a given seed).
- Compiler is `-Wall -Wextra -Wpedantic -Wshadow -Wmissing-prototypes`; new
  functions need prototypes (no implicit, no unused).
- Keep the fixed-timestep contract: 240 Hz substeps, `MAX_SUB` clamp, offscreen
  mode runs exactly 4 substeps/frame for determinism. No trig in `sim.c`
  (`ang` update is `+= omega*dt` plus an `fmodf` wrap); rotation belongs to
  the shader side.
