# sim2d

A 2D bouncing-ball simulator in plain C, using **XCB + EGL + OpenGL ES 2**
directly — no SDL, no GLU, no Xlib in application code, no textures.

## Model: bodies are made of particles

The only collision primitive is the **particle** — a disc with a radius. A
**body** is a rigid set of particles: each carries a fixed offset from the
body's centre of mass, and the body carries one position, velocity, angle and
angular velocity. Every contact is particle-against-particle or
particle-against-wall, so the narrow phase never grows: the several particle
pairs a body pair generates *are* its contact manifold, and the walls need no
corner or edge cases.

A plain billiard ball is the degenerate case — a body with one particle at
offset `(0,0)` — not a special case in the solver. Convex shapes (rectangles,
triangles, capsules) are hexagonal packings of touching particles; what that
costs is spelled out in `sim.h`: rounded outline of radius `r`, `2r` minimum
feature, a `0.268·r` gear-mesh between equal-radius lattices (which is why each
body gets its *own* lattice radius), and particles drawn with a 1.2x skin so the
hex holes close visually while the physics stays overlap-free.

Solved with **XPBD** (position projection, then iterated one-sided velocity
impulses). There used to be a second, sequential-impulse solver and a linear
drag term inside xpbd; both are deleted, both measurements are in `AGENTS.md`
— momentum is now conserved exactly (`0.000 %` drift in a head-on hit) and an
elastic billiard table keeps `100 %` of its energy over 10 s.

* Billiard-style physics: no gravity by default, bodies bounce off the four
  walls and each other (restitution `e = 0.9` by default).
* **Balls spin**: each ball carries an angular velocity `ω` and a disk
  inertia `I = 0.5·m·r²`; Coulomb friction `μ` (`--mu`, default 0.2, toggle
  with `f`) couples spin with translation at every contact, so bouncing
  balls grab the floor, roll and exchange spin. Rotation is shown by a pair
  of antipodal "billiard dots" rendered procedurally in the fragment shader
  (no textures).
* **`--mu 0` really means frictionless**: a contact can then only push along
  its normal, so `ω` is preserved *exactly* through any number of wall bounces
  and collisions. Torque comes from friction and from rolling resistance, and
  rolling resistance is driven by the *sustained* load a contact carries
  (`λ/dt`), never by the impact impulse — one bounce may only take the spin
  Coulomb grants it (`|Δω| ≤ μ(1+e)|v_n|·2/r`), while a ball rolling on the
  floor is braked to a stop.
