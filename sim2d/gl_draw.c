/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 * gl_draw.c - see gl_draw.h. Background is white; everything else is drawn
 * on top of it (particles as SDF discs, drag preview and neighbour grid as
 * quads). Bodies contribute one quad per particle, in particle order.
 *
 */

#include "gl_draw.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FLOATS_PER_VERTEX 11
#define VERTS_PER_PARTICLE 4

/* PARTICLE_SKIN: a composite body's particles are drawn this much wider than
 * they collide, so the hex interstices read as solid fill while the physics
 * stays overlap-free. The geometric covering radius is 1/cos 30deg = 1.1547
 * (a hole centre sits exactly 2r/sqrt(3) from three lattice points) - but that
 * leaves zero margin, and the FS AA band starts 1 px INSIDE the rim, so at
 * lattice radii of a few pixels the three covering discs do not quite reach
 * opacity together and the packing shows as a honeycomb of light dots. 1.2 is
 * measured clean at r >= 4 px; the cost is 0.2*r of visual overhang instead of
 * 0.15*r, most visible at corners. Single-particle bodies are NOT skinned -
 * they are discs already. */
#define PARTICLE_SKIN 1.2f

/* DOT_OFFSET: spin-marker dot centre in disc units (r = 1); defined inside
 * the shader source so GLSL can see it too. */
static const char *VS_BALL =
    "#define DOT_OFFSET 0.55\n"
    "attribute vec2 a_corner;\n"
    "attribute vec2 a_center;\n"
    "attribute float a_radius;\n"
    "attribute vec3 a_color;\n"
    "attribute float a_alpha;\n"
    "attribute float a_ang;\n"
    "attribute float a_flat;\n"
    "uniform vec2 u_res;\n"
    "varying vec2 v_disc;\n"
    "varying vec3 v_color;\n"
    "varying float v_alpha;\n"
    "varying float v_aa;\n"
    "varying vec2 v_dot;\n"
    "varying float v_dot_on;\n"
    "varying float v_flat;\n"
    "void main(void) {\n"
    "    vec2 p = a_center + a_corner * a_radius;\n"
    "    vec2 clip = (p / u_res) * 2.0 - 1.0;\n"
    "    gl_Position = vec4(clip.x, -clip.y, 0.0, 1.0);\n"
    "    v_disc = a_corner;\n"
    "    v_color = a_color;\n"
    "    v_alpha = a_alpha;\n"
    "    v_aa = 1.0 / max(a_radius, 1.0);\n" /* one pixel, in disc units */
    /* rotate the marker centre by the ball angle (VS is highp, FS is not) */
    "    float ca = cos(a_ang < 0.0 ? 0.0 : a_ang);\n"
    "    float sa = sin(a_ang < 0.0 ? 0.0 : a_ang);\n"
    "    v_dot = vec2(ca, sa) * DOT_OFFSET;\n"
    "    v_dot_on = a_ang < 0.0 ? 0.0 : 1.0;\n" /* negative angle: no marker */
    "    v_flat = a_flat;\n"
    "}\n";

static const char *FS_BALL =
    "precision mediump float;\n"
    "varying vec2 v_disc;\n"
    "varying vec3 v_color;\n"
    "varying float v_alpha;\n"
    "varying float v_aa;\n"
    "varying vec2 v_dot;\n"
    "varying float v_dot_on;\n"
    "varying float v_flat;\n"
    "void main(void) {\n"
    "    float d = length(v_disc);\n"
    "    float a = v_alpha * (1.0 - smoothstep(1.0 - v_aa, 1.0 + v_aa, d));\n"
    "    if (a <= 0.002) discard;\n"
    /* the radial gradient is what makes a disc read as a ball; on a raft of
     * particles the same gradient draws every particle as its own bubble, so
     * composite bodies ask for flat fill and read as one solid shape */
    "    float shade = mix(mix(0.70, 1.0, smoothstep(1.0, 0.40, d)), 1.0, v_flat);\n"
    "    vec3 col = v_color * shade;\n"
    /* antipodal spin markers: two dark dots on a rotating diameter */
    "    float dd = min(length(v_disc - v_dot), length(v_disc + v_dot));\n"
    "    float dotm = v_dot_on * (1.0 - smoothstep(0.15, 0.21, dd));\n"
    "    col = mix(col, col * 0.40, dotm);\n"
    "    gl_FragColor = vec4(col * a, a);\n" /* premultiplied */
    "}\n";

