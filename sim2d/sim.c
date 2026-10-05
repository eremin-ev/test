/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 * sim.c - physics implementation (see sim.h).
 *
 */

#include "sim.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define CORR_PERCENT 0.8f /* positional de-overlap strength */
#define CORR_SLOP 0.01f   /* px of overlap we allow before correcting */
#define ITER 2            /* collision relaxation iterations per substep */
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
#define XPBD_DRAG 0.4f         /* 1/s: mild velocity+spin damping, see step 1 */
#define XPBD_ROLL 0.15f        /* rolling resistance: tangential impulse at a
                                * contact opposing spin, proportional to the
                                * normal load jn. Without it a pile of hard
                                * disks is a ball pit: rolling costs nothing,
                                * so the surface churns forever down slopes. */
#define XPBD_MARGIN 2.0f       /* px: contacts are collected slightly before
                                 * touching and projected only when actually
                                 * overlapped; the margin catches overlaps a
                                 * projection creates against neighbours that
                                 * were not in this substep's contact set */

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

/* Hue ramp from cool (light) to warm (heavy). Normalised on r in [4,64] px. */
static void mass_color(float r, int mass_exp, float *cr, float *cg, float *cb)
{
    float lo = powf(4.0f, (float)mass_exp);
    float hi = powf(64.0f, (float)mass_exp);
    float t = (powf(r, (float)mass_exp) - lo) / (hi - lo);
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
    if (s->cap > s->next_cap) {
        free(s->next);
        s->next_cap = s->cap;
        s->next = malloc((size_t)s->next_cap * sizeof *s->next);
    }
}

static void grid_build(Sim *s)
{
    grid_alloc(s);
    for (int i = 0; i < s->gw * s->gh; i++) s->head[i] = -1;
    for (int i = 0; i < s->n; i++) {
        int cx = (int)(s->b[i].x / (float)s->cell);
        int cy = (int)(s->b[i].y / (float)s->cell);
        if (cx < 0) cx = 0;
        if (cx >= s->gw) cx = s->gw - 1;
        if (cy < 0) cy = 0;
        if (cy >= s->gh) cy = s->gh - 1;
        int c = cy * s->gw + cx;
        s->next[i] = s->head[c];
        s->head[c] = i;
    }
}

/* ------------------------------------------------------------------ core */

void sim_init(Sim *s, int cap, float w, float h, float restitution,
              float density, int mass_exp)
{
    memset(s, 0, sizeof *s);
    s->cap = cap > 0 ? cap : 1;
    s->b = calloc((size_t)s->cap, sizeof *s->b);
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
    free(s->b);
    free(s->head);
    free(s->next);
    free(s->xpair_buf);
    free(s->xwall_buf);
    free(s->nc);
    memset(s, 0, sizeof *s);
}

void sim_clear(Sim *s)
{
    s->n = 0;
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
    /* pull balls that are now outside back inside */
    for (int i = 0; i < s->n; i++) {
        float r = s->b[i].r;
        if (s->b[i].x < r) s->b[i].x = r;
        if (s->b[i].x > w - r) s->b[i].x = w - r;
        if (s->b[i].y < r) s->b[i].y = r;
        if (s->b[i].y > h - r) s->b[i].y = h - r;
    }
    grid_alloc(s);
}

float sim_mass(const Sim *s, float r)
{
    return s->density * powf(r, (float)s->mass_exp);
}

int sim_add(Sim *s, float x, float y, float vx, float vy, float r)
{
    if (s->n >= s->cap || r <= 0.0f) return -1;
    Ball *b = &s->b[s->n];
    b->r = r;
    b->m = sim_mass(s, r);
    b->inv_m = 1.0f / b->m;
    b->I = 0.5f * b->m * r * r; /* solid disk */
    b->inv_I = 1.0f / b->I;
    b->x = x;
    b->y = y;
    b->vx = vx;
    b->vy = vy;
    b->ang = 0.0f;
    b->omega = 0.0f;
    mass_color(r, s->mass_exp, &b->cr, &b->cg, &b->cb);
    if (r > s->max_r) s->max_r = r;
    return s->n++;
}

