# AGENTS.md — working notes for this repo

Human-facing docs (options table, controls, build deps, architecture prose):
**README.md**. This file only adds what an agent needs to not break things.

## Verify before claiming done

```sh
make            # must stay warning-free
make test       # headless selftest; exit 0 = pass  <- the gate
```

There is no unit-test framework: `./sim2d --selftest` *is* the test suite
(physics invariants + `glReadPixels` pixel checks). Adding a feature usually
means adding a `check(...)` there.

A refactor that is supposed to preserve the physics has a stronger gate than
"the metrics look the same" — it must reproduce them **bit-for-bit** on the
no-spin, frictionless run, where no round-off path is involved:

```sh
./sim2d --stress --spin 0 --mu 0 --seed 7 --quiet   # compare against the previous revision
```

That is how the body/particle split was validated: 2000 disks x 7200 substeps,
mean-pen/jitter/KE/`|v_com|` and both contact counters identical to the old
binary, plus a per-substep state-hash probe over 2000 substeps. With `mu > 0`
spin gets generated and chaos amplifies 1-ulp differences, so there the gate
is metric-level (a few percent), not bitwise.

To look at actual pixels (the only way to see rendering here):

```sh
./sim2d --mode off --size 800x600 --frames 120 --dump-ppm shot.ppm --quiet
magick convert shot.ppm shot.png     # then open the image
```

`make clean` removes objects, `sim2d`, `frame.ppm` and `selftest.ppm`.

Separately, `./sim2d --stress` (physics-only, no EGL/GL, ~13 s at 30 s settle)
settles a 2000-disk gravity pile and prints stability metrics (mean/max
penetration, residual & jitter velocity, |v_com| drift, depth profile, KE decay,
worst wall excursion). Current disk baseline, seeds 7/42/12345/999: mean-pen
0.474-0.503 px, max-pen 3.2-5.7 px, max|v| and jitter 60.5-64.2 px/s, KE
residual 1.4-1.5%, wall excursion 0.00 px.
The gates carry ~1.5x headroom on those numbers and are deliberately tighter
than the retired impulse solver's were (it measured 0.93 px / 124 px/s and
would fail them). Change solver behaviour and the metric lines, not the gates,
are the comparison.

`--stress --shapes mixed` (auto-capped to 400 bodies at 8-24 px, ~30 s wall)
is a second scenario, not a re-tuning of the first: flat faces stack better
(mean-pen 0.17-0.23 px, jitter 19-24 px/s) but add wall excursion, 0.4-1.6 px
worst, which is why that gate is on the worst excursion (3 px) rather than on a
nonzero count. If you change the gates, say which scenario they belong to.

## The solver: xpbd, and only xpbd

`sim_step` is one XPBD pass sequence (predict → collect → project → restart
velocity → iterated impulses + friction + roll). The old sequential-impulse
solver is deleted; do not "re-add it as an option": two solvers meant two
regression matrices, and it was strictly worse at resting contact (0.93 px
mean-pen, 124 px/s jitter against 0.49 / 62). It is 2.3x cheaper per substep,
which is the only thing it still buys.

The boiling-bed fix: the velocity pass (step 5) is swept `XPBD_VITER`
= 8 times instead of once. One sweep is one Gauss-Seidel pass, so residual
approach velocity always survives at already-processed contacts; that
residue becomes penetration, the bed collapses forever (measured: centre of
mass sinking at a steady ~136 px/s), and gravity feeding that collapse is
what fluidized the pile. Iterating to (nearly) the fixed point "no contact
approaches" jams the bed and shuts the feed off — no warm-starting needed.
Cost: ~2x the single-sweep xpbd step (~2x realtime in `--stress`).
Diminishing returns are clean: 4→136, 8→61, 16→42 px/s jitter, cost
proportional.

Measured dead ends (all reproduced, numbers in git history):

- **Never derive velocity from position** (`(x-xp)/dt`) or from accumulated
  `lam`: unwinding many overlaps in one substep is unbounded → pile boils
  to the speed cap. Velocity restarts from `v_pre` (step 4) and only the
  sequential impulse pass touches it.
