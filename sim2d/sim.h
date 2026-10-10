/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 * sim.h - 2D rigid bodies, solved as collections of circular PARTICLES with
 * XPBD. No gravity by default, no rendering, no X/GL deps.
 *
 * Model
 * -----
 * The one and only collision primitive is the particle: a disc with a radius.
 * A BODY is a rigid set of particles that do not overlap each other (they may
 * touch); every particle carries an offset in the body frame and a copy of
 * its world position, rebuilt from the body transform. A plain disc is a body
 * with exactly one particle at offset (0,0), so the classic billiard ball is
 * the degenerate case of the general model, not a special case in the code.
 *
 * Convex shapes (rectangles, triangles, capsules) are approximated by a
 * hexagonal packing of touching particles, generated over the shape ERODED by
 * one particle radius - so the outline is the shape rounded to that radius,
 * the packed body never pokes outside its nominal size (placement against
 * walls stays safe), and 2*r is the minimum feature that survives. Measured
 * coverage of the nominal area is 0.55-0.75 at the resolutions this uses;
 * 0.907 is the infinite-packing limit, reachable only when the shape is many
 * particle radii across. Other consequences, all geometric and not tunable:
 * the deepest a foreign particle can sink into a hex interstice is 0.1547*r,
 * an equal-radius raft riding on another dips (2-sqrt(3))*r = 0.268*r (the
 * "gear mesh" that makes neighbouring bodies ratchet - which is why bodies get
 * per-body lattice radii), and past pr ~ 0.25 of the thinnest feature a
 * packing degenerates to one or two particles (see sim_add_sized).
 *
 * Mass model: m = density * reff^mass_exp with reff = sqrt(sum r_particle^2),
 * i.e. mass grows with the particle area a body carries. For a one-particle
 * body reff == r, so the disk law m = density * r^mass_exp is unchanged.
 *
 * Solver: XPBD only (see sim.c for why the old sequential-impulse solver and
 * the XPBD_DRAG term are gone).
 */

#ifndef SIM_H
#define SIM_H

#include <stdbool.h>
#include <stdint.h>

#define SIM_MAX_SPEED 4000.0f /* px/s, keeps per-substep travel below the smallest particle */
#define SIM_MAX_OMEGA 60.0f   /* rad/s, keeps spin visually readable and energy bounded */

#define SIM_MAX_BODIES 2000
#define SIM_MAX_PARTICLES 20000
#define SIM_MAX_PARTS_PER_BODY 256 /* pathological guard, not a design limit: an
                                    * oversized body is packed coarser rather
                                    * than refused (see add_lattice), and the
                                    * spawner's own ceilings keep ordinary bodies
                                    * near 20 particles. Set this too low and it
                                    * silently coarsens lattices, which moves a
                                    * body's mass by tens of percent. */
#define SIM_MIN_PARTICLE 2.0f     /* px: floor for a generated lattice radius */

/* Shape tag: bookkeeping, spawning and the renderer's spin-marker/flat-fill
 * rule. The solver never looks at it - it only sees particle sets. */
enum {
    SIM_SHAPE_DISC = 0,   /* one particle */
    SIM_SHAPE_BOX = 1,    /* hex lattice of a w x h rectangle */
    SIM_SHAPE_TRI = 2,    /* hex lattice of an equilateral triangle */
    SIM_SHAPE_CAPSULE = 3 /* hex lattice of a stadium */
};

/* A particle is the collision primitive: a disc of radius r. ox/oy are its
 * fixed offset in the body frame; x/y are world positions, kept in sync with
 * the body transform whenever the solver or the caller needs them (a
 * one-particle body has ox == oy == 0, so x/y == the body position).
 *
 * x/y are a SNAPSHOT. The solver never trusts them while the bodies are
 * moving: it recomputes them from (body x, y, ca, sa, ox, oy), because a
 * projection moves a body several times inside one Gauss-Seidel sweep and a
 * stale snapshot silently under-solves exactly the contacts that matter.
 * Anything outside sim.c may read x/y: sim_step leaves them current. */
typedef struct {
    float x, y;
    float r;
    float ox, oy;
    int b; /* owning body index */
} Particle;

typedef struct {
    float x, y;   /* centre of mass, pixels, origin top-left, +y down */
    float vx, vy; /* px/s */
    float ang;    /* orientation, rad, wrapped to [0, 2pi) */
    float ca, sa; /* cos/sin(ang), cached: refresh in sync_particles before use */
    float omega;  /* angular velocity, rad/s (screen: + is clockwise) */
    float m, inv_m;
    float I, inv_I; /* about the centre of mass, from the particle set */
    float rad;      /* circumradius: max over particles of |offset| + r */
    float reff;     /* sqrt(sum r^2): drives the mass law and the colour ramp */
    int p0, pn;     /* this body's particles are Sim.p[p0 .. p0+pn) */
    int shape;      /* SIM_SHAPE_* */
    int contacts;   /* particle contacts in the current substep (impact gate) */

    float xp, yp;   /* xpbd scratch: position before the substep */
    float vpx, vpy; /* xpbd scratch: velocity before the contact solve */

    float cr, cg, cb;
} Body;