static int overlaps(const Sim *s, float x, float y, float r)
{
    if (x < r || y < r || x > s->w - r || y > s->h - r) return 1;
    for (int i = 0; i < s->n; i++) {
        float dx = s->b[i].x - x, dy = s->b[i].y - y;
        float rs = s->b[i].r + r;
        if (dx * dx + dy * dy < rs * rs) return 1;
    }
    return 0;
}

int sim_add_random(Sim *s, float rmin, float rmax, float speed, float spin)
{
    if (s->n >= s->cap) return -1;
    for (int attempt = 0; attempt < 64; attempt++) {
        float r = rmin + (rmax - rmin) * frand01(s);
        float x = r + (s->w - 2.0f * r) * frand01(s);
        float y = r + (s->h - 2.0f * r) * frand01(s);
        if (overlaps(s, x, y, r)) continue;
        float ang = 2.0f * 3.14159265f * frand01(s);
        int idx = sim_add(s, x, y, speed * cosf(ang), speed * sinf(ang), r);
        /* only draw the spin RNG when spin is on: keeps frictionless runs
         * byte-identical to the pre-spin code for a given seed */
        if (idx >= 0 && spin > 0.0f)
            s->b[idx].omega = (2.0f * frand01(s) - 1.0f) * 10.0f * spin;
        return idx;
    }
    return -1;
}

void sim_remove(Sim *s, int i)
{
    if (i < 0 || i >= s->n) return;
    memmove(&s->b[i], &s->b[i + 1], (size_t)(s->n - i - 1) * sizeof *s->b);
    s->n--;
    s->max_r = 8.0f;
    for (int k = 0; k < s->n; k++)
        if (s->b[k].r > s->max_r) s->max_r = s->b[k].r;
}

/* Coulomb friction against a static wall. n = wall inward normal, jn = the
 * normal impulse just applied by the bounce. The tangential impulse acts on
 * the contact point, so it trades tangential velocity for spin (and back). */
static void wall_friction(Ball *b, float nx, float ny, float mu, float jn)
{
    if (mu <= 0.0f || jn <= 0.0f) return;
    float px = -b->r * nx, py = -b->r * ny; /* contact point rel. centre */
    float tx = -ny, ty = nx;                /* contact tangent */
    /* surface velocity at contact: v + omega x p (2D: w x p = w*(-py, px)) */
    float vt = (b->vx - b->omega * py) * tx + (b->vy + b->omega * px) * ty;
    float denom = b->inv_m + b->r * b->r * b->inv_I;
    float jt = -vt / denom;                       /* stick the contact */
    float jmax = mu * jn;                         /* Coulomb limit */
    if (jt > jmax) jt = jmax;
    else if (jt < -jmax) jt = -jmax;
    b->vx += jt * b->inv_m * tx;
    b->vy += jt * b->inv_m * ty;
    b->omega += (px * ty - py * tx) * jt * b->inv_I;
}

/* ---------------------------------------------------------------- friction */

/* Coulomb friction impulse along the tangent of a ball-ball contact.
 * jn = magnitude of the normal impulse at this contact (momentum). */
static void pair_friction(Ball *bi, Ball *bj, float nxx, float nyy,
                          float mu, float jn)
{
    if (mu <= 0.0f || jn <= 0.0f) return;
    float tx = -nyy, ty = nxx;
    /* surface velocities at the contact point: v + omega x r */
    float uix = bi->vx - bi->omega * (nyy * bi->r);
    float uiy = bi->vy + bi->omega * (nxx * bi->r);
    float ujx = bj->vx + bj->omega * (nyy * bj->r);
    float ujy = bj->vy - bj->omega * (nxx * bj->r);
    float vt = (ujx - uix) * tx + (ujy - uiy) * ty;
    float K = bi->inv_m + bj->inv_m + bi->r * bi->r * bi->inv_I +
              bj->r * bj->r * bj->inv_I;
    float jt = -vt / K; /* stick the contact */
    float jmax = mu * jn;
    if (jt > jmax) jt = jmax;
    else if (jt < -jmax) jt = -jmax;
    /* impulse +jt*t on j, -jt*t on i */
    bi->vx -= jt * bi->inv_m * tx;
    bi->vy -= jt * bi->inv_m * ty;
    bj->vx += jt * bj->inv_m * tx;
    bj->vy += jt * bj->inv_m * ty;
    bi->omega -= jt * bi->r * bi->inv_I;
    bj->omega -= jt * bj->r * bj->inv_I;
}

