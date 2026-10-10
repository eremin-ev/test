/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 * main.c - bouncing balls: xcb + EGL + OpenGL ES (see README.md).
 *
 */

#include "gl_draw.h"
#include "platform.h"
#include "sim.h"

#include <math.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PHYS_DT (1.0f / 240.0f) /* physics substep, s */
#define FRAME_DT (1.0f / 60.0f) /* simulated frame budget, s */
#define MAX_SUB 16              /* max substeps per frame (anti spiral-of-death) */
#define DEF_CAP SIM_MAX_BODIES     /* bodies allocated */
#define DEF_PCAP SIM_MAX_PARTICLES /* particles allocated (sim + renderer) */
#define GRAV 1000.0f            /* px/s^2 used when gravity is toggled on */
#define DEF_MU 0.2f             /* default Coulomb friction for spin coupling */

/* Which bodies to spawn. SIM_SHAPE_* map 1:1 onto the first four; MIXED draws
 * one per body (discs included, so a mixed table still has real billiard balls
 * rolling around in it). 'b' cycles this at runtime. */
enum { SH_DISC = 0, SH_BOX, SH_TRI, SH_CAPSULE, SH_MIXED, SH_COUNT };
static const char *const SHAPE_NAME[SH_COUNT] = { "disc", "box", "tri",
                                                  "capsule", "mixed" };
static const int SHAPE_SIM[SH_COUNT] = { SIM_SHAPE_DISC, SIM_SHAPE_BOX,
                                         SIM_SHAPE_TRI, SIM_SHAPE_CAPSULE, -1 };

typedef struct {
    int off;                /* 0 = X11 window (default), 1 = offscreen */
    int w, h;
    int balls;
    float rmin, rmax;
    float rest;
    float mu;               /* Coulomb friction (tangential impulses) */
    float compliance;       /* xpbd softness alpha (0 = rigid projection) */
    int substeps;           /* physics substeps per (60 fps) frame */
    int mass_exp;
    int shapes;             /* SH_*: which body shapes to spawn */
    float pmin, pmax;       /* lattice particle radius range (px) */
    int gravity;            /* start with gravity on */
    int spin;               /* spawn balls with random spin */
    int frames;             /* 0 = run until closed (window mode) */
    int selftest;
    int stress;             /* physics-only pile stability harness */
    int benchmark;
    int quiet;
    unsigned seed;
    char dump[512];
    int dump_frame;         /* -1 = last frame */
} Opts;

static double now_s(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + 1e-9 * (double)ts.tv_nsec;
}

static void usage(const char *argv0)
{
    fprintf(stderr,
            "Usage: %s [options]\n"
            "  --mode x11|off       on-screen window (default) or offscreen\n"
            "  --size WxH           window / framebuffer size (default 1024x768)\n"
            "  --balls N            number of bodies (default 30, max %d)\n"
            "  --rmin R --rmax R    radius range in px (default 8..48)\n"
            "  --rest E             restitution 0..1 (default 0.9)\n"
            "  --mu MU              Coulomb friction 0..2 (default 0.2; 0 = no spin coupling)\n"
            "  --compliance A         xpbd softness (0 = rigid, try 1e-4..1e-2)\n"
            "  --substeps N           substeps per stress frame (default 4 = 240 Hz)\n"
            "  --mass-exp 1|2|3     mass ~ r^exp (default 2 = area, 2D disks)\n"
            "  --shapes MODE        disc|box|tri|capsule|mixed (default mixed); shapes are\n"
            "                       hex packings of touching particles (rounded outline)\n"
            "  --pmin R --pmax R    lattice particle radius range (default 3..8, min %.0f);\n"
            "                       capped per body at 0.2*size and 0.25*thinnest feature\n"
            "  --gravity            start with gravity on (1000 px/s^2)\n"
            "  --spin 0|1           spawn balls with random spin (default 1)\n"
            "  --frames N           exit after N frames (offscreen default 300)\n"
            "  --seed S             RNG seed (default 12345)\n"
            "  --dump-ppm FILE      write a PPM screenshot (offscreen)\n"
            "  --dump-frame N       which frame to dump (default: last)\n"
            "  --selftest           headless correctness test, exit 0 = pass\n"
            "  --stress             pile stability harness: 2000 disks r 2..8 under gravity,\n"
            "                       no rendering; prints penetration/jitter/KE metrics\n"
            "                       (--frames N overrides the 30 s settle length)\n"
            "  --benchmark          print a one-line fps summary at the end\n"
            "  --quiet              suppress stats output\n"
            "\n"
            "Keys: space=pause  r=reset  c=clear  +/-=add/remove  1/2/3=mass exp\n"
            "      b=cycle body shape  g=gravity  f=friction  n=neighbour grid  i=stats\n"
            "      h=help  Esc/q=quit\n"
            "Mouse: left-drag = slingshot a new body, right-click = remove it,\n"
            "       wheel = size of the next one\n",
            argv0, DEF_CAP, (double)SIM_MIN_PARTICLE);
    exit(1);
}

static void parse_size(const char *s, int *w, int *h)
{
    if (sscanf(s, "%dx%d", w, h) != 2 || *w < 64 || *h < 64 || *w > 8192 ||
        *h > 8192) {
        fprintf(stderr, "bad --size '%s' (want WxH, 64..8192)\n", s);
        exit(1);
    }
}

