# Concepts

Theory, maths and algorithms behind `imp2d`: a 2D disc (billiard-ball)
simulation solved with **sequential impulses + Coulomb friction + Baumgarte
positional correction**, rendered as SDF discs with OpenGL ES 2.

Human-facing text lives in `README.md` (options, controls, build) and
`AGENTS.md` (working rules). This file is the *why*: what the physics model is,
which equations the code solves, which algorithms it uses, and where the
numerical traps are. Line references point at the current tree. Any
abbreviation you do not recognise is defined in **Terms**, the last section.

---

## Overview

**The idea.** Simulate a pool table: N rigid discs of random radius on a
rectangular field, no gravity by default, bouncing off four walls and off each
other, with spin. Do it from scratch, in plain C11, against only
`egl glesv2 xcb` — so that every line of physics and rendering is readable and
questionable. Keep the physics module (`sim.c`) free of X/GL so it can run and
be verified headless, and verify it with *physical invariants* (momentum,
energy sign, containment, determinism) rather than golden screenshots — plus a
few `glReadPixels` pixel checks for the renderer. The result doubles as the
impulse baseline against which the XPBD sibling `../sim2d` is compared.

**Design commitments that follow from that idea**

| commitment | consequence in the code |
|---|---|
| rigid discs, not particles | each body has orientation θ, angular velocity ω and inertia `I = ½mr²`; contacts act at a *point*, so friction produces torque |
| impulse solver only | momentum is exactly conserved by internal contacts; an elastic table keeps 100 % of its energy (selftest gates) |
| fixed timestep | 240 Hz substeps driven by an accumulator, decoupled from the frame rate; offscreen mode runs exactly 4 substeps/frame → bit-reproducible runs |
| screen units (px, s) | no camera/matrix stack: physics coordinates *are* pixels, y down |
| no textures, no MSAA needed | discs are carved by a signed distance function in the fragment shader |
| uniform grid broad phase | O(n) per substep instead of O(n²); the grid is also a debug overlay (`n`) |
| headless-first | `--selftest` (invariants + pixels) and `--stress` (pile stability metrics) run with no X server |

**Per-substep pipeline** (`sim_step()`, `sim.c:418`)

```
for each ball:   v += g·dt            (semi-implicit Euler, gravity)
                 cap |v|              (SIM_MAX_SPEED)
                 x += v·dt
                 θ += ω·dt  (mod 2π)
walls()                              (position clamp + normal impulse + friction)
grid_build()                         (broad phase, rebuilt every substep)
repeat ITER=2:  collide_pairs()      (narrow phase + one-sided normal impulse
                                      + Coulomb tangential impulse
                                      + positional de-overlap)
walls()                              (a de-overlapped ball may touch a wall now)
```

**Process layers** (`main.c` loop → `sim.c` physics, `gl_draw.c` render,
`platform_x11.c` window/EGL/context + event translation). Data flows one way:
events → `Sim` mutation; `Sim` → vertex staging buffer → one indexed draw call.

---

## Physics principles

### 1. Rigid discs in a plane

A body has 3 degrees of freedom: `(x, y, θ)`. Its state is position plus the
spatial velocity `(v_x, v_y)` and the angular velocity `ω` about the axis
perpendicular to the screen. Because a disc is rotationally symmetric, θ has
no dynamical effect at all (its contacts are always "face-on") — it exists so
that *spin* is visible and so that the tangential contact kinematics are
correct. θ is integrated but never read by the solver (`sim.h` comment:
"rendering only", the friction code reads `omega`, not `ang`).

Two balance laws do all the work:

