# imp2d

A 2D bouncing-ball simulator in plain C, using **XCB + EGL + OpenGL ES 2**
directly — no SDL, no GLU, no Xlib in application code, no textures.

**One solver: sequential impulses** (plus Baumgarte-style positional
correction). It conserves momentum exactly in a head-on hit, keeps an elastic
billiard table at 100 % of its energy, and reads naturally on a modest number
of relatively large balls. The sibling `../sim2d` solves the same scene with
XPBD instead: calmer in dense gravity jams of small disks, but stiffer-looking
here. `imp2d` is the impulse reference kept for that comparison — the
`--solver impulse|xpbd` switch and the whole xpbd path are gone from it; the
xpbd measurements live in the git history and in `../sim2d/AGENTS.md`.

* Billiard-style physics: no gravity, balls bounce off the four walls and
  each other with impulse-based collisions (restitution `e = 0.9` by default).
* **Balls spin**: each ball carries an angular velocity `ω` and a disk
  inertia `I = 0.5·m·r²`; Coulomb friction `μ` (`--mu`, default 0.2, toggle
  with `f`) couples spin with translation at every contact, so bouncing
  balls grab the floor, roll and exchange spin. Rotation is shown by a pair
  of antipodal "billiard dots" rendered procedurally in the fragment shader
  (no textures).
* **Mass is proportional to radius²** (2D disks: `m = density · r²`);
  `--mass-exp 1|3` selects `m ∝ r` or `m ∝ r³` at runtime (`1/2/3` keys).
* Balls have random radii (default 8–48 px) and a mass-tinted colour ramp on a
  white background.
* Rendering: one indexed draw call for all balls; each ball is a screen-space
  quad whose fragment shader carves an anti-aliased disc with a signed
  distance function (`fwidth`-free, works on ES 2). Pure geometry, no images.
* Broad phase: uniform grid rebuilt per substep, half-neighbourhood scan
  (`n` draws it: cell lines plus an outline around every occupied cell).
* Fixed-timestep physics (240 Hz substeps) with an accumulator, decoupled
  from rendering.
* Offscreen mode (EGL surfaceless / Mesa llvmpipe) for headless CI:
  self-test + PPM screenshots.

## Build

Dependencies: a C11 compiler, `pkg-config`, and the dev packages for
`egl`, `glesv2`, `xcb` (e.g. `libegl-dev libgles2-mesa-dev libxcb1-dev`
on Debian/Ubuntu; `mesa libxcb` on Arch).

```sh
make            # -> ./imp2d
make test       # headless self-test (no X server needed)
make demo-ppm   # headless render -> frame.ppm
```

## Run

```sh
./imp2d                          # X11 window, 30 balls, 1024x768
./imp2d --balls 200 --rmin 4 --rmax 24 --rest 1.0
./imp2d --mode off --frames 240 --dump-ppm frame.ppm   # headless
./imp2d --selftest               # physics + rendering correctness, exit 0 = pass
```

### Options

| flag | default | meaning |
|---|---|---|
| `--mode x11\|off` | `x11` | on-screen window or offscreen FBO |
| `--size WxH` | `1024x768` | window / framebuffer size |
| `--balls N` | `30` | initial ball count (max 2000) |
| `--rmin R --rmax R` | `8 48` | spawn radius range, px |
| `--rest E` | `0.9` | restitution (1.0 = perfectly elastic) |
| `--mu MU` | `0.2` | Coulomb friction for tangential impulses (0 = no spin coupling) |
| `--substeps N` | `4` | substeps per frame in `--stress` (the live loop keeps its 16-substep catch-up budget) |
| `--mass-exp 1\|2\|3` | `2` | `m = density · r^exp` |
| `--gravity` | off | start with gravity on (1000 px/s²) |
| `--spin 0\|1` | `1` | spawn balls with random angular velocity |
| `--frames N` | 0 / 300 | auto-exit after N frames |
| `--seed S` | `12345` | deterministic spawns |
| `--dump-ppm FILE` | – | write a PPM (offscreen mode) |
| `--dump-frame N` | last | which frame to dump |
| `--selftest` | – | headless correctness test |
| `--stress` | – | physics-only pile stability harness (see below) |
| `--benchmark` | – | one-line fps summary |
| `--quiet` | – | no stats output |

There is no `--solver` switch: this build only knows the impulse solver.

### Stress harness

`./imp2d --stress` is a pure-physics (no GL) stability scenario: 2000
disks, radii 2–8 px, gravity on, a 30 s settle, then pile metrics —
interpenetration (mean/max), residual and jitter velocity, KE decay, pile
height. Fully deterministic per seed; ~6 s wall on llvmpipe. The printed
metrics are the numbers to compare when changing solver behaviour; the
`check(...)` gates are regression bounds around the impulse solver's
measured baseline (seeds 7/42/12345/999: mean-pen ≈ 0.93 px, max-pen < 7 px,
max|v| < 130 px/s, jitter ≈ 110–145 px/s). `--frames N` shortens the
settle; `--rmin/--rmax/--balls/--mu/--rest/--seed` all apply.

### Controls

| key / mouse | action |
|---|---|
| left-drag | slingshot a new ball (ghost preview + aim line) |
| right-click | remove the ball under the cursor |
| wheel | radius of the next spawned ball |
| `space` | pause / resume |
| `r` / `c` | reset scene / clear all |
| `+` / `-` | add / remove a ball |
| `1` `2` `3` | mass ∝ r¹, r², r³ |
| `g` | toggle gravity (1000 px/s² down; billiards mode is off by default) |
| `f` | toggle friction (spin ↔ translation coupling at contacts) |
| `n` | show / hide the broad-phase neighbour grid |
| `i` | print stats (KE, momentum, collision counts) |
| `h` | help, `Esc` / `q` | quit |

## Layout

```
sim.h / sim.c                  physics only: no X, no GL
platform.h / platform_x11.c    xcb window + EGL bootstrap (+ surfaceless)
gl_draw.h / gl_draw.c          OpenGL ES 2 renderer (SDF discs, lines, PPM)
main.c                         options, fixed-step loop, events, selftest
```

## Notes

* The EGL display is created with `eglGetPlatformDisplay(EGL_PLATFORM_XCB_EXT,
  xcb_connection, NULL)`; `EGL_NO_X11` keeps Xlib headers out. A plain
  `eglGetDisplay()` fallback covers drivers without `EGL_EXT_platform_xcb`.
* Offscreen mode uses `EGL_MESA_platform_surfaceless` (Mesa) and falls back
  to a pbuffer; that is what `make test` exercises — no X server required.
* The self-test verifies: containment, finite state, speed cap, collision
  activity, energy non-increase (e ≤ 1), exact momentum conservation in a
  head-on hit, the mass law, determinism for equal seeds, and pixel-level
  correctness of the SDF disc rendering, of the rotating spin marker and of
  the neighbour-grid overlay via `glReadPixels`; spin-specific checks cover
  angle wrap, wall spin→tangential transfer and energy dissipation of the
  full spin+friction system.
* `--mode off` renders as fast as possible (no vsync): ~1900 fps @ 30 balls,
  ~1500 fps @ 400 balls on llvmpipe; on real GPUs the windowed path is
  vsync-bound.