static void walls(Sim *s)
{
    float e = s->restitution;
    float mu = s->friction;
    for (int i = 0; i < s->n; i++) {
        Ball *b = &s->b[i];
        if (b->x - b->r < 0.0f) {
            b->x = b->r;
            if (b->vx < 0) {
                float jn = -(1.0f + e) * b->vx * b->m;
                b->vx = -b->vx * e;
                wall_friction(b, 1.0f, 0.0f, mu, jn);
                s->wall_hits++;
            }
        } else if (b->x + b->r > s->w) {
            b->x = s->w - b->r;
            if (b->vx > 0) {
                float jn = (1.0f + e) * b->vx * b->m;
                b->vx = -b->vx * e;
                wall_friction(b, -1.0f, 0.0f, mu, jn);
                s->wall_hits++;
            }
        }
        if (b->y - b->r < 0.0f) {
            b->y = b->r;
            if (b->vy < 0) {
                float jn = -(1.0f + e) * b->vy * b->m;
                b->vy = -b->vy * e;
                wall_friction(b, 0.0f, 1.0f, mu, jn);
                s->wall_hits++;
            }
        } else if (b->y + b->r > s->h) {
            b->y = s->h - b->r;
            if (b->vy > 0) {
                float jn = (1.0f + e) * b->vy * b->m;
                b->vy = -b->vy * e;
                wall_friction(b, 0.0f, -1.0f, mu, jn);
                s->wall_hits++;
            }
        }
    }
}

/* Half neighbourhood: own cell (pairs taken once via j > i) plus the four
 * "forward" cells, which together cover every adjacent cell pair exactly once. */
static const int OFF_X[5] = { 0, 1, -1, 0, 1 };
static const int OFF_Y[5] = { 0, 0, 1, 1, 1 };

static void collide_pairs(Sim *s, int count)
{
    float e = s->restitution;
    for (int cy = 0; cy < s->gh; cy++) {
        for (int cx = 0; cx < s->gw; cx++) {
            for (int i = s->head[cy * s->gw + cx]; i >= 0; i = s->next[i]) {
                Ball *bi = &s->b[i];
                for (int k = 0; k < 5; k++) {
                    int nx = cx + OFF_X[k];
                    int ny = cy + OFF_Y[k];
                    if (nx < 0 || ny < 0 || nx >= s->gw || ny >= s->gh) continue;
                    for (int j = s->head[ny * s->gw + nx]; j >= 0; j = s->next[j]) {
                        if (k == 0 && j <= i) continue; /* each pair once */
                        Ball *bj = &s->b[j];
                        float dx = bj->x - bi->x, dy = bj->y - bi->y;
                        float rs = bi->r + bj->r;
                        float d2 = dx * dx + dy * dy;
                        if (d2 >= rs * rs || d2 <= 0.0f) continue;

                        float d = sqrtf(d2);
                        float nxx = dx / d, nyy = dy / d;
                        float vn = (bj->vx - bi->vx) * nxx + (bj->vy - bi->vy) * nyy;
                        float imsum = bi->inv_m + bj->inv_m;

                        float jimp = 0.0f;
                        if (vn < 0.0f) {
                            if (count) s->collisions++;
                            jimp = -(1.0f + e) * vn / imsum;
                            bi->vx -= jimp * bi->inv_m * nxx;
                            bi->vy -= jimp * bi->inv_m * nyy;
                            bj->vx += jimp * bj->inv_m * nxx;
                            bj->vy += jimp * bj->inv_m * nyy;
                        }
                        /* tangential (Coulomb) impulse: couples spin with
                         * translation; internal, so linear momentum is
                         * still conserved exactly */
                        if (s->friction > 0.0f && jimp != 0.0f)
                            pair_friction(bi, bj, nxx, nyy, s->friction,
                                          fabsf(jimp));
                        /* positional correction, split by inverse mass */
                        float pen = rs - d - CORR_SLOP;
                        if (pen > 0.0f) {
                            float c = CORR_PERCENT * pen / imsum;
                            bi->x -= c * bi->inv_m * nxx;
                            bi->y -= c * bi->inv_m * nyy;
                            bj->x += c * bj->inv_m * nxx;
                            bj->y += c * bj->inv_m * nyy;
                        }
                    }
                }
            }
        }
    }
}

