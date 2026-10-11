/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 * platform_x11.c - XCB window + EGL context (see platform.h).
 *
 * Link requirements: -lEGL -lxcb (no Xlib: EGL_NO_X11 is defined and the
 * display is created through EGL_EXT_platform_xcb from the xcb_connection_t).
 */

#include "platform.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef EGL_PLATFORM_SURFACELESS_MESA
#define EGL_PLATFORM_SURFACELESS_MESA 0x31DD
#endif

static char g_err[512];

const char *plat_last_error(void)
{
    return g_err;
}

static void fail(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(g_err, sizeof g_err, fmt, ap);
    va_end(ap);
}

/* ---------------------------------------------------------------- config */

/* Try, in order: ES3+MSAA, ES3, ES2+MSAA, ES2. */
static int pick_config(Plat *p, int msaa, EGLint surface_type, EGLConfig *out)
{
    static const EGLint renderables[2] = { EGL_OPENGL_ES3_BIT, EGL_OPENGL_ES2_BIT };

    for (int r = 0; r < 2; r++) {
        for (int m = msaa > 0 ? 0 : 1; m <= 1; m++) {
            EGLint samples = (m == 0) ? msaa : 0;
            EGLint attrs[] = {
                EGL_SURFACE_TYPE, surface_type,
                EGL_RENDERABLE_TYPE, renderables[r],
                EGL_RED_SIZE, 8,
                EGL_GREEN_SIZE, 8,
                EGL_BLUE_SIZE, 8,
                EGL_ALPHA_SIZE, 0,
                EGL_DEPTH_SIZE, 0,
                EGL_SAMPLE_BUFFERS, samples > 0 ? 1 : 0,
                EGL_SAMPLES, samples,
                EGL_NONE
            };
            EGLConfig cfg;
            EGLint n = 0;
            if (eglChooseConfig(p->dpy, attrs, &cfg, 1, &n) && n >= 1) {
                *out = cfg;
                return renderables[r] == EGL_OPENGL_ES3_BIT ? 3 : 2;
            }
        }
    }
    return 0;
}

static int create_context(Plat *p, int want_es3)
{
    EGLint attrs_es3[] = { EGL_CONTEXT_MAJOR_VERSION, 3, EGL_NONE };
    EGLint attrs_es2[] = { EGL_CONTEXT_MAJOR_VERSION, 2, EGL_NONE };
    p->ctx = eglCreateContext(p->dpy, p->cfg, EGL_NO_CONTEXT,
                              want_es3 ? attrs_es3 : attrs_es2);
    return p->ctx != EGL_NO_CONTEXT;
}

/* ---------------------------------------------------------------- keymap */

static void load_keymap(Plat *p)
{
    const xcb_setup_t *su = xcb_get_setup(p->xc);
    uint8_t first = su->min_keycode;                 /* normally 8 */
    uint8_t count = (uint8_t)(su->max_keycode - su->min_keycode + 1);
    xcb_get_keyboard_mapping_cookie_t ck =
        xcb_get_keyboard_mapping(p->xc, first, count);
    xcb_get_keyboard_mapping_reply_t *rep =
        xcb_get_keyboard_mapping_reply(p->xc, ck, NULL);
    if (!rep)
        return;

    int per = rep->keysyms_per_keycode;
    /* The reply lists keysyms starting at the FIRST KEYCODE WE ASKED FOR
     * (min_keycode, normally 8), not at 0: index by real keycode. */
    if (per >= 1) {
        const xcb_keysym_t *d = xcb_get_keyboard_mapping_keysyms(rep);
        int n = (int)xcb_get_keyboard_mapping_keysyms_length(rep) / per;
        for (int k = 0; k < n; k++) {
            int code = first + k;
            if (code >= 256)
                break;
            p->ks[code] = d[(size_t)k * (size_t)per];
            p->ks_shift[code] =
                per > 1 ? d[(size_t)k * (size_t)per + 1] : p->ks[code];
        }
    }
    free(rep);
}

static unsigned keysym_of(const Plat *p, xcb_keycode_t code, uint16_t state)
{
    int shift = (state & (XCB_MOD_MASK_SHIFT | XCB_MOD_MASK_LOCK)) != 0;
    return shift ? p->ks_shift[code] : p->ks[code];
}

/* --------------------------------------------------------------- window */

static xcb_atom_t intern(xcb_connection_t *c, const char *name)
{
    xcb_intern_atom_cookie_t ck = xcb_intern_atom(c, 0, (uint16_t)strlen(name), name);
    xcb_intern_atom_reply_t *rep = xcb_intern_atom_reply(c, ck, NULL);
    xcb_atom_t a = XCB_ATOM_NONE;
    if (rep) {
        a = rep->atom;
        free(rep);
    }
    return a;
}

