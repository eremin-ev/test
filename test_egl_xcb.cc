/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

/* glCreateShader, glAttachShader, etc. from <GL/glext.h> */
#define GL_GLEXT_PROTOTYPES	1

#include <GL/gl.h>
#include <GL/glext.h>
#include <EGL/egl.h>
#include <xcb/xcb.h>

#include <stdio.h>

struct test_egl {
    xcb_connection_t *conn;
    xcb_screen_t *screen;

    EGLConfig config;
    EGLDisplay display;
    EGLContext context;
    EGLSurface surface;

    /* initialized by native display */
    union {
        EGLNativeWindowType window;
        //EGLNativePixmapType pixmap;
        //EGLSurface surface; /* pbuffer or screen surface */
    };

    int width;
    int height;
};

enum test_egl_flags {
    F_RUN = 1 << 0,
};

struct test_state {
    unsigned flags;
    unsigned frame;
};

static xcb_connection_t *egl_connect(struct test_egl *egl)
{
    /* Open the connection to the X server. Use the
     * DISPLAY environment variable */
    int nr;
    egl->conn = xcb_connect(nullptr, &nr);

    /* Get the screen whose number is screenNum */

    const xcb_setup_t *setup = xcb_get_setup(egl->conn);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);  

    //printf("# connected to X, conn %lu, nr %i\n", (long) egl->conn, nr);

    // we want the screen at index screenNum of the iterator
    for (int i = 0; i < nr; ++i) {
        xcb_screen_next(&iter);
    }
    egl->screen = iter.data;

    /* report */
    printf ("# Informations of screen %ld:\n", (long) egl->screen->root);
    printf ("#   width.........: %d\n", egl->screen->width_in_pixels);
    printf ("#   height........: %d\n", egl->screen->height_in_pixels);
    printf ("#   white pixel...: 0x%lx\n", (long) egl->screen->white_pixel);
    printf ("#   black pixel...: 0x%lx\n", (long) egl->screen->black_pixel);

    return egl->conn;
}

static EGLConfig egl_choose_config(struct test_egl *egl)
{
    EGLint num_configs;
    EGLint config_attr[] = {
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
        EGL_RED_SIZE,        1,
        EGL_GREEN_SIZE,      1,
        EGL_BLUE_SIZE,       1,
        EGL_ALPHA_SIZE,      1,
        EGL_DEPTH_SIZE,      1,
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_NONE,            EGL_NONE,
    };

    if (!eglChooseConfig(egl->display,
                         config_attr,
                         &egl->config,
                         1,
                         &num_configs) || !num_configs) {
        printf("failed to choose a config\n");
        return nullptr;
    }

    //printf("# num_configs: %i\n", num_configs);

    return egl->config;
}

static void egl_create_context(struct test_egl *egl)
{
    EGLint context_attribs[] = {
        EGL_NONE,
    };

    egl->context = eglCreateContext(egl->display,
                                    egl->config,
                                    EGL_NO_CONTEXT,
                                    context_attribs);
}

static void egl_create_window(struct test_egl *egl, int width, int height)
{
    unsigned values[1] = {
        XCB_EVENT_MASK_BUTTON_PRESS |
            XCB_EVENT_MASK_BUTTON_RELEASE |
            XCB_EVENT_MASK_KEY_PRESS |
            XCB_EVENT_MASK_KEY_RELEASE,
    };

    /* Create the window */
    xcb_window_t window = xcb_generate_id(egl->conn);

    // height
    egl->width = width;
    egl->height = height;

	xcb_create_window(egl->conn,                /* Connection           */
			          XCB_COPY_FROM_PARENT,     /* depth (same as root) */
                      window,                   /* window Id            */
                      egl->screen->root,		/* parent window	*/
                      0, 0,                     /* x, y			*/
                      width, height,            /* width, height	*/
                      10,                       /* border_width		*/
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, /* class		*/
                      egl->screen->root_visual,
                      XCB_CW_EVENT_MASK,
                      values);

	egl->window = window;

	/* Map the window on the screen */
	xcb_map_window(egl->conn, window);

	/* Make sure commands are sent before we pause so that the window gets shown */
	xcb_flush(egl->conn);
}