/* omega clamp + angle integration/wrap (wrap keeps mediump shaders happy) */
static void step_ang(Ball *b, float dt)
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

static void cap_speed(Ball *b)
{
    float sp2 = b->vx * b->vx + b->vy * b->vy;
    if (sp2 > SIM_MAX_SPEED * SIM_MAX_SPEED) {
        float k = SIM_MAX_SPEED / sqrtf(sp2);
        b->vx *= k;
        b->vy *= k;
    }
}

static void sim_step_xpbd(Sim *s, float dt);

void sim_step(Sim *s, float dt)
{
    if (s->solver == SIM_SOLVER_XPBD) {
        sim_step_xpbd(s, dt);
        return;
    }
    for (int i = 0; i < s->n; i++) {
        Ball *b = &s->b[i];
        b->vy += s->gravity * dt;
        cap_speed(b);
        b->x += b->vx * dt;
        b->y += b->vy * dt;
        step_ang(b, dt);
    }
    walls(s);
    grid_build(s);
    for (int it = 0; it < ITER; it++) collide_pairs(s, it == 0);
    walls(s);
}

/* -------------------------------------------------------------------- xpbd */
/* Position-based dynamics split into a hard position solve and a velocity
 * solve (Muller 2007; XPBD, Macklin 2019; small-step dynamics following
 * Kleinsteuber/Chojecki, where restitution and sleep live at velocity
 * level and penetration is killed at position level):
 *   1. save post-gravity velocity v_pre, integrate a prediction
 *   2. collect contacts (disc-disc, disc-wall) on the predicted positions
 *   3. Gauss-Seidel project the position constraints (XPBD, compliance
 *      alpha~ = alpha/dt^2, alpha = 0 rigid) with a per-contact Lagrange
 *      multiplier clamped >= 0; contacts push but never pull
 *   4. velocity restarts from v_pre: position corrections NEVER become
 *      velocity (that leak boils piles - tried and verified the hard way)
 *   5. one-sided velocity impulses on the contacts, iterated as projected
 *      Gauss-Seidel until (nearly) no contact approaches: reflect while
 *      approaching faster than XPBD_REST_THRESH, stop below it (sleep
 *      rule) -> piles jam and reach zero residual velocity
 *   6. Coulomb friction with jn = lam/dt (the support force the position
 *      pass actually applied) + the contact's normal impulse
 * Stiffness scales with XPBD_ITER x substeps, both measurable in the
 * stress harness. Deterministic: fixed traversal order, no atomics. */

typedef struct { int i, j; float lam; } XPair;
typedef struct { int i; float nx, ny, c0, lam; } XWall; /* n.p + c0 = dist */

#define XPAIR(s) ((XPair *)(s)->xpair_buf)
#define XWALL(s) ((XWall *)(s)->xwall_buf)

static int xpair_add(Sim *s, int i, int j)
{
    if (s->xn_pair >= s->xpair_cap) {
        int cap = s->xpair_cap ? s->xpair_cap * 2 : 256;
        void *p = realloc(s->xpair_buf, (size_t)cap * sizeof(XPair));
        if (!p) return 0;
        s->xpair_buf = p;
        s->xpair_cap = cap;
    }
    XPair *c = &XPAIR(s)[s->xn_pair++];
    c->i = i;
    c->j = j;
    c->lam = 0.0f;
    return 1;
}

static int xwall_add(Sim *s, int i, float nx, float ny, float c0)
{
    if (s->xn_wall >= s->xwall_cap) {
        int cap = s->xwall_cap ? s->xwall_cap * 2 : 256;
        void *p = realloc(s->xwall_buf, (size_t)cap * sizeof(XWall));
        if (!p) return 0;
        s->xwall_buf = p;
        s->xwall_cap = cap;
    }
    XWall *c = &XWALL(s)[s->xn_wall++];
    c->i = i;
    c->nx = nx;
    c->ny = ny;
    c->c0 = c0;
    c->lam = 0.0f;
    return 1;
}