static void parse_args(int argc, char **argv, Opts *o)
{
    o->off = 0;
    o->w = 1024;
    o->h = 768;
    o->balls = 30;
    o->rmin = 8.0f;
    o->rmax = 48.0f;
    o->rest = 0.9f;
    o->mu = DEF_MU;
    o->compliance = 0.0f;
    o->substeps = 4;
    o->mass_exp = 2;
    o->shapes = SH_MIXED;
    o->pmin = 3.0f;
    o->pmax = 8.0f;
    o->gravity = 0;
    o->spin = 1;
    o->frames = 0;
    o->selftest = 0;
    o->stress = 0;
    o->benchmark = 0;
    o->quiet = 0;
    o->seed = 12345;
    o->dump[0] = '\0';
    o->dump_frame = -1;
    int sdef = 0; /* --stress defaults applied once, so user flags may override */

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        const char *v = (i + 1 < argc) ? argv[i + 1] : NULL;
#define NEED()                                                   \
    do {                                                         \
        if (!v) {                                                \
            fprintf(stderr, "%s needs a value\n", a);            \
            exit(1);                                             \
        }                                                        \
        i++;                                                     \
    } while (0)
        if (!strcmp(a, "--mode")) { NEED(); o->off = !strcmp(v, "off"); }
        else if (!strcmp(a, "--size")) { NEED(); parse_size(v, &o->w, &o->h); }
        else if (!strcmp(a, "--balls")) { NEED(); o->balls = atoi(v); }
        else if (!strcmp(a, "--rmin")) { NEED(); o->rmin = (float)atof(v); }
        else if (!strcmp(a, "--rmax")) { NEED(); o->rmax = (float)atof(v); }
        else if (!strcmp(a, "--rest")) { NEED(); o->rest = (float)atof(v); }
        else if (!strcmp(a, "--mu")) { NEED(); o->mu = (float)atof(v); }
        else if (!strcmp(a, "--compliance")) { NEED(); o->compliance = (float)atof(v); }
        else if (!strcmp(a, "--substeps")) { NEED(); o->substeps = atoi(v); }
        else if (!strcmp(a, "--mass-exp")) { NEED(); o->mass_exp = atoi(v); }
        else if (!strcmp(a, "--shapes")) {
            NEED();
            int k;
            for (k = 0; k < SH_COUNT; k++)
                if (!strcmp(v, SHAPE_NAME[k]))
                    break;
            if (k == SH_COUNT) {
                fprintf(stderr, "error: --shapes wants disc|box|tri|capsule|mixed\n");
                exit(1);
            }
            o->shapes = k;
        }
        else if (!strcmp(a, "--pmin")) { NEED(); o->pmin = (float)atof(v); }
        else if (!strcmp(a, "--pmax")) { NEED(); o->pmax = (float)atof(v); }
        else if (!strcmp(a, "--gravity")) o->gravity = 1;
        else if (!strcmp(a, "--spin")) { NEED(); o->spin = atoi(v); }
        else if (!strcmp(a, "--frames")) { NEED(); o->frames = atoi(v); }
        else if (!strcmp(a, "--seed")) { NEED(); o->seed = (unsigned)strtoul(v, NULL, 0); }
        else if (!strcmp(a, "--dump-ppm")) { NEED(); snprintf(o->dump, sizeof o->dump, "%s", v); }
        else if (!strcmp(a, "--dump-frame")) { NEED(); o->dump_frame = atoi(v); }
        else if (!strcmp(a, "--selftest")) o->selftest = 1;
        else if (!strcmp(a, "--stress")) {
            o->stress = 1; /* later explicit flags still override these */
            if (!sdef) {
                o->balls = 2000;
                o->rmin = 2.0f;
                o->rmax = 8.0f;
                o->shapes = SH_DISC; /* the metric baseline is discs; pass
                                      * --shapes after --stress to override */
                o->frames = 1800; /* 30 s at 60 fps */
                sdef = 1;
            }
        }
        else if (!strcmp(a, "--benchmark")) o->benchmark = 1;
        else if (!strcmp(a, "--quiet")) o->quiet = 1;
        else if (!strcmp(a, "--help") || !strcmp(a, "-h")) usage(argv[0]);
        else {
            fprintf(stderr, "unknown option '%s'\n", a);
            usage(argv[0]);
        }
#undef NEED
    }

    if (o->selftest) {
        o->off = 1;
        if (!o->frames)
            o->frames = 600;
    }
    if (o->off && !o->frames)
        o->frames = 300;
    if (o->balls < 0)
        o->balls = 0;
    if (o->balls > DEF_CAP)
        o->balls = DEF_CAP;
    if (o->rmin < 2.0f)
        o->rmin = 2.0f;
    if (o->rmax < o->rmin)
        o->rmax = o->rmin;
    if (o->pmin < SIM_MIN_PARTICLE)
        o->pmin = SIM_MIN_PARTICLE;
    if (o->pmax < o->pmin)
        o->pmax = o->pmin;
    if (o->stress && o->shapes != SH_DISC) {
        /* Two adjustments when --stress piles up shapes.
         * SIM_MAX_PARTICLES is the real capacity limit: a composite body costs
         * a handful of particles, so a 2000-body shape pile would be measuring
         * spawn starvation rather than stability. And the stress size range is a
         * DISC RADIUS range - for shapes sz is the nominal radius, so 2..8 px
         * means a body of one or two particles, which tests nothing. Both are
         * overridable by passing --balls/--rmin/--rmax after --stress. */
        if (o->balls > 400)
            o->balls = 400;
        if (o->rmin <= 2.0f && o->rmax <= 8.0f) {
            o->rmin = 8.0f;
            o->rmax = 24.0f;
        }
    }
    if (o->mass_exp < 1)
        o->mass_exp = 1;
    if (o->mass_exp > 3)
        o->mass_exp = 3;
    if (o->rest < 0.0f)
        o->rest = 0.0f;
    if (o->rest > 1.0f)
        o->rest = 1.0f;
    if (o->mu < 0.0f)
        o->mu = 0.0f;
    if (o->mu > 2.0f)
        o->mu = 2.0f;
    o->spin = o->spin ? 1 : 0;
    if (o->substeps < 1)
        o->substeps = 1;
    if (o->substeps > 32)
        o->substeps = 32;
}

static void print_stats(const Sim *s, const char *tag)
{
    float px, py, wsum = 0.0f;
    sim_momentum(s, &px, &py);
    for (int i = 0; i < s->nb; i++)
        wsum += fabsf(s->body[i].omega);
    printf("[stats] %s bodies=%d particles=%d contacts=%llu wall-hits=%llu KE=%.0f |p|=%.0f spin=%.1f rad/s\n",
           tag, s->nb, s->np, (unsigned long long)s->collisions,
           (unsigned long long)s->wall_hits, sim_kinetic(s), hypotf(px, py),
           wsum);
}

/* One body shape for the current --shapes mode. MIXED draws per body from the
 * SIM rng (so a mixed scene is reproducible from its seed alone); a fixed mode
 * draws nothing, which is what keeps --shapes disc runs bit-identical to the
 * pre-shape code for a seed. */
static int shape_draw(Sim *s, int mode)
{
    if (mode != SH_MIXED)
        return SHAPE_SIM[mode];
    int k = (int)(sim_rand01(s) * (float)SH_MIXED);
    if (k >= SH_MIXED)
        k = SH_MIXED - 1;
    return SHAPE_SIM[k];
}

/* Spawn n bodies of the option's shape mode; returns how many fitted. */
static int spawn_bodies(Sim *s, int n, const Opts *o)
{
    int ok = 0;
    for (int i = 0; i < n; i++)
        if (sim_add_random_shape(s, shape_draw(s, o->shapes), o->rmin, o->rmax,
                                 o->pmin, o->pmax, 320.0f,
                                 o->spin ? 1.0f : 0.0f) >= 0)
            ok++;
    return ok;
}

static void spawn_n(Sim *s, int n, float rmin, float rmax, float spin)
{
    for (int i = 0; i < n; i++)
        sim_add_random(s, rmin, rmax, 320.0f, spin);
}

/* ------------------------------------------------------------------ selftest */

static int g_fail;

static void check(int ok, const char *name, const char *detail)
{
    printf("  %-4s %s%s%s\n", ok ? "PASS" : "FAIL", name,
           detail && *detail ? ": " : "", detail ? detail : "");
    if (!ok)
        g_fail = 1;
}

static void run_frames(Sim *s, int frames)
{
    for (int f = 0; f < frames; f++)
        for (int k = 0; k < 4; k++)
            sim_step(s, PHYS_DT);
}

/* Worst particle-pair gap inside one body: >= 0 means the lattice does not
 * overlap itself (touching is expected and fine). Uses body-frame offsets, so
 * it is independent of where the body happens to be. */
static float body_min_gap(const Sim *s, int i)
{
    const Body *b = &s->body[i];
    float worst = 1e9f;
    for (int k = 0; k < b->pn; k++)
        for (int l = k + 1; l < b->pn; l++) {
            float gap = hypotf(s->p[b->p0 + k].ox - s->p[b->p0 + l].ox,
                               s->p[b->p0 + k].oy - s->p[b->p0 + l].oy) -
                        s->p[b->p0 + k].r - s->p[b->p0 + l].r;
            if (gap < worst)
                worst = gap;
        }
    return b->pn > 1 ? worst : 0.0f;
}