static bool egl_init(struct test_egl *egl, int width, int height)
{
    egl_connect(egl);

    egl->display = eglGetDisplay((NativeDisplayType)nullptr);

    EGLBoolean inited = eglInitialize(egl->display, nullptr, nullptr);
    if (inited != EGL_TRUE) {
        printf("# egl failed to initialize\n");
        return false;
    }

    printf("# EGL_VERSION = %s\n",
            eglQueryString(egl->display, EGL_VERSION));
    printf("# EGL_VENDOR = %s\n",
            eglQueryString(egl->display, EGL_VENDOR));
    printf("# EGL_EXTENSIONS = %s\n",
            eglQueryString(egl->display, EGL_EXTENSIONS));
    printf("# EGL_CLIENT_APIS = %s\n",
            eglQueryString(egl->display, EGL_CLIENT_APIS));

    egl_choose_config(egl);

    eglBindAPI(EGL_OPENGL_API);

    egl_create_context(egl);

    egl_create_window(egl, width, height);

    egl->surface = eglCreateWindowSurface(egl->display,
                                            egl->config,
                                            egl->window,
                                            nullptr);
    if (egl->surface == EGL_NO_SURFACE){
        printf("failed to create surface\n");
        return false;
    }

    if (!eglMakeCurrent(egl->display,
                        egl->surface,
                        egl->surface,
                        egl->context)) {
        printf("failed to make window current\n");
        return false;
    }

    glClearColor(1.0, 1.0, 1.0, 0.0);
    //glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    //glPointSize(CONFIG_POINT_SIZE);

    return true;
}

void egl_fini(void)
{
    glDisable(GL_POINT_SMOOTH);
    glBlendFunc(GL_NONE, GL_NONE);
    glDisable(GL_BLEND);
}

static bool shader_init()
{
    const int info_len = 512;
    char info_str[info_len];

    // vertex shader
    const char *vertex_code =
        "#version 400\n"
        "layout (location = 0) in vec2 aPos;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = vec4(aPos.x, aPos.y, 0, 1.0);\n"
        "    gl_PointSize = 4.0;\n"
        "}\n";

    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertex_code, nullptr);
    glCompileShader(vertex);

    int success;
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, info_len, nullptr, info_str);
        printf("%s Error: vertex compilation failed: %s\n", __func__, info_str);
        return false;
    };

    // fragment shader
    const char *fragment_code =
        "#version 400\n"
        "in vec4 inColor;\n"
        "out vec4 outColor;\n"
        "void main()\n"
        "{\n"
        //"    outColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
        "    outColor = inColor;\n"
        "    vec2 circCoord = 2.0 * gl_PointCoord - 1.0;\n"
        "    if (dot(circCoord, circCoord) > 1.0) {\n"
        "        discard;\n"
        "    }\n"
        "}\n";

    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragment_code, nullptr);
    glCompileShader(fragment);

    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, info_len, nullptr, info_str);
        printf("%s Error: fragment compilation failed: %s\n", __func__, info_str);
        return false;
    };

    // shader program
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vertex);
    glAttachShader(prog, fragment);
    glLinkProgram(prog);

    // print linking errors if any
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(prog, info_len, nullptr, info_str);
        printf("%s Error: linking failed: %s\n", __func__, info_str);
        return false;
    }

    // delete the shaders as they're linked into our
    // program now and no longer necessary
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return true;
}

static void egl_reshape(int width, int height)
{
    float ar = (float) width / height;

    /*
     * See also on glViewport:
     *
     * - What is glViewport for and why it is not necessary sometimes?
     *   https://gamedev.stackexchange.com/questions/147522/what-is-glviewport-for-and-why-it-is-not-necessary-sometimes
     *
     * - My C++/OpenGL "Hello Triangle" Code Does Not Draw A Triangle When Using SDL As A Window Library
     *   https://gamedev.stackexchange.com/questions/131690/my-c-opengl-hello-triangle-code-does-not-draw-a-triangle-when-using-sdl-as-a/131691
     */
    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-ar, ar, -1, 1, 5.0, 60.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0, 0.0, -10.0);
}

static void draw_all(struct test_state *S)
{
	glClear(GL_COLOR_BUFFER_BIT);

	//glColorPointer(3, REAL_T, 0, colors);
	//glEnableClientState(GL_COLOR_ARRAY);

#if 0
	glColor4f(0.5, 0.5, 0.5, 1.0);
	glVertexPointer(2, REAL_T, 0, verts);
	glEnableClientState(GL_VERTEX_ARRAY);
	glDrawArrays(GL_POINTS, 0, c4*n4);
	glDisableClientState(GL_VERTEX_ARRAY);
#endif

	glFlush();
}

int main(void)
{
    struct test_state S;
    struct test_egl egl;

    egl_init(&egl, 720, 720);
    shader_init();
    egl_reshape(egl.width, egl.height);

    for (; S.flags & F_RUN /* && j < N*/; ) {

        S.frame++;

        draw_all(&S);
        eglSwapBuffers(egl.display, egl.surface);

#if CONFIG_DRAW
        xcb_generic_event_t *event;
        while ((event = xcb_poll_for_event(egl.conn))) {
            event_handle(&S, event, S.verts, S.velo, S.ctct);
        }
#endif
    }

    egl_fini();
    eglTerminate(egl.display);
    xcb_disconnect(egl.conn);

    return 0;
}
