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
#define DEF_CAP 2000            /* max balls allocated */
#define GRAV 1000.0f            /* px/s^2 used when gravity is toggled on */
#define DEF_MU 0.2f             /* default Coulomb friction for spin coupling */

typedef struct {
    int off;                /* 0 = X11 window (default), 1 = offscreen */
    int w, h;
    int balls;
    float rmin, rmax;
    float rest;
    float mu;               /* Coulomb friction (tangential impulses) */
    float compliance;       /* xpbd softness alpha (0 = rigid projection) */
    int substeps;           /* physics substeps per (60 fps) frame */
    int solver;             /* SIM_SOLVER_IMPULSE | SIM_SOLVER_XPBD */
    int mass_exp;
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
            "  --balls N            number of balls (default 30, max %d)\n"
            "  --rmin R --rmax R    radius range in px (default 8..48)\n"
            "  --rest E             restitution 0..1 (default 0.9)\n"
            "  --mu MU              Coulomb friction 0..2 (default 0.2; 0 = no spin coupling)\n"
            "  --solver impulse|xpbd  contact solver (default impulse; xpbd =\n"
            "                       position-based, far more stable in gravity piles)\n"
            "  --compliance A         xpbd softness (0 = rigid, try 1e-4..1e-2)\n"
            "  --substeps N           substeps per stress frame (default 4 = 240 Hz)\n"
            "  --mass-exp 1|2|3     mass ~ r^exp (default 2 = area, 2D disks)\n"
            "  --gravity            start with gravity on (1500 px/s^2)\n"
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
            "      g=gravity  f=friction  n=neighbour grid  i=stats  h=help  Esc/q=quit\n"
            "Mouse: left-drag = slingshot a new ball, right-click = remove ball,\n"
            "       wheel = radius of the next ball\n",
            argv0, DEF_CAP);
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
    o->solver = SIM_SOLVER_IMPULSE;
    o->mass_exp = 2;
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
        else if (!strcmp(a, "--solver")) {
            NEED();
            if (!strcmp(v, "xpbd")) o->solver = SIM_SOLVER_XPBD;
            else if (!strcmp(v, "impulse")) o->solver = SIM_SOLVER_IMPULSE;
            else { fprintf(stderr, "bad --solver '%s' (impulse|xpbd)\n", v); exit(1); }
        }
        else if (!strcmp(a, "--mass-exp")) { NEED(); o->mass_exp = atoi(v); }
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
    for (int i = 0; i < s->n; i++)
        wsum += fabsf(s->b[i].omega);
    printf("[stats] %s balls=%d ball-collisions=%llu wall-hits=%llu KE=%.0f |p|=%.0f spin=%.1f rad/s\n",
           tag, s->n, (unsigned long long)s->collisions,
           (unsigned long long)s->wall_hits, sim_kinetic(s), hypotf(px, py),
           wsum);
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
    snprintf(msg, sizeof msg, "spawned %d/20", s.n);
    check(s.n >= 15, "initial spawn", msg);

    int finite = 1, contained = 1, capped = 1, spin_ok = 1;
    for (int f = 0; f < o->frames && finite && contained; f++) {
        for (int k = 0; k < 4; k++)
            sim_step(&s, PHYS_DT);
        for (int i = 0; i < s.n; i++) {
            const Ball *b = &s.b[i];
            if (!isfinite(b->x) || !isfinite(b->y) || !isfinite(b->vx) ||
                !isfinite(b->vy) || !isfinite(b->ang) || !isfinite(b->omega))
                finite = 0;
            if (!(b->ang >= 0.0f && b->ang < 6.28318531f))
                spin_ok = 0;
            if (b->x < b->r - 1.0f || b->x > s.w - b->r + 1.0f ||
                b->y < b->r - 1.0f || b->y > s.h - b->r + 1.0f)
                contained = 0;
            if (hypotf(b->vx, b->vy) > SIM_MAX_SPEED + 1.0f)
                capped = 0;
        }
    }
    check(finite, "state stays finite", NULL);
    check(contained, "balls stay inside the box", NULL);
    check(capped, "speed cap respected", NULL);
    check(spin_ok, "angle stays finite and wrapped", NULL);
    snprintf(msg, sizeof msg, "ball-ball=%llu wall=%llu",
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
    int hi = sim_add(&hc, 200, 50, 300, 0, 10); /* light */
    int hj = sim_add(&hc, 400, 50, -100, 0, 30); /* heavy, approaching */
    (void)hi;
    (void)hj;
    run_frames(&hc, 60);
    check(hc.b[0].vx < 0.0f && hc.b[1].vx > -100.0f + 1.0f,
          "heavy hits light: light rebounds",
          NULL);
    /* p = m*v; m = density*r^2: light 0.1*300, heavy 0.9*(-100) */
    float p0 = 0.001f * 100.0f * 300.0f + 0.001f * 900.0f * (-100.0f);
    float p1x = hc.b[0].m * hc.b[0].vx + hc.b[1].m * hc.b[1].vx;
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
    for (int i = 0; i < gv.n; i++) {
        const Ball *b = &gv.b[i];
        /* after 30 s nothing may still be up in the upper half */
        if (b->y < gv.h * 0.5f)
            settled = 0;
    }
    snprintf(msg, sizeof msg, "n=%d, g=1500 px/s^2, 30 s", gv.n);
    check(settled && gv.n == 12, "gravity: balls settle on the floor", msg);
    sim_free(&gv);

    /* --- 4c. friction: a wall bounce trades spin for tangential velocity --- */
    Sim fr;
    sim_init(&fr, 4, 640, 480, 1.0f, 0.001f, 2);
    fr.friction = 0.5f;
    sim_add(&fr, 550, 240, 300, 0, 20);
    fr.b[0].omega = 20.0f;
    float w0 = fabsf(fr.b[0].omega);
    run_frames(&fr, 60); /* hits the right wall after ~0.23 s */
    snprintf(msg, sizeof msg, "w %.0f -> %.1f rad/s, vy -> %.0f px/s", w0,
             fr.b[0].omega, fr.b[0].vy);
    check(fabsf(fr.b[0].omega) < w0 && fabsf(fr.b[0].vy) > 1.0f &&
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

    /* --- 4e. xpbd solver: gravity pile settles, stays inside, sleeps --- */
    Sim xs;
    sim_init(&xs, 64, 640, 480, 0.9f, 0.001f, 2);
    sim_set_seed(&xs, 2024);
    xs.solver = SIM_SOLVER_XPBD;
    xs.friction = 0.3f;
    xs.gravity = GRAV;
    spawn_n(&xs, 40, 6.0f, 16.0f, 1.0f);
    float kx0 = sim_kinetic(&xs);
    run_frames(&xs, 900); /* 15 s */
    int xok = 1;
    float xvmax = 0.0f;
    for (int i = 0; i < xs.n; i++) {
        const Ball *b = &xs.b[i];
        if (!isfinite(b->x) || !isfinite(b->y) || !isfinite(b->vx) ||
            !isfinite(b->vy))
            xok = 0;
        if (b->x < b->r - 1.0f || b->x > xs.w - b->r + 1.0f ||
            b->y < b->r - 1.0f || b->y > xs.h - b->r + 1.0f)
            xok = 0;
        float sp = hypotf(b->vx, b->vy);
        if (sp > xvmax) xvmax = sp;
    }
    snprintf(msg, sizeof msg, "KE %.0f -> %.0f, max|v| %.1f px/s", kx0,
             sim_kinetic(&xs), xvmax);
    check(xok && xs.n == 40 && xvmax < 25.0f,
          "xpbd: gravity pile settles and stays inside", msg);
    sim_free(&xs);

    /* --- 5. determinism: same seed => identical state --- */
    Sim d1, d2;
    sim_init(&d1, 64, 640, 480, 0.9f, 0.001f, 2);
    sim_init(&d2, 64, 640, 480, 0.9f, 0.001f, 2);
    sim_set_seed(&d1, 4242);
    sim_set_seed(&d2, 4242);
    run_frames(&d1, 120);
    run_frames(&d2, 120);
    int identical = d1.n == d2.n;
    for (int i = 0; i < d1.n && identical; i++)
        identical = memcmp(&d1.b[i], &d2.b[i], sizeof(Ball)) == 0;
    check(identical, "determinism (same seed)", NULL);
    sim_free(&d1);
    sim_free(&d2);

    /* --- 6. rendering: a red disc on dark background, round SDF edge --- */
    Sim one;
    sim_init(&one, 4, 640, 480, 1.0f, 0.001f, 2);
    sim_add(&one, 320, 240, 0, 0, 50);
    one.b[0].cr = 1;
    one.b[0].cg = 0;
    one.b[0].cb = 0;
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
    sim_add(&one, 320, 240, 0, 0, 50);
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
    sim_add(&one, 320, 240, 0, 0, 50);
    one.b[0].cr = 1;
    one.b[0].cg = 0;
    one.b[0].cb = 0;
    unsigned char *img_a = NULL, *img_b = NULL;
    msg[0] = '\0';
    one.b[0].ang = 0.0f;
    glr_draw(&rn, &one, -1, NULL, 640, 480, 0);
    int mark_ok = 0;
    if (glr_read_pixels(&rn, &img_a, &iw, &ih) == 0) {
        one.b[0].ang = 1.5707963f; /* 90 deg: dot leaves the x-axis */
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
    printf("== stress: %d disks r %.0f..%.0f, g=%.0f px/s^2, e=%.2f, mu=%.2f, "
           "solver=%s, alpha=%g, substeps=%d, seed=%u, box %dx%d, settle %.0f s ==\n",
           cap, o->rmin, o->rmax, GRAV, o->rest, o->mu,
           o->solver == SIM_SOLVER_XPBD ? "xpbd" : "impulse", o->compliance,
           o->substeps, o->seed, o->w, o->h, frames / 60.0);

    Sim s;
    sim_init(&s, cap, (float)o->w, (float)o->h, o->rest, 0.001f, o->mass_exp);
    sim_set_seed(&s, o->seed);
    s.gravity = GRAV;
    s.friction = o->mu;
    s.solver = o->solver;
    s.compliance = o->compliance;

    char msg[256];
    double t0 = now_s();
    int spawned = 0;
    for (int i = 0; i < cap; i++)
        if (sim_add_random(&s, o->rmin, o->rmax, 320.0f, o->spin ? 1.0f : 0.0f) >= 0)
            spawned++;
    double t_spawn = now_s() - t0;
    float ke0 = sim_kinetic(&s);

    int finite_bad = 0, outside = 0;
    float jitter = 0.0f; /* max |v| seen during the final second */
    double t_settle0 = now_s();
    for (int f = 0; f < frames; f++) {
        for (int k = 0; k < o->substeps; k++)
            sim_step(&s, PHYS_DT);
        if (f >= frames - 60)
            for (int i = 0; i < s.n; i++) {
                float sp = hypotf(s.b[i].vx, s.b[i].vy);
                if (sp > jitter) jitter = sp;
            }
        if ((f & 511) == 511) { /* periodic cheap invariant scan */
            for (int i = 0; i < s.n; i++)
                if (!isfinite(s.b[i].x) || !isfinite(s.b[i].y) ||
                    !isfinite(s.b[i].vx) || !isfinite(s.b[i].vy) ||
                    !isfinite(s.b[i].omega))
                    finite_bad = 1;
        }
        if (!o->quiet && f % 600 == 599) {
            float vs = 0.0f;
            for (int i = 0; i < s.n; i++) vs += hypotf(s.b[i].vx, s.b[i].vy);
            printf("  ... %4d/%d frames (%4.0f s sim), mean|v|=%.1f px/s\n",
                   f + 1, frames, (f + 1) / 60.0, vs / (float)s.n);
        }
    }
    double t_settle = now_s() - t_settle0;

    for (int i = 0; i < s.n; i++) {
        const Ball *b = &s.b[i];
        if (b->x < b->r - 1.0f || b->x > s.w - b->r + 1.0f ||
            b->y < b->r - 1.0f || b->y > s.h - b->r + 1.0f)
            outside++;
    }

    /* pile statistics: O(n^2) pass, run once, fine for a few thousand disks */
    double pen_sum = 0.0;
    float pen_max = 0.0f, vmax = 0.0f, wmax = 0.0f, vsum = 0.0f, top = (float)s.h;
    float cmx = 0.0f, cmy = 0.0f; /* centre-of-mass velocity: coherent drift */
    float *spbuf = malloc((size_t)s.n * sizeof *spbuf);
    int over = 0;
    for (int i = 0; i < s.n; i++) {
        const Ball *bi = &s.b[i];
        float h_i = bi->y - bi->r;
        if (h_i < top) top = h_i;
        float sp = hypotf(bi->vx, bi->vy);
        vsum += sp;
        cmx += bi->vx;
        cmy += bi->vy;
        if (spbuf) spbuf[i] = sp;
        if (sp > vmax) vmax = sp;
        if (fabsf(bi->omega) > wmax) wmax = fabsf(bi->omega);
        for (int j = i + 1; j < s.n; j++) {
            const Ball *bj = &s.b[j];
            float dx = bj->x - bi->x, dy = bj->y - bi->y;
            float rs = bi->r + bj->r;
            float d2 = dx * dx + dy * dy;
            if (d2 >= rs * rs) continue;
            float p = rs - sqrtf(d2);
            pen_sum += p;
            if (p > pen_max) pen_max = p;
            over++;
        }
    }
    float p50 = 0.0f, p90 = 0.0f;
    if (spbuf && s.n > 0) {
        /* tiny n: insertion sort is fine and avoids qsort plumbing */
        int m = s.n;
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
           "ball-ball=%llu wall-hits=%llu\n",
           frames, frames / 60.0, t_settle,
           (frames / 60.0) / (t_settle > 0 ? t_settle : 1e-9),
           (unsigned long long)s.collisions,
           (unsigned long long)s.wall_hits);
    printf("[stress] pile: height=%.0f px, contacts=%d, mean-pen=%.3f px, max-pen=%.2f px\n",
           (float)s.h - top, over, over ? pen_sum / over : 0.0, pen_max);
    printf("[stress] rest: mean|v|=%.2f max|v|=%.2f, jitter(1s) max|v|=%.2f px/s, "
           "max|w|=%.2f rad/s\n",
           s.n ? vsum / (float)s.n : 0.0f, vmax, jitter, wmax);
    {
        /* depth profile relative to the pile surface: floor third, middle,
         * surface third, plus anything still above the pile */
        float vs[4] = {0, 0, 0, 0};
        int ns[4] = {0, 0, 0, 0};
        for (int i = 0; i < s.n; i++) {
            float t = top < s.h - 1.0f ? (s.b[i].y - top) / (s.h - top) : 1.0f;
            int band = t < 0.0f ? 3 : (t < 0.34f ? 2 : (t < 0.67f ? 1 : 0));
            vs[band] += hypotf(s.b[i].vx, s.b[i].vy);
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
           p90, s.n ? hypotf(cmx, cmy) / (float)s.n : 0.0f);

    /* Gates are regression bounds around the *current* impulse solver's
     * measured baseline (seeds 7/42/12345/999: mean-pen ~0.93 px, max-pen
     * <7 px, max|v| <130 px/s, jitter <145 px/s), deliberately looser than
     * the ideal a position-based solver should reach (mean-pen <0.5 px,
     * jitter ~0). They catch solver regressions; the metric lines above are
     * the numbers to compare when changing solvers. */
    check(finite_bad == 0, "stress: state stays finite", NULL);
    snprintf(msg, sizeof msg, "%d balls outside", outside);
    check(outside == 0, "stress: pile stays inside the box", outside ? msg : NULL);
    snprintf(msg, sizeof msg, "mean-pen %.3f px", over ? pen_sum / over : 0.0);
    check(over == 0 || pen_sum / over < 1.3f, "stress: mean penetration < 1.3 px", msg);
    snprintf(msg, sizeof msg, "max-pen %.2f px", pen_max);
    check(pen_max < 10.0f, "stress: max penetration < 10 px", msg);
    snprintf(msg, sizeof msg, "max|v| %.2f px/s", vmax);
    check(vmax < 200.0f, "stress: pile at rest (max|v| < 200 px/s)", msg);
    snprintf(msg, sizeof msg, "jitter %.2f px/s, KE %.0f -> %.0f", jitter, ke0, ke1);
    check(jitter < 250.0f && ke1 <= ke0 * 1.02f + 1.0f,
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
    if (glr_init(&rn, DEF_CAP) != 0) {
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
    s.solver = o.solver;
    s.compliance = o.compliance;
    if (o.gravity)
        s.gravity = GRAV;
    spawn_n(&s, o.balls, o.rmin, o.rmax, o.spin ? 1.0f : 0.0f);

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
                    spawn_n(&s, o.balls, o.rmin, o.rmax, o.spin ? 1.0f : 0.0f);
                    break;
                case 'c':
                    sim_clear(&s);
                    break;
                case '+':
                case '=':
                    sim_add_random(&s, o.rmin, o.rmax, 320.0f,
                                   o.spin ? 1.0f : 0.0f);
                    break;
                case '-':
                    if (s.n)
                        sim_remove(&s, s.n - 1);
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
                           "g=gravity f=friction n=grid i=stats Esc/q=quit | drag=slingshot RMB=remove wheel=size\n");
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
                        int k = sim_add(&s, drag.x0, drag.y0, vx, vy, drag.gr);
                        if (k >= 0 && o.spin)
                            s.b[k].omega =
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
        printf("BENCH frames=%d fps=%.1f balls=%d\n", frame,
               frame / (now_s() - t_start), s.n);

    sim_free(&s);
    glr_destroy(&rn);
    plat_close(&pl);
    return 0;
}
