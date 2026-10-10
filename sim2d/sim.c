/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 * sim.c - physics implementation (see sim.h).
 *
 * Everything is a particle (a disc). Bodies are rigid sets of particles and
 * the solver sees one contact type only: particle against particle, or
 * particle against a wall plane. A one-particle body with its particle at
 * offset (0,0) is the classic billiard disc, and it is bit-for-bit the disc
 * solver that was here before the body/particle split: every lever arm below
 * is written so that it evaluates to exactly zero (disc) or exactly r (wall
 * friction) instead of accumulating round-off through a generic cross
 * product. Verify with `make test` plus
 *   ./sim2d --stress --seed 7      (mean-pen 0.51 px, jitter 63 px/s)
 *
 * Solver: XPBD, position pass then velocity pass.
 *   1. save post-gravity velocity v_pre, integrate a prediction
 *   2. collect contacts (particle-particle, particle-wall) on the predicted
 *      positions; count contacts per BODY for the sparse-impact gate
 *   3. Gauss-Seidel project the position constraints (XPBD, compliance
 *      alpha~ = alpha/dt^2, alpha = 0 rigid) with a per-contact Lagrange
 *      multiplier clamped >= 0; contacts push but never pull. Projection is
 *      rigid: it moves and turns the body through the contact lever arm.
 *   4. velocity restarts from v_pre: position corrections NEVER become
 *      velocity (that leak boils piles - tried and verified the hard way)
 *   5. one-sided velocity impulses on the contacts, iterated as projected
 *      Gauss-Seidel until (nearly) no contact approaches: reflect while
 *      approaching faster than XPBD_REST_THRESH, stop below it (sleep
 *      rule) -> piles jam and reach zero residual velocity
 *   6. Coulomb friction with jn = lam/dt (the support force the position
 *      pass actually applied) + the contact's normal impulse, plus rolling
 *      resistance against the same load
 * Stiffness scales with XPBD_ITER x substeps, both measurable in the stress
 * harness. Deterministic: fixed traversal order, no atomics.
 *
 * Deleted on the way here, both measured, both in git history:
 * - the sequential-impulse solver (~190 lines). It was the default until
 *   bodies existed, but it is strictly worse at resting contact (30 s,
 *   2000 disks: mean-pen 0.93 px / jitter 124 px/s against xpbd's
 *   0.51 / 63) and keeping two solvers doubled the regression matrix.
 *   Cost of the switch: 2x the time per substep.
 * - XPBD_DRAG. A 0.4 1/s linear bleed that looked load-bearing and was not:
 *   at 30 s the pile is identical with and without it (jitter 63 px/s both,
 *   mean-pen 0.51 px both) because XPBD_ROLL is what actually stops the
 *   churn, while the drag destroyed the two properties worth having -
 *   momentum conservation (18% gone in 0.5 s, now 0.000%) and elastic
 *   billiards (0% of KE kept after 10 s, now 100%).
 */

#include "sim.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define SIM_TWO_PI 6.28318531f

#define XPBD_ITER 4       /* Gauss-Seidel position iterations per substep */
#define XPBD_VITER 8      /* velocity sweeps per substep (pass 5). One sweep
                           * is one projected Gauss-Seidel pass over the
                           * one-sided non-penetration constraint; only the
                           * fixed point of that iteration means "no contact
                           * approaches". A single sweep is not enough for a
                           * deep pile - see the note at pass 5. */
#define XPBD_REST_THRESH 16.0f /* px/s: below this no bounce is re-added,
                                 * which is what lets gravity piles sleep;
                                 * must exceed g*dt (~4 px/s at 240 Hz) */
#define XPBD_ROLL 0.15f        /* rolling resistance: torque at a contact
                                * opposing spin, proportional to the SUPPORT
                                * force lam/dt. Only the sustained load counts,
                                * never the impact impulse, and only when the
                                * contact has friction - see particle_roll.
                                * Without it a pile of hard disks is a ball
                                * pit: rolling costs nothing, so the surface
                                * churns forever down slopes. */
#define XPBD_MARGIN 2.0f       /* px: contacts are collected slightly before
                                * touching and projected only when actually
                                * overlapped; the margin catches overlaps a
                                * projection creates against neighbours that
                                * were not in this substep's contact set */
#define GATE_CONTACTS 4        /* a contact reflects (instead of just stopping)
                                * only while both bodies are poorly supported:
                                * a billiard hit, not a jammed pile. Counted
                                * per BODY, not per particle - a box resting
                                * face-down has many particle contacts and is
                                * by definition jammed. */

/* ------------------------------------------------------------------ rng */