- **Reflecting restitution across a jammed contact graph is a Fermi
  accelerator** (opposing contacts re-bounce the same ball). Reflection is
  gated to *sparse* impacts (`nc[i]+nc[j] <= 4`); jammed contacts stop.
- **Restitution targets from stale pre-solve velocities ratchet** energy
  in dense graphs; use current sequential `vn`.
- Compliance stabilizes (monotonic: alpha 1e-2 quenches flow) but the bed
  becomes mush (height 40–85 px, mean-pen 2.4 px) — traded stability for
  softness, not a real fix.
- More substeps make the motor worse, not better (extra projection sweeps
  pump more momentum).
- ~~The real fix is persistent contact IDs + warm-started λ~~ — proven
  wrong: iterating the velocity sweeps to convergence fixed it; warm-start
  is optional polish, not required.

## Rolling resistance (`XPBD_ROLL`): needs friction and a sustained load

`particle_roll` is the only thing in the solver that removes spin, and until
it got two gates it removed spin on contact — the report "balls stop spinning
after colliding", true of xpbd the whole time it existed. Reference scenario
(spin 20, vx 300, walls, 2.5 s): old xpbd 0.2 rad/s (and `|v|` 300 -> 5.5, that
was drag); the old impulse solver 20.0 and 300.0. Both gates are selftested
(4e, 4f), and each fails only for its own defect:

- **`friction <= 0` means no torque.** A frictionless contact pushes along its
  normal, so nothing may write `omega` — the invariant is exact equality, and
  it holds: `cn == 0` for a disc, friction is 0, roll returns early.
- **The load is `lam/dt`, not `lam/dt + jr`.** An impact is not a sustained
  normal force; feeding the collision impulse in made every bounce take a
  rolling torque proportional to the hit, zeroing the spin in one bounce. The
  physical bound is Coulomb's, `|dw| <= mu*(1+e)*|vn|*2/r`, and the solver now
  lands exactly on it (20 -> 8.0 rad/s at r=20, vn=300, mu=0.2). Friction
  converting spin into slide during a hit is legitimate and stays.