* linear: `m·dv/dt = ΣF`, impulse form `m·Δv = ΣJ`
* angular (about the body's own centre): `I·dω/dt = Σ (p × J)`, where `p` is
  the contact point relative to the centre and `×` is the 2D scalar cross
  product `a × b = a_x b_y − a_y b_x`.

Coordinate handedness: x right, **y down** (screen). The solver keeps the
formal right-handed 2D identities (`ω × p = ω·(−p_y, p_x)`), which makes a
positive `ω` rotate a disc *clockwise as seen on screen* — the point at
`(+r, 0)` moves to `+y`, i.e. downward. Everything else follows from the
algebra; no sign flip is needed anywhere.

### 2. Mass and inertia model

`m = density · r^k`, `k = mass_exp ∈ {1, 2, 3}` (`sim_mass()`, `sim.c:175`).
`k = 2` is the physically right law for coplanar discs of constant areal
density (mass ∝ area); `k = 1` mimics a hoop/line, `k = 3` exaggerates size
differences for visual effect. Solid-disc inertia is

```
I = ½ m r²        ⟹  r² / I = 2 / m
```

which is used repeatedly below and makes two results exact: the tangential
effective mass of a disc–disc contact is exactly ⅓ of its normal effective
mass, and the wall contact's is `m/3`.

The inverse quantities `inv_m`, `inv_I` are precomputed at spawn
(`sim_add()`, `sim.c:180`) — impulse solvers divide by mass on every contact,
and dividing in float is both slower and less accurate than once at birth.

### 3. Newton's impact law (restitution)

For a contact with unit normal `n` and pre-impact relative normal velocity
`v_n⁻ = (v_j − v_i)·n` (negative = approaching), Poisson's/Newton's restitution
postulate demands

```
v_n⁺ = −e · v_n⁻ ,      e ∈ [0, 1]
```

`e = 1` perfectly elastic (energy-conserving), `e = 0` perfectly inelastic
along the normal. Implemented in one shot per pair, not as a soft constraint:
`j = −(1+e)·v_n / k_n` (`collide_pairs()`, `sim.c:365`) and for walls as the
equivalent velocity flip `v_n = −e·v_n` (`walls()`, `sim.c:290`).

### 4. Unilateral contacts (compression-only)

A contact can push but never pull, and only exists while bodies are (nearly)
touching. Mathematically this is a complementarity condition

```
g ≥ 0 (gap),    λ ≥ 0 (normal impulse),    λ·g = 0
```

The solver's cheap reading of it: apply an impulse **only** if the pair
overlaps (`d < r_i + r_j`) *and* is approaching (`v_n < 0`), and never apply a
normal impulse with the wrong sign. `v_n ≥ 0` pairs are skipped, which is what
lets a stack separate again instead of sticking. Restitution and friction are
both gated on such an impulse having just been produced.

### 5. Coulomb friction, and why the balls spin

At a contact with normal impulse magnitude `j_n`, the tangential impulse obeys

```
|j_t| ≤ μ · j_n          (Coulomb cone, approximated here by a box)
```

with the *sticking* value `j_t = −v_t / k_t` (`v_t` = relative **surface**
velocity along the tangent, `k_t` = tangential effective mass) clamped into the
cone. Sliding contacts saturate the bound, sticking contacts use less.

Friction is what couples the two worlds: the tangential impulse acts at the
surface, so it delivers both `Δv = j_t·t/m` and `Δω = (p × t)·j_t/I`. Hence a
bouncing ball "grabs" the floor, converts slip into roll (or spin into
translation — selftest 4c shoots a spinning ball at a wall and checks that its
`ω` drops while `|v_y|` grows), and two balls exchange spin.

Two consequences worth knowing when tuning `--mu`:

* maximum spin change per wall bounce: `|Δω| = 2|j_t|/(m r) ≤ 2 μ (1+e) |v_n| / r`
* a contact *sticks* (its slip is killed by one impulse) iff
  `|v_t| / k_t ≤ μ j_n`. For solid discs `k_t = 3 k_n` and
  `j_n = (1+e)|v_n|/k_n`, so the criterion is the same for wall and pair
  contacts: `|v_t| ≤ 3 μ (1+e) |v_n|`. With the defaults `μ = 0.2, e = 0.9`
  side slip below ≈ 1.14 × the approach speed disappears in a single bounce;
  above it the contact slides and `Δω` saturates at the bound above.

There is **no rolling resistance** and **no air drag**: once a disc reaches the
no-slip state at the floor (`v_t = 0`, i.e. `v·t = ω r` in the wall convention
derived below), friction asks for zero impulse and the disc rolls forever. (The XPBD sibling added rolling resistance; keeping it
out here is what makes the elastic-table energy gate exact.)

### 6. Non-penetration and its stabilization

True non-penetration is a *position* constraint `g = d − (r_i + r_j) ≥ 0`, but
the impulse solve only talks about velocities; discrete integration always
produces some overlap. Two mechanisms fight the drift:

* velocity-level: the one-sided normal impulse (stops further sinking);
* position-level: a Baumgarte-style partial correction (`CORR_PERCENT = 0.8`,
  `CORR_SLOP = 0.01 px`) that moves overlapping pairs apart by 80 % of the
  penetration beyond a 0.01 px slack, split by inverse mass (`sim.c:378`).

Because the displacement is inverse-mass weighted,
`Σ m·Δx = −c·n + c·n = 0`: the pair's centre of mass does not move, no velocity
is touched (so no kinetic energy is injected), and the correction is
momentum-neutral and rotation-neutral.

### 7. What is deliberately *not* in the model

gravity-free billiards is the default; no drag, no rolling resistance, no
sleeping bodies, no restitution velocity threshold, no continuous (swept)
collision detection, no restitution for tangential impulses, no torsional/
rolling friction moments about the normal, no joints/constraints, no
many-to-many manifolds (one contact point per pair), no sub-stepping of
rotation-dependent effects (irrelevant for discs).

---

## Mathematical models

### Symbols

| symbol | meaning | source in code |
|---|---|---|
| `x, y` | centre, px (origin top-left, +y down) | `Ball.x, .y` |
| `v = (vx, vy)` | centre velocity, px/s | `Ball.vx, .vy` |
| `r` | radius, px | `Ball.r` |
| `m = ρ r^k`, `1/m` | mass, inverse mass | `Ball.m, .inv_m` |
| `θ, ω` | angle (rad, wrapped to `[0,2π)`), angular velocity rad/s | `Ball.ang, .omega` |
| `I = ½ m r²`, `1/I` | inertia of a solid disc | `Ball.I, .inv_I` |
| `e` | restitution, walls and pairs alike | `Sim.restitution` |
| `μ` | Coulomb coefficient | `Sim.friction` |
| `g` | gravity, px/s², +y | `Sim.gravity` |
| `Δt` | substep, `1/240` s | `PHYS_DT` |
| `n` | contact unit normal, `i → j` | `nxx, nyy` |
| `t = (−n_y, n_x)` | contact unit tangent | `tx, ty` |
| `p` | contact point relative to a centre | `px, py` |
| `j_n, j_t` | normal / tangential impulse (momentum units) | `jimp`, `jt` |

### Geometry of a pair contact

```
d  = |x_j − x_i|,  n = (x_j − x_i)/d,  penetration = r_i + r_j − d
```

`d² < 0` guards (`d2 <= 0.0f`) reject the degenerate coincident-centre case,
which would otherwise divide by zero and fling a ball to infinity. The test is
done on squared distances; the single `sqrtf` is taken only for real contacts
(`collide_pairs()`, `sim.c:355`–`357`).

### Time integration

Semi-implicit (symplectic) Euler: velocity first, then position.

```
v⁺ = v + g·Δt          x⁺ = x + v⁺·Δt
```

For a conservative system it tracks a *shadow* Hamiltonian `H + O(Δt)` instead
of spiralling out like explicit Euler does — the reason an `e = 1` scene stays
at 100 % energy over thousands of substeps. Rotation uses the exact linear
update `θ⁺ = θ + ω Δt` plus a wrap (`step_ang()`, `sim.c:394`); no `sin`/`cos`
anywhere in `sim.c` (a hard rule of the module — trig lives on the shader side).

### Normal impulse — full derivation

> The complete version of this derivation — from `m·dv/dt = ΣF` through the
> impulsive limit, with figures of the contact geometry and of the before/after
> velocities — is in **`tmp/phys.tex`** (`pdflatex tmp/phys.tex`). What follows
> is the summary of it.

Relative normal velocity and normal effective mass:

```
v_n = (v_j − v_i)·n                      ( < 0 ⟺ approaching )
k_n = 1/m_i + 1/m_j
```

Apply `J = j·n` to `j` and `−J` to `i`:

```
v_i⁺ = v_i − (j/m_i) n
v_j⁺ = v_j + (j/m_j) n
v_n⁺ = v_n + j·k_n
```

Imposing `v_n⁺ = −e v_n⁻` gives

```
        −(1 + e) · v_n
j  =  ─────────────────────                (sim.c:365)
           1/m_i + 1/m_j
```

Consequences (all of them are asserted by the selftest):

* **Momentum is exact.** `Δp = m_i(−j n/m_i) + m_j(+j n/m_j) = 0`. Only walls
  and gravity change `p`; nothing damps anything. Gate: `momentum conserved`
  (selftest 4) compares `p_x` before/after a head-on hit to 2 %.
* **Energy never grows** for `e ≤ 1`:
  ```
  ΔKE = j·v_n + ½ j² k_n = − (1 − e²) · v_n² / (2 k_n)  ≤ 0
  ```
  (an elastic hit gives `ΔKE = 0` identically — hence "an elastic billiard
  table keeps 100 % of its energy", selftest 2).
* **Mass ratio behaviour** is automatic through `1/m` weighting: heavy barely
  moves, light flies (selftest 4).

**Wall.** A wall is an infinitely massive body, so `k_n = 1/m` and
`j_n = (1+e)·m·|v_n|`. The code applies the equivalent velocity flip and keeps
`j_n` only as the Coulomb budget for friction (`sim.c:299`, `307`, `316`, `324`).

### Contact kinematics with rotation

Surface velocity of a body at a point `p` (relative to its centre):

```
u(p) = v + ω × p ,  with ω × p = ω·(−p_y, p_x)      (2D)
```

Wall: `p = −r·n` (the contact lies opposite the inward normal), and `p × t = −r`.
Ball `i`: `p_i = +r_i·n`; ball `j`: `p_j = −r_j·n`; `p_i × t = +r_i`,
`p_j × t = −r_j` because `n × t = +1`. Slip velocity:

```
v_t = (u_j − u_i)·t                        (pair_friction, sim.c:264)
v_t = (v + ω × p)·t                        (wall_friction,  sim.c:243)
```

### Tangential impulse

Tangential effective mass (the `v_t` ↔ `j_t` compliance at the contact):

```
k_t = 1/m_i + 1/m_j + (p_i × t)²/I_i + (p_j × t)²/I_j
    = 1/m_i + 1/m_j + r_i²/I_i + r_j²/I_j
    = 3·(1/m_i + 1/m_j) = 3 k_n            (for solid discs, I = ½mr²)
```

Sticking solution and Coulomb clamp:

```
j_t  = − v_t / k_t
j_t  ← clamp(j_t, −μ j_n, +μ j_n)
```

Application (`pair_friction`):

```
v_i⁺ = v_i − (j_t/m_i) t            ω_i⁺ = ω_i − j_t r_i / I_i
v_j⁺ = v_j + (j_t/m_j) t            ω_j⁺ = ω_j − j_t r_j / I_j
```

(The signs come straight out of `p × t`: `n × t = +1`, so `p_i × t = +r_i`,
`p_j × t = −r_j`; the code writes the cross product explicitly as
`px*ty − py*tx` in `wall_friction` so the identity is visible rather than
assumed.)

* Tangential impulses are internal and equal-and-opposite ⇒ `p` is still exact.
* Writing `j_t = −α v_t/k_t` with `α ∈ [0,1]` (which is what clamping to the
  cone gives, `α = min(1, μ j_n k_t/|v_t|)`), the energy change is
  `ΔKE = −(α − α²/2)·v_t²/k_t ≤ 0`. **Friction can only remove energy**, which
  is why the spin+friction gate (selftest 4d) is a plain `KE` non-increase test.
* Unclamped, `v_t⁺ = 0` exactly: the contact sticks. For a disc against a wall,
  `k_t = 3/m`, so `j_t = −m v_t/3`, `Δv_t = −v_t/3`, `Δω = 2 v_t/(3r)`.

Friction is only invoked where a normal impulse was just produced
(`s->friction > 0 && jimp != 0`, `sim.c:374`) — i.e. it is *causally* tied to a
compressive contact, not applied to everything that merely overlaps.

### Positional projection

```
pen = (r_i + r_j) − d − SLOP
if pen > 0:  c  = β · pen / (1/m_i + 1/m_j)      β = CORR_PERCENT = 0.8
             x_i⁺ = x_i − c·(1/m_i)·n
             x_j⁺ = x_j + c·(1/m_j)·n
```

This is relaxation/projection: it fixes the *configuration*, not the velocity
(no Baumgarte bias term is folded into the impulse itself). With `ITER = 2` and
one-sided impulses it leaves a residual overlap (measured: ≈ 0.9 px mean, < 7 px
max in the 2000-disk stress pile), which is the deliberate softness trade — the
alternative (many iterations, or a full LCP) is what makes piles stiff and
expensive.

### Angle bookkeeping

```
ω ← clamp(ω, −SIM_MAX_OMEGA, +SIM_MAX_OMEGA)        (60 rad/s)
θ ← θ + ω Δt ;  θ ← θ − 2π·⌊θ/2π⌋   (written with fmodf + sign fix-up)
```

Wrapping keeps `θ ∈ [0, 2π)` so the `mediump` shaders never see a large angle
(`sin`/`cos` of 10⁴ rad in fp16-class precision is garbage) and so the selftest
can assert `0 ≤ θ < 2π` after 600 frames.

### Colour model (mass ramp)

`mass_color()`, `sim.c:49`: normalise the mass proxy on `r ∈ [4, 64]`,

```
t = clamp( (r^k − 4^k) / (64^k − 4^k), 0, 1 )
hue = 0.60 − 0.60·t          (0.60 = cool blue, light  →  0.0 = red, heavy)
```

then the classic HSL(h, 1, ½)→RGB piecewise mapping (`p = 1 − |((6h) mod 2) − 1|`
and a 6-segment switch), finally lifted into a pastel band
`c = 0.25 + 0.70·c'` so nothing is pure black on the white background. Note the
ramp uses `k` too, so `--mass-exp` changes the palette, not only the dynamics.

### Rendering maths (the model the shader implements)

* Screen → clip (per vertex, highp): `clip = (p/res)·2 − 1`, `y` negated
  (`VS_BALL`), with `p = centre + corner·radius` — a screen-space quad that is
  exactly `2r` wide.
* Disc SDF in quad space: `d = |v_disc|`, i.e. `|p − centre| / r`.
* Anti-aliasing without `fwidth` (ES 2 safe): the AA half-band is one physical
  pixel expressed in disc units, `v_aa = 1/max(r, 1)`, computed in the vertex
  shader and interpolated:
  `α = α₀·(1 − smoothstep(1 − v_aa, 1 + v_aa, d))`, `discard` when `α ≤ 0.002`.
* Fake lighting: `shade = mix(0.70, 1.0, smoothstep(1.0, 0.40, d))` — limb
  darkening at the rim, brighter at the centre, so a flat disc reads as a ball.
* Spin marker: two antipodal dots along the diameter rotated by θ; the dot
  centre is `v_dot = (cos θ, sin θ)·0.55` computed **in the vertex shader**
  (highp) and interpolated, masked by `1 − smoothstep(0.15, 0.21, min(|u−v_dot|, |u+v_dot|))`,
  darkening the colour by ×0.40. A negative `ang` disables the marker (ghost ball).
* Compositing is **premultiplied alpha**: `glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA)`
  with `gl_FragColor = vec4(col·a, a)` (Porter–Duff "source over").
* Readback: GL origin is bottom-left, PPM/physics are top-left → rows flipped
  in `glr_dump_ppm()` (`gl_draw.c:456`), and the selftest indexes
  `img[(H−1−y)·W + x]`.

### Statistics / metrics model

| quantity | formula | code |
|---|---|---|
| kinetic energy | `Σ ½m|v|² + ½Iω²` (rotational included — every energy gate must use it) | `sim_kinetic()` `sim.c:436` |
| linear momentum | `(Σ m v_x, Σ m v_y)` | `sim_momentum()` `sim.c:445` |
| penetration | `mean/max over overlapping pairs of (r_i+r_j − d)` | `stress()` `main.c:510`+ |
| residual velocity | `mean \|v\|`, `max \|v\|` | `stress()` |
| jitter | `max \|v\|` over any ball during the last simulated second | `stress()` |
| coherent drift | `\|Σv\|/n` (unweighted mean velocity) | `v_com` line |
| depth profile | mean `\|v\|` in thirds of the pile height + above-pile band | `stress()` |
| KE decay | `KE_final/KE_initial` | `stress()` |
| activity | ball-ball contact count, wall-hit count | `Sim.collisions/wall_hits` |

---

## Algorithms

### Top-level loop (fixed timestep + accumulator, `main.c:730`+)

```
d = min(now − t_last, 0.25 s)                 # wall-clock delta, clamped
if offscreen: acc = FRAME_DT                  # deterministic 4 substeps/frame
elif not paused: acc += d
while acc ≥ PHYS_DT and nsub < MAX_SUB:       # 1/240 s, MAX_SUB = 16
    sim_step(dt); acc -= PHYS_DT; nsub++
if nsub == MAX_SUB: acc = 0                   # drop the backlog (anti spiral-of-death)
glr_draw(); plat_swap(); poll(fd, 4 ms)
```

Three ideas: (a) physics frequency independent of display frequency;
(b) a hard catch-up budget so a slow frame cannot cascade into an unbounded
one; (c) in `--mode off` the accumulator is *replaced* by a constant, which
removes wall-clock from the physics entirely — this is what makes headless runs
byte-reproducible and the selftest meaningful.

### Broad phase: uniform grid + half-neighbourhood (`sim.c:76`, `107`, `335`)

```
cell = max(8, int(2·max_r));  gw = int(w/cell)+2;  gh = int(h/cell)+2
head[cy·gw + cx] = linked-list head per cell (-1 = empty), next[i] = intrusive list
build: reset head[] to −1, then for each ball push into its clamped cell   # O(n + gw·gh)
scan : for each cell (cx,cy), for each ball i in it, for k in 0..4:
           (cx,cy) + [(0,0), (1,0), (−1,1), (0,1), (1,1)]
           for each ball j in that cell:  if k == 0 and j ≤ i: continue
```

*Correctness argument.* Touching discs satisfy
`d < r_i + r_j ≤ 2·max_r = cell`, hence their cell indices differ by at most 1
on each axis, so the 3×3 ring around every ball is exhaustive. The 5-offset
"forward" half of that ring meets every unordered *cell* pair exactly once, and
the `j > i` rule inside the own cell meets every *ball* pair exactly once — no
duplicate narrow-phase tests, no second pass.

*Cost.* Build is `O(n + gw·gh)` (the `head` reset dominates for a big box with
few balls); the scan is `O(Σ_cells k_cell · 5)` with `k_cell` the occupancy —
linear in `n` as long as the cell size is matched to the disc size. That is why
`cell` is derived from `max_r`: one cell per largest disc diameter keeps
`k_cell ≈ 1` on average.

*Rebuild policy.* Rebuilt every substep (`grid_build()` in `sim_step`), and
re-`alloc`ed only when `cell`, `gw` or `gh` change (`grid_alloc`). Consequence
you must respect: `head[]` is initialised to `−1` at allocation, because the
renderer draws the grid before the first build (frame 0 / paused) — see
`AGENTS.md`.

### Narrow phase + sequential impulses (`collide_pairs()`, `sim.c:338`)

```
for each candidate pair (i, j):
    d²  = |Δx|²;  if d² ≥ (r_i+r_j)² or d² ≤ 0: continue
    d   = sqrt(d²);  n = Δx/d
    v_n = (v_j − v_i)·n;  k = 1/m_i + 1/m_j
    j_n = 0
    if v_n < 0:                          # unilateral + approaching
        j_n = −(1+e)·v_n / k             # one-sided normal impulse
        apply ∓j_n·n·(1/m)               # (collision counter: first pass only)
    if μ > 0 and j_n ≠ 0:
        friction(i, j, n, μ, |j_n|)      # Coulomb tangential impulse → spin
    project overlap by β=0.8 beyond slop # positional correction
```

The outer solver is **Gauss–Seidel relaxation over contacts**: each contact is
solved to satisfaction *immediately* against the current (already updated)
state of both bodies, and the sweep is repeated `ITER = 2` times. `ITER` passes
reuse one broad-phase snapshot. Sweeping the grid in cell order also gives a
reasonable impulse order (bottom-left to top-right), which matters for stacks:
solving a pile from the bottom up propagates support upward within a few passes,
whereas a random order converges more slowly.

Order dependence, one-sided impulses, and the Coulomb box are exactly what make
this a *relaxation* of an LCP rather than a solution of one — see
Numerical considerations.

### Walls (`walls()`, `sim.c:290`)

Per axis and side: clamp the position to the box, and if the velocity still
points outward, reflect it with `e` and apply wall friction with the bounce's
`j_n` as the Coulomb budget. Walls are handled **twice per substep** (before
and after the pair solve) because positional de-overlap can push a ball into or
out of the boundary. Position clamping (rather than impulse-based gap fixing)
means the box can never be escaped, which is the containment invariant the
selftest and stress harness assert.

### Scene editing

| operation | algorithm | complexity |
|---|---|---|
| `sim_add_random` | up to 64 rejection samples: `r ∈ [rmin,rmax]`, uniform position inset by `r`, reject on `overlaps()` (O(n)) or out-of-box | `O(n)` per spawn worst case, fails (`−1`) when the box is crowded |
| `sim_remove` | `memmove` shift-down (order-preserving) + `O(n)` rescan for `max_r` | `O(n)`; order is preserved on purpose so indices survive between a `sim_pick` and the delete, and colours/stacking stay stable |
| `sim_pick` | reverse-order scan, squared distance ≤ r² | `O(n)`, "topmost" = last spawned |
| `sim_resize` | clamp every ball inside the new box, then `grid_alloc` | `O(n)` |
| slingshot | launch `v = 4·(p₀ − p₁)` from the press point ("opposite the pull"), random `ω` when `--spin` | `main.c` button handler |

### Deterministic RNG

`splitmix64` (`sim.c:23`) — 64-bit state, three xorshift/multiply rounds;
`frand01()` takes the top 53 bits and scales by `2⁻⁵³` so the value is exactly
representable and lands in `[0,1)`. The state lives in `Sim.rng`, so a scene is
reproducible from `--seed` alone, and the RNG is *never* used inside
`sim_step` — physics is a deterministic function of the initial state, which is
what makes the determinism check (`memcmp` of two `Ball` arrays, selftest 5)
meaningful. Note the deliberate asymmetry: the spin draw happens only when
`spin > 0` (`sim.c:223`) so frictionless runs stay byte-identical to older
runs of the same seed.

### Rendering algorithms

* **One indexed draw call for all discs.** 4 vertices/ball (10 floats/vertex),
  a static element buffer `QUAD = {0,1,2, 2,1,3}` replicated with a per-ball
  base offset (`glr_init`, `gl_draw.c:163`), and a CPU staging array refilled
  each frame and pushed with one `glBufferData`. Capacity is fixed
  (`DEF_CAP = 2000`), so no reallocation and no per-frame allocation at all.
* **Line batching** (`lines_reserve`/`lines_push`/`lines_flush`, `gl_draw.c:259`+):
  a segment is
  extruded along its normal into a `width`-px quad (6 verts, two triangles),
  queued in a growable buffer, and flushed as one `glDrawArrays` with a uniform
  colour. The drag aim line, the grid lines and the occupied-cell outlines are
  each one flush. Degenerate segments (`len < 1e-3f`) are dropped.
* **Grid overlay** (`draw_grid`, `gl_draw.c:322`): the *physics* grid state
  (`sim->cell/gw/gh/head`) rendered directly, capped at `GRID_MAX_SEGS = 8192`
  segments so a huge box cannot blow the batcher.
* **Screenshot**: `glReadPixels` from the FBO → PPM P6 with the row order
  flipped.

### Platform algorithms (`platform_x11.c`)

* Display acquisition: `eglGetPlatformDisplay(EGL_PLATFORM_XCB_EXT, xcb_connection, …)`
  with a plain `eglGetDisplay` fallback; offscreen uses
  `EGL_PLATFORM_SURFACELESS_MESA`, falls back to a pbuffer, and prefers a truly
  surfaceless `eglMakeCurrent`. `EGL_NO_X11` keeps Xlib out of the headers.
* Config selection `pick_config()` is a graceful-degradation ladder that tries
  ES3 before ES2, and (only when an MSAA sample count is requested) the sampled
  variant of each first; `plat_open_window` currently passes `msaa = 0`, so the
  ladder degenerates to ES3 → ES2, and context creation retries ES3 → ES2 as
  well. That is what lets one binary run on llvmpipe and on a real GPU.
* Keymap: fetch `xcb_get_keyboard_mapping` once into
  `ks[256]/ks_shift[256]`, index by *real* keycode (the reply starts at
  `min_keycode`, not 0 — a bug that has bitten), resolve with the Shift/CapsLock
  modifier mask. Printable keys arrive as ASCII, so `'q'`, `' '`, `'+'` need no
  tables; only non-printables (`KS_ESCAPE`) are constants.
* Event loop is non-blocking `xcb_poll_for_event` drained into a small tagged
  union (`PlatEvent`), with the connection fd exported for `poll(..., 4 ms)` so
  one thread waits on either X traffic or a 4 ms tick. `WM_DELETE_WINDOW` is
  translated to `PE_QUIT`, as is a dead connection.

### Verification algorithms

* **`--selftest`** — physics invariants: finiteness, containment (±1 px),
  speed cap, angle wrap, collision activity counters, `KE` non-increase for
  `e = 1` and `e = 0.9`, the mass law, head-on momentum conservation (2 %),
  "gravity settles everything into the lower half after 30 s", wall
  spin→tangential transfer, `KE(trans+rot)` non-increase with `μ = 0.5`,
  bitwise determinism for equal seeds — plus pixel checks via `glReadPixels`
  (disc on white, round SDF edge at x = 385, grid overlay row derived from
  `sim.cell`, spin marker moves between `θ = 0` and `θ = π/2`).
* **`--stress`** — physics-only pile: spawn 2000 disks (r 2–8) in 1024×768
  under 1000 px/s² gravity, settle for 30 s of simulated time, then measure the
  penetration/velocity/KE statistics of the table above and compare them to
  regression gates tuned to *this* solver's signature. Deterministic per seed,
  no EGL/GL, ~6 s wall.

---

## Data structures

### `Ball` — 14 floats, 56 B (`sim.h:21`)

| field | unit | note |
|---|---|---|
| `x, y` | px | top-left origin, +y down; also the render centre |
| `vx, vy` | px/s | |
| `r` | px | |
| `m`, `inv_m` | — | `m = ρr^k`; inverse cached to avoid division in the solver |
| `ang`, `omega` | rad, rad/s | `ang` wrapped to `[0,2π)`, only read by the renderer |
| `I`, `inv_I` | — | solid disc `½mr²`, inverse cached |
| `cr, cg, cb` | 0–1 | derived from `mass_color` at spawn, not from state |

AoS layout chosen on purpose: a contact touches *all* fields of both balls, so
spatial locality per pair beats per-array streaming; 56 B is a hair over one
cache line, so a pair costs ~2–3 lines. No pointer/indirection, so `Sim.b` is
one contiguous block (2000 × 56 B ≈ 112 KB) that `memmove`s cheaply on removal.

### `Sim` (`sim.h:34`)

```
Ball  *b; int n, cap;                 # dense array + capacity, no growth in the hot loop
float  w, h;                          # the box (window client area)
float  restitution, friction, gravity, density; int mass_exp;
uint64_t collisions, wall_hits, rng;  # counters + RNG state (⇒ reproducible scenes)
int    cell, gw, gh;                  # broad-phase geometry
int   *head, head_cap;  *next, next_cap;
float  max_r;                         # drives cell size; maintained incrementally
```

Everything the solver needs is here and nothing else — no X/GL types, no
allocation inside `sim_step`.

### Broad-phase buckets: `head[]` + `next[]`

Intrusive singly linked lists, one per cell: `head[c]` = first ball index,
`next[i]` = next ball in the same cell, `−1` terminates. Chosen because it
needs no sorting, no dynamic arrays, no per-frame allocation (both buffers are
allocated once to `gw·gh` / `cap` and reused), insertion is `O(1)` and
"empty the grid" is a single `memset`-style pass over `head`.

Trade-offs to know: `next[]` is indexed by ball id, so list walking is strided
over `Ball`-sized distances (fine at 2000 bodies); and `head[]` must be kept
`−1`-initialised because the renderer reads it before/without a build.

### `Renderer` (`gl_draw.h:29`)

| member | role |
|---|---|
| `prog_ball`, `prog_line` | the only two GL programs (SDF discs; flat lines) |
| `a_*`, `u_*` | cached attribute/uniform locations |
| `vbo` | **shared** GPU buffer between the ball pass and the line batcher — hence the "always flush before queueing something else" rule |
| `ebo` | static index buffer, 6 indices per ball |
| `v` | CPU staging, `cap·4·10` floats |
| `lines`, `line_cap`, `line_n` | line staging, doubling growth, zeroed by flush |
| `fbo`, `fbo_tex`, `fbo_w/h` | offscreen target for `--mode off`, `glReadPixels`, PPM |

Vertex layout (10 floats, `push_ball`, `gl_draw.c:232`):
`[corner(2), center(2), radius(1), color(3), alpha(1), ang(1)]` — corner is the
unit quad coordinate, everything else is per-ball constant replicated ×4. The
`ang` field doubles as a flag: `ang < 0` = "no spin marker" (ghost ball).

### Other

`Plat` (window + EGL handles + keycode→keysym tables), `PlatEvent` (tagged
union, 6 event kinds), `Drag` (slingshot start/end/ghost radius), `Opts`
(parsed+clamped command line). Ownership is flat: `sim_free`/`glr_destroy`/
`plat_close` each `memset` themselves to zero after freeing, so double-free and
use-after-free show up immediately.

---

## Numerical considerations

### Representation

Everything is `float` (fp32, ~7 significant digits). Ranges in this app:
positions ~10³ px, velocities ~10³ px/s, masses `0.001·r^k` ≈ 10⁻³–10⁰,
`KE` up to ~10⁵, `I = ½mr²` up to ~10³. Worst products (`m·v²`, `r²·inv_I`)
stay within 3–4 orders of magnitude, so no rescaling is needed — but note that
`KE` in float accumulates `O(n)` terms and the selftest tolerances (2 %) are
set accordingly, not at 1e-6.

### Determinism vs. chaos

Fixed operation order + fixed `dt` + RNG only at spawn ⇒ two runs with the same
seed are *bitwise* identical (`memcmp` over the `Ball` array, selftest 5). That
is a strong statement, and it is fragile by nature: any reordering of the
solver sweep, any changed constant, or a compiler that contracts
`a*b+c` into an FMA changes the trajectory. Chaos makes this worse — with
friction on, a 1-ulp difference in one contact grows exponentially, so
refactoring gates are only trustworthy bitwise with `--spin 0 --mu 0`; with
spin, compare metrics within a few percent.

### Time step, and the absence of CCD

The discrete contact test only sees positions at substep boundaries, so a pair
tunnels if their **relative** travel per substep exceeds `r_i + r_j`. With
`Δt = 1/240 s` and `SIM_MAX_SPEED = 4000 px/s` a single ball moves ≤ 16.7 px
per substep and an approaching pair ≤ 33.4 px, which is *more* than the default
minimum diameter (2·8 px) and much more than the stress disks (2·2 px). So:

* the speed cap is a **mitigation, not a proof** (the `sim.h` comment
  "keeps per-substep travel below min radius" is the ideal, not the guarantee);
* `dt` is the primary defence: 240 Hz quartering a 60 Hz frame cuts the
  per-step travel to a quarter, which makes billiard-speed contacts
  (≤ 1000 px/s → ≤ 4.2 px/substep) reliably detected;
* walls cannot be tunnelled at all because they are position clamps, not tests
  — that is why "balls stay inside the box" is a hard invariant even when a pair
  passes through each other;
* if you need real robustness for small, fast bodies, the additions would be a
  swept-sphere (quadratic in `Δt`) test or substepping by `max(|v|·Δt)/r`, not
  more iterations.

### Stability of the impulse solve

* **Energy sign.** `ΔKE = −(1−e²)v_n²/(2k_n)` per normal impulse and
  `ΔKE ≤ 0` per Coulomb-clamped tangential impulse; both are *algebraic*
  guarantees, so the gates are "energy never grows", with only a 2 % float
  headroom. Anything that makes energy grow (a bias term with the wrong sign, a
  restitution applied twice, friction applied without a normal impulse) fails
  `make test`.
* **Positional correction is the one energy loophole.** It changes positions,
  so in a gravity field it changes potential energy: `β = 0.8` pushes
  interpenetrating bodies apart and can hand a pile a little extra height
  ("popping"). Lower `CORR_PERCENT` → softer, deeper piles; higher → jitter and
  popping. Keeping it out of the velocity (no Baumgarte `bias = β/dt·g` term)
  is what stops the classic limit-cycle where a resting stack accumulates
  bias velocity and vibrates.
* **Slop.** `CORR_SLOP = 0.01 px` gives the constraint a dead band: without it,
  a contact that is exactly at `g = 0` gets corrected every substep forever.
* **Iteration count.** `ITER = 2` is deliberately small: contacts are soft, the
  pile never sleeps, and residual mean|v| ≈ 35 px/s with jitter ≈ 124 px/s is
  the solver's *signature*, not a bug (see `AGENTS.md`). Stiffening it means
  more iterations and/or a restitution velocity threshold — but a threshold is
  a form of damping and would break the "elastic table keeps 100 %" gate, which
  is exactly why `../sim2d` (which has gating) and `imp2d` (which does not)
  disagree about dense piles.
* **Why piles jitter here.** Three effects compound: gravity is applied *before*
  the solve to every body every substep (so resting balls always re-enter the
  contact with `v_n = gΔt ≈ 4 px/s` and get a full `e`-weighted bounce back),
  restitution has no low-velocity cutoff, and Gauss–Seidel order dependence
  means the impulse a contact receives depends on sweep order, so a stack has
  no consistent pressure solution. Result: sustained ~10¹ px/s agitation. Fixing
  it properly means either a warm-started/accumulated impulse with clamping
  (`0 ≤ λ_acc`, i.e. Box2D-style warm starting) or switching to a position-based
  solver — the latter is the existence proof of `../sim2d`.
* **Order dependence & symmetry.** A single Gauss–Seidel sweep is not
  symmetric under swapping `i` and `j`; results depend on grid traversal order.
  Determinism holds (order is fixed), physical symmetry only approximately.
* **Mass conditioning.** `--mass-exp 3` with `r ∈ [2, 120]` spans a mass ratio
  of 2.2·10⁵; the inverse-mass split still behaves (the heavy body simply does
  not move), but the light one takes the entire positional correction, so
  extreme ratios look sloppy and converge slower. `density` also scales `m`
  uniformly — impulses and gravity are only affected through ratios, so
  `density` is inert apart from float range.

### Guards against undefined behaviour

`d² ≤ 0` (coincident centres) → skip the pair; `r ≤ 0` or full array → `sim_add`
returns `−1`; `inv_m, inv_I > 0` always, so `k_n, k_t > 0` and the impulse
denominators never vanish; `|v|` is clamped before the position update and `ω`
in the same body block (`cap_speed` `sim.c:406`, `step_ang` `sim.c:394`), so a
bad state cannot reach `inf/NaN` — and `isfinite` is polled in the selftest and
the stress loop anyway; `sin/cos` never called with unbounded arguments because `θ` is
wrapped; line extrusion drops segments shorter than `1e-3f`.

### Coupling and portability traps (the ones that are already documented in AGENTS.md, restated as theory)

* The AA band `1/max(r,1)` assumes **one pixel ≈ 1/max(r,1) disc units**, i.e.
  `r ≥ 1` and a 1:1 pixel mapping. Tiny radii (`--rmin 2` is fine) or a future
  zoom/camera would break the edge quality — reintroducing `fwidth` (ES 3 /
  `GL_OES_standard_derivatives`) is the general fix.
* `mediump` in the fragment shader is why the rotation matrix (`cos`, `sin`) is
  evaluated in the vertex shader and interpolated.
* Float `ang` in the vertex attribute must stay small (hence the wrap) for the
  same reason.
* The grid is built once per substep but used for `ITER` relaxation passes while
  positions move by up to `β·pen` per pass; the cell-size margin (`cell = 2·max_r`
  against a `r_i + r_j ≤ 2 max_r` contact distance) absorbs normal-sized
  corrections, but a pathologically large correction could move a ball out of
  every scanned neighbour cell and drop a contact until the next rebuild.

### Measurement caveats for the stress metrics

Mean penetration is averaged over *overlapping* pairs only (`over`), so it
rises when the solver gets softer *and* when contacts get more numerous — read
it together with `contacts`. `|v_com|` in the printout is the unweighted mean
velocity `|Σv|/n`, not the true mass-weighted centre-of-mass velocity; with
ranging radii the two differ, and the mass-weighted one is what the momentum
gate (`sim_momentum`) describes. Percentiles `p50/p90` come from an in-place
insertion sort of a scratch buffer (`O(n²)` worst case, deliberately chosen to
avoid `qsort` comparator plumbing at n = 2000).

---

## References and inspiration

### Impact, contacts and friction theory

* **Newton** impact law / **Poisson** restitution hypothesis — `v_n⁺ = −e·v_n⁻`.
* **Goldstein, Poole & Safko**, *Classical Mechanics* — rigid-body planar
  motion, `I`, angular momentum about the centre, `ΔL = p × J`.
* **Mirtich, B.**, *Impulse-based Dynamic Simulation of Rigid Body Systems*,
  PhD thesis, UNC Chapel Hill (1996) — the clearest derivation of point-contact
  impulses with restitution and friction for rigid bodies; the derivation
  reproduced above is essentially his notation.
* **Stewart, D. & Trinkle, J.**, "An implicit time-stepping scheme for rigid
  body dynamics with inelastic collisions and Coulomb friction", *Int. J. Numer.
  Methods Eng.* 39 (1996) — the LCP formulation that `v_n < 0` / `λ ≥ 0` /
  `λ·g = 0` relaxes here.
* **Anitescu, Potra & Stewart**, "Time-stepping in rigid multi-body systems",
  *CMAME* 142 (1997) — complementarity view of contacts and friction cones.
* **Cottle, Pang & Stone**, *The Linear Complementarity Problem*, Cambridge
  (1992) — `0 ≤ λ ⊥ g ≥ 0` notation.
* **Baraff, D.**, "Fast contact forces for rigid body simulation" (CMU-CS-93-116)
  and Baraff, Witkin & Kassner, "Physically based modeling of colliding rigid
  bodies: a technical overview", SIGGRAPH 2000 course notes — the practical
  framing of interpenetration, restitution and friction for graphics.
* **Weller, Sharf & Fiume**, "Noninterpenetration and contact resolution for
  rigid and articulated systems: a tutorial", *CGF* 25(3) (2006), and their SCA
  2006 paper on historical evaluation of collision impulses — what sequential
  impulses do and do not converge to.
* **Erleben, E.**, *Velocity-Based Physics: Numerical Dynamics for Computer
  Graphics* (open book, 2022) — modern taxonomy of velocity-level solvers,
  Gauss–Seidel, LCP and projections; the best single reference for why this
  solver behaves the way it does in piles.

### Sequential impulses, stabilization and jitter

* **Erin Catto** — "Impulse Dynamics" (GDC 2005) and "Sequential Impulses" /
  Box2D sources & GDC 2006 tutorial: the "solve contacts one at a time, clamp
  one-sided, relax N times, then fix positions with a percentage and a slop"
  recipe is directly what `collide_pairs()` implements (`CORR_PERCENT`/
  `CORR_SLOP` are Box2D's `baumgarte`/`allowedPenetration` idea).
* **Baumgarte, J.**, "Stabilization of constraints and integrals of motion in
  dynamical systems", *CMAME* 1 (1972) 1–16 — the constraint-stabilization
  family that "Baumgarte-style positional correction" names.
* **Rudy, D.**, "Improving Stationarity of Friction Simulation", GDC 2021 —
  why gravity-before-solve plus a no-threshold restitution leaves stacks
  jittering; exactly the mechanism described under Numerical considerations,
  and the reason `../sim2d` exists.
* **Box2D** (Catto, open source) and **Bullet** (Couprie & Coumans) — readable
  engineering implementations to compare against; both use the grid/SAP broad
  phase + sequential impulses + positional correction combination.

### Broad phase

* **Larsson & Akenine-Möller**, *Interactive Geometric Tools*, and **Ericson**,
  *Real-Time Collision Detection* — uniform grid, sweep-and-prune, bounding
  volumes; the `cell = 2·max_r` argument and the half-neighbourhood offset
  table are standard content of these texts.
* **Cohen, Lin, Manocha & Ponamgi**, "I-COLLIDE: An interactive and exact
  collision detection system for large-scale environments", SIGGRAPH 1995 —
  sweep-and-prune, the alternative that loses O(n) rebuild simplicity.
* **Gonzales, Kaxiras & Sornmo**, "A fast collision-detection algorithm for
  particle systems based on the tetrahedral fixed-grid method", *J. Comp. Phys.*
  225 (2007); **Tessier et al.**, "Collision detection for heterogeneous
  systems of particles by extension of the uniform grid method" (2017) — fixed
  grids for polydisperse discs, i.e. the `max_r` sizing question.

### Time integration and determinism

* **Hairer, Lubich & Wanner**, *Geometric Numerical Integration* — symplecticity
  of semi-implicit Euler and the shadow-Hamiltonian argument for energy
  boundedness.
* **Steele, Lea & Flood**, "Fast Splittable Pseudorandom Number Generators"
  (WSU technical report, 2006/2014) — SplitMix64; the mixing constants used in
  `splitmix64()` come from that family (via Stockmeyer's list).
* `clock_gettime(CLOCK_MONOTONIC)` + accumulator + frame budget: the standard
  game-loop pattern (e.g. Katanuk, "Fix your timestep!", 2004) including the
  spiral-of-death clamp.

### Rendering

* **Thibault, Pluess & Aila**, "Distance estimation anti-aliasing", *J. Computer
  Graphics Techniques* 1 (2012) — supersampling-free AA of SDFs; and Quilez'
  "2D distance functions"/"Improved anti-aliasing with distance estimation"
  articles for the `1/r`-band trick used instead of `fwidth` (ES 2 has no
  derivatives).
* **Forchier & Canas**, "Rendering Vector Art on the GPU", *GPU Gems 3* ch. 11 —
  procedural shapes from quads without textures.
* **Porter & Duff**, "Compositing digital images", SIGGRAPH 1984 — the
  premultiplied `src-over` blend used for the discs.
* **Smith, Alvy & Reeves**, "Color gamut transform pairs", SIGGRAPH 1978 — the
  HSL→RGB conversion inside `mass_color()`.
* Khronos **EGL 1.5**, `EGL_EXT_platform_xcb`, `EGL_MESA_platform_surfaceless`
  specs, plus the XCB protocol docs — the platform bootstrap and the reason
  `EGL_NO_X11` is defined.

### Billiards (the physics playground itself)

* **Tabachnikov**, *Billiards* (Publish or Perish, 1997) — elastic point/ball
  collisions in a rectangular table: exactly the `e = 1`, no-gravity limit the
  selftest asserts momentum conservation and 100 % energy retention in.
* **Galperin & Zemlyakov**, "Mathematical billiards", *Russian Math. Surveys*
  (1991) — integrability of the rectangular table, which is what a solver
  without numerical dissipation should look like.

### Inside this repository

* **`../sim2d`** (and its `AGENTS.md`) — the XPBD sibling: predict → project →
  restart velocity → iterated impulses, with restitution gating and rolling
  resistance. The two projects exist to bracket the same scene from both sides;
  the numbers to compare are in both `AGENTS.md` files.
* **`tmp/phys.tex`** — a standalone LaTeX/TikZ write-up of the two-disc impulse
  derivation (`pdflatex phys.tex` in that directory): Newton's law → impulsive
  limit → restitution → momentum/energy bookkeeping → the tangential (spin)
  impulse, with a contact-geometry figure and a before/after velocity figure, and
  an equation-to-source-line table for `sim.c`.
* **Git history**: `83eeb2d` ("add a 2d bouncing-ball simulator with an impulse
  solver") forked from `sim2d` while it still had both solvers; `5f5e7be`
  ("add rigid bodies as particle sets, drop the impulse solver") moved XPBD
  out and left `imp2d` as the impulse reference. The retired
  `--solver/--compliance` switch and all XPBD measurements live in that
  history — do not port them back.
* **`AGENTS.md`** — the empirical half of this document: measured solver
  signatures, selftest/stress gates, environment constraints (no X server,
  read-only `/tmp`, `-lxcb` not emitted by pkg-config) and the coupled
  invariants that have historically broken.

---

## Terms

Abbreviations and jargon used above, in this order: contact mechanics, numerics,
collision detection, graphics, platform and repo vocabulary. Each entry says what
the thing is *here*, not in general.

### Contact mechanics and rigid bodies

* **impulse `J`** — momentum delivered by a collision (`∫F dt`). Linear:
  `Δv = J/m`; applied at a surface point it is also angular: `Δω = (p × J)/I`.
  Measured in mass·px/s, not in newtons.
* **normal `n` / tangent `t`** — the contact directions: along the centre line
  `x_j − x_i`, and perpendicular to it (`t = (−n_y, n_x)`).
* **restitution `e`** — the fraction of the approach speed that survives a
  bounce along `n`: `v_n⁺ = −e·v_n⁻`. `e = 1` elastic, `e = 0` dead.
* **Newton's / Poisson's impact law** — that restitution postulate itself;
  an empirical replacement for solving the actual deformation of the impact.
* **unilateral contact** — a contact that can push but never pull, and that
  exists only while the bodies touch. Enforced by applying impulses only for
  overlap *and* `v_n < 0`.
* **LCP (Linear Complementarity Problem)** — the exact "solve all contacts at
  once" formulation, `0 ≤ λ ⊥ g ≥ 0` (impulse non-negative, gap non-negative,
  one of them zero). Sequential impulses is a cheap relaxation of an LCP, not a
  solution of one.
* **effective mass `k_n`, `k_t`** — how much impulse a contact direction needs
  per unit of velocity change: `k_n = Σ 1/m`, `k_t = Σ 1/m + Σ r²/I`. Impulse =
  `Δv/k`; large `k` = stiff direction.
* **reduced mass `m_eff`** — `1/(1/m_i + 1/m_j)`, the single mass that a
  two-body collision "weighs" along its normal; it multiplies `v_n²` in `ΔKE`.
* **slip velocity `v_t`** — relative **surface** velocity along `t` at the
  contact point (includes spin). `v_t = 0` means the contact grips.
* **Coulomb cone / friction box** — `|j_t| ≤ μ j_n`: tangential impulse is
  bounded by the normal load. A circular cone in 3D; a two-sided **box** here.
* **sticking vs. sliding** — sticking = the Coulomb budget suffices to bring
  `v_t` to 0 (use `j_t = −v_t/k_t`); sliding = it does not, use `j_t = ±μ j_n`.
* **rolling without slipping** — the no-slip state at a floor contact,
  `v·t = ω r`. Absent rolling resistance, it is permanent once reached.
* **rolling resistance / torsional friction** — extra contact moments that would
  brake a rolling or spinning body. **Not modelled here** (added in `../sim2d`).
* **moment of inertia `I`** — rotational mass; for a solid disc `I = ½ m r²`,
  which is why `r²/I = 2/m` everywhere below.
* **Baumgarte stabilization** — feeding a constraint *error* (here: overlap)
  back into the solve as a proportional correction, so errors decay instead of
  persisting. Here it is the explicit position projection with strength
  `CORR_PERCENT = 0.8` (no bias velocity term — see the jitter discussion).
* **slop** — an intentional gap tolerance (`CORR_SLOP = 0.01 px`) below which
  nothing is corrected; without it a contact sitting exactly at `g = 0` gets
  nudged forever.
* **penetration / overlap / de-overlap** — `r_i + r_j − d > 0`, and the act of
  separating the pair.
* **sleeping** — freezing bodies whose motion stays below a threshold for a
  while. Not implemented; that is why a gravity pile never goes quiet.
* **warm starting** — caching the previous substep's impulses per contact and
  starting the next solve from them (Box2D, Dantzig-type solvers). Not
  implemented; would reduce pile jitter and penetration at the cost of contact
  identity tracking across substeps.
* **PBD / XPBD** — (eXtended) Position Based Dynamics: integrate, *project*
  constraint violations in position space, then recompute velocities. The
  sibling `../sim2d` solver; `imp2d` is its impulse-side control.

### Numerics

* **semi-implicit (symplectic) Euler** — update velocity first, then position
  with the *new* velocity. Drifts energy by `O(dt)` instead of pumping it.
* **shadow Hamiltonian** — the modified energy `H + O(dt)` that a symplectic
  integrator really conserves; why an `e = 1` scene holds 100 % energy.
* **fixed timestep / substep** — physics always advances by `PHYS_DT = 1/240 s`;
  one rendered frame runs as many substeps as its real time allows.
* **accumulator** — the leftover real time carried from frame to frame to decide
  that substep count (`acc += d; while (acc ≥ PHYS_DT) …`).
* **spiral of death** — physics slower than real time ⇒ bigger backlog ⇒ more
  substeps ⇒ even slower. Cut off by the `MAX_SUB = 16` budget, which drops the
  backlog instead of paying for it.
* **CFL-like condition** — any step-size bound of the form "nothing may travel
  further than the smallest feature per step". Here `|v|·Δt` versus radius.
* **CCD (continuous collision detection)** — computing *when within the step*
  two bodies first touch (swept test) instead of noticing after the fact.
* **tunnelling** — the failure CCD prevents: two bodies swapping sides inside
  one step, unnoticed.
* **jitter** — persistent small-scale agitation of bodies that should be at
  rest (a velocity-level symptom of a contact solver).
* **residual velocity / coherent drift** — `mean |v|` and `max |v|` of a settled
  pile, and `|Σv|/n` (a drift-like number that separates "everything wobbles in
  place" from "the pile is sliding").
* **bitwise determinism** — same seed, same code path, same machine ⇒
  byte-identical state (`memcmp` of the `Ball` array is a valid test).
* **chaos / Lyapunov growth** — repeated contacts amplify a 1-ulp difference
  exponentially, which is why refactors are gated bitwise only with
  `--spin 0 --mu 0`.
* **ulp** — unit in the last place: the smallest change a float can represent.
* **FMA (fused multiply–add)** — `a*b + c` in one rounding; faster, but a
  *different result*, hence a determinism hazard when compilers introduce it.
* **fp32** — IEEE-754 single precision, `float`, ~7 significant decimal digits.
* **RNG (SplitMix64)** — the deterministic 64-bit pseudo-random generator whose
  state lives in `Sim.rng`; used only at spawn, never inside `sim_step`, so a
  scene is reproducible from `--seed` alone.
* **packing fraction** — covered area / box area. Sets how often the spawn
  rejection sampler succeeds (≈ 0.2 in the stress scenario, comfortably low).

### Collision detection

* **broad phase** — the cheap pass that answers "which pairs *could* be
  touching" (the uniform grid). Never computes a response.
* **narrow phase** — the exact geometry of one candidate pair: `d`, `n`,
  penetration; then the impulses.
* **contact manifold** — the full set of contact points between two bodies.
  Discs need one, so `imp2d` has exactly one per pair; `../sim2d` gets several
  for free by treating bodies as particle sets.
* **uniform grid (spatial hash)** — partition space into equal cells; only the
  same and the 8 adjacent cells can hold a touching body.
* **cell size** — chosen as `2·max_r`, which is what makes the 3×3 ring
  exhaustive (see the argument under Algorithms).
* **half-neighbourhood** — scanning self + the four "forward" cells
  (`(0,0), (1,0), (−1,1), (0,1), (1,1)`), so every neighbouring *cell* pair is
  met exactly once and no pair is tested twice.
* **intrusive linked list** — the link lives in the item: `next[i]` is the next
  ball index in ball `i`'s cell. No allocator, no sort, `O(1)` insert.
* **SAP / sweep-and-prune** — sort bodies by `x` (or space-filling code) and
  test only overlapping intervals; the usual grid alternative, cheaper to update
  incrementally, worse to rebuild.
* **BVH (bounding-volume hierarchy)** — tree of enclosing volumes; the general
  heavy-duty alternative, pointless for equal-size discs.

### Data layout

* **AoS / SoA** — array of structs (one `Ball` holding all its 14 floats) versus
  struct of arrays (one array per field). `imp2d` is AoS because a contact reads
  *every* field of both bodies, so per-body locality wins.
* **cache line** — the 64-byte chunk the CPU fetches at a time. `Ball` is 56 B,
  so a pair costs ~2–3 lines; `next[]` traversal is strided by `sizeof(Ball)`,
  the price of the linked-list buckets.
* **staging buffer** — the CPU-side array refilled each frame and uploaded with
  one `glBufferData` (`Renderer.v`, `Renderer.lines`).
* **sentinel `−1`** — the empty-cell / end-of-list marker in `head[]` and
  `next[]`, which lets a bucket be walked without storing counts; `head[]` must
  be initialised to it at allocation because the renderer reads the grid before
  the first build.

### Graphics and rendering

* **SDF (signed distance field)** — a function returning the signed distance
  from any point to a shape's boundary (negative inside). Here the disc SDF is
  `d = |v_disc|` — distance from the ball centre in units of the radius —
  evaluated per pixel, so a *square* quad becomes a *round* disc by drawing
  only where `d < 1`. Cheap, exact, resolution independent, no textures.
* **MSAA (multi-sample anti-aliasing)** — hardware anti-aliasing that evaluates
  a shape at several sample positions per pixel and averages; requested through
  the EGL config attributes `EGL_SAMPLE_BUFFERS`/`EGL_SAMPLES`. **Not used**:
  an SDF edge can be anti-aliased analytically, which is both sharper and
  cheaper than sampling.
* **`fwidth`** — the GPU's per-pixel derivative of an interpolated value: the
  textbook way to size an anti-aliasing band automatically ("how fast does `d`
  change per pixel?"). GL ES 2 has no derivatives without an extension, so
  `imp2d` derives the band as `1/max(r,1)` — one screen pixel expressed in disc
  units — and computes it in the vertex shader.
* **`smoothstep(a, b, x)`** — GLSL's smooth 0→1 ramp between `a` and `b`;
  used for the rim, the fake lighting and the spin dots.
* **vertex shader (VS) / fragment shader (FS)** — the program run per vertex
  (position, and here the rotation `cos/sin`) and the program run per covered
  pixel (SDF, colour, alpha).
* **`highp` / `mediump`** — GL ES 2 precision qualifiers. FS floats may be as
  narrow as fp16-class, which is why `θ`'s `sin`/`cos` is computed in the
  `highp` VS and only the result is interpolated.
* **attribute / uniform / varying** — a per-vertex input, a per-draw constant,
  and a VS→FS interpolated output.
* **VBO / EBO** — vertex buffer object (GPU-side vertex array) / element buffer
  object (GPU-side index array). One `vbo` is shared by the ball pass and the
  line batcher, which is the coupling `AGENTS.md` warns about.
* **indexed draw call** — a single `glDrawElements` that reuses a static index
  pattern (`{0,1,2, 2,1,3}` per ball) over all balls' vertices, instead of one
  call per ball.
* **batching** — merging many primitives into one draw call (all discs in one;
  all grid lines in one; the aim line in one). Draw calls, not triangles, are
  usually the cost.
* **premultiplied alpha** — storing `rgb·a` rather than `rgb`, so
  `glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA)` gives correct "source over"
  compositing (Porter–Duff) without dark halos.
* **HSL → RGB** — hue/saturation/lightness converted to red/green/blue; the
  mass→colour ramp (`hue = 0.60 − 0.60·t`, saturation 1).
* **FBO / pbuffer / surfaceless** — the three ways to render without a window:
  a GL *framebuffer object* (an off-screen render target, used for
  `glReadPixels`/PPM), an EGL *pixel buffer* surface, or a *surfaceless*
  context with no surface at all (preferred here, `EGL_NO_SURFACE`).
* **`glReadPixels`** — copy the current framebuffer into CPU memory; the basis
  of both the pixel self-tests and the PPM screenshots.
* **PPM (P6)** — the trivial raw image format, `P6\n<w> <h>\n255\n` + RGB
  bytes; the screenshot format so headless output is viewable without a GL.
* **llvmpipe** — Mesa's software rasteriser: what actually draws in
  `--mode off` here (~1900 fps at 30 balls), no GPU involved.

### Platform, API and repo vocabulary

* **GL / OpenGL ES 2** — the graphics API used (`glesv2`): the embedded, no-
  fixed-function profile. Everything "GL" above means ES 2 unless stated; ES 3
  is only attempted opportunistically at context creation.
* **XCB** — X C Bindings, the low-level X11 client library; the only thing
  `imp2d` talks to X with (no Xlib in application code).
* **EGL** — the glue between a GL API and the native window system: pick a
  display, choose a config, create a context and a surface, `eglSwapBuffers`.
* **`EGL_NO_X11`** — define that keeps Xlib types out of the EGL headers so the
  display can be created from an `xcb_connection_t` via
  `EGL_EXT_platform_xcb`.
* **keysym** — a *symbolic* key code (`'q'`, `' '`, `0xFF1B` = Escape) as
  opposed to the hardware keycode X delivers; resolved through the mapping table
  fetched once at startup.
* **`--mode off`** — headless run: surfaceless/pbuffer EGL, an FBO, and exactly
  4 physics substeps per frame so the run is reproducible.
* **selftest (`--selftest`)** — the built-in test suite: physical invariants
  plus `glReadPixels` pixel checks; exit 0 = pass. It *is* the test framework.
* **stress (`--stress`)** — the built-in pile-stability harness (2000 disks,
  gravity, 30 s settle, no GL) reporting penetration/velocity/energy metrics.
* **gate vs. metric** — a **gate** is a pass/fail regression bound checked by
  `check(...)`; a **metric** is a printed number to compare across revisions.
  `AGENTS.md`: when you change solver behaviour, the metrics are the comparison
  — do not "fix" the gates to make a change pass.
* **solver signature** — the characteristic metric set of a given solver
  (impulse here: mean-pen ≈ 0.93 px, jitter ≈ 124 px/s, ≈ 1.3 % KE residual, and
  a gravity pile that never fully sleeps).
* **constants**: `PHYS_DT` 1/240 s substep · `FRAME_DT` 1/60 s frame budget ·
  `MAX_SUB` 16 substeps/frame cap · `ITER` 2 relaxation passes per substep ·
  `CORR_PERCENT` 0.8 positional correction strength · `CORR_SLOP` 0.01 px slack ·
  `SIM_MAX_SPEED` 4000 px/s speed cap · `SIM_MAX_OMEGA` 60 rad/s spin cap ·
  `DEF_CAP` 2000 bodies allocated up front · `GRAV` 1000 px/s² when gravity is
  toggled on · `DEF_MU` 0.2 Coulomb coefficient.