static uint64_t splitmix64(uint64_t *st)
{
    uint64_t z = (*st += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

static float frand01(Sim *s)
{
    return (float)((double)(splitmix64(&s->rng) >> 11) * (1.0 / 9007199254740992.0));
}

void sim_set_seed(Sim *s, uint64_t seed)
{
    s->rng = seed ? seed : 0x2545F4914F6CDD1Dull;
}

float sim_rand01(Sim *s)
{
    return frand01(s);
}

/* ---------------------------------------------------------------- colour */

/* Hue ramp from cool (light) to warm (heavy). Normalised on reff in [4,64] px
 * (for a one-particle body reff is exactly its radius). */
static void mass_color(float reff, int mass_exp, float *cr, float *cg, float *cb)
{
    float lo = powf(4.0f, (float)mass_exp);
    float hi = powf(64.0f, (float)mass_exp);
    float t = (powf(reff, (float)mass_exp) - lo) / (hi - lo);
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    float hue = 0.60f - 0.60f * t; /* 0.60 blue -> 0.0 red */
    float p = 1.0f - fabsf(fmodf(hue * 6.0f, 2.0f) - 1.0f);
    float rf = 0, gf = 0, bf = 0;
    int seg = (int)(hue * 6.0f);
    switch (seg) {
    case 0: rf = 1; gf = p; bf = 0; break;
    case 1: rf = p; gf = 1; bf = 0; break;
    case 2: rf = 0; gf = 1; bf = p; break;
    case 3: rf = 0; gf = p; bf = 1; break;
    case 4: rf = p; gf = 0; bf = 1; break;
    default: rf = 1; gf = 0; bf = p; break;
    }
    *cr = 0.25f + 0.70f * rf;
    *cg = 0.25f + 0.70f * gf;
    *cb = 0.25f + 0.70f * bf;
}

/* --------------------------------------------------------------- geometry */

static float cross2(float ax, float ay, float bx, float by)
{
    return ax * by - ay * bx;
}

/* Offset of a particle from its centre of mass, in world axes: the body-frame
 * offset rotated by ang. Exact zero for a centred particle, which is what
 * keeps the disc path free of round-off. */
static void part_off(const Body *b, const Particle *p, float *ox, float *oy)
{
    if (p->ox == 0.0f && p->oy == 0.0f) {
        *ox = 0.0f;
        *oy = 0.0f;
        return;
    }
    *ox = p->ox * b->ca - p->oy * b->sa;
    *oy = p->ox * b->sa + p->oy * b->ca;
}

/* Live world position of a particle. Particle.x/y is only a snapshot (see
 * sim.h), so every contact computation goes through here while bodies are
 * being moved. */
static void part_world(const Body *b, const Particle *p, float *x, float *y)
{
    float ox, oy;
    part_off(b, p, &ox, &oy);
    *x = b->x + ox;
    *y = b->y + oy;
}

/* ------------------------------------------------------------ grid setup */

static void grid_alloc(Sim *s)
{
    int cell = (int)(2.0f * s->max_r);
    if (cell < 8) cell = 8;
    int gw = (int)(s->w / (float)cell) + 2;
    int gh = (int)(s->h / (float)cell) + 2;
    if (gw < 1) gw = 1;
    if (gh < 1) gh = 1;

    if (cell != s->cell || gw != s->gw || gh != s->gh) {
        s->cell = cell;
        s->gw = gw;
        s->gh = gh;
        if (gw * gh > s->head_cap) {
            free(s->head);
            s->head_cap = gw * gh;
            s->head = malloc((size_t)s->head_cap * sizeof *s->head);
            /* keep it consistent before the first grid_build (the renderer
             * may read it to draw the grid) */
            if (s->head)
                for (int i = 0; i < s->head_cap; i++)
                    s->head[i] = -1;
        }
    }
    if (s->p_cap > s->next_cap) {
        free(s->next);
        s->next_cap = s->p_cap;
        s->next = malloc((size_t)s->next_cap * sizeof *s->next);
    }
}

static void grid_build(Sim *s)
{
    grid_alloc(s);
    for (int i = 0; i < s->gw * s->gh; i++) s->head[i] = -1;
    for (int i = 0; i < s->np; i++) {
        int cx = (int)(s->p[i].x / (float)s->cell);
        int cy = (int)(s->p[i].y / (float)s->cell);
        if (cx < 0) cx = 0;
        if (cx >= s->gw) cx = s->gw - 1;
        if (cy < 0) cy = 0;
        if (cy >= s->gh) cy = s->gh - 1;
        int c = cy * s->gw + cx;
        s->next[i] = s->head[c];
        s->head[c] = i;
    }
}

/* Write every particle's world position from its body transform. Called
 * whenever the bodies have moved (after integration, after a position
 * projection that turned something, and on the way out of sim_step so that
 * callers - the renderer, sim_pick - always read a consistent state). */
static void sync_particles(Sim *s)
{
    for (int i = 0; i < s->nb; i++) {
        Body *b = &s->body[i];
        /* one particle at the origin: the body position, no trig */
        if (b->pn == 1 && s->p[b->p0].ox == 0.0f && s->p[b->p0].oy == 0.0f) {
            b->ca = 1.0f;
            b->sa = 0.0f;
            s->p[b->p0].x = b->x;
            s->p[b->p0].y = b->y;
            continue;
        }
        b->ca = cosf(b->ang);
        b->sa = sinf(b->ang);
        for (int k = b->p0; k < b->p0 + b->pn; k++) {
            s->p[k].x = b->x + s->p[k].ox * b->ca - s->p[k].oy * b->sa;
            s->p[k].y = b->y + s->p[k].ox * b->sa + s->p[k].oy * b->ca;
        }
    }
}

/* ------------------------------------------------------------------ core */

void sim_init(Sim *s, int body_cap, float w, float h, float restitution,
              float density, int mass_exp)
{
    memset(s, 0, sizeof *s);
    s->body_cap = body_cap > 0 ? body_cap : 1;
    s->body = calloc((size_t)s->body_cap, sizeof *s->body);
    s->p_cap = s->body_cap < SIM_MAX_PARTICLES ? s->body_cap : SIM_MAX_PARTICLES;
    s->p = calloc((size_t)s->p_cap, sizeof *s->p);
    s->w = w;
    s->h = h;
    s->restitution = restitution;
    s->density = density;
    s->mass_exp = mass_exp;
    s->max_r = 8.0f;
    sim_set_seed(s, 1);
    grid_alloc(s);
}

void sim_free(Sim *s)
{
    free(s->body);
    free(s->p);
    free(s->head);
    free(s->next);
    free(s->pair_buf);
    free(s->wall_buf);
    memset(s, 0, sizeof *s);
}

void sim_clear(Sim *s)
{
    s->nb = 0;
    s->np = 0;
    s->max_r = 8.0f;
    s->collisions = 0;
    s->wall_hits = 0;
}

void sim_resize(Sim *s, float w, float h)
{
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    s->w = w;
    s->h = h;
    /* pull bodies that are now outside back inside */
    for (int i = 0; i < s->nb; i++) {
        Body *b = &s->body[i];
        if (b->x < b->rad) b->x = b->rad;
        if (b->x > w - b->rad) b->x = w - b->rad;
        if (b->y < b->rad) b->y = b->rad;
        if (b->y > h - b->rad) b->y = h - b->rad;
    }
    sync_particles(s);
    grid_alloc(s);
}

float sim_mass(const Sim *s, float r)
{
    return s->density * powf(r, (float)s->mass_exp);
}

/* Derived per-body quantities, from the particle set:
 *   reff   = sqrt(sum r^2)  (mass law and colour ramp; == r for one particle)
 *   m      = density * reff^mass_exp, shared equally by the particles
 *   origin shifted onto the particle centroid, so x/y IS the centre of mass
 *   I      = sum m_p*(|offset|^2) + sum 0.5*m_p*r^2
 *            (= 0.5*m*r^2 for a one-particle body, i.e. a solid disc)
 *   rad    = circumradius, for containment and the sim_pick reject test */
static void body_finish(Sim *s, int i)
{
    Body *b = &s->body[i];
    Particle *pp = s->p + b->p0;

    float sum_r2 = 0.0f;
    for (int k = 0; k < b->pn; k++)
        sum_r2 += pp[k].r * pp[k].r;
    b->reff = sqrtf(sum_r2);
    b->m = sim_mass(s, b->reff);
    b->inv_m = 1.0f / b->m;
    float mp = b->m / (float)b->pn; /* equal particles */

    float cx = 0.0f, cy = 0.0f;
    for (int k = 0; k < b->pn; k++) {
        cx += pp[k].ox;
        cy += pp[k].oy;
    }
    cx /= (float)b->pn;
    cy /= (float)b->pn;

    float I = 0.0f, rad = 0.0f;
    for (int k = 0; k < b->pn; k++) {
        pp[k].ox -= cx;
        pp[k].oy -= cy;
        I += mp * (pp[k].ox * pp[k].ox + pp[k].oy * pp[k].oy) +
             0.5f * mp * pp[k].r * pp[k].r;
        float d = sqrtf(pp[k].ox * pp[k].ox + pp[k].oy * pp[k].oy) + pp[k].r;
        if (d > rad) rad = d;
    }
    b->I = I;
    b->inv_I = 1.0f / I;
    b->rad = rad;
    /* side effect worth remembering: re-centring leaves a one-particle body
     * with its particle at exactly (0,0), which is what lets the solver and
     * the renderer treat "np == nb" as "nothing here can rotate relative to
     * its centre" */
    mass_color(b->reff, s->mass_exp, &b->cr, &b->cg, &b->cb);
}

int sim_add_body(Sim *s, const float *off, const float *rad, int np, float x,
                 float y, float vx, float vy, int shape)
{
    if (s->nb >= s->body_cap || np <= 0 || np > SIM_MAX_PARTICLES) return -1;
    if (s->np + np > SIM_MAX_PARTICLES) return -1;
    if (s->np + np > s->p_cap) {
        /* double UNTIL the whole body fits: a composite body arrives with np
         * particles at once, and a single doubling from a small p_cap is
         * enough for discs (np == 1) and nothing else */
        int cap = s->p_cap > 0 ? s->p_cap : 64;
        while (cap < s->np + np && cap < SIM_MAX_PARTICLES) cap *= 2;
        if (cap > SIM_MAX_PARTICLES) cap = SIM_MAX_PARTICLES;
        if (s->np + np > cap) return -1;
        void *q = realloc(s->p, (size_t)cap * sizeof *s->p);
        if (!q) return -1;
        s->p = q;
        s->p_cap = cap;
    }

    Body *b = &s->body[s->nb];
    int i = s->nb;
    b->p0 = s->np;
    b->pn = np;
    b->shape = shape;
    b->x = x;
    b->y = y;
    b->vx = vx;
    b->vy = vy;
    b->ang = 0.0f;
    b->omega = 0.0f;
    b->contacts = 0;
    for (int k = 0; k < np; k++) {
        Particle *p = &s->p[b->p0 + k];
        p->ox = off ? off[2 * k] : 0.0f;
        p->oy = off ? off[2 * k + 1] : 0.0f;
        p->r = rad[k];
        p->b = i;
        p->x = x + p->ox;
        p->y = y + p->oy;
        if (p->r > s->max_r) s->max_r = p->r;
    }
    body_finish(s, i);
    s->np += np;
    s->nb++;
    sync_particles(s); /* the centroid shift moved the particles */
    return i;
}

int sim_add_disc(Sim *s, float x, float y, float vx, float vy, float r)
{
    if (r <= 0.0f) return -1;
    const float off[2] = { 0.0f, 0.0f };
    return sim_add_body(s, off, &r, 1, x, y, vx, vy, SIM_SHAPE_DISC);
}

static int overlaps(const Sim *s, float x, float y, float rad)
{
    if (x < rad || y < rad || x > s->w - rad || y > s->h - rad) return 1;
    for (int i = 0; i < s->nb; i++) {
        float dx = s->body[i].x - x, dy = s->body[i].y - y;
        float rs = s->body[i].rad + rad;
        if (dx * dx + dy * dy < rs * rs) return 1;
    }
    return 0;
}

int sim_add_random(Sim *s, float rmin, float rmax, float speed, float spin)
{
    if (s->nb >= s->body_cap) return -1;
    for (int attempt = 0; attempt < 64; attempt++) {
        float r = rmin + (rmax - rmin) * frand01(s);
        float x = r + (s->w - 2.0f * r) * frand01(s);
        float y = r + (s->h - 2.0f * r) * frand01(s);
        if (overlaps(s, x, y, r)) continue;
        float ang = 2.0f * 3.14159265f * frand01(s);
        int idx = sim_add_disc(s, x, y, speed * cosf(ang), speed * sinf(ang), r);
        /* only draw the spin RNG when spin is on: keeps frictionless runs
         * byte-identical to the pre-spin code for a given seed */
        if (idx >= 0 && spin > 0.0f)
            s->body[idx].omega = (2.0f * frand01(s) - 1.0f) * 10.0f * spin;
        return idx;
    }
    return -1;
}

/* ------------------------------------------------------------ shape lattices
 * A convex shape becomes a hexagonal packing of equal touching particles:
 * lattice basis (2r, 0) and (r, sqrt(3) r), so neighbours touch and never
 * overlap (the one rule a body must obey).
 *
 * A lattice point is kept when its CENTRE is inside the nominal shape - not
 * when its whole disc fits, which is the obvious rule and wrong: eroding by r
 * loses a ring of material one lattice radius wide, so sum(r_particle^2) - and
 * therefore the mass - drifts with resolution. Measured on a 120x60 box,
 * reff^2/A_nominal was 0.240 at pr 2 but 0.100 at pr 12: the same nominal body
 * weighed 2.7x less when packed coarse. With centres inside, one lattice cell
 * covers 2*sqrt(3)*r^2, so sum(r^2) = 0.289*A_nominal for any r and any convex
 * shape: mass follows the shape, and --pmin/--pmax become a fidelity knob
 * rather than a mass knob.
 *
 * What that costs: the outline straddles the nominal boundary instead of
 * sitting inside it (up to one particle radius proud of it; corners come out
 * rounded to r regardless), so placement bounds must allow for pr - and a neck
 * narrower than about 2r still holds nothing, so 2r stays the minimum feature
 * size.
 */

/* dim[]: BOX {w, h}, TRI {circumradius}, CAPSULE {segment length, radius}. */
static int lattice_inside(int shape, const float *dim, float x, float y)
{
    switch (shape) {
    case SIM_SHAPE_BOX: {
        float hx = dim[0] * 0.5f, hy = dim[1] * 0.5f;
        return x >= -hx && x <= hx && y >= -hy && y <= hy;
    }
    case SIM_SHAPE_TRI: {
        /* equilateral, vertices at R*(cos,sin) of -90, 30, 150 deg. Edge i has
         * outward normal -V_i and support equal to the apothem R/2, so the
         * inside test is dot(V_i, p) >= -R/2. */
        float a = dim[0] * 0.5f;
        for (int i = 0; i < 3; i++) {
            float th = -1.5707963f + 2.0943951f * (float)i;
            if (x * cosf(th) + y * sinf(th) < -a) return 0;
        }
        return 1;
    }
    case SIM_SHAPE_CAPSULE: {
        float half = dim[0] * 0.5f, R = dim[1];
        float qx = fabsf(x) > half ? fabsf(x) - half : 0.0f;
        return qx * qx + y * y <= R * R;
    }
    default:
        return 0; /* discs are single particles and never reach a lattice */
    }
}

/* Half extents of the nominal shape, to bound the lattice scan. */
static void lattice_bounds(int shape, const float *dim, float *bx, float *by)
{
    switch (shape) {
    case SIM_SHAPE_BOX: *bx = dim[0] * 0.5f; *by = dim[1] * 0.5f; break;
    case SIM_SHAPE_TRI: *bx = dim[0] * 0.866f; *by = dim[0]; break;
    default: *bx = dim[0] * 0.5f + dim[1]; *by = dim[1]; break;
    }
}

/* Returns the particle count, 0 if pr cannot fit the shape at all, or
 * -needed when the count exceeds maxn (so the caller knows which way to
 * resize the lattice). */
static int lattice_fill(int shape, const float *dim, float pr, float *off,
                        int maxn)
{
    float bx, by;
    lattice_bounds(shape, dim, &bx, &by);
    const float dx = 2.0f * pr, dy = 1.7320508f * pr;
    int need = 0, n = 0;
    for (int j = -(int)(by / dy) - 1; j <= (int)(by / dy) + 1; j++) {
        float y = (float)j * dy;
        float xoff = (j & 1) ? pr : 0.0f;
        for (int i = -(int)(bx / dx) - 2; i <= (int)(bx / dx) + 2; i++) {
            float x = xoff + (float)i * dx;
            if (!lattice_inside(shape, dim, x, y)) continue;
            if (n < maxn) {
                off[2 * n] = x;
                off[2 * n + 1] = y;
            }
            n++;
        }
    }
    need = n;
    return n > maxn ? -need : n;
}

static int add_lattice(Sim *s, int shape, const float *dim, float x, float y,
                       float vx, float vy, float pr, float tilt)
{
    float off[SIM_MAX_PARTS_PER_BODY * 2];
    float rad[SIM_MAX_PARTS_PER_BODY];
    int n = 0;
    /* Two ways to fail, so two ways to fix: too fine for the per-body budget
     * (coarsen) and too coarse to hold a single particle (refine). A coarse
     * packing is still an honest - just chunkier and more rounded - version
     * of the shape, so this never refuses on account of count. */
    for (int attempt = 0; attempt < 16; attempt++) {
        n = lattice_fill(shape, dim, pr, off, SIM_MAX_PARTS_PER_BODY);
        if (n > 0) break;
        pr *= (n == 0 ? 0.7f : 1.35f);
    }
    if (n <= 0) return -1;

    float cr = cosf(tilt), sr = sinf(tilt); /* bake the spawn orientation in;
                                             * ang stays 0 and body_finish owns
                                             * the centre of mass from here on */
    for (int k = 0; k < n; k++) {
        float ox = off[2 * k], oy = off[2 * k + 1];
        off[2 * k] = ox * cr - oy * sr;
        off[2 * k + 1] = ox * sr + oy * cr;
        rad[k] = pr;
    }
    return sim_add_body(s, off, rad, n, x, y, vx, vy, shape);
}

int sim_add_box(Sim *s, float x, float y, float vx, float vy, float bw, float bh,
                float pr)
{
    if (bw <= 0.0f || bh <= 0.0f || pr <= 0.0f) return -1;
    const float dim[2] = { bw, bh };
    return add_lattice(s, SIM_SHAPE_BOX, dim, x, y, vx, vy, pr, 0.0f);
}

int sim_add_tri(Sim *s, float x, float y, float vx, float vy, float R, float pr)
{
    if (R <= 0.0f || pr <= 0.0f) return -1;
    const float dim[2] = { R, 0.0f };
    return add_lattice(s, SIM_SHAPE_TRI, dim, x, y, vx, vy, pr, 0.0f);
}

int sim_add_capsule(Sim *s, float x, float y, float vx, float vy, float L, float R,
                    float pr)
{
    if (R <= 0.0f || pr <= 0.0f || L < 0.0f) return -1;
    const float dim[2] = { L, R };
    return add_lattice(s, SIM_SHAPE_CAPSULE, dim, x, y, vx, vy, pr, 0.0f);
}

/* Thinnest feature the shape has: the lattice radius must stay well under it
 * or the packing degenerates (measured, over a 60x30 box: pr = 0.25*thickness
 * still holds two rows, pr = 0.3*sz leaves a single particle and a triangle
 * with it). */
static float shape_min_feature(int shape, const float *dim)
{
    switch (shape) {
    case SIM_SHAPE_BOX: return dim[0] < dim[1] ? dim[0] : dim[1];
    case SIM_SHAPE_TRI: return dim[0]; /* 2 * apothem */
    default: return 2.0f * dim[1]; /* capsule: the cap diameter */
    }
}

/* Placement bound for a shape of nominal size sz: an upper bound over whatever
 * aspect ratio and lattice radius the draws inside sim_add_sized produce (pr is
 * capped at 0.2*sz there, and the outermost particle sticks out by pr past the
 * last centre that passed the inside test), so the caller can pick a spot
 * before the shape exists. */
static float shape_nominal_bound(int shape, float sz)
{
    switch (shape) {
    case SIM_SHAPE_BOX: return 1.62f * sz; /* hypot(2sz,2sz)/2 + pr */
    case SIM_SHAPE_TRI: return 1.6f * sz;
    default: return 1.6f * sz; /* 1.4sz/2 + 0.7sz + pr */
    }
}

/* Nominal dimensions of a shape grown from nominal size sz, and the lattice
 * radius to pack it with. sz is "the radius a disc of similar bulk would
 * have", so the four shapes stay comparable to each other and to the old
 * ball scenes. DIMS ARE RULES, not tunables: box 2sz x 1.4sz (aspect drawn
 * 0.5..1 so piles jam less than identical planks would), triangle
 * circumradius 1.4sz, capsule straight part 1.4sz with 0.7sz end caps. */
static void shape_dims(Sim *s, int shape, float sz, float *dim)
{
    switch (shape) {
    case SIM_SHAPE_BOX:
        dim[0] = 2.0f * sz;
        dim[1] = 2.0f * sz * (0.5f + 0.5f * frand01(s)); /* planks to squares */
        break;
    case SIM_SHAPE_TRI:
        dim[0] = 1.4f * sz;
        dim[1] = 0.0f;
        break;
    default: /* CAPSULE */
        dim[0] = 1.4f * sz; /* straight part */
        dim[1] = 0.7f * sz; /* end caps */
        break;
    }
}

int sim_add_sized(Sim *s, int shape, float x, float y, float vx, float vy,
                  float sz, float pmin, float pmax)
{
    if (sz <= 0.0f) return -1;
    if (shape == SIM_SHAPE_DISC) return sim_add_disc(s, x, y, vx, vy, sz);
    if (s->nb >= s->body_cap) return -1;

    float dim[2];
    shape_dims(s, shape, sz, dim);

    /* Lattice radius: drawn per body, then ceilinged and floored. Per body is
     * what stops two composite bodies gear-meshing (an equal-radius raft sinks
     * (2-sqrt3)*r into its neighbour, which reads as ratcheting and never
     * settles); the ceilings keep the packing from degenerating - past ~0.25
     * of the thinnest feature there is nowhere for a second particle to sit. */
    float pr = pmin + (pmax - pmin) * frand01(s);
    float cap = 0.2f * sz;
    float mf = shape_min_feature(shape, dim);
    if (0.25f * mf < cap)
        cap = 0.25f * mf;
    if (pr > cap)
        pr = cap;
    if (pr < SIM_MIN_PARTICLE)
        pr = SIM_MIN_PARTICLE;

    return add_lattice(s, shape, dim, x, y, vx, vy, pr,
                       2.0f * 3.14159265f * frand01(s)); /* random orientation */
}

int sim_add_random_shape(Sim *s, int shape, float smin, float smax, float pmin,
                         float pmax, float speed, float spin)
{
    /* Discs keep their own code path AND its RNG draw order, so existing seeds
     * and the pre-shape stress baseline stay bit-identical. */
    if (shape == SIM_SHAPE_DISC) return sim_add_random(s, smin, smax, speed, spin);
    if (s->nb >= s->body_cap) return -1;

    for (int attempt = 0; attempt < 64; attempt++) {
        float sz = smin + (smax - smin) * frand01(s);
        float bound = shape_nominal_bound(shape, sz);
        if (bound > s->w * 0.5f || bound > s->h * 0.5f) continue;
        float x = bound + (s->w - 2.0f * bound) * frand01(s);
        float y = bound + (s->h - 2.0f * bound) * frand01(s);
        if (overlaps(s, x, y, bound)) continue;

        float ang = 2.0f * 3.14159265f * frand01(s);
        int idx = sim_add_sized(s, shape, x, y, speed * cosf(ang),
                                speed * sinf(ang), sz, pmin, pmax);
        if (idx < 0) continue;
        if (spin > 0.0f)
            s->body[idx].omega = (2.0f * frand01(s) - 1.0f) * 10.0f * spin;
        return idx;
    }
    return -1;
}

/* Bodies stay contiguous, and so do the particle runs they point at: drop
 * this body's particles, shift the later runs down, re-index owners. */
void sim_remove(Sim *s, int i)
{
    if (i < 0 || i >= s->nb) return;
    Body *b = &s->body[i];
    memmove(&s->p[b->p0], &s->p[b->p0 + b->pn],
            (size_t)(s->np - b->p0 - b->pn) * sizeof *s->p);
    s->np -= b->pn;
    for (int k = i + 1; k < s->nb; k++)
        s->body[k].p0 -= b->pn;
    memmove(b, b + 1, (size_t)(s->nb - i - 1) * sizeof *s->body);
    s->nb--;

    for (int k = 0; k < s->nb; k++)
        for (int q = s->body[k].p0; q < s->body[k].p0 + s->body[k].pn; q++)
            s->p[q].b = k;

    s->max_r = 8.0f;
    for (int k = 0; k < s->np; k++)
        if (s->p[k].r > s->max_r) s->max_r = s->p[k].r;
}

/* --------------------------------------------------------- contact frame */
/* Everything both solve passes need about one contact, from the particle
 * offsets and the unit normal. n points from A to B (particle-particle) or
 * inward (particle-wall).
 *
 * The contact point is on the rims, q = pA + rA*n = pB - rB*n, so its offset
 * from a centre of mass is (particle offset) +/- r*n. The two lever arms are
 * therefore cross(offset, n) for the normal impulse and
 * cross(offset, t) +/- r for the tangential one. Writing them that way
 * instead of cross(q, n) is deliberate: for a one-particle body (offset 0)
 * the normal lever is then EXACTLY zero and the tangential lever EXACTLY
 * +/- r, which is what keeps the disc path bit-identical to the pre-body
 * solver instead of merely close. */
typedef struct {
    float nx, ny, tx, ty;
    float cnA, cnB; /* normal impulse lever arms */
    float ctA, ctB; /* tangential (friction) lever arms */
} Cont;

static void cont_pair(Cont *c, const Body *A, const Body *B, const Particle *pi,
                      const Particle *pj, float nxx, float nyy)
{
    float oax, oay, obx, oby;
    part_off(A, pi, &oax, &oay);
    part_off(B, pj, &obx, &oby);
    c->nx = nxx;
    c->ny = nyy;
    c->tx = -nyy;
    c->ty = nxx;
    c->cnA = cross2(oax, oay, nxx, nyy);
    c->cnB = cross2(obx, oby, nxx, nyy);
    c->ctA = cross2(oax, oay, c->tx, c->ty) + pi->r;
    c->ctB = cross2(obx, oby, c->tx, c->ty) - pj->r;
}

/* A wall is a body with inv_m = inv_I = 0: the contact is on the particle's
 * inner rim, hence the -r tangential lever (and no B side). */
static void cont_wall(Cont *c, const Body *b, const Particle *p, float nxx,
                      float nyy)
{
    float oax, oay;
    part_off(b, p, &oax, &oay);
    c->nx = nxx;
    c->ny = nyy;
    c->tx = -nyy;
    c->ty = nxx;
    c->cnA = cross2(oax, oay, nxx, nyy);
    c->cnB = 0.0f;
    c->ctA = cross2(oax, oay, c->tx, c->ty) - p->r;
    c->ctB = 0.0f;
}

/* Approach speed of a contact: relative velocity of the two contact points
 * along n. Written as the linear part plus omega x lever (rather than by
 * building the two surface velocities) so that a disc, whose rim normal lever
 * is zero, gets the plain linear term. */
static float cont_vn(const Cont *c, const Body *A, const Body *B)
{
    return (B->vx - A->vx) * c->nx + (B->vy - A->vy) * c->ny +
           B->omega * c->cnB - A->omega * c->cnA;
}

/* ---------------------------------------------------------------- friction */

/* Coulomb friction impulse along the contact tangent. jn = magnitude of the
 * normal impulse at this contact (momentum). Internal to the pair, so linear
 * momentum is conserved exactly. */
static void pair_friction(Body *A, Body *B, const Cont *c, float mu, float jn)
{
    if (mu <= 0.0f || jn <= 0.0f) return;
    float vt = (B->vx - A->vx) * c->tx + (B->vy - A->vy) * c->ty +
               B->omega * c->ctB - A->omega * c->ctA;
    float K = A->inv_m + B->inv_m + c->ctA * c->ctA * A->inv_I +
              c->ctB * c->ctB * B->inv_I;
    float jt = -vt / K; /* stick the contact */
    float jmax = mu * jn;
    if (jt > jmax) jt = jmax;
    else if (jt < -jmax) jt = -jmax;
    /* impulse +jt*t on B, -jt*t on A */
    A->vx -= jt * A->inv_m * c->tx;
    A->vy -= jt * A->inv_m * c->ty;
    B->vx += jt * B->inv_m * c->tx;
    B->vy += jt * B->inv_m * c->ty;
    A->omega -= jt * c->ctA * A->inv_I;
    B->omega += jt * c->ctB * B->inv_I;
}

/* Same, against a static wall. */
static void wall_friction(Body *b, const Cont *c, float mu, float jn)
{
    if (mu <= 0.0f || jn <= 0.0f) return;
    float vt = b->vx * c->tx + b->vy * c->ty + b->omega * c->ctA;
    float denom = b->inv_m + c->ctA * c->ctA * b->inv_I;
    float jt = -vt / denom; /* stick the contact */
    float jmax = mu * jn;   /* Coulomb limit */
    if (jt > jmax) jt = jmax;
    else if (jt < -jmax) jt = -jmax;
    b->vx += jt * b->inv_m * c->tx;
    b->vy += jt * b->inv_m * c->ty;
    b->omega += c->ctA * jt * b->inv_I;
}

/* Rolling resistance: torque = XPBD_ROLL * r * N, N = the SUPPORT force this
 * contact carries (lam/dt), bleed off spin (clamped so it can never reverse
 * it). The lever is the rolling particle's own radius: in a composite body
 * the particles cannot roll independently, so the load only has to turn the
 * whole body a little.
 *
 * Two gates, both learned from "balls stop spinning after a collision":
 *   - friction <= 0: a frictionless contact cannot transmit torque, and spin
 *     must survive it untouched. That is an exact invariant (the old impulse
 *     solver had it; xpbd did not: mu=0, spin 20 -> 0 in 2.5 s).
 *   - the load is lam/dt, NOT lam/dt + jr. An impact is not a sustained
 *     normal force; feeding the collision impulse in here meant every bounce
 *     took a chunk of spin (wall hit: ~9 rad/s of 20 gone at r=20, v=300),
 *     which is the other half of the same complaint. Coulomb friction does
 *     legitimately convert spin to slide during a hit - that path stays. */
static void particle_roll(Sim *s, Body *b, float r, float support)
{
    if (s->friction <= 0.0f || support <= 0.0f || b->omega == 0.0f) return;
    float dw = XPBD_ROLL * support * r * b->inv_I;
    if (dw > fabsf(b->omega)) dw = fabsf(b->omega);
    b->omega += b->omega > 0.0f ? -dw : dw;
}

/* --------------------------------------------------------------- contacts */

typedef struct { int i, j; float lam; } PCpair; /* particle indices */
typedef struct { int i; float nx, ny, c0, lam; } PCwall; /* n.p + c0 = dist */

#define PPC(s) ((PCpair *)(s)->pair_buf)
#define PWC(s) ((PCwall *)(s)->wall_buf)

static int pc_pair_add(Sim *s, int i, int j)
{
    if (s->np_pair >= s->pair_cap) {
        int cap = s->pair_cap ? s->pair_cap * 2 : 256;
        void *p = realloc(s->pair_buf, (size_t)cap * sizeof(PCpair));
        if (!p) return 0;
        s->pair_buf = p;
        s->pair_cap = cap;
    }
    PPC(s)[s->np_pair].i = i;
    PPC(s)[s->np_pair].j = j;
    PPC(s)[s->np_pair].lam = 0.0f;
    s->np_pair++;
    return 1;
}

static int pc_wall_add(Sim *s, int i, float nx, float ny, float c0)
{
    if (s->np_wall >= s->wall_cap) {
        int cap = s->wall_cap ? s->wall_cap * 2 : 256;
        void *p = realloc(s->wall_buf, (size_t)cap * sizeof(PCwall));
        if (!p) return 0;
        s->wall_buf = p;
        s->wall_cap = cap;
    }
    PWC(s)[s->np_wall].i = i;
    PWC(s)[s->np_wall].nx = nx;
    PWC(s)[s->np_wall].ny = ny;
    PWC(s)[s->np_wall].c0 = c0;
    PWC(s)[s->np_wall].lam = 0.0f;
    s->np_wall++;
    return 1;
}

/* Half neighbourhood: own cell (pairs taken once via j > i) plus the four
 * "forward" cells, which together cover every adjacent cell pair exactly
 * once. Contact distance is at most 2*max_r == one cell, so that is enough.
 * Two particles of the same body never collide; the several particle pairs a
 * body pair produces in one scan ARE its contact manifold. */
static const int OFF_X[5] = { 0, 1, -1, 0, 1 };
static const int OFF_Y[5] = { 0, 0, 1, 1, 1 };

static void collect(Sim *s)
{
    s->np_pair = 0;
    s->np_wall = 0;
    for (int i = 0; i < s->nb; i++)
        s->body[i].contacts = 0;
    grid_build(s);

    for (int cy = 0; cy < s->gh; cy++) {
        for (int cx = 0; cx < s->gw; cx++) {
            for (int i = s->head[cy * s->gw + cx]; i >= 0; i = s->next[i]) {
                const Particle *pi = &s->p[i];
                for (int k = 0; k < 5; k++) {
                    int nx = cx + OFF_X[k];
                    int ny = cy + OFF_Y[k];
                    if (nx < 0 || ny < 0 || nx >= s->gw || ny >= s->gh) continue;
                    for (int j = s->head[ny * s->gw + nx]; j >= 0; j = s->next[j]) {
                        if (k == 0 && j <= i) continue; /* each pair once */
                        const Particle *pj = &s->p[j];
                        if (pi->b == pj->b) continue; /* rigidly joined */
                        float dx = pj->x - pi->x, dy = pj->y - pi->y;
                        float rs = pi->r + pj->r + XPBD_MARGIN;
                        float d2 = dx * dx + dy * dy;
                        if (d2 >= rs * rs || d2 <= 0.0f) continue;
                        if (!pc_pair_add(s, i, j)) return;
                        s->body[pi->b].contacts++;
                        s->body[pj->b].contacts++;
                        /* the printed contact counter, on centre velocity
                         * exactly as before the body/particle split (the
                         * solver itself uses cont_vn) */
                        const Body *ba = &s->body[pi->b], *bb = &s->body[pj->b];
                        float d = sqrtf(d2);
                        float vn = (bb->vpx - ba->vpx) * dx / d +
                                   (bb->vpy - ba->vpy) * dy / d;
                        if (vn < 0.0f) s->collisions++;
                    }
                }
            }
        }
    }

    for (int i = 0; i < s->np; i++) {
        const Particle *p = &s->p[i];
        const Body *b = &s->body[p->b];
        float ox, oy;
        part_off(b, p, &ox, &oy);
        /* n is the INWARD normal, so a hit is v . n < 0 at the rim - which is
         * how a coming-down corner of a body registers too */
#define WALL_VN(NX, NY)                                                        \
    ((NX) * b->vpx + (NY) * b->vpy + b->omega * cross2(ox, oy, NX, NY))
        if (p->x - p->r < XPBD_MARGIN &&
            pc_wall_add(s, i, 1.0f, 0.0f, 0.0f) && WALL_VN(1.0f, 0.0f) < 0.0f)
            s->wall_hits++;
        else if (p->x + p->r > s->w - XPBD_MARGIN &&
                 pc_wall_add(s, i, -1.0f, 0.0f, s->w) &&
                 WALL_VN(-1.0f, 0.0f) < 0.0f)
            s->wall_hits++;
        if (p->y - p->r < XPBD_MARGIN &&
            pc_wall_add(s, i, 0.0f, 1.0f, 0.0f) && WALL_VN(0.0f, 1.0f) < 0.0f)
            s->wall_hits++;
        else if (p->y + p->r > s->h - XPBD_MARGIN &&
                 pc_wall_add(s, i, 0.0f, -1.0f, s->h) &&
                 WALL_VN(0.0f, -1.0f) < 0.0f)
            s->wall_hits++;
#undef WALL_VN
    }
}

/* ---------------------------------------------------------- solve: pass 3 */
/* Gauss-Seidel projection of the non-penetration constraints. Rigid: a
 * projection at an off-centre particle both shifts and turns the body, and
 * the multiplier is clamped >= 0 so contacts push but never attract. */
static void project(Sim *s, float at)
{
    PCpair *pc = PPC(s);
    PCwall *wc = PWC(s);

    for (int it = 0; it < XPBD_ITER; it++) {
        int turned = 0;
        for (int c = 0; c < s->np_pair; c++) {
            Particle *pi = &s->p[pc[c].i], *pj = &s->p[pc[c].j];
            Body *A = &s->body[pi->b], *B = &s->body[pj->b];
            float pix, piy, pjx, pjy;
            part_world(A, pi, &pix, &piy);
            part_world(B, pj, &pjx, &pjy);
            float dx = pjx - pix, dy = pjy - piy;
            float d2 = dx * dx + dy * dy;
            if (d2 <= 0.0f) continue;
            float d = sqrtf(d2);
            float C = d - (pi->r + pj->r); /* <= 0 desired */
            if (C > 0.0f) { /* separated: release, contacts never pull */
                pc[c].lam = 0.0f;
                continue;
            }
            float nxx = dx / d, nyy = dy / d;
            Cont k;
            cont_pair(&k, A, B, pi, pj, nxx, nyy);
            float dl = (-C - at * pc[c].lam) /
                       (A->inv_m + B->inv_m + A->inv_I * k.cnA * k.cnA +
                        B->inv_I * k.cnB * k.cnB + at);
            float lam2 = pc[c].lam + dl;
            if (lam2 < 0.0f) lam2 = 0.0f; /* push only, never attract */
            dl = lam2 - pc[c].lam;
            pc[c].lam = lam2;
            A->x -= dl * A->inv_m * nxx;
            A->y -= dl * A->inv_m * nyy;
            B->x += dl * B->inv_m * nxx;
            B->y += dl * B->inv_m * nyy;
            A->ang -= dl * A->inv_I * k.cnA; /* exactly 0 for a disc */
            B->ang += dl * B->inv_I * k.cnB;
            if (k.cnA != 0.0f || k.cnB != 0.0f) turned = 1;
        }
        for (int c = 0; c < s->np_wall; c++) {
            Particle *p = &s->p[wc[c].i];
            Body *b = &s->body[p->b];
            float px, py;
            part_world(b, p, &px, &py);
            float dist = wc[c].nx * px + wc[c].ny * py + wc[c].c0;
            float C = dist - p->r; /* <= 0 desired */
            if (C > 0.0f) { /* separated: release, walls never pull */
                wc[c].lam = 0.0f;
                continue;
            }
            Cont k;
            cont_wall(&k, b, p, wc[c].nx, wc[c].ny);
            float dl = (-C - at * wc[c].lam) /
                       (b->inv_m + b->inv_I * k.cnA * k.cnA + at);
            float lam2 = wc[c].lam + dl;
            if (lam2 < 0.0f) lam2 = 0.0f;
            dl = lam2 - wc[c].lam;
            wc[c].lam = lam2;
            b->x += dl * b->inv_m * wc[c].nx; /* inward */
            b->y += dl * b->inv_m * wc[c].ny;
            b->ang += dl * b->inv_I * k.cnA;
            if (k.cnA != 0.0f) turned = 1;
        }
        /* the projection turned bodies: their particles must sit where the
         * bodies now are before the next sweep measures anything */
        if (turned)
            sync_particles(s);
    }
}

/* ------------------------------------------------------ solve: passes 5+6 */
/* Sequential velocity impulses over the same contacts: bounce while
 * approaching faster than XPBD_REST_THRESH, inelastic stop below it (the
 * sleep rule), one-sided (a contact can push but never pull). Uses the
 * *current* sequential vn, so opposing contacts cancel instead of ratcheting
 * on stale approach targets. The position pass above is what removes
 * penetration; this pass is what makes the pile sleep.
 *
 * The sweep is iterated XPBD_VITER times, and that is what keeps a dense bed
 * calm. A single sweep is one Gauss-Seidel pass: stopping a late contact
 * shoves bodies back toward contacts already processed, so in a jammed
 * contact graph residual approach velocity always survives. That residue
 * becomes penetration next substep, the bed keeps collapsing, and gravity
 * feeding the collapse is the motor that kept the old xpbd bed boiling
 * (measured: centre of mass sinking at a steady ~136 px/s forever, ~26%
 * residual KE). Iterating to (nearly) the fixed point "no contact approaches"
 * jams the bed and shuts the feed off: same 2000-disk scenario, mean-pen
 * 1.45 -> 0.48 px, jitter 272 -> 61 px/s, residual KE 25.8% -> 1.4%.
 * Diminishing returns are clean: 4 -> 136, 8 -> 61, 16 -> 42 px/s jitter,
 * cost proportional. */
static void impulse_sweeps(Sim *s, float dt)
{
    float e = s->restitution;
    float mu = s->friction;
    PCpair *pc = PPC(s);
    PCwall *wc = PWC(s);

    for (int vp = 0; vp < XPBD_VITER; vp++) {
        for (int c = 0; c < s->np_pair; c++) {
            Particle *pi = &s->p[pc[c].i], *pj = &s->p[pc[c].j];
            Body *A = &s->body[pi->b], *B = &s->body[pj->b];
            float pix, piy, pjx, pjy;
            part_world(A, pi, &pix, &piy);
            part_world(B, pj, &pjx, &pjy);
            float dx = pjx - pix, dy = pjy - piy;
            float d2 = dx * dx + dy * dy;
            if (d2 <= 0.0f) continue;
            float d = sqrtf(d2);
            float nxx = dx / d, nyy = dy / d;
            Cont k;
            cont_pair(&k, A, B, pi, pj, nxx, nyy);
            float vn = cont_vn(&k, A, B);
            float jr = 0.0f;
            if (vn < 0.0f) {
                /* Reflect only genuine, sparse impacts (a billiard hit: both
                 * bodies poorly supported). Reflecting simultaneously across
                 * a jammed contact graph is a Fermi accelerator: opposing
                 * contacts bounce the same body against each other and the
                 * pile boils. In jammed regions the contact just stops
                 * (inelastic), which is both stable and what a dense pile
                 * should do. Jammed-ness is counted per BODY: a box resting
                 * face-down has a dozen particle contacts and is jammed by
                 * definition, which is exactly the intent. */
                int sparse = A->contacts + B->contacts <= GATE_CONTACTS;
                float jimp =
                    sparse && vn < -XPBD_REST_THRESH ? -(1.0f + e) * vn : -vn;
                jimp /= A->inv_m + B->inv_m + A->inv_I * k.cnA * k.cnA +
                        B->inv_I * k.cnB * k.cnB;
                A->vx -= jimp * A->inv_m * nxx;
                A->vy -= jimp * A->inv_m * nyy;
                B->vx += jimp * B->inv_m * nxx;
                B->vy += jimp * B->inv_m * nyy;
                A->omega -= jimp * k.cnA * A->inv_I;
                B->omega += jimp * k.cnB * B->inv_I;
                jr = fabsf(jimp);
            }
            /* friction rides along, with jn = support impulse lam/dt plus
             * this contact's normal impulse */
            float jn = pc[c].lam / dt + jr;
            pair_friction(A, B, &k, mu, jn);
            particle_roll(s, A, pi->r, pc[c].lam / dt);
            particle_roll(s, B, pj->r, pc[c].lam / dt);
        }
        for (int c = 0; c < s->np_wall; c++) {
            Particle *p = &s->p[wc[c].i];
            Body *b = &s->body[p->b];
            float nx = wc[c].nx, ny = wc[c].ny;
            Cont k;
            cont_wall(&k, b, p, nx, ny); /* offsets from the live transform */
            float vn = b->vx * nx + b->vy * ny + b->omega * k.cnA;
            float jr = 0.0f;
            if (vn < 0.0f) {
                float want = vn < -XPBD_REST_THRESH ? -(1.0f + e) * vn : -vn;
                float den = b->inv_m + k.cnA * k.cnA * b->inv_I;
                /* cn == 0: the normal goes through the centre of mass, no
                 * spin is involved, and the whole velocity change is the
                 * wanted one (no divide/multiply round trip - this is also
                 * what keeps the disc numbers exact) */
                if (k.cnA == 0.0f) {
                    b->vx += want * nx;
                    b->vy += want * ny;
                    jr = want * b->m;
                } else {
                    float j = want / den;
                    b->vx += j * b->inv_m * nx;
                    b->vy += j * b->inv_m * ny;
                    b->omega += k.cnA * j * b->inv_I;
                    jr = j;
                }
            }
            float jn = wc[c].lam / dt + jr;
            wall_friction(b, &k, mu, jn);
            particle_roll(s, b, p->r, wc[c].lam / dt);
        }
    }
}

/* omega clamp + angle integration/wrap (wrap keeps mediump shaders happy) */
static void step_ang(Body *b, float dt)
{
    if (b->omega > SIM_MAX_OMEGA)
        b->omega = SIM_MAX_OMEGA;
    else if (b->omega < -SIM_MAX_OMEGA)
        b->omega = -SIM_MAX_OMEGA;
    b->ang += b->omega * dt;
    if (b->ang >= SIM_TWO_PI || b->ang < 0.0f)
        b->ang = fmodf(b->ang, SIM_TWO_PI) +
                 ((b->ang < 0.0f) ? SIM_TWO_PI : 0.0f);
}

static void cap_speed(Body *b)
{
    float sp2 = b->vx * b->vx + b->vy * b->vy;
    if (sp2 > SIM_MAX_SPEED * SIM_MAX_SPEED) {
        float k = SIM_MAX_SPEED / sqrtf(sp2);
        b->vx *= k;
        b->vy *= k;
    }
}

void sim_step(Sim *s, float dt)
{
    float at = s->compliance / (dt * dt); /* alpha tilde */

    /* 1. predict (gravity only; nothing here may leak position into
     * velocity - see the note in the velocity pass) */
    for (int i = 0; i < s->nb; i++) {
        Body *b = &s->body[i];
        b->vy += s->gravity * dt;
        cap_speed(b);
        b->xp = b->x;
        b->yp = b->y;
        b->vpx = b->vx;
        b->vpy = b->vy;
        b->x += b->vx * dt;
        b->y += b->vy * dt;
    }
    sync_particles(s);

    /* 2. contacts on the predicted positions */
    collect(s);

    /* 3. position solve */
    project(s, at);

    /* 4. reset velocity to the pre-solve state: projections must never
     * become velocity. Deriving v from the position displacement or from the
     * accumulated lam double-pays overlaps that other contacts created
     * during the sweep, and that positive feedback boils dense piles. All
     * velocity change comes from the impulse sweeps below. */
    for (int i = 0; i < s->nb; i++) {
        Body *b = &s->body[i];
        b->vx = b->vpx;
        b->vy = b->vpy;
    }
    /* the projection can have turned bodies: refresh the snapshots the
     * impulse sweeps measure lever arms from. Skipped when every body is a
     * centred disc (np == nb implies that, because body_finish re-centres
     * offsets, so rotation cannot matter there and the pass is a no-op) */
    if (s->np != s->nb)
        sync_particles(s);

    /* 5. sequential impulses, 6. friction and rolling resistance */
    impulse_sweeps(s, dt);

    for (int i = 0; i < s->nb; i++)
        step_ang(&s->body[i], dt);
    sync_particles(s); /* unconditional: Sim.p must be current on the way out,
                        * the projection moved bodies after the last sync */
}

/* ----------------------------------------------------------------- stats */

float sim_kinetic(const Sim *s)
{
    float ke = 0.0f;
    for (int i = 0; i < s->nb; i++)
        ke += 0.5f * s->body[i].m *
                      (s->body[i].vx * s->body[i].vx + s->body[i].vy * s->body[i].vy) +
              0.5f * s->body[i].I * s->body[i].omega * s->body[i].omega;
    return ke;
}

void sim_momentum(const Sim *s, float *px, float *py)
{
    float x = 0.0f, y = 0.0f;
    for (int i = 0; i < s->nb; i++) {
        x += s->body[i].m * s->body[i].vx;
        y += s->body[i].m * s->body[i].vy;
    }
    *px = x;
    *py = y;
}

int sim_pick(const Sim *s, float x, float y)
{
    for (int i = s->nb - 1; i >= 0; i--) {
        const Body *b = &s->body[i];
        float dx = b->x - x, dy = b->y - y;
        if (dx * dx + dy * dy > b->rad * b->rad) continue; /* cheap reject */
        for (int k = b->p0; k < b->p0 + b->pn; k++) {
            float px = s->p[k].x - x, py = s->p[k].y - y;
            if (px * px + py * py <= s->p[k].r * s->p[k].r) return i;
        }
    }
    return -1;
}