* **Mass is proportional to radius²** (2D disks: `m = density · r²`);
  `--mass-exp 1|3` selects `m ∝ r` or `m ∝ r³` at runtime (`1/2/3` keys). For a
  packed body `r` is `reff = sqrt(sum r_particle²)`, and because a hex lattice
  cell of radius `r` covers `2√3·r²`, `reff²` is `0.289 × area` for *any* lattice
  radius — so `--pmin/--pmax` change how smooth a body looks, not what it weighs.
  (Generating the packing the "obvious" way — keeping only particles whose whole
  disc fits inside the shape — breaks this: it shaves off a ring one lattice
  radius wide and made the same nominal box 2.7x lighter when packed coarse.)
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
make            # -> ./sim2d
make test       # headless self-test (no X server needed)
make demo-ppm   # headless render -> frame.ppm
```

## Run

```sh
./sim2d                          # X11 window, 30 balls, 1024x768
./sim2d --balls 200 --rmin 4 --rmax 24 --rest 1.0
./sim2d --mode off --frames 240 --dump-ppm frame.ppm   # headless
./sim2d --selftest               # physics + rendering correctness, exit 0 = pass
```

### Options

| flag | default | meaning |
|---|---|---|
| `--mode x11\|off` | `x11` | on-screen window or offscreen FBO |
| `--size WxH` | `1024x768` | window / framebuffer size |
| `--balls N` | `30` | initial body count (max 2000) |
| `--rmin R --rmax R` | `8 48` | spawn radius range, px |
| `--rest E` | `0.9` | restitution (1.0 = perfectly elastic) |
| `--mu MU` | `0.2` | Coulomb friction for tangential impulses (0 = no spin coupling) |
| `--compliance A` | `0` | xpbd softness (0 = rigid projection; 1e-3..1e-2 softer/stabler) |
| `--substeps N` | `4` | substeps per frame in `--stress` (the live loop keeps its 16-substep catch-up budget) |
| `--mass-exp 1\|2\|3` | `2` | `m = density · r^exp` |
| `--shapes MODE` | `mixed` | `disc` / `box` / `tri` / `capsule` / `mixed` — shapes are hex packings of particles |
| `--pmin R --pmax R` | `3 8` | lattice particle radius, px: smaller = smoother outline, more particles, slower |
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

### XPBD solver

The one solver: predict → collect contacts (particle-particle, particle-wall)
→ Gauss–Seidel project the distance/plane constraints (Lagrange multipliers,
one-sided, optional compliance, applied *rigidly* through the contact lever
arm so off-centre particles both shift and turn a body) → velocity solved
separately with one-sided impulses swept to (near) convergence, a sleep
threshold (bounce only above 16 px/s, inelastic stop below), restitution
gated on how poorly supported *both bodies* are, and rolling resistance
proportional to the sustained support force `λ/dt` (impulses excluded).

Measured score (see the stress harness; 2000 disks, 30 s settle, seeds
7/42/12345/999):

| | value |
|---|---|
| mean penetration | 0.48–0.50 px |
| max penetration | 3.5–4.3 px |
| jitter (max\|v\|, final second) | 60.8–64.3 px/s |
| KE residual | 1.4–1.5 % |
| dense-bed stability | OK — bed jams and calms |
| **`--shapes mixed`** (400 bodies) mean-pen / jitter | 0.17–0.23 px / 19–24 px/s |
| **`--shapes mixed`** worst wall excursion | 0.4–1.6 px |

Flat-faced bodies stack better than disks: the mixed pile is about 3x more
stable on both penetration and jitter. Its one new artefact is *wall* excursion
— a body pinned between neighbours and a wall trades those contacts off and
leaves a particle up to ~1.6 px proud of the wall. It is bounded (same value at
30 s and 60 s settle), and disks are 0.0 px because a one-sided projection stops
exactly at contact.

The key to a calm dense bed is iterating the velocity-impulse sweep (8
passes): a single Gauss–Seidel sweep leaves residual approach velocity, which
becomes penetration, which keeps the bed collapsing — and a forever-collapsing
bed fed by gravity is exactly what used to make it boil (history in
AGENTS.md). The retired impulse solver measured 0.93 px mean penetration and
124 px/s jitter on the same scenario; the stress gates are now set well below
those numbers, so re-introducing a weaker contact model trips them.

### Stress harness

`./sim2d --stress` is a pure-physics (no GL) stability scenario: 2000
disks, radii 2–8 px, gravity on, a 30 s settle, then pile metrics —
interpenetration (mean/max), residual and jitter velocity, KE decay, pile
height. Fully deterministic per seed; ~6 s wall on llvmpipe. The printed
metrics are the numbers to compare when changing solver behaviour; the
`check(...)` gates are regression bounds with ~1.5x headroom on the current
baseline (mean-pen ≈ 0.49 px, jitter ≈ 62 px/s) — tighter than the retired
impulse solver could pass, on purpose. `--frames N` shortens the
settle; `--rmin/--rmax/--balls/--mu/--rest/--seed` all apply.

`--shapes mixed` (or any shape) also piles, with two adjustments so it measures
stability rather than something else: the body count drops to 400 and, unless
you pass sizes, the size range becomes 8–24 px — the stress default of 2–8 is a
*disk radius* range, and a 2 px "box" is one particle. Expect ~30 s wall for the
30 s settle (vs 13 s for disks) since cost is linear in *particle* count. It also
prints `walls: worst particle excursion X px`, the shape-mode artefact described
above.

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
| `b` | cycle the body shape used by the next spawn |
| `g` | toggle gravity (1000 px/s² down; billiards mode is off by default) |
| `f` | toggle friction (spin ↔ translation coupling at contacts) |
| `n` | show / hide the broad-phase neighbour grid |
| `i` | print stats (KE, momentum, collision counts) |
| `h` | help, `Esc` / `q` | quit |

## Layout

```
sim.h / sim.c                  physics only: bodies + particles, no X, no GL
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
  angle wrap, wall spin→tangential transfer, energy dissipation of the full
  spin+friction system, exact spin conservation at `μ=0`, the Coulomb bound on
  how much spin one bounce may take, and that rolling resistance brings a
  spinning ball on the floor to rest (it fails if `XPBD_ROLL` is deleted).
  Shape checks cover determinism of a *mixed* scene (bodies and particles
  compared byte-for-byte — this test used to spawn nothing and pass trivially),
  lattice self-overlap over 18 shapes/radii, mass independence of lattice
  resolution, and a tilted plank tipping over to rest flat on the floor — the
  check that a particle set really behaves as one rigid body.
* `--mode off` renders as fast as possible (no vsync): ~39000 fps @ 30 disks,
  ~1300 fps @ 30 mixed bodies (~600 particles), ~240 fps @ 100 mixed bodies on
  llvmpipe (best of 3); on real GPUs the windowed path is vsync-bound. Cost is
  linear in *particles*, so a shape scene costs its particle count, not its body
  count — 20 particles per body is a ~20x scene cost.
* Cost, physics only (`--stress`, 2000 disks, 30 s settle, best of 3): 4.7 s
  for the retired impulse solver, 8.2 s for pre-body xpbd, 13.7 s now — the
  body/particle split is **1.67x** on top of xpbd. A `Body` is 100 B vs the
  old `Ball`'s 68 B (2000 of them leave L1D), particles are re-derived from
  the body transform at every contact evaluation, and `Sim.p` is synced twice
  per substep. In the GL-bound windowed path the same change is worth ~0
  (30 bodies) to 11 % (400) — measure with `--stress`, not with fps.