int plat_open_window(Plat *p, int w, int h, const char *title, int msaa)
{
    memset(p, 0, sizeof *p);
    p->surf = EGL_NO_SURFACE;
    p->ctx = EGL_NO_CONTEXT;
    p->dpy = EGL_NO_DISPLAY;

    int screen = 0;
    p->xc = xcb_connect(NULL, &screen);
    if (!p->xc || xcb_connection_has_error(p->xc)) {
        fail("cannot connect to the X server (DISPLAY=%s)",
             getenv("DISPLAY") ? getenv("DISPLAY") : "unset");
        return -1;
    }

    const xcb_setup_t *setup = xcb_get_setup(p->xc);
    xcb_screen_iterator_t it = xcb_setup_roots_iterator(setup);
    for (int i = 0; i < screen; i++)
        xcb_screen_next(&it);
    xcb_screen_t *scr = it.data;
    if (!scr) {
        fail("screen %d not found", screen);
        return -1;
    }

    p->dpy = eglGetPlatformDisplay(EGL_PLATFORM_XCB_EXT, (void *)p->xc, NULL);
    if (p->dpy == EGL_NO_DISPLAY) {
        /* Driver without EGL_EXT_platform_xcb: plain eglGetDisplay() still
         * works because the native window is the same XID either way. */
        p->dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    }
    if (p->dpy == EGL_NO_DISPLAY) {
        fail("no EGL display");
        return -1;
    }

    EGLint major = 0, minor = 0;
    if (!eglInitialize(p->dpy, &major, &minor)) {
        fail("eglInitialize failed (EGL error 0x%04x)", eglGetError());
        return -1;
    }
    eglBindAPI(EGL_OPENGL_ES_API);

    int es = pick_config(p, msaa, EGL_WINDOW_BIT, &p->cfg);
    if (!es) {
        fail("no suitable EGL config (need RGBA8 window, OpenGL ES)");
        return -1;
    }

    EGLint visual_id = 0;
    if (!eglGetConfigAttrib(p->dpy, p->cfg, EGL_NATIVE_VISUAL_ID, &visual_id) ||
        visual_id == 0)
        visual_id = scr->root_visual;

    p->width = w;
    p->height = h;
    p->win = xcb_generate_id(p->xc);

    uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t vals[2] = {
        scr->black_pixel,
        XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY |
            XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
            XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
            XCB_EVENT_MASK_POINTER_MOTION
    };
    xcb_void_cookie_t ck = xcb_create_window_checked(
        p->xc, XCB_COPY_FROM_PARENT, p->win, scr->root, 0, 0,
        (uint16_t)w, (uint16_t)h, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
        (uint32_t)visual_id, mask, vals);
    xcb_generic_error_t *err = xcb_request_check(p->xc, ck);
    if (err) {
        int code = err->error_code;
        free(err);
        fail("xcb_create_window failed (X error %d) - visual 0x%x", code,
             (unsigned)visual_id);
        return -1;
    }

    /* WM close button + window title, without xcb-icccm/xcb-ewmh. */
    xcb_atom_t wm_protocols = intern(p->xc, "WM_PROTOCOLS");
    p->atom_delete = intern(p->xc, "WM_DELETE_WINDOW");
    xcb_atom_t net_name = intern(p->xc, "_NET_WM_NAME");
    xcb_atom_t utf8 = intern(p->xc, "UTF8_STRING");

    if (wm_protocols != XCB_ATOM_NONE && p->atom_delete != XCB_ATOM_NONE)
        xcb_change_property(p->xc, XCB_PROP_MODE_REPLACE, p->win, wm_protocols,
                            XCB_ATOM_ATOM, 32, 1, &p->atom_delete);
    if (net_name != XCB_ATOM_NONE && utf8 != XCB_ATOM_NONE)
        xcb_change_property(p->xc, XCB_PROP_MODE_REPLACE, p->win, net_name, utf8,
                            8, (uint32_t)strlen(title), title);
    else
        xcb_change_property(p->xc, XCB_PROP_MODE_REPLACE, p->win, XCB_ATOM_WM_NAME,
                            XCB_ATOM_STRING, 8, (uint32_t)strlen(title), title);

    load_keymap(p);

    xcb_map_window(p->xc, p->win);
    xcb_flush(p->xc);

    p->surf = eglCreateWindowSurface(p->dpy, p->cfg,
                                     (EGLNativeWindowType)(uintptr_t)p->win, NULL);
    if (p->surf == EGL_NO_SURFACE) {
        fail("eglCreateWindowSurface failed (EGL error 0x%04x)", eglGetError());
        return -1;
    }
    if (!create_context(p, es == 3) && !create_context(p, 2)) {
        fail("eglCreateContext failed (EGL error 0x%04x)", eglGetError());
        return -1;
    }
    if (!eglMakeCurrent(p->dpy, p->surf, p->surf, p->ctx)) {
        fail("eglMakeCurrent failed (EGL error 0x%04x)", eglGetError());
        return -1;
    }

    p->windowed = true;
    return 0;
}

/* ------------------------------------------------------------- offscreen */

