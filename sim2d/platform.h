/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 * platform.h - window + EGL bootstrap.
 *
 * Windowed mode: pure XCB (no Xlib in our code) + EGL_EXT_platform_xcb.
 * Offscreen mode: EGL_MESA_platform_surfaceless, for headless self-tests.
 */

#ifndef PLATFORM_H
#define PLATFORM_H

/* We speak XCB, not Xlib: keep X11/Xlib.h out of the EGL headers. */
#ifndef EGL_NO_X11
#define EGL_NO_X11 1
#endif

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <stdbool.h>
#include <stdint.h>
#include <xcb/xcb.h>

/* Keysyms: printable keys arrive as their Latin-1/ASCII value, so the keymap
 * lookup gives us 'q', ' ', '+' ... for the rest we need explicit constants. */
#define KS_ESCAPE 0xFF1Bu
#define KS_ENTER 0xFF0Du

typedef enum { PE_NONE, PE_QUIT, PE_RESIZE, PE_KEY, PE_BUTTON, PE_MOTION } PeType;

typedef struct {
    PeType type;
    int x, y;         /* pointer, window relative */
    unsigned keysym;  /* PE_KEY */
    int press;        /* 1 press, 0 release */
    int button;       /* 1,2,3; 4/5 = wheel up/down */
    int w, h;         /* PE_RESIZE */
} PlatEvent;

typedef struct {
    bool windowed;
    int width, height;

    EGLDisplay dpy;
    EGLConfig cfg;
    EGLContext ctx;
    EGLSurface surf; /* EGL_NO_SURFACE in surfaceless mode */

    xcb_connection_t *xc;
    xcb_window_t win;
    xcb_atom_t atom_delete;

    uint32_t ks[256];     /* keycode -> keysym, unshifted */
    uint32_t ks_shift[256];
} Plat;

/* Both return 0 on success, -1 on failure (see plat_last_error()). */
int plat_open_window(Plat *p, int w, int h, const char *title, int msaa);
int plat_open_offscreen(Plat *p, int w, int h);

void plat_close(Plat *p);
int plat_fd(const Plat *p);            /* fd to poll(), -1 when offscreen */
int plat_poll(Plat *p, PlatEvent *ev); /* 1 = event returned in *ev, 0 = none */
void plat_swap(Plat *p);
const char *plat_last_error(void);

#endif /* PLATFORM_H */