What the term IS for: a ball dropped spinning onto a rough floor. With roll it
comes to a complete stop by 6 s; with `XPBD_ROLL 0` it locks at vx 133.3,
omega 6.67 and rolls forever (selftest 4g — it fails on deletion). In the
2000-disk pile it is worth jitter 64 vs 90 px/s and settled `max|omega|` 1.43
vs 6.80 rad/s. Two things it is *not* for: a ball bouncing between two rough
walls should lose its spin (the rolling condition has the opposite sign at each
wall — correct physics, don't chase it), and it is not a substitute for missing
tangential dissipation.

## XPBD_DRAG: deleted, do not re-add

There used to be a `XPBD_DRAG = 0.4 1/s` linear velocity+spin bleed applied
every substep, documented as what keeps a pile from churning. It is not. At
the 30 s settle it changed nothing that `XPBD_ROLL` does not already do
(jitter 63.1 px/s without it vs 88.4 with, mean-pen 0.506 vs 0.501), while
destroying the two properties worth having:

| | with drag | without |
|---|---|---|
| momentum drift, head-on hit, 0.5 s | **+18.1 %** | 0.000 % |
| KE kept, elastic billiards, 10 s | **0.0 %** (mean\|v\| 300→5 px/s) | 100 % (300→296) |

Rolling resistance is what quiets the bed. If something starts churning
again, find the missing dissipation instead of restoring a global bleed —
it silently voids the momentum-conservation selftest.

## Shape lattices (box / tri / capsule)

Rules the generators in `sim.c` follow, each learned by measurement:

- **Keep a lattice point whose CENTRE is inside the nominal shape.** Do NOT
  switch to "the whole disc must fit" because it looks tidier (bounding stays
  inside nominal): it shaves a ring one lattice radius off the packing, so
  `sum(r²)` - hence mass - drifts with resolution. Measured `reff²/A_nominal` on
  a 120x60 box: 0.240 at pr 2 down to 0.100 at pr 12, i.e. the same nominal box
  weighed **2.7x less** when packed coarse. With centres inside, one lattice cell
  covers `2√3·r²` and `reff² = 0.289·A_nominal` for any r and any convex shape,
  so `--pmin/--pmax` is a fidelity knob and not a mass knob. Selftest 5c guards
  this (spread over pr 4..10 must stay under 1.2x; erosion gives 1.6-2.7x).
- **Consequence of that rule:** the outline straddles the nominal boundary (up
  to `pr` proud of it), so any placement bound must add `pr` — see
  `shape_nominal_bound`, which folds in the `0.2·sz` cap as well.
- **`SIM_MAX_PARTS_PER_BODY` must stay generous (256).** `add_lattice` responds
  to "too many particles" by coarsening rather than refusing, so a small cap
  silently repacks every big body at a coarser lattice and moves its mass by
  tens of percent. The spawner's own ceilings normally produce ~20 particles per
  body, so 256 only catches pathological requests.
- **Lattice radius ceilings, not just a floor:** `sim_add_sized` caps pr at
  `0.2·sz` and at `0.25·min_feature`. Past ~0.25 of the thinnest feature a
  packing *degenerates* - a triangle at pr = 0.3·sz comes out as a single
  particle. Measured coverage of the nominal shape is 0.55-0.75 at these
  resolutions; 0.907 is the infinite-packing limit only.
- **`sim_add_body` must double the particle buffer until the body fits.**
  A single doubling is enough for discs (`np == 1`) and for nothing else: with a
  small `p_cap` every composite body silently returned -1. Symptom was "shapes
  never spawn", no error.
- **Cost is linear in particles, not bodies** (measured: 1250/1672/4618 particles
  → 1081/830/284 fps). A body pair whose particle sets overlap generates `pA·pB`
  *candidate* pairs, most rejected by the same-body test; that is not yet the
  bottleneck, so if shapes ever get slow, the fix is a body-AABB pre-filter in
  the broad phase rather than a cleverer narrow phase.

## Environment quirks

- **The sandbox has no reachable X11 server.** `DISPLAY` is usually set (e.g.
  `:0.0`) but nothing is listening behind it: `/tmp/.X11-unix` is empty, TCP
  `127.0.0.1:6000` is refused and `XAUTHORITY` is unset, so the windowed path
  dies with `cannot connect to the X server (DISPLAY=...)`. A set `DISPLAY`
  therefore proves nothing — probe it, don't assume:

  ```sh
  timeout 5 ./sim2d --frames 3 --quiet   # exit 1 + "cannot connect" = no display
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
- **fps numbers on this box are noisy** — single runs of the same scenario
  have differed by 10x. Benchmark with best-of-5 (`--benchmark`), never
  conclude anything from one run; 30 s `--stress` wall time is stable enough
  (~13 s) to use as a cost signal.

## Coupled invariants (these have bitten before)

- **`Particle.x/y` is a snapshot, never the truth while bodies move.** A
  projection moves a body several times inside one Gauss-Seidel sweep, and its
  particles only follow at a `sync_particles()`. Contact distances and lever
  arms must go through `part_world()` / `part_off()`; reading `p->x` in
  `project()` was the first bug here and it under-solved exactly the loaded
  contacts (mean-pen 0.94 px instead of 0.51, bodies pushed outside the box).
  `Sim.p` is current *outside* `sim_step` — that is all the snapshot promises.
- **Write the lever arms as `cross(offset, n)` and `cross(offset, t) ± r`,**
  not as `cross(contact_point, n)`. Algebraically the same; numerically it is
  what makes a one-particle body's normal lever exactly `0` and its friction
  lever exactly `±r`, which keeps the disc path bit-identical to the pre-body
  solver. See `Cont` in `sim.c`.
- **`pn == 1` implies its particle sits at exactly `(0,0)`** — `body_finish()`
  re-centres every body on its particle centroid, so a single particle always
  lands on the centre of mass. Code is allowed to rely on that (and does:
  the no-trig fast paths in `sync_particles`/`part_off`, keyed on offsets
  being zero).
- **The restitution gate counts contacts per BODY** (`Body.contacts`,
  `GATE_CONTACTS`), not per particle. Per particle it reads "jammed" for a
  lone disc touching one other disc, and boxes would never bounce.
- **The renderer draws one quad per PARTICLE**, so `Renderer` capacity and
  `glr_init(cap)` are in particles (`SIM_MAX_PARTICLES`), while `sim_init`
  capacity is in bodies (`SIM_MAX_BODIES`). `hover` is a body index and
  darkens the whole body; the spin marker is drawn only for `pn == 1`
  (negative `ang` means "no marker", also used by the ghost preview).

- **Background is white** (`glClearColor` in `glr_init`). The selftest pixel
  checks assume it: background `> 200`, hover/ghost/aim-line colours are
  *dark* (hover darkens a body by `0.65×`, it does not brighten it). Change
  the clear colour → update the checks in `main.c`.
- **Grid overlay** (`n`, `draw_grid` in `gl_draw.c`): samples the physics grid
  state, cell size = `2 * largest particle radius` (`Sim::max_r`, which is
  about particles, not bodies), lines at multiples of `cell`. The selftest
  derives its sample row from `sim.cell` — don't hard-code a pixel.
- **Containment is a statement about PARTICLES.** Any "did it stay in the box"
  check must iterate `Sim.p` (`p->x ± p->r`), not compare `Body.x/y` against
  `Body.rad`: `rad` is a *circumradius*, so a plank lying flat has its centre far
  closer to the floor than `rad` and reads as "35 bodies outside" while sitting
  perfectly still. That false positive lived in `stress()` until shapes made it
  obvious.
- **A test that spawns nothing proves nothing.** The determinism check compared
  two empty sims for a long time; it now spawns a *mixed* scene and compares
  bodies and particles byte-for-byte, which is also the only guard that shape
  choice, proportions, lattice radius, tilt and spin all come out of the seeded
  RNG.
- **`grid_alloc()` must leave `head[]` at `-1`.** The renderer reads the grid
  before the first `grid_build()` (frame 0, or while paused); uninitialised
  `head` draws garbage cell outlines.
- **`Renderer.vbo` is shared** between the ball pass and the line batcher
  (`lines_reserve` / `lines_push` / `lines_flush`). Always `lines_flush` before
  queueing something else, and reset `line_n` (flush does it).
- `glr_draw()` takes `show_grid` as its last argument; all call sites are in
  `main.c` (main loop + 3 in the selftest).
- **Particle vertex layout is 11 floats** (`corner2, center2, radius1, color3,
  alpha1, ang1, flat1` in `gl_draw.c::push_particle`, 4 vertices per particle,
  `VERTS_PER_PARTICLE`); the FS draws antipodal spin "billiard dots" rotated by
  `ang` (rotated in the *vertex* shader — the FS is mediump). A **negative `ang`
  disables the marker** (ghost preview and every particle of a composite body);
  the dots are dark on white, which selftest 6c pixel-checks. `flat1` kills the
  radial gradient: without it every particle of a composite body shades as its
  own bubble and the body reads as a raft instead of a shape.
- **Everything "is this one body or a raft?" keys on `pn == 1`, never on
  `Body.shape`** — marker, `flat` and the `PARTICLE_SKIN` (1.2) expansion. A
  coarse lattice legitimately reduces a small box to a single particle, and then
  it IS a disc. The skin is 1.2 rather than the geometric covering radius
  1.1547 because the FS AA band starts 1 px *inside* the rim, so three covering
  discs do not quite reach opacity at a hole centre and the packing shows a
  honeycomb of light dots; measured clean at r >= 4 px.
- **Spin state lives in `Body`** (`ang, ca, sa, omega, I, inv_I`); `sim_kinetic()`
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
  mode runs exactly 4 substeps/frame for determinism. `ang` update stays
  `+= omega*dt` plus an `fmodf` wrap. Trig in `sim.c` is confined to
  `sync_particles()`: one `cosf/sinf` per multi-particle body per sync, and
  none at all for a centred single particle. Never trig inside a contact loop,
  and never to recover a position the body transform already knows.