int plat_open_offscreen(Plat *p, int w, int h)
{
    memset(p, 0, sizeof *p);
    p->surf = EGL_NO_SURFACE;
    p->ctx = EGL_NO_CONTEXT;
    p->dpy = EGL_NO_DISPLAY;
    p->width = w;
    p->height = h;

    p->dpy = eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA,
                                   EGL_DEFAULT_DISPLAY, NULL);
    if (p->dpy == EGL_NO_DISPLAY)
        p->dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (p->dpy == EGL_NO_DISPLAY) {
        fail("no EGL display for offscreen rendering");
        return -1;
    }

    EGLint major = 0, minor = 0;
    if (!eglInitialize(p->dpy, &major, &minor)) {
        fail("eglInitialize failed (EGL error 0x%04x)", eglGetError());
        return -1;
    }
    eglBindAPI(EGL_OPENGL_ES_API);

    if (!pick_config(p, 0, EGL_PBUFFER_BIT, &p->cfg)) {
        fail("no suitable EGL config for pbuffer rendering");
        return -1;
    }
    if (!create_context(p, 1) && !create_context(p, 2)) {
        fail("eglCreateContext failed (EGL error 0x%04x)", eglGetError());
        return -1;
    }

    /* Prefer a surfaceless context; fall back to a pbuffer. */
    if (eglMakeCurrent(p->dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, p->ctx))
        return 0;

    EGLint pattrs[] = { EGL_WIDTH, w, EGL_HEIGHT, h, EGL_NONE };
    p->surf = eglCreatePbufferSurface(p->dpy, p->cfg, pattrs);
    if (p->surf == EGL_NO_SURFACE ||
        !eglMakeCurrent(p->dpy, p->surf, p->surf, p->ctx)) {
        fail("cannot enter EGL offscreen context (EGL error 0x%04x)",
             eglGetError());
        return -1;
    }
    return 0;
}

/* ----------------------------------------------------------------- misc */

int plat_fd(const Plat *p)
{
    return p->windowed && p->xc ? xcb_get_file_descriptor(p->xc) : -1;
}

void plat_swap(Plat *p)
{
    if (p->windowed && p->surf != EGL_NO_SURFACE)
        eglSwapBuffers(p->dpy, p->surf);
}

void plat_close(Plat *p)
{
    if (p->dpy != EGL_NO_DISPLAY) {
        eglMakeCurrent(p->dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (p->surf != EGL_NO_SURFACE)
            eglDestroySurface(p->dpy, p->surf);
        if (p->ctx != EGL_NO_CONTEXT)
            eglDestroyContext(p->dpy, p->ctx);
        eglTerminate(p->dpy);
    }
    if (p->xc) {
        if (p->win)
            xcb_destroy_window(p->xc, p->win);
        xcb_disconnect(p->xc);
    }
    memset(p, 0, sizeof *p);
}

int plat_poll(Plat *p, PlatEvent *ev)
{
    if (!p->windowed)
        return 0;

    xcb_generic_event_t *e;
    while ((e = xcb_poll_for_event(p->xc)) != NULL) {
        int type = e->response_type & ~0x80;
        memset(ev, 0, sizeof *ev);

        switch (type) {
        case XCB_CLIENT_MESSAGE: {
            xcb_client_message_event_t *cm = (xcb_client_message_event_t *)e;
            if ((uint32_t)cm->data.data32[0] == p->atom_delete) {
                ev->type = PE_QUIT;
                free(e);
                return 1;
            }
            break;
        }
        case XCB_CONFIGURE_NOTIFY: {
            xcb_configure_notify_event_t *cn = (xcb_configure_notify_event_t *)e;
            if (cn->window == p->win &&
                ((int)cn->width != p->width || (int)cn->height != p->height)) {
                p->width = cn->width;
                p->height = cn->height;
                ev->type = PE_RESIZE;
                ev->w = cn->width;
                ev->h = cn->height;
                free(e);
                return 1;
            }
            break;
        }
        case XCB_KEY_PRESS:
        case XCB_KEY_RELEASE: {
            xcb_key_press_event_t *k = (xcb_key_press_event_t *)e;
            ev->type = PE_KEY;
            ev->press = (type == XCB_KEY_PRESS);
            ev->keysym = keysym_of(p, k->detail, k->state);
            free(e);
            return 1;
        }
        case XCB_BUTTON_PRESS:
        case XCB_BUTTON_RELEASE: {
            xcb_button_press_event_t *b = (xcb_button_press_event_t *)e;
            ev->type = PE_BUTTON;
            ev->press = (type == XCB_BUTTON_PRESS);
            ev->button = b->detail;
            ev->x = b->event_x;
            ev->y = b->event_y;
            free(e);
            return 1;
        }
        case XCB_MOTION_NOTIFY: {
            xcb_motion_notify_event_t *m = (xcb_motion_notify_event_t *)e;
            ev->type = PE_MOTION;
            ev->x = m->event_x;
            ev->y = m->event_y;
            free(e);
            return 1;
        }
        default:
            break;
        }
        free(e);
    }

    if (xcb_connection_has_error(p->xc)) {
        memset(ev, 0, sizeof *ev);
        ev->type = PE_QUIT;
        return 1;
    }
    return 0;
}