typedef struct {
    Body *body;
    int nb, body_cap;
    Particle *p;
    int np, p_cap;

    float w, h;             /* box size (the window client area) */
    float restitution;      /* e for particle-particle and particle-wall */
    float friction;         /* Coulomb mu for tangential impulses; 0 = frictionless */
    float compliance;       /* xpbd softness alpha; 0 = rigid projection */
    float gravity;          /* px/s^2, +y (down the screen); 0 = billiards */
    float density;          /* mass per px^mass_exp */
    int mass_exp;           /* 1, 2 or 3 */

    uint64_t collisions;    /* particle-particle contacts resolved */
    uint64_t wall_hits;     /* particle-wall contacts resolved */
    uint64_t rng;

    /* broad phase: uniform grid over PARTICLES (so a body that spans several
     * cells costs nothing extra), cell = 2 * largest particle radius; the
     * several particle contacts one body pair generates are the manifold */
    int cell, gw, gh;
    int head_cap, *head;
    int next_cap, *next;
    float max_r;

    /* xpbd scratch: contact lists with accumulated Lagrange multipliers,
     * valid within one substep (PCpair/PCwall live in sim.c) */
    void *pair_buf, *wall_buf;
    int pair_cap, wall_cap, np_pair, np_wall;
} Sim;

void sim_init(Sim *s, int body_cap, float w, float h, float restitution,
              float density, int mass_exp);
void sim_free(Sim *s);
void sim_resize(Sim *s, float w, float h);
void sim_clear(Sim *s);
void sim_set_seed(Sim *s, uint64_t seed);

/* Add a rigid body. off is 2*np floats (ox,oy in the body frame), rad np
 * particle radii. Offsets are re-centred on the particle centroid, so pass
 * them in whatever frame the shape generator produced. Returns the body index
 * or -1 if there is no room. */
int sim_add_body(Sim *s, const float *off, const float *rad, int np, float x,
                 float y, float vx, float vy, int shape);
/* A one-particle body: the classic billiard disc. */
int sim_add_disc(Sim *s, float x, float y, float vx, float vy, float r);
/* Add one body of random size at a random free spot.
 * spin in [0,1] scales a random initial angular velocity (0 = no spin).
 * Always a disc, and keeps its historical RNG draw order. */
int sim_add_random(Sim *s, float rmin, float rmax, float speed, float spin);

/* Bodies made of a hexagonal lattice of touching particles of radius pr, in
 * the shape of a bw x bh rectangle, an equilateral triangle of circumradius
 * R, or a stadium (straight part L, end caps R). The lattice is generated
 * over the shape eroded by pr, so the outline is rounded to radius pr, 2*pr is
 * the smallest feature that survives, and the packed body never exceeds the
 * nominal size. If pr cannot fit (or cannot fit *few enough* particles, see
 * SIM_MAX_PARTS_PER_BODY) it is resized rather than the call failing. */
int sim_add_box(Sim *s, float x, float y, float vx, float vy, float bw, float bh,
                float pr);
int sim_add_tri(Sim *s, float x, float y, float vx, float vy, float R, float pr);
int sim_add_capsule(Sim *s, float x, float y, float vx, float vy, float L, float R,
                    float pr);
/* Build one body of the given shape at (x,y) from a nominal size sz - "the
 * radius a disc of similar bulk would have" - so all four shapes stay
 * comparable. Shape proportions and the lattice radius are drawn from the sim
 * RNG: pr within [pmin,pmax], then ceilinged at 0.2*sz and at 0.25 of the
 * thinnest feature (past that a packing degenerates to one or two particles)
 * and floored at SIM_MIN_PARTICLE. SIM_SHAPE_DISC is exactly sim_add_disc. */
int sim_add_sized(Sim *s, int shape, float x, float y, float vx, float vy,
                  float sz, float pmin, float pmax);
/* Random body of the given shape at a random free spot; see sim_add_sized for
 * how size and lattice radius are drawn. SIM_SHAPE_DISC forwards to
 * sim_add_random, keeping that path's historical RNG order. */
int sim_add_random_shape(Sim *s, int shape, float smin, float smax, float pmin,
                         float pmax, float speed, float spin);
/* Remove body at index i (order-preserving; its particles go with it). */
void sim_remove(Sim *s, int i);

/* Advance one substep of dt seconds (integrate, contacts, walls, solve).
 * Leaves Sim.p in sync with the body transforms. */
void sim_step(Sim *s, float dt);

/* m = density * r^mass_exp */
float sim_mass(const Sim *s, float r);

float sim_kinetic(const Sim *s);  /* sum 0.5*m*v^2 + 0.5*I*w^2 */
float sim_rand01(Sim *s);         /* deterministic [0,1) draw from the sim RNG */
void sim_momentum(const Sim *s, float *px, float *py);
int sim_pick(const Sim *s, float x, float y); /* topmost body at point, -1 none */

#endif /* SIM_H */