static int selftest(const Opts *o)
{
    char msg[256];
    printf("== selftest (offscreen EGL) ==\n");

    Plat pl;
    if (plat_open_offscreen(&pl, 640, 480) != 0) {
        check(0, "offscreen EGL init", plat_last_error());
        return 1;
    }
    printf("  ---- GL: %s / %s\n", (const char *)glGetString(GL_VENDOR),
           (const char *)glGetString(GL_VERSION));

    Renderer rn;
    if (glr_init(&rn, 64) != 0) {
        check(0, "renderer init", "shader/buffer failure (see stderr)");
        plat_close(&pl);
        return 1;
    }
    if (glr_enable_fbo(&rn, 640, 480) != 0) {
        check(0, "offscreen FBO", "incomplete");
        glr_destroy(&rn);
        plat_close(&pl);
        return 1;
    }

    /* --- 1. long run: finite, contained, capped, collisions happen --- */
    Sim s;
    sim_init(&s, 64, 640, 480, 0.9f, 0.001f, 2);
    sim_set_seed(&s, o->seed);
    spawn_n(&s, 20, 6.0f, 26.0f, 1.0f); /* spin on: exercises ang integration */
    snprintf(msg, sizeof msg, "spawned %d/20", s.nb);
    check(s.nb >= 15, "initial spawn", msg);

    int finite = 1, contained = 1, capped = 1, spin_ok = 1;
    for (int f = 0; f < o->frames && finite && contained; f++) {
        for (int k = 0; k < 4; k++)
            sim_step(&s, PHYS_DT);
        for (int i = 0; i < s.nb; i++) {
            const Body *b = &s.body[i];
            if (!isfinite(b->x) || !isfinite(b->y) || !isfinite(b->vx) ||
                !isfinite(b->vy) || !isfinite(b->ang) || !isfinite(b->omega))
                finite = 0;
            if (!(b->ang >= 0.0f && b->ang < 6.28318531f))
                spin_ok = 0;
            if (b->x < b->rad - 1.0f || b->x > s.w - b->rad + 1.0f ||
                b->y < b->rad - 1.0f || b->y > s.h - b->rad + 1.0f)
                contained = 0;
            if (hypotf(b->vx, b->vy) > SIM_MAX_SPEED + 1.0f)
                capped = 0;
        }
    }
    check(finite, "state stays finite", NULL);
    check(contained, "bodies stay inside the box", NULL);
    check(capped, "speed cap respected", NULL);
    check(spin_ok, "angle stays finite and wrapped", NULL);
    snprintf(msg, sizeof msg, "contacts=%llu wall=%llu",
             (unsigned long long)s.collisions,
             (unsigned long long)s.wall_hits);
    check(s.collisions > 50 && s.wall_hits > 20, "collisions happen", msg);

    /* --- 2. energy never grows (e=1 elastic, e=0.9 damped) --- */
    Sim e1;
    sim_init(&e1, 64, 640, 480, 1.0f, 0.001f, 2);
    sim_set_seed(&e1, 777);
    spawn_n(&e1, 12, 8.0f, 24.0f, 0.0f);
    float ke0 = sim_kinetic(&e1);
    run_frames(&e1, 600);
    float ke1 = sim_kinetic(&e1);
    snprintf(msg, sizeof msg, "KE %.0f -> %.0f", ke0, ke1);
    check(ke1 <= ke0 * 1.02f + 1.0f, "e=1.0: energy not growing", msg);

    float ke_a = sim_kinetic(&s);
    run_frames(&s, 300);
    float ke_b = sim_kinetic(&s);
    snprintf(msg, sizeof msg, "KE %.0f -> %.0f", ke_a, ke_b);
    check(ke_b <= ke_a * 1.02f + 1.0f, "e=0.9: energy not growing", msg);

    /* --- 3. mass model: m = density * r^2 --- */
    Sim mm;
    sim_init(&mm, 4, 100, 100, 0.9f, 2.0f, 2);
    int ok_mass = sim_mass(&mm, 3.0f) == 2.0f * 9.0f &&
                  sim_mass(&mm, 10.0f) == 2.0f * 100.0f;
    check(ok_mass, "mass ~ r^2 (2D disks)", NULL);
    sim_free(&mm);

    /* --- 4. head-on collision: heavy ball barely slows, light one flies --- */
    Sim hc;
    sim_init(&hc, 4, 1000, 100, 1.0f, 0.001f, 2);
    int hi = sim_add_disc(&hc, 200, 50, 300, 0, 10); /* light */
    int hj = sim_add_disc(&hc, 400, 50, -100, 0, 30); /* heavy, approaching */
    (void)hi;
    (void)hj;
    run_frames(&hc, 60);
    check(hc.body[0].vx < 0.0f && hc.body[1].vx > -100.0f + 1.0f,
          "heavy hits light: light rebounds",
          NULL);
    /* p = m*v; m = density*r^2: light 0.1*300, heavy 0.9*(-100) */
    float p0 = 0.001f * 100.0f * 300.0f + 0.001f * 900.0f * (-100.0f);
    float p1x = hc.body[0].m * hc.body[0].vx + hc.body[1].m * hc.body[1].vx;
    snprintf(msg, sizeof msg, "px %.1f -> %.1f", p0, p1x);
    check(fabsf(p1x - p0) < fabsf(p0) * 0.02f + 1.0f, "momentum conserved", msg);
    sim_free(&hc);

    /* --- 4b. gravity: with g on, everything settles on the floor --- */
    Sim gv;
    sim_init(&gv, 32, 640, 480, 0.9f, 0.001f, 2);
    sim_set_seed(&gv, 31337);
    gv.gravity = 1500.0f;
    spawn_n(&gv, 12, 8.0f, 20.0f, 0.0f);
    run_frames(&gv, 1800); /* 30 s */
    int settled = 1;
    for (int i = 0; i < gv.nb; i++) {
        const Body *b = &gv.body[i];
        /* after 30 s nothing may still be up in the upper half */
        if (b->y < gv.h * 0.5f)
            settled = 0;
    }
    snprintf(msg, sizeof msg, "n=%d, g=1500 px/s^2, 30 s", gv.nb);
    check(settled && gv.nb == 12, "gravity: balls settle on the floor", msg);
    sim_free(&gv);

    /* --- 4c. friction: a wall bounce trades spin for tangential velocity --- */
    Sim fr;
    sim_init(&fr, 4, 640, 480, 1.0f, 0.001f, 2);
    fr.friction = 0.5f;
    sim_add_disc(&fr, 550, 240, 300, 0, 20);
    fr.body[0].omega = 20.0f;
    float w0 = fabsf(fr.body[0].omega);
    run_frames(&fr, 60); /* hits the right wall after ~0.23 s */
    snprintf(msg, sizeof msg, "w %.0f -> %.1f rad/s, vy -> %.0f px/s", w0,
             fr.body[0].omega, fr.body[0].vy);
    check(fabsf(fr.body[0].omega) < w0 && fabsf(fr.body[0].vy) > 1.0f &&
              fr.collisions == 0 && fr.wall_hits >= 1,
          "friction: wall converts spin into tangential motion", msg);
    sim_free(&fr);

    /* --- 4d. full spin+friction system: total energy never grows --- */
    Sim fe;
    sim_init(&fe, 64, 640, 480, 0.9f, 0.001f, 2);
    sim_set_seed(&fe, 555);
    fe.friction = 0.5f;
    spawn_n(&fe, 16, 8.0f, 26.0f, 1.0f);
    float fe0 = sim_kinetic(&fe); /* includes rotational 0.5*I*w^2 */
    run_frames(&fe, 600);
    float fe1 = sim_kinetic(&fe);
    snprintf(msg, sizeof msg, "KE(trans+rot) %.0f -> %.0f, mu=0.5", fe0, fe1);
    check(fe1 <= fe0 * 1.02f + 1.0f, "spin+friction: energy not growing", msg);
    sim_free(&fe);

    /* --- 4e. a frictionless contact may not touch spin AT ALL ---
     * Exact equality is the point: with mu = 0 nothing in the solver writes
     * omega (the normal lever of a disc is 0, friction is 0, and rolling
     * resistance needs friction). xpbd used to bleed spin here - see
     * particle_roll. */
    Sim fc;
    sim_init(&fc, 8, 640, 480, 1.0f, 0.001f, 2);
    fc.friction = 0.0f;
    sim_add_disc(&fc, 100, 240, 300, 0, 20);
    sim_add_disc(&fc, 400, 250, -200, 0, 15);
    fc.body[0].omega = 20.0f;
    fc.body[1].omega = -13.0f;
    run_frames(&fc, 600); /* several wall bounces and a ball-ball hit */
    snprintf(msg, sizeof msg, "w 20 -> %.4f, w -13 -> %.4f, hits %llu/%llu",
             fc.body[0].omega, fc.body[1].omega,
             (unsigned long long)fc.collisions,
             (unsigned long long)fc.wall_hits);
    check(fc.body[0].omega == 20.0f && fc.body[1].omega == -13.0f &&
              fc.collisions >= 1 && fc.wall_hits >= 1,
          "mu=0: contacts never change spin", msg);
    sim_free(&fc);

    /* --- 4f. one bounce may only take the spin Coulomb grants it ---
     * Roll used to be driven by lam/dt + impact impulse, so a hit applied a
     * rolling torque proportional to the collision itself and one wall bounce
     * zeroed the spin. The bound for a disc hitting a rigid wall: the normal
     * impulse is m*(1+e)*|vn|, so |dw| <= mu*m*(1+e)*|vn|*r/I =
     * mu*(1+e)*|vn|*2/r. With mu=.2, e=1, |vn|=300 (exact here: no gravity,
     * vertical wall), r=20 that is 12 rad/s; 10% tolerance. */
    Sim rr;
    sim_init(&rr, 4, 640, 480, 1.0f, 0.001f, 2);
    rr.friction = 0.2f;
    sim_add_disc(&rr, 550, 240, 300, 0, 20);
    rr.body[0].omega = 20.0f;
    int rf = 0;
    while (rr.wall_hits == 0 && rf < 240) {
        run_frames(&rr, 1);
        rf++;
    }
    snprintf(msg, sizeof msg, "w 20 -> %.1f after the first hit at f=%d, "
                             "Coulomb allows %.0f", rr.body[0].omega, rf, 12.0f);
    check(fabsf(rr.body[0].omega) >= 20.0f - 12.0f * 1.1f && rr.wall_hits >= 1,
          "roll: a bounce takes only its Coulomb share of the spin", msg);
    sim_free(&rr);

    /* --- 4g. rolling resistance still does its actual job ---
     * A ball dropped spinning onto a rough floor: friction turns the spin into
     * translation, and the sustained floor load must then brake the roll to a
     * full stop. Delete XPBD_ROLL and this state is absorbing: vx 133.3 and
     * w 6.7 are preserved to the end of time (measured over 12 s). */
    Sim rl;
    sim_init(&rl, 4, 4000, 480, 0.9f, 0.001f, 2);
    rl.gravity = 1500.0f;
    rl.friction = 0.2f;
    sim_add_disc(&rl, 200, 400, 0, 0, 20);
    rl.body[0].omega = 20.0f;
    run_frames(&rl, 720); /* 12 s */
    snprintf(msg, sizeof msg, "vx -> %.2f px/s, w -> %.2f rad/s", rl.body[0].vx,
             rl.body[0].omega);
    check(fabsf(rl.body[0].vx) < 1.0f && fabsf(rl.body[0].omega) < 1.0f &&
              rl.body[0].y > rl.h * 0.5f,
          "roll: a spinning ball on the floor comes to rest", msg);
    sim_free(&rl);

    /* --- 4e. gravity pile settles, stays inside and sleeps --- */
    Sim xs;
    sim_init(&xs, 64, 640, 480, 0.9f, 0.001f, 2);
    sim_set_seed(&xs, 2024);
    xs.friction = 0.3f;
    xs.gravity = GRAV;
    spawn_n(&xs, 40, 6.0f, 16.0f, 1.0f);
    float kx0 = sim_kinetic(&xs);
    run_frames(&xs, 900); /* 15 s */
    int xok = 1;
    float xvmax = 0.0f;
    for (int i = 0; i < xs.nb; i++) {
        const Body *b = &xs.body[i];
        if (!isfinite(b->x) || !isfinite(b->y) || !isfinite(b->vx) ||
            !isfinite(b->vy))
            xok = 0;
        if (b->x < b->rad - 1.0f || b->x > xs.w - b->rad + 1.0f ||
            b->y < b->rad - 1.0f || b->y > xs.h - b->rad + 1.0f)
            xok = 0;
        float sp = hypotf(b->vx, b->vy);
        if (sp > xvmax) xvmax = sp;
    }
    snprintf(msg, sizeof msg, "KE %.0f -> %.0f, max|v| %.1f px/s", kx0,
             sim_kinetic(&xs), xvmax);
    check(xok && xs.nb == 40 && xvmax < 25.0f,
          "xpbd: gravity pile settles and stays inside", msg);
    sim_free(&xs);

    /* --- 5. determinism: same seed => identical state ---
     * With shapes this is a real test rather than a formality: shape choice,
     * proportions, lattice radius, spawn tilt and spin all come out of the sim
     * RNG, and particles have to match too. (It used to spawn nothing, which
     * made it pass trivially.) */
    Sim d1, d2;
    sim_init(&d1, 64, 640, 480, 0.9f, 0.001f, 2);
    sim_init(&d2, 64, 640, 480, 0.9f, 0.001f, 2);
    sim_set_seed(&d1, 4242);
    sim_set_seed(&d2, 4242);
    Opts mix = *o;
    mix.shapes = SH_MIXED;
    int nd1 = spawn_bodies(&d1, 24, &mix);
    int nd2 = spawn_bodies(&d2, 24, &mix);
    run_frames(&d1, 120);
    run_frames(&d2, 120);
    int identical = d1.nb == d2.nb && d1.np == d2.np && nd1 == nd2 && d1.nb > 8;
    for (int i = 0; i < d1.nb && identical; i++)
        identical = memcmp(&d1.body[i], &d2.body[i], sizeof(Body)) == 0;
    for (int i = 0; i < d1.np && identical; i++)
        identical = memcmp(&d1.p[i], &d2.p[i], sizeof(Particle)) == 0;
    snprintf(msg, sizeof msg, "%d bodies, %d particles, mixed shapes",
             d1.nb, d1.np);
    check(identical, "determinism (same seed)", msg);
    sim_free(&d1);
    sim_free(&d2);

    /* --- 5b. a lattice may never overlap its own body's particles ---
     * Touching is the whole idea of a hex packing; overlapping is invisible in
     * the render and silently wrong in the inertia and in every contact. */
    int lat_ok = 1, lat_n = 0;
    float lat_worst = 0.0f;
    for (int sh = SIM_SHAPE_BOX; sh <= SIM_SHAPE_CAPSULE; sh++) {
        for (int k = 0; k < 6; k++) {
            float sz = 8.0f + 9.0f * (float)k;
            Sim t;
            sim_init(&t, 2, 900, 700, 0.9f, 0.001f, 2);
            int idx = sim_add_sized(&t, sh, 450, 350, 0.0f, 0.0f, sz, 2.0f,
                                    3.0f + (float)k);
            if (idx >= 0) {
                float g = body_min_gap(&t, idx);
                lat_n++;
                if (g < lat_worst)
                    lat_worst = g;
                if (g < -0.01f)
                    lat_ok = 0;
                if (t.body[idx].pn < 1 || t.body[idx].pn > SIM_MAX_PARTS_PER_BODY)
                    lat_ok = 0;
            }
            sim_free(&t);
        }
    }
    snprintf(msg, sizeof msg, "%d lattices, worst gap %.4f px", lat_n, lat_worst);
    check(lat_ok && lat_n == 18, "lattice: particles never overlap their own body",
          msg);

    /* --- 5c. mass is set by the shape, not by the lattice resolution ---
     * One hex lattice cell of particle radius r covers 2*sqrt(3)*r^2, so
     * reff^2 = sum(r^2) should be 0.289*A_nominal whatever r is: same nominal
     * box, same mass. The residual +-10% here is the discrete lattice phase
     * (how many rows/columns fall in), not lost material - the old
     * eroded-shape rule swung 2.7x over the same range, which is what this
     * gate is really watching for. */
    float mmin = 1e30f, mmax = 0.0f;
    int mn_ok = 1, mpn_min = 1 << 30;
    const float latt[3] = { 4.0f, 7.0f, 10.0f };
    for (int k = 0; k < 3; k++) {
        Sim t;
        sim_init(&t, 2, 400, 400, 0.9f, 1.0f, 2);
        int i = sim_add_box(&t, 200, 200, 0.0f, 0.0f, 120.0f, 60.0f, latt[k]);
        if (i < 0) {
            mn_ok = 0;
        } else {
            if (t.body[i].m < mmin)
                mmin = t.body[i].m;
            if (t.body[i].m > mmax)
                mmax = t.body[i].m;
            if (t.body[i].pn < mpn_min)
                mpn_min = t.body[i].pn;
        }
        sim_free(&t);
    }
    snprintf(msg, sizeof msg, "m %.0f..%.0f over pr 4..10, spread %.2fx, min pn %d",
             mmin, mmax, mmax / mmin, mpn_min);
    check(mn_ok && mmax < 1.2f * mmin && mpn_min >= 8,
          "lattice: mass independent of particle count", msg);

    /* --- 5d. a body made of particles is a RIGID body ---
     * A plank dropped at 0.5 rad tips over and comes to rest flat on the
     * floor: the contacts at its two ends carry different loads, and only a
     * solver with real angular momentum does this. It also stays inside, and
     * the widest extent ends up horizontal. */
    Sim pk;
    sim_init(&pk, 4, 640, 480, 0.2f, 0.001f, 2);
    pk.gravity = GRAV;
    pk.friction = 0.4f;
    int pi = sim_add_box(&pk, 320, 120, 0.0f, 0.0f, 240.0f, 60.0f, 7.0f);
    pk.body[pi].ang = 0.5f; /* poked after the spawn: the next sim_step syncs */
    run_frames(&pk, 1200);
    float px0 = 1e9f, px1 = -1e9f, py0 = 1e9f, py1 = -1e9f;
    int inside = 1;
    const Body *pb = &pk.body[pi];
    for (int k = pb->p0; k < pb->p0 + pb->pn; k++) {
        const Particle *p = &pk.p[k];
        if (p->x - p->r < px0) px0 = p->x - p->r;
        if (p->x + p->r > px1) px1 = p->x + p->r;
        if (p->y - p->r < py0) py0 = p->y - p->r;
        if (p->y + p->r > py1) py1 = p->y + p->r;
        if (p->x < -0.5f || p->x > pk.w + 0.5f || p->y < -0.5f ||
            p->y > pk.h + 0.5f)
            inside = 0;
    }
    snprintf(msg, sizeof msg,
             "%.0fx%.0f px, bottom %.1f (floor %.0f), |v| %.1f, |w| %.2f, %d particles",
             px1 - px0, py1 - py0, py1, pk.h,
             hypotf(pb->vx, pb->vy), fabsf(pb->omega), pb->pn);
    check(pi >= 0 && inside && px1 - px0 > 2.0f * (py1 - py0) &&
              py1 > pk.h - 3.0f && fabsf(pb->omega) < 0.1f &&
              hypotf(pb->vx, pb->vy) < 5.0f,
          "rigid: a tilted plank settles flat on the floor", msg);
    sim_free(&pk);
    sim_free(&d1);
    sim_free(&d2);

    /* --- 6. rendering: a red disc on dark background, round SDF edge --- */
    Sim one;
    sim_init(&one, 4, 640, 480, 1.0f, 0.001f, 2);
    sim_add_disc(&one, 320, 240, 0, 0, 50);
    one.body[0].cr = 1;
    one.body[0].cg = 0;
    one.body[0].cb = 0;
    glr_draw(&rn, &one, -1, NULL, 640, 480, 0);
    unsigned char *img = NULL;
    int iw = 0, ih = 0;
    if (glr_read_pixels(&rn, &img, &iw, &ih) == 0) {
        const unsigned char *c = img + ((size_t)(480 - 1 - 240) * 640 + 320) * 4;
        const unsigned char *bg = img + ((size_t)(480 - 1 - 10) * 640 + 10) * 4;
        snprintf(msg, sizeof msg, "centre=(%u,%u,%u) corner=(%u,%u,%u)",
                 c[0], c[1], c[2], bg[0], bg[1], bg[2]);
        check(c[0] > 120 && c[1] < 80 && bg[0] > 200 && bg[1] > 200,
              "pixel: disc drawn on white bg", msg);
        /* 65 px right of centre: outside r=50 disc -> background */
        const unsigned char *ot = img + ((size_t)(480 - 1 - 240) * 640 + 385) * 4;
        snprintf(msg, sizeof msg, "x=385 -> (%u,%u,%u)", ot[0], ot[1], ot[2]);
        check(ot[0] > 200, "pixel: SDF edge is round", msg);
        free(img);
    } else {
        check(0, "glReadPixels", "failed");
    }
    sim_free(&one);

    /* --- 6b. neighbour grid overlay: grey lines on the white background --- */
    sim_init(&one, 4, 640, 480, 1.0f, 0.001f, 2);
    sim_add_disc(&one, 320, 240, 0, 0, 50);
    glr_draw(&rn, &one, -1, NULL, 640, 480, 1);
    if (glr_read_pixels(&rn, &img, &iw, &ih) == 0) {
        int gy = (int)one.cell; /* first horizontal grid line */
        const unsigned char *on = img + ((size_t)(480 - 1 - gy) * 640 + 600) * 4;
        const unsigned char *off =
            img + ((size_t)(480 - 1 - (gy + 8)) * 640 + 600) * 4;
        snprintf(msg, sizeof msg, "cell=%d on=(%u,%u,%u) off=(%u,%u,%u)", gy,
                 on[0], on[1], on[2], off[0], off[1], off[2]);
        check(on[0] < 250 && on[2] > on[0] && off[0] > 250,
              "pixel: neighbour grid overlay", msg);
        free(img);
    } else {
        check(0, "glReadPixels (grid)", "failed");
    }
    sim_free(&one);

    /* --- 6c. spin marker: rotating the ball must move the billiard dots --- */
    sim_init(&one, 4, 640, 480, 1.0f, 0.001f, 2);
    sim_add_disc(&one, 320, 240, 0, 0, 50);
    one.body[0].cr = 1;
    one.body[0].cg = 0;
    one.body[0].cb = 0;
    unsigned char *img_a = NULL, *img_b = NULL;
    msg[0] = '\0';
    one.body[0].ang = 0.0f;
    glr_draw(&rn, &one, -1, NULL, 640, 480, 0);
    int mark_ok = 0;
    if (glr_read_pixels(&rn, &img_a, &iw, &ih) == 0) {
        one.body[0].ang = 1.5707963f; /* 90 deg: dot leaves the x-axis */
        glr_draw(&rn, &one, -1, NULL, 640, 480, 0);
        if (glr_read_pixels(&rn, &img_b, &iw, &ih) == 0) {
            /* dot centre sits 0.55*r = 27 px right of centre at ang=0 */
            const unsigned char *mk0 =
                img_a + ((size_t)(480 - 1 - 240) * 640 + 347) * 4;
            const unsigned char *mk1 =
                img_b + ((size_t)(480 - 1 - 240) * 640 + 347) * 4;
            snprintf(msg, sizeof msg, "ang0=(%u,%u,%u) ang90=(%u,%u,%u)",
                     mk0[0], mk0[1], mk0[2], mk1[0], mk1[1], mk1[2]);
            mark_ok = mk0[0] < 170 && mk1[0] > 230 && mk0[1] < 80 && mk1[1] < 80;
            free(img_b);
        }
        free(img_a);
    }
    check(mark_ok, "pixel: spin marker rotates with ang", mark_ok ? NULL : msg);
    sim_free(&one);
    Sim sc;
    sim_init(&sc, 64, 640, 480, 0.9f, 0.001f, 2);
    sim_set_seed(&sc, 99);
    spawn_n(&sc, 15, 8.0f, 30.0f, 1.0f);
    run_frames(&sc, 30);
    glr_draw(&rn, &sc, -1, NULL, 640, 480, 0);
    if (glr_dump_ppm(&rn, "selftest.ppm") == 0) {
        printf("  ---- wrote selftest.ppm\n");
        check(1, "PPM dump", NULL);
    } else {
        check(0, "PPM dump", NULL);
    }
    sim_free(&sc);
    sim_free(&s);
    sim_free(&e1);

    glr_destroy(&rn);
    plat_close(&pl);

    printf(g_fail ? "== SELFTEST FAILED ==\n" : "== SELFTEST OK ==\n");
    return g_fail ? 1 : 0;
}

