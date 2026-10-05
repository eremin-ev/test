/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 * sim.h - 2D billiard-ball physics: no gravity, no rendering, no X/GL deps.
 *
 * Mass model: m = density * r^mass_exp  (mass_exp = 2 for 2D disks, i.e.
 * mass proportional to area; 1 or 3 selectable from the command line).
 */

#ifndef SIM_H
#define SIM_H

#include <stdbool.h>
#include <stdint.h>

#define SIM_MAX_SPEED 4000.0f /* px/s, keeps per-substep travel below min radius */
#define SIM_MAX_OMEGA 60.0f   /* rad/s, keeps spin visually readable and energy bounded */

/* Contact solver for sim_step(). IMPULSE is the original scheme (sequential
 * impulses + Baumgarte-style positional correction). XPBD solves position
 * constraints (disc-disc distance, disc-wall plane) with Gauss-Seidel and
 * Lagrange multipliers, restarts velocity from the pre-solve state and
 * solves contact impulses separately (iterated one-sided sweeps, bounce
 * only above a speed threshold) so resting piles jam and sleep. */
enum { SIM_SOLVER_IMPULSE = 0, SIM_SOLVER_XPBD = 1 };

typedef struct {
    float x, y;   /* centre in pixels, origin top-left, +y down */
    float vx, vy; /* px/s */
    float r;      /* radius, px */
    float m;      /* mass */
    float inv_m;  /* 1/m, precomputed for impulses */
    float ang;    /* orientation, rad, wrapped to [0, 2pi); rendering only */
    float omega;  /* angular velocity, rad/s (screen: + is clockwise) */
    float I;      /* moment of inertia, solid disk: 0.5 * m * r^2 */
    float inv_I;  /* 1/I, precomputed for angular impulses */
    float xp, yp;   /* xpbd scratch: position before the substep */
    float vpx, vpy; /* xpbd scratch: velocity before the contact solve */
    float cr, cg, cb;
} Ball;

typedef struct {
    Ball *b;
    int n, cap;

    float w, h;             /* box size (the window client area) */
    float restitution;      /* e for ball-ball and ball-wall */
    float friction;         /* Coulomb mu for tangential impulses; 0 = frictionless */
    int solver;             /* SIM_SOLVER_IMPULSE (default) or SIM_SOLVER_XPBD */
    float compliance;       /* xpbd softness alpha; 0 = rigid projection */
    float gravity;          /* px/s^2, +y (down the screen); 0 = billiards */
    float density;          /* mass per px^mass_exp */
    int mass_exp;           /* 1, 2 or 3 */

    uint64_t collisions;    /* ball-ball contacts resolved */
    uint64_t wall_hits;
    uint64_t rng;

    /* broad phase: uniform grid, one ball per cell bucket, rebuilt per substep */
    int cell, gw, gh;
    int head_cap, *head;
    int next_cap, *next;
    float max_r;

    /* xpbd scratch: contact lists with accumulated Lagrange multipliers,
     * valid within one substep (XPair/XWall live in sim.c); nc[] counts
     * pair contacts per ball for the sparse-impact gate */
    void *xpair_buf, *xwall_buf;
    int xpair_cap, xwall_cap, xn_pair, xn_wall;
    int nc_cap, *nc;
} Sim;

void sim_init(Sim *s, int cap, float w, float h, float restitution,
              float density, int mass_exp);
void sim_free(Sim *s);
void sim_resize(Sim *s, float w, float h);
void sim_clear(Sim *s);
void sim_set_seed(Sim *s, uint64_t seed);

/* Add one ball; returns its index or -1 if full / no room. */
int sim_add(Sim *s, float x, float y, float vx, float vy, float r);
/* Add a ball of random radius rmin..rmax at a random free spot.
 * spin in [0,1] scales a random initial angular velocity (0 = no spin). */
int sim_add_random(Sim *s, float rmin, float rmax, float speed, float spin);
/* Remove ball at index i (order-preserving swap). */
void sim_remove(Sim *s, int i);

/* Advance one substep of dt seconds (integrate, walls, then collisions). */
void sim_step(Sim *s, float dt);

/* m = density * r^mass_exp */
float sim_mass(const Sim *s, float r);

float sim_kinetic(const Sim *s);  /* sum 0.5*m*v^2 + 0.5*I*w^2 */
float sim_rand01(Sim *s);         /* deterministic [0,1) draw from the sim RNG */
void sim_momentum(const Sim *s, float *px, float *py);
int sim_pick(const Sim *s, float x, float y); /* topmost ball at point, -1 none */

#endif /* SIM_H */