static const char *VS_LINE =
    "attribute vec2 a_pos;\n"
    "uniform vec2 u_res;\n"
    "void main(void) {\n"
    "    vec2 clip = (a_pos / u_res) * 2.0 - 1.0;\n"
    "    gl_Position = vec4(clip.x, -clip.y, 0.0, 1.0);\n"
    "}\n";

static const char *FS_LINE =
    "precision mediump float;\n"
    "uniform vec4 u_color;\n"
    "void main(void) { gl_FragColor = u_color; }\n";

/* ------------------------------------------------------------- utilities */

static GLuint compile(GLenum type, const char *src)
{
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, NULL);
    glCompileShader(sh);
    GLint ok = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(sh, sizeof log, NULL, log);
        fprintf(stderr, "shader compile failed: %s\n", log);
        glDeleteShader(sh);
        return 0;
    }
    return sh;
}

static GLuint link(const char *vs, const char *fs)
{
    GLuint v = compile(GL_VERTEX_SHADER, vs);
    GLuint f = compile(GL_FRAGMENT_SHADER, fs);
    if (!v || !f)
        return 0;
    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    glDeleteShader(v);
    glDeleteShader(f);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(p, sizeof log, NULL, log);
        fprintf(stderr, "program link failed: %s\n", log);
        glDeleteProgram(p);
        return 0;
    }
    return p;
}

/* ----------------------------------------------------------------- setup */

int glr_init(Renderer *r, int cap)
{
    memset(r, 0, sizeof *r);
    r->cap = cap;
    r->v = malloc((size_t)cap * VERTS_PER_PARTICLE * FLOATS_PER_VERTEX * sizeof *r->v);
    if (!r->v)
        return -1;

    r->prog_ball = link(VS_BALL, FS_BALL);
    /* one quad per particle, not per body: capacity is in particles */
    r->prog_line = link(VS_LINE, FS_LINE);
    if (!r->prog_ball || !r->prog_line)
        return -1;

    r->a_corner = glGetAttribLocation(r->prog_ball, "a_corner");
    r->a_center = glGetAttribLocation(r->prog_ball, "a_center");
    r->a_radius = glGetAttribLocation(r->prog_ball, "a_radius");
    r->a_color = glGetAttribLocation(r->prog_ball, "a_color");
    r->a_alpha = glGetAttribLocation(r->prog_ball, "a_alpha");
    r->a_ang = glGetAttribLocation(r->prog_ball, "a_ang");
    r->a_flat = glGetAttribLocation(r->prog_ball, "a_flat");
    r->u_res_ball = glGetUniformLocation(r->prog_ball, "u_res");

    r->u_res_line = glGetUniformLocation(r->prog_line, "u_res");
    r->u_color_line = glGetUniformLocation(r->prog_line, "u_color");
    r->a_pos_line = glGetAttribLocation(r->prog_line, "a_pos");

    glGenBuffers(1, &r->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, r->vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 (GLsizeiptr)cap * VERTS_PER_PARTICLE * FLOATS_PER_VERTEX * sizeof(float),
                 NULL, GL_DYNAMIC_DRAW);

    /* Static index buffer: 4 vertices per ball, 2 triangles per quad. */
    static const unsigned char QUAD[6] = { 0, 1, 2, 2, 1, 3 };
    GLuint *idx = malloc((size_t)cap * 6 * sizeof *idx);
    if (!idx)
        return -1;
    for (int i = 0; i < cap; i++)
        for (int k = 0; k < 6; k++)
            idx[i * 6 + k] = (GLuint)(i * VERTS_PER_PARTICLE + QUAD[k]);

    glGenBuffers(1, &r->ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cap * 6 * sizeof(GLuint),
                 idx, GL_STATIC_DRAW);
    free(idx);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); /* premultiplied */
    glDisable(GL_DEPTH_TEST);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f); /* white background */
    return 0;
}

