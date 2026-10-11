/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 * gl_draw.h - OpenGL ES 2 renderer: one indexed draw call for all balls.
 *
 * Balls are screen-space quads; the fragment shader turns each quad into an
 * anti-aliased disc with a signed distance function (no textures, no MSAA
 * required). A pair of antipodal "billiard dots" (procedural, darkened in
 * the FS) makes the ball's rotation visible. A second tiny program draws
 * flat lines (drag preview, grid).
 */

#ifndef GL_DRAW_H
#define GL_DRAW_H

#include "sim.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <stdbool.h>

typedef struct {
    float x0, y0;   /* drag start (also where the ghost ball appears) */
    float x1, y1;   /* current drag end */
    float gr;       /* ghost ball radius */
} Drag;

typedef struct {
    GLuint prog_ball, prog_line;
    GLint a_corner, a_center, a_radius, a_color, a_alpha, a_ang;
    GLint a_pos_line;
    GLint u_res_ball, u_res_line, u_color_line;
    GLuint vbo, ebo;
    int cap;
    float *v; /* CPU staging buffer, 10 floats per vertex, 4 vertices per ball */

    float *lines;   /* line staging buffer, 2 floats per vertex (drag, grid) */
    int line_cap;   /* vertices allocated */
    int line_n;     /* vertices queued since the last flush */

    GLuint fbo, fbo_tex; /* offscreen target (selftest), 0 = default framebuffer */
    int fbo_w, fbo_h;
} Renderer;

int glr_init(Renderer *r, int cap);
void glr_destroy(Renderer *r);
int glr_enable_fbo(Renderer *r, int w, int h);
/* show_grid != 0 draws the broad-phase neighbour grid (toggled with 'n'). */
void glr_draw(Renderer *r, const Sim *sim, int hover, const Drag *drag, int w,
              int h, int show_grid);
int glr_dump_ppm(Renderer *r, const char *path); /* reads the current target */
/* Read the offscreen FBO into a newly malloc'ed RGBA buffer (caller frees). */
int glr_read_pixels(Renderer *r, unsigned char **out, int *w, int *h);

#endif /* GL_DRAW_H */