/* ------------------------------------------------------------------ stress */
/* Physics-only stability harness: pack the box with many small disks under
 * gravity, let the pile settle, then measure what a stable solver should
 * keep small: interpenetration, residual and jitter velocity, KE decay.
 * No EGL/GL, fully deterministic for a given seed; meant for comparing
 * solver changes on equal footing. */

static int stress(const Opts *o)
{
    int cap = o->balls > DEF_CAP ? DEF_CAP : o->balls;
    int frames = o->frames > 0 ? o->frames : 1800; /* 30 s at 60 fps */
    printf("== stress: %d %s bodies r %.0f..%.0f (lattice %.0f..%.0f), "
           "g=%.0f px/s^2, e=%.2f, mu=%.2f, alpha=%g, substeps=%d, seed=%u, "
           "box %dx%d, settle %.0f s ==\n",
           cap, SHAPE_NAME[o->shapes], o->rmin, o->rmax, o->pmin, o->pmax, GRAV,
           o->rest, o->mu, o->compliance, o->substeps, o->seed, o->w, o->h,
           frames / 60.0);

    Sim s;
    sim_init(&s, cap, (float)o->w, (float)o->h, o->rest, 0.001f, o->mass_exp);
    sim_set_seed(&s, o->seed);
    s.gravity = GRAV;
    s.friction = o->mu;
    s.compliance = o->compliance;

    char msg[256];
    double t0 = now_s();
    int spawned = spawn_bodies(&s, cap, o);
    double t_spawn = now_s() - t0;
    float ke0 = sim_kinetic(&s);

    int finite_bad = 0, outside = 0;
    float jitter = 0.0f; /* max |v| seen during the final second */
    double t_settle0 = now_s();
    for (int f = 0; f < frames; f++) {
        for (int k = 0; k < o->substeps; k++)
            sim_step(&s, PHYS_DT);
        if (f >= frames - 60)
            for (int i = 0; i < s.nb; i++) {
                float sp = hypotf(s.body[i].vx, s.body[i].vy);
                if (sp > jitter) jitter = sp;
            }
        if ((f & 511) == 511) { /* periodic cheap invariant scan */
            for (int i = 0; i < s.nb; i++)
                if (!isfinite(s.body[i].x) || !isfinite(s.body[i].y) ||
                    !isfinite(s.body[i].vx) || !isfinite(s.body[i].vy) ||
                    !isfinite(s.body[i].omega))
                    finite_bad = 1;
        }
        if (!o->quiet && f % 600 == 599) {
            float vs = 0.0f;
            for (int i = 0; i < s.nb; i++) vs += hypotf(s.body[i].vx, s.body[i].vy);
            printf("  ... %4d/%d frames (%4.0f s sim), mean|v|=%.1f px/s\n",
                   f + 1, frames, (f + 1) / 60.0, vs / (float)s.nb);
        }
    }
    double t_settle = now_s() - t_settle0;

    /* Containment is a statement about PARTICLES: they are what the walls
     * constrain. Testing it on Body.rad (a circumradius) is wrong for any
     * non-circular body - a plank lying flat has its centre far closer to the
     * floor than rad, so it reads as "outside" while sitting perfectly still. */
    float outside_worst = 0.0f; /* px a particle pokes past a wall plane */
    for (int k = 0; k < s.np; k++) {
        const Particle *p = &s.p[k];
        float e = 0.0f; /* how far past a wall plane, 0 if inside */
        if (p->x - p->r < 0.0f && p->r - p->x > e)
            e = p->r - p->x;
        if (p->x + p->r > s.w && p->x + p->r - s.w > e)
            e = p->x + p->r - s.w;
        if (p->y - p->r < 0.0f && p->r - p->y > e)
            e = p->r - p->y;
        if (p->y + p->r > s.h && p->y + p->r - s.h > e)
            e = p->y + p->r - s.h;
        if (e > 1.0f)
            outside++;
        if (e > outside_worst)
            outside_worst = e;
    }

    /* Penetration and pile height are PARTICLE quantities - that is what the
     * solver constrains - while the velocity statistics are per body. The
     * penetration pass is O(np^2), run once, fine for a few thousand
     * particles; particles of one body never count (they are rigid). */
    double pen_sum = 0.0;
    float pen_max = 0.0f, vmax = 0.0f, wmax = 0.0f, vsum = 0.0f, top = (float)s.h;
    float cmx = 0.0f, cmy = 0.0f; /* centre-of-mass velocity: coherent drift */
    float *spbuf = malloc((size_t)s.nb * sizeof *spbuf);
    int over = 0;
    for (int i = 0; i < s.nb; i++) {
        const Body *bi = &s.body[i];
        float sp = hypotf(bi->vx, bi->vy);
        vsum += sp;
        cmx += bi->vx;
        cmy += bi->vy;
        if (spbuf) spbuf[i] = sp;
        if (sp > vmax) vmax = sp;
        if (fabsf(bi->omega) > wmax) wmax = fabsf(bi->omega);
    }
    for (int i = 0; i < s.np; i++) {
        const Particle *pi = &s.p[i];
        float h_i = pi->y - pi->r;
        if (h_i < top) top = h_i;
        for (int j = i + 1; j < s.np; j++) {
            const Particle *pj = &s.p[j];
            if (pi->b == pj->b) continue;
            float dx = pj->x - pi->x, dy = pj->y - pi->y;
            float rs = pi->r + pj->r;
            float d2 = dx * dx + dy * dy;
            if (d2 >= rs * rs) continue;
            float p = rs - sqrtf(d2);
            pen_sum += p;
            if (p > pen_max) pen_max = p;
            over++;
        }
    }
    float p50 = 0.0f, p90 = 0.0f;
    if (spbuf && s.nb > 0) {
        /* tiny n: insertion sort is fine and avoids qsort plumbing */
        int m = s.nb;
        for (int i = 1; i < m; i++) {
            float v = spbuf[i];
            int j = i - 1;
            while (j >= 0 && spbuf[j] > v) { spbuf[j + 1] = spbuf[j]; j--; }
            spbuf[j + 1] = v;
        }
        p50 = spbuf[m / 2];
        p90 = spbuf[m * 9 / 10];
    }
    free(spbuf);

    float ke1 = sim_kinetic(&s);

    printf("[stress] spawned %d/%d in %.2fs\n", spawned, cap, t_spawn);
    printf("[stress] settled %d frames (%.0f s sim) in %.1fs wall (%.0fx realtime), "
           "particle-contacts=%llu wall-hits=%llu\n",
           frames, frames / 60.0, t_settle,
           (frames / 60.0) / (t_settle > 0 ? t_settle : 1e-9),
           (unsigned long long)s.collisions,
           (unsigned long long)s.wall_hits);
    printf("[stress] pile: height=%.0f px, contacts=%d, mean-pen=%.3f px, max-pen=%.2f px\n",
           (float)s.h - top, over, over ? pen_sum / over : 0.0, pen_max);
    printf("[stress] rest: mean|v|=%.2f max|v|=%.2f, jitter(1s) max|v|=%.2f px/s, "
           "max|w|=%.2f rad/s\n",
           s.nb ? vsum / (float)s.nb : 0.0f, vmax, jitter, wmax);
    {
        /* depth profile relative to the pile surface: floor third, middle,
         * surface third, plus anything still above the pile */
        float vs[4] = {0, 0, 0, 0};
        int ns[4] = {0, 0, 0, 0};
        for (int i = 0; i < s.nb; i++) {
            float t = top < s.h - 1.0f ? (s.body[i].y - top) / (s.h - top) : 1.0f;
            int band = t < 0.0f ? 3 : (t < 0.34f ? 2 : (t < 0.67f ? 1 : 0));
            vs[band] += hypotf(s.body[i].vx, s.body[i].vy);
            ns[band]++;
        }
        printf("[stress] profile: mean|v| floor-3rd=%.1f (n=%d) mid=%.1f (n=%d) "
               "surf-3rd=%.1f (n=%d) above-pile=%.1f (n=%d)\n",
               ns[0] ? vs[0] / ns[0] : 0.0f, ns[0],
               ns[1] ? vs[1] / ns[1] : 0.0f, ns[1],
               ns[2] ? vs[2] / ns[2] : 0.0f, ns[2],
               ns[3] ? vs[3] / ns[3] : 0.0f, ns[3]);
    }
    printf("[stress] KE %.0f -> %.0f (%.2f%% residual)\n", ke0, ke1,
           ke0 > 0.0f ? 100.0 * ke1 / ke0 : 0.0);
    printf("[stress] vpercentiles: p50=%.1f p90=%.1f, |v_com|=%.1f px/s\n", p50,
           p90, s.nb ? hypotf(cmx, cmy) / (float)s.nb : 0.0f);

    /* Gates are regression bounds with ~1.5x headroom on the measured
     * baseline of the only solver there is (seeds 7/42/12345/999, 30 s,
     * 2000 disks r 2..8: mean-pen 0.48-0.51 px, max-pen <=3.7 px,
     * max|v| <=63.4 px/s, jitter <=63.4 px/s, KE residual <=1.6%).
     * They are TIGHTER than the old impulse solver's gates (1.3 px / 250 px/s)
     * on purpose: that solver measured 0.93 px and 124 px/s here and would
     * fail these, so re-introducing a weaker contact model trips them.
     * The metric lines above are the numbers to compare when changing the
     * solver; the gates only catch regressions. */
    check(finite_bad == 0, "stress: state stays finite", NULL);
    printf("[stress] walls: worst particle excursion %.2f px (%d beyond 1 px)\n",
           outside_worst, outside);
    /* A soft-constraint solver in a jammed pile leaves small residual wall
     * violations - a body pinned by its neighbours while another of its
     * particles is pushed in trades those positions off. What must not happen
     * is an escape (tens of px, growing), so the gate is on the worst
     * excursion rather than on a nonzero count. Discs stay under ~1 px; a mixed
     * pile of flat-faced bodies was measured at 1.6 px worst. */
    snprintf(msg, sizeof msg, "worst %.2f px, %d beyond 1 px", outside_worst,
             outside);
    check(outside_worst < 3.0f, "stress: pile stays inside the box", msg);
    snprintf(msg, sizeof msg, "mean-pen %.3f px", over ? pen_sum / over : 0.0);
    check(over == 0 || pen_sum / over < 0.8f, "stress: mean penetration < 0.8 px", msg);
    snprintf(msg, sizeof msg, "max-pen %.2f px", pen_max);
    check(pen_max < 6.0f, "stress: max penetration < 6 px", msg);
    snprintf(msg, sizeof msg, "max|v| %.2f px/s", vmax);
    check(vmax < 100.0f, "stress: pile at rest (max|v| < 100 px/s)", msg);
    snprintf(msg, sizeof msg, "jitter %.2f px/s, KE %.0f -> %.0f", jitter, ke0, ke1);
    check(jitter < 100.0f && ke1 <= ke0 * 1.02f + 1.0f,
          "stress: jitter bounded, KE decays", msg);

    printf(g_fail ? "== STRESS FAILED ==\n" : "== STRESS OK ==\n");
    return g_fail ? 1 : 0;
}