void glr_destroy(Renderer *r)
{
    if (r->fbo)
        glDeleteFramebuffers(1, &r->fbo);
    if (r->fbo_tex)
        glDeleteTextures(1, &r->fbo_tex);
    if (r->vbo)
        glDeleteBuffers(1, &r->vbo);
    if (r->ebo)
        glDeleteBuffers(1, &r->ebo);
    if (r->prog_ball)
        glDeleteProgram(r->prog_ball);
    if (r->prog_line)
        glDeleteProgram(r->prog_line);
    free(r->v);
    free(r->lines);
    memset(r, 0, sizeof *r);
}

int glr_enable_fbo(Renderer *r, int w, int h)
{
    glGenTextures(1, &r->fbo_tex);
    glBindTexture(GL_TEXTURE_2D, r->fbo_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &r->fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, r->fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           r->fbo_tex, 0);
    GLenum st = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (st != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "framebuffer incomplete (0x%04x)\n", st);
        return -1;
    }
    r->fbo_w = w;
    r->fbo_h = h;
    return 0;
}

/* ----------------------------------------------------------------- draw */

/* alpha < 0 in `ang` disables the spin marker (ghost preview, and every
 * particle of a composite body). */
static void push_particle(float **v, float x, float y, float r, float cr,
                          float cg, float cb, float alpha, float ang, float flat)
{
    static const float cx[VERTS_PER_PARTICLE] = { -1, 1, -1, 1 };
    static const float cy[VERTS_PER_PARTICLE] = { -1, -1, 1, 1 };
    float *p = *v;
    for (int i = 0; i < VERTS_PER_PARTICLE; i++) {
        p[0] = cx[i];
        p[1] = cy[i];
        p[2] = x;
        p[3] = y;
        p[4] = r;
        p[5] = cr;
        p[6] = cg;
        p[7] = cb;
        p[8] = alpha;
        p[9] = ang;
        p[10] = flat;
        p += FLOATS_PER_VERTEX;
    }
    *v = p;
}

/* ------------------------------------------------------- line batching */
/* Lines are screen-space quads queued in r->lines and flushed as one
 * glDrawArrays call, so the drag preview and the whole grid cost a single
 * draw call each. */

static int lines_reserve(Renderer *r, int verts)
{
    if (verts <= r->line_cap)
        return 0;
    int cap = r->line_cap ? r->line_cap : 256;
    while (cap < verts)
        cap *= 2;
    float *p = realloc(r->lines, (size_t)cap * 2 * sizeof *p);
    if (!p)
        return -1;
    r->lines = p;
    r->line_cap = cap;
    return 0;
}

/* Queue one segment as a `width` px wide quad. Returns 1 if it was added. */
static int lines_push(Renderer *r, float x0, float y0, float x1, float y1,
                      float width)
{
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 1e-3f)
        return 0;
    if (lines_reserve(r, r->line_n + 6) != 0)
        return 0;
    float nx = -dy / len * width * 0.5f, ny = dx / len * width * 0.5f;
    float *p = r->lines + (size_t)r->line_n * 2;
    p[0] = x0 + nx; p[1] = y0 + ny;
    p[2] = x1 + nx; p[3] = y1 + ny;
    p[4] = x1 - nx; p[5] = y1 - ny;
    p[6] = x0 + nx; p[7] = y0 + ny;
    p[8] = x1 - nx; p[9] = y1 - ny;
    p[10] = x0 - nx; p[11] = y0 - ny;
    r->line_n += 6;
    return 1;
}

static void lines_flush(Renderer *r, float cr, float cg, float cb, float a,
                        int w, int h)
{
    if (r->line_n <= 0)
        return;
    glUseProgram(r->prog_line);
    GLint a_pos = r->a_pos_line;
    glUniform2f(r->u_res_line, (float)w, (float)h);
    glUniform4f(r->u_color_line, cr * a, cg * a, cb * a, a);
    glBindBuffer(GL_ARRAY_BUFFER, r->vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)r->line_n * 2 * sizeof(float),
                 r->lines, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray((GLuint)a_pos);
    glVertexAttribPointer((GLuint)a_pos, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float),
                          (const void *)0);
    glDrawArrays(GL_TRIANGLES, 0, r->line_n);
    glDisableVertexAttribArray((GLuint)a_pos);
    r->line_n = 0;
}