/* rolling resistance: bleed off spin against the contact load (clamped so
 * it can never reverse the spin) */
static void pair_roll(Ball *b, float jn)
{
    if (jn <= 0.0f || b->omega == 0.0f) return;
    float dw = XPBD_ROLL * jn * b->r * b->inv_I;
    if (dw > fabsf(b->omega)) dw = fabsf(b->omega);
    b->omega += b->omega > 0.0f ? -dw : dw;
}

static void sim_step_xpbd(Sim *s, float dt)
{
    float e = s->restitution;
    float mu = s->friction;
    float at = s->compliance / (dt * dt); /* alpha tilde */

    /* Mild linear drag, xpbd only. Without any rolling resistance a pile of
     * hard disks is a ball pit: surface balls roll down slopes and collide,
     * and nothing ever removes the tangential energy (measured: mean 126
     * px/s still moving after 90 s). Drag time constant 1/XPBD_DRAG s gives
     * slopes something to roll against; terminal fall speed g/drag = 2500
     * px/s is far above normal play, so it only quiets slow churn. The
     * impulse solver stays perfectly conservative. */
    const float drag = 1.0f / (1.0f + XPBD_DRAG * dt);

    /* 1. predict */
    for (int i = 0; i < s->n; i++) {
        Ball *b = &s->b[i];
        b->vy += s->gravity * dt;
        b->vx *= drag;
        b->vy *= drag;
        b->omega *= drag;
        cap_speed(b);
        b->xp = b->x;
        b->yp = b->y;
        b->vpx = b->vx;
        b->vpy = b->vy;
        b->x += b->vx * dt;
        b->y += b->vy * dt;
    }

    /* 2. contacts on predicted positions; nc[] counts pair contacts per
     * ball and drives the sparse-impact gate in the velocity pass */
    s->xn_pair = 0;
    s->xn_wall = 0;
    if (s->n > s->nc_cap) {
        free(s->nc);
        s->nc_cap = s->n < s->cap ? s->cap : s->n;
        s->nc = malloc((size_t)s->nc_cap * sizeof *s->nc);
    }
    if (s->nc)
        for (int i = 0; i < s->n; i++) s->nc[i] = 0;
    grid_build(s);
    for (int cy = 0; cy < s->gh; cy++) {
        for (int cx = 0; cx < s->gw; cx++) {
            for (int i = s->head[cy * s->gw + cx]; i >= 0; i = s->next[i]) {
                Ball *bi = &s->b[i];
                for (int k = 0; k < 5; k++) {
                    int nx = cx + OFF_X[k];
                    int ny = cy + OFF_Y[k];
                    if (nx < 0 || ny < 0 || nx >= s->gw || ny >= s->gh) continue;
                    for (int j = s->head[ny * s->gw + nx]; j >= 0; j = s->next[j]) {
                        if (k == 0 && j <= i) continue; /* each pair once */
                        Ball *bj = &s->b[j];
                        float dx = bj->x - bi->x, dy = bj->y - bi->y;
                        float rs = bi->r + bj->r + XPBD_MARGIN;
                        float d2 = dx * dx + dy * dy;
                        if (d2 >= rs * rs || d2 <= 0.0f) continue;
                        if (xpair_add(s, i, j)) {
                            if (s->nc) {
                                s->nc[i]++;
                                s->nc[j]++;
                            }
                            float d = sqrtf(d2);
                            float vn = (bj->vpx - bi->vpx) * dx / d +
                                       (bj->vpy - bi->vpy) * dy / d;
                            if (vn < 0.0f) s->collisions++;
                        }
                    }
                }
            }
        }
    }
    for (int i = 0; i < s->n; i++) {
        Ball *b = &s->b[i];
        if (b->x - b->r < XPBD_MARGIN && xwall_add(s, i, 1.0f, 0.0f, 0.0f) &&
            b->vpx < 0.0f)
            s->wall_hits++;
        else if (b->x + b->r > s->w - XPBD_MARGIN &&
                 xwall_add(s, i, -1.0f, 0.0f, s->w) && b->vpx > 0.0f)
            s->wall_hits++;
        if (b->y - b->r < XPBD_MARGIN && xwall_add(s, i, 0.0f, 1.0f, 0.0f) &&
            b->vpy < 0.0f)
            s->wall_hits++;
        else if (b->y + b->r > s->h - XPBD_MARGIN &&
                 xwall_add(s, i, 0.0f, -1.0f, s->h) && b->vpy > 0.0f)
            s->wall_hits++;
    }

    /* 3. Gauss-Seidel projection */
    for (int it = 0; it < XPBD_ITER; it++) {
        XPair *pc = XPAIR(s);
        for (int c = 0; c < s->xn_pair; c++) {
            Ball *bi = &s->b[pc[c].i], *bj = &s->b[pc[c].j];
            float dx = bj->x - bi->x, dy = bj->y - bi->y;
            float d2 = dx * dx + dy * dy;
            if (d2 <= 0.0f) continue;
            float d = sqrtf(d2);
            float C = d - (bi->r + bj->r); /* <= 0 desired */
            if (C > 0.0f) { /* separated: release, contacts never pull */
                pc[c].lam = 0.0f;
                continue;
            }
            float nxx = dx / d, nyy = dy / d;
            float dl = (-C - at * pc[c].lam) /
                       (bi->inv_m + bj->inv_m + at);
            float lam2 = pc[c].lam + dl;
            if (lam2 < 0.0f) lam2 = 0.0f; /* push only, never attract */
            dl = lam2 - pc[c].lam;
            pc[c].lam = lam2;
            bi->x -= dl * bi->inv_m * nxx;
            bi->y -= dl * bi->inv_m * nyy;
            bj->x += dl * bj->inv_m * nxx;
            bj->y += dl * bj->inv_m * nyy;
        }
        XWall *wc = XWALL(s);
        for (int c = 0; c < s->xn_wall; c++) {
            Ball *b = &s->b[wc[c].i];
            float dist = wc[c].nx * b->x + wc[c].ny * b->y + wc[c].c0;
            float C = dist - b->r; /* <= 0 desired */
            if (C > 0.0f) { /* separated: release, walls never pull */
                wc[c].lam = 0.0f;
                continue;
            }
            float dl = (-C - at * wc[c].lam) / (b->inv_m + at);
            float lam2 = wc[c].lam + dl;
            if (lam2 < 0.0f) lam2 = 0.0f;
            dl = lam2 - wc[c].lam;
            wc[c].lam = lam2;
            b->x += dl * b->inv_m * wc[c].nx; /* inward */
            b->y += dl * b->inv_m * wc[c].ny;
        }
    }

    /* 4. reset velocity to the pre-solve state (gravity only): position
     * projections must never leak into velocity. Deriving v from position
     * displacement or from accumulated lam double-pays overlaps that other
     * contacts created during the sweep, and that positive feedback boils
     * dense piles. All velocity change comes from the impulse pass below. */
    for (int i = 0; i < s->n; i++) {
        Ball *b = &s->b[i];
        b->vx = b->vpx;
        b->vy = b->vpy;
    }

    /* 5. sequential velocity impulses over the same contacts: bounce while
     * approaching faster than XPBD_REST_THRESH, inelastic stop below it
     * (the sleep rule), one-sided (a contact can push but never pull).
     * Uses the *current* sequential vn, so opposing contacts cancel instead
     * of ratcheting on stale approach targets. The position pass above is
     * what removes penetration; this pass is what makes the pile sleep.
     * 6. friction rides along, with jn = support impulse from lam/dt plus
     * this contact's normal impulse.
     *
     * The sweep is iterated XPBD_VITER times, and that is what keeps a
     * dense bed calm. A single sweep is one Gauss-Seidel pass: stopping a
     * late contact shoves bodies back toward contacts already processed,
     * so in a jammed graph residual approach velocity always survives.
     * That residue becomes penetration next substep, the bed keeps
     * collapsing, and gravity feeding the collapse is the motor that kept
     * the old xpbd bed boiling (measured: centre of mass sinking at a
     * steady ~136 px/s forever, ~26% residual KE). Iterating to (nearly)
     * the fixed point "no contact approaches" jams the bed and shuts the
     * feed off: same 2000-disk scenario, mean-pen 1.45 -> 0.48 px,
     * jitter 272 -> 61 px/s, residual KE 25.8% -> 1.4%. */
    for (int vp = 0; vp < XPBD_VITER; vp++) {
        XPair *pc = XPAIR(s);
        for (int c = 0; c < s->xn_pair; c++) {
            Ball *bi = &s->b[pc[c].i], *bj = &s->b[pc[c].j];
            float dx = bj->x - bi->x, dy = bj->y - bi->y;
            float d2 = dx * dx + dy * dy;
            if (d2 <= 0.0f) continue;
            float d = sqrtf(d2);
            float nxx = dx / d, nyy = dy / d;
            float vn = (bj->vx - bi->vx) * nxx + (bj->vy - bi->vy) * nyy;
            float jr = 0.0f;
            if (vn < 0.0f) {
                /* Reflect only genuine, sparse impacts (a billiard hit: both
                 * balls low-degree). Reflecting simultaneously across a jammed
                 * contact graph is a Fermi accelerator: opposing contacts
                 * bounce the same ball against each other and the pile boils.
                 * In jammed regions the contact just stops (inelastic), which
                 * is both stable and what a dense pile should do. */
                int sparse = s->nc ? s->nc[pc[c].i] + s->nc[pc[c].j] <= 4 : 1;
                float jimp =
                    sparse && vn < -XPBD_REST_THRESH ? -(1.0f + e) * vn : -vn;
                jimp /= bi->inv_m + bj->inv_m;
                bi->vx -= jimp * bi->inv_m * nxx;
                bi->vy -= jimp * bi->inv_m * nyy;
                bj->vx += jimp * bj->inv_m * nxx;
                bj->vy += jimp * bj->inv_m * nyy;
                jr = fabsf(jimp);
            }
            float jn = pc[c].lam / dt + jr;
            pair_friction(bi, bj, nxx, nyy, mu, jn);
            pair_roll(bi, jn);
            pair_roll(bj, jn);
        }
        XWall *wc = XWALL(s);
        for (int c = 0; c < s->xn_wall; c++) {
            Ball *b = &s->b[wc[c].i];
            float nx = wc[c].nx, ny = wc[c].ny;
            float vn = b->vx * nx + b->vy * ny;
            float jr = 0.0f;
            if (vn < 0.0f) {
                float dv = vn < -XPBD_REST_THRESH ? -(1.0f + e) * vn : -vn;
                b->vx += dv * nx;
                b->vy += dv * ny;
                jr = dv * b->m;
            }
            float jn = wc[c].lam / dt + jr;
            wall_friction(b, nx, ny, mu, jn);
            pair_roll(b, jn);
        }
    }

    for (int i = 0; i < s->n; i++)
        step_ang(&s->b[i], dt);
}

/* ----------------------------------------------------------------- stats */

float sim_kinetic(const Sim *s)
{
    float ke = 0.0f;
    for (int i = 0; i < s->n; i++)
        ke += 0.5f * s->b[i].m * (s->b[i].vx * s->b[i].vx + s->b[i].vy * s->b[i].vy) +
              0.5f * s->b[i].I * s->b[i].omega * s->b[i].omega;
    return ke;
}

void sim_momentum(const Sim *s, float *px, float *py)
{
    float x = 0.0f, y = 0.0f;
    for (int i = 0; i < s->n; i++) {
        x += s->b[i].m * s->b[i].vx;
        y += s->b[i].m * s->b[i].vy;
    }
    *px = x;
    *py = y;
}

int sim_pick(const Sim *s, float x, float y)
{
    for (int i = s->n - 1; i >= 0; i--) {
        float dx = s->b[i].x - x, dy = s->b[i].y - y;
        if (dx * dx + dy * dy <= s->b[i].r * s->b[i].r) return i;
    }
    return -1;
}