/* ------------------------------------------------------------------ main */

int main(int argc, char **argv)
{
    Opts o;
    parse_args(argc, argv, &o);

    if (o.selftest)
        return selftest(&o);
    if (o.stress)
        return stress(&o);

    Plat pl;
    int rc = o.off ? plat_open_offscreen(&pl, o.w, o.h)
                   : plat_open_window(&pl, o.w, o.h, "sim2d - xcb + EGL + OpenGL ES", 0);
    if (rc != 0) {
        fprintf(stderr, "error: %s\n", plat_last_error());
        if (!o.off)
            fprintf(stderr, "hint: headless machine? try --mode off --dump-ppm out.ppm\n");
        return 1;
    }

    Renderer rn;
    if (glr_init(&rn, DEF_PCAP) != 0) {
        fprintf(stderr, "error: renderer init failed (see stderr above)\n");
        plat_close(&pl);
        return 1;
    }
    if (o.off && glr_enable_fbo(&rn, o.w, o.h) != 0) {
        fprintf(stderr, "error: cannot create offscreen framebuffer\n");
        glr_destroy(&rn);
        plat_close(&pl);
        return 1;
    }

    Sim s;
    sim_init(&s, DEF_CAP, pl.width, pl.height, o.rest, 0.001f, o.mass_exp);
    sim_set_seed(&s, o.seed);
    s.friction = o.mu;
    s.compliance = o.compliance;
    if (o.gravity)
        s.gravity = GRAV;
    spawn_bodies(&s, o.balls, &o);

    if (!o.quiet)
        printf("GL: %s / %s\n", (const char *)glGetString(GL_VENDOR),
               (const char *)glGetString(GL_VERSION));

    float next_r = 18.0f;
    int paused = 0, hover = -1;
    int show_grid = 0; /* 'n' toggles the broad-phase neighbour grid */
    int dragging = 0;
    Drag drag = {0};
    drag.gr = next_r;
    float mx = 0, my = 0;

    int frame = 0;
    double t_start = now_s(), t_last = t_start;
    float acc = 0;

    while (1) {
        /* ---- events ---- */
        PlatEvent ev;
        while (plat_poll(&pl, &ev)) {
            switch (ev.type) {
            case PE_QUIT:
                goto done;
            case PE_RESIZE:
                sim_resize(&s, (float)ev.w, (float)ev.h);
                break;
            case PE_KEY:
                if (!ev.press)
                    break;
                switch (ev.keysym) {
                case 'q':
                    goto done;
                case KS_ESCAPE:
                    goto done;
                case ' ':
                    paused = !paused;
                    break;
                case 'r':
                    sim_clear(&s);
                    sim_set_seed(&s, o.seed);
                    spawn_bodies(&s, o.balls, &o);
                    break;
                case 'c':
                    sim_clear(&s);
                    break;
                case '+':
                case '=':
                    sim_add_random_shape(&s, shape_draw(&s, o.shapes), o.rmin,
                                         o.rmax, o.pmin, o.pmax, 320.0f,
                                         o.spin ? 1.0f : 0.0f);
                    break;
                case 'b':
                    o.shapes = (o.shapes + 1) % SH_COUNT;
                    printf("next body shape: %s\n", SHAPE_NAME[o.shapes]);
                    break;
                case '-':
                    if (s.nb)
                        sim_remove(&s, s.nb - 1);
                    break;
                case '1':
                    s.mass_exp = 1;
                    printf("mass ~ r^1\n");
                    break;
                case '2':
                    s.mass_exp = 2;
                    printf("mass ~ r^2\n");
                    break;
                case '3':
                    s.mass_exp = 3;
                    printf("mass ~ r^3\n");
                    break;
                case 'g':
                    s.gravity = s.gravity > 0.0f ? 0.0f : GRAV;
                    printf("gravity %s\n", s.gravity > 0.0f ? "on" : "off");
                    break;
                case 'f':
                    s.friction = s.friction > 0.0f ? 0.0f : (o.mu > 0.0f ? o.mu : DEF_MU);
                    printf("friction %s (mu=%.2f)\n",
                           s.friction > 0.0f ? "on" : "off", s.friction);
                    break;
                case 'n':
                    show_grid = !show_grid;
                    printf("neighbour grid %s (cell %d px, %dx%d)\n",
                           show_grid ? "on" : "off", s.cell, s.gw, s.gh);
                    break;
                case 'i':
                    print_stats(&s, "manual");
                    break;
                case 'h':
                    printf("space=pause r=reset c=clear +/-=add/remove 1/2/3=mass exp "
                           "b=shape g=gravity f=friction n=grid i=stats Esc/q=quit | "
                           "drag=slingshot RMB=remove wheel=size\n");
                    break;
                default:
                    break;
                }
                break;
            case PE_BUTTON:
                mx = (float)ev.x;
                my = (float)ev.y;
                if (ev.button == 1) {
                    if (ev.press) {
                        dragging = 1;
                        drag.x0 = mx;
                        drag.y0 = my;
                        drag.x1 = mx;
                        drag.y1 = my;
                        drag.gr = next_r;
                    } else if (dragging) {
                        dragging = 0;
                        /* slingshot: launch opposite to the pull */
                        float vx = (drag.x0 - drag.x1) * 4.0f;
                        float vy = (drag.y0 - drag.y1) * 4.0f;
                        int k = sim_add_sized(&s, shape_draw(&s, o.shapes),
                                              drag.x0, drag.y0, vx, vy, drag.gr,
                                              o.pmin, o.pmax);
                        if (k >= 0 && o.spin)
                            s.body[k].omega =
                                (sim_rand01(&s) * 2.0f - 1.0f) * 10.0f;
                    }
                } else if (ev.button == 3 && ev.press) {
                    int k = sim_pick(&s, mx, my);
                    if (k >= 0)
                        sim_remove(&s, k);
                } else if (ev.press && (ev.button == 4 || ev.button == 5)) {
                    next_r += (ev.button == 4) ? 2.0f : -2.0f;
                    if (next_r < 4.0f)
                        next_r = 4.0f;
                    if (next_r > 120.0f)
                        next_r = 120.0f;
                }
                break;
            case PE_MOTION:
                mx = (float)ev.x;
                my = (float)ev.y;
                if (dragging) {
                    drag.x1 = mx;
                    drag.y1 = my;
                }
                break;
            case PE_NONE:
                break;
            }
        }

        hover = sim_pick(&s, mx, my);

        /* ---- physics ---- */
        double tnow = now_s();
        float d = (float)(tnow - t_last);
        t_last = tnow;
        if (d > 0.25f)
            d = 0.25f;

        if (o.off)
            acc = FRAME_DT; /* deterministic: fixed substep budget per frame */
        else if (!paused)
            acc += d;

        if (!paused) {
            int nsub = 0;
            while (acc >= PHYS_DT && nsub < MAX_SUB) {
                sim_step(&s, PHYS_DT);
                acc -= PHYS_DT;
                nsub++;
            }
            if (nsub == MAX_SUB)
                acc = 0; /* note: --substeps is a --stress knob; the live loop
                          * always keeps the MAX_SUB catch-up budget */
        }

        /* ---- render ---- */
        glr_draw(&rn, &s, hover, dragging ? &drag : NULL, pl.width, pl.height,
                 show_grid);
        plat_swap(&pl);

        if (o.dump[0] &&
            (o.dump_frame < 0 ? frame == o.frames - 1 : frame == o.dump_frame)) {
            if (glr_dump_ppm(&rn, o.dump) == 0 && !o.quiet)
                printf("wrote %s\n", o.dump);
        }

        frame++;
        if (o.frames && frame >= o.frames)
            break;

        /* ---- wait for X events or a short tick ---- */
        if (!o.off) {
            struct pollfd pfd = {.fd = plat_fd(&pl), .events = POLLIN};
            poll(&pfd, 1, 4);
        }
    }

done:
    if (!o.quiet) {
        double el = now_s() - t_start;
        print_stats(&s, "final");
        printf("%d frames in %.2fs (%.1f fps)\n", frame, el,
               frame / (el > 0 ? el : 1e-9));
    }
    if (o.benchmark)
        printf("BENCH frames=%d fps=%.1f bodies=%d particles=%d\n", frame,
               frame / (now_s() - t_start), s.nb, s.np);

    sim_free(&s);
    glr_destroy(&rn);
    plat_close(&pl);
    return 0;
}