/* --------------------------------------------------- broad-phase grid */
/* The neighbour grid the physics uses: cell = 2 * largest radius. Cells are
 * drawn faintly, cells holding at least one ball are outlined so you can see
 * where the half-neighbourhood scan actually does work. */
#define GRID_MAX_SEGS 8192

static void draw_grid(Renderer *r, const Sim *sim, int w, int h)
{
    int cell = sim->cell, gw = sim->gw, gh = sim->gh;
    if (cell <= 0 || gw <= 0 || gh <= 0 || !sim->head)
        return;

    /* grid lines */
    r->line_n = 0;
    for (int i = 0; i <= gw; i++) {
        float x = (float)i * cell;
        if (x > w) break;
        lines_push(r, x, 0.0f, x, (float)h, 1.0f);
    }
    for (int j = 0; j <= gh; j++) {
        float y = (float)j * cell;
        if (y > h) break;
        lines_push(r, 0.0f, y, (float)w, y, 1.0f);
    }
    lines_flush(r, 0.35f, 0.40f, 0.50f, 0.35f, w, h);

    /* occupied cells */
    r->line_n = 0;
    int segs = 0;
    for (int cy = 0; cy < gh && segs < GRID_MAX_SEGS; cy++) {
        for (int cx = 0; cx < gw && segs < GRID_MAX_SEGS; cx++) {
            if (sim->head[cy * gw + cx] < 0)
                continue;
            float x0 = (float)cx * cell, y0 = (float)cy * cell;
            float x1 = x0 + cell, y1 = y0 + cell;
            lines_push(r, x0, y0, x1, y0, 2.0f);
            lines_push(r, x1, y0, x1, y1, 2.0f);
            lines_push(r, x1, y1, x0, y1, 2.0f);
            lines_push(r, x0, y1, x0, y0, 2.0f);
            segs += 4;
        }
    }
    lines_flush(r, 0.10f, 0.45f, 0.85f, 0.75f, w, h);
}

void glr_draw(Renderer *r, const Sim *sim, int hover, const Drag *drag, int w,
              int h, int show_grid)
{
    if (r->fbo)
        glBindFramebuffer(GL_FRAMEBUFFER, r->fbo);
    glViewport(0, 0, w, h);
    glClear(GL_COLOR_BUFFER_BIT);

    int room = r->cap;
    if (drag && sim->np < room)
        room--; /* one slot for the ghost particle */

    float *v = r->v;
    int nq = 0; /* quads queued */
    for (int i = 0; i < sim->nb && nq < room; i++) {
        const Body *b = &sim->body[i];
        float k = (i == hover) ? 0.65f : 1.0f; /* darken on white */
        float cr = b->cr * k, cg = b->cg * k, cb = b->cb * k;
        if (cr > 1) cr = 1;
        if (cg > 1) cg = 1;
        if (cb > 1) cb = 1;
        /* Keyed on pn, not on the shape tag: a coarse lattice can legitimately
         * reduce a small box to a single particle, and then it IS a disc -
         * marker on, no skin, radial shading. Negative ang means "no marker". */
        int one = b->pn == 1;
        float mark = one ? b->ang : -1.0f;
        float skin = one ? 1.0f : PARTICLE_SKIN;
        for (int q = b->p0; q < b->p0 + b->pn && nq < room; q++, nq++)
            push_particle(&v, sim->p[q].x, sim->p[q].y, sim->p[q].r * skin, cr,
                          cg, cb, 1.0f, mark, one ? 0.0f : 1.0f);
    }
    if (drag && nq < r->cap)
        /* ghost preview (dark: background is white) */
        push_particle(&v, drag->x0, drag->y0, drag->gr, 0.15f, 0.15f, 0.15f,
                      0.30f, -1.0f, 0.0f), nq++;
    int count = nq;

    glUseProgram(r->prog_ball);
    glUniform2f(r->u_res_ball, (float)w, (float)h);
    glBindBuffer(GL_ARRAY_BUFFER, r->vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)count * VERTS_PER_PARTICLE *
                                       FLOATS_PER_VERTEX * sizeof(float),
                 r->v, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r->ebo);

    const int stride = FLOATS_PER_VERTEX * (int)sizeof(float);
    glEnableVertexAttribArray((GLuint)r->a_corner);
    glEnableVertexAttribArray((GLuint)r->a_center);
    glEnableVertexAttribArray((GLuint)r->a_radius);
    glEnableVertexAttribArray((GLuint)r->a_color);
    glEnableVertexAttribArray((GLuint)r->a_alpha);
    glEnableVertexAttribArray((GLuint)r->a_ang);
    glEnableVertexAttribArray((GLuint)r->a_flat);
    glVertexAttribPointer((GLuint)r->a_corner, 2, GL_FLOAT, GL_FALSE, stride,
                          (const void *)0);
    glVertexAttribPointer((GLuint)r->a_center, 2, GL_FLOAT, GL_FALSE, stride,
                          (const void *)(2 * sizeof(float)));
    glVertexAttribPointer((GLuint)r->a_radius, 1, GL_FLOAT, GL_FALSE, stride,
                          (const void *)(4 * sizeof(float)));
    glVertexAttribPointer((GLuint)r->a_color, 3, GL_FLOAT, GL_FALSE, stride,
                          (const void *)(5 * sizeof(float)));
    glVertexAttribPointer((GLuint)r->a_alpha, 1, GL_FLOAT, GL_FALSE, stride,
                          (const void *)(8 * sizeof(float)));
    glVertexAttribPointer((GLuint)r->a_ang, 1, GL_FLOAT, GL_FALSE, stride,
                          (const void *)(9 * sizeof(float)));
    glVertexAttribPointer((GLuint)r->a_flat, 1, GL_FLOAT, GL_FALSE, stride,
                          (const void *)(10 * sizeof(float)));
    glDrawElements(GL_TRIANGLES, count * 6, GL_UNSIGNED_INT, 0);

    if (show_grid)
        draw_grid(r, sim, w, h);

    if (drag) {
        r->line_n = 0;
        lines_push(r, drag->x0, drag->y0, drag->x1, drag->y1, 3.0f);
        lines_flush(r, 0.10f, 0.10f, 0.10f, 0.8f, w, h);
    }

    if (r->fbo)
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/* ------------------------------------------------------------ readback */

int glr_read_pixels(Renderer *r, unsigned char **out, int *w, int *h)
{
    int W = r->fbo ? r->fbo_w : 0, H = r->fbo ? r->fbo_h : 0;
    if (W <= 0 || H <= 0) {
        fprintf(stderr, "read_pixels needs an offscreen target\n");
        return -1;
    }
    unsigned char *rgba = malloc((size_t)W * H * 4);
    if (!rgba)
        return -1;
    glBindFramebuffer(GL_FRAMEBUFFER, r->fbo);
    glReadPixels(0, 0, W, H, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    *out = rgba;
    if (w)
        *w = W;
    if (h)
        *h = H;
    return 0;
}

int glr_dump_ppm(Renderer *r, const char *path)
{
    unsigned char *rgba = NULL;
    int w = 0, h = 0;
    if (glr_read_pixels(r, &rgba, &w, &h) != 0)
        return -1;
    unsigned char *row = malloc((size_t)w * 3);
    if (!row) {
        free(rgba);
        return -1;
    }

    FILE *f = fopen(path, "wb");
    if (!f) {
        free(rgba);
        free(row);
        return -1;
    }
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    for (int y = 0; y < h; y++) { /* GL origin is bottom-left, PPM top-left */
        const unsigned char *src = rgba + (size_t)(h - 1 - y) * (size_t)w * 4;
        for (int x = 0; x < w; x++) {
            row[x * 3 + 0] = src[x * 4 + 0];
            row[x * 3 + 1] = src[x * 4 + 1];
            row[x * 3 + 2] = src[x * 4 + 2];
        }
        fwrite(row, 1, (size_t)w * 3, f);
    }
    fclose(f);
    free(rgba);
    free(row);
    return 0;
}
