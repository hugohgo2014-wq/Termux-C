#include <android/log.h>
#include <android_native_app_glue.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <stdint.h>
#include <stdbool.h>

#define LOG_TAG "Terminal"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

typedef struct {
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    EGLConfig config;
    int32_t width;
    int32_t height;
} Graphics;

static Graphics g;

static bool graphics_init(ANativeWindow *window) {
    const EGLint config_attrs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8, EGL_NONE
    };
    const EGLint context_attrs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    EGLint count = 0;

    g.display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (g.display == EGL_NO_DISPLAY || !eglInitialize(g.display, 0, 0)) {
        LOGE("eglInitialize failed");
        return false;
    }
    if (!eglChooseConfig(g.display, config_attrs, &g.config, 1, &count) || count < 1) {
        LOGE("No EGL config for OpenGL ES 3");
        return false;
    }
    g.surface = eglCreateWindowSurface(g.display, g.config, window, NULL);
    g.context = eglCreateContext(g.display, g.config, EGL_NO_CONTEXT, context_attrs);
    if (g.surface == EGL_NO_SURFACE || g.context == EGL_NO_CONTEXT ||
        !eglMakeCurrent(g.display, g.surface, g.surface, g.context)) {
        LOGE("Could not create GLES3 context");
        return false;
    }
    eglQuerySurface(g.display, g.surface, EGL_WIDTH, &g.width);
    eglQuerySurface(g.display, g.surface, EGL_HEIGHT, &g.height);
    glViewport(0, 0, g.width, g.height);
    LOGI("OpenGL ES renderer: %s", glGetString(GL_RENDERER));
    return true;
}

static void graphics_draw(void) {
    /* Pixel-art palette; nearest-neighbour presentation is intentional. */
    glViewport(0, 0, g.width, g.height);
    glDisable(GL_DITHER);
    glClearColor(0.012f, 0.020f, 0.035f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* Minimal native splash: colored terminal prompt blocks. */
    glEnable(GL_SCISSOR_TEST);
    glScissor(g.width / 10, g.height / 2, g.width * 8 / 10, g.height / 3);
    glClearColor(0.025f, 0.075f, 0.090f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glScissor(g.width / 5, g.height / 2, g.width / 12, g.height / 12);
    glClearColor(0.10f, 0.95f, 0.62f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glScissor(g.width / 5 + g.width / 12, g.height / 2, g.width / 12, g.height / 12);
    glClearColor(0.10f, 0.95f, 0.62f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
    eglSwapBuffers(g.display, g.surface);
}

static void graphics_term(void) {
    if (g.display != EGL_NO_DISPLAY) {
        eglMakeCurrent(g.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (g.context != EGL_NO_CONTEXT) eglDestroyContext(g.display, g.context);
        if (g.surface != EGL_NO_SURFACE) eglDestroySurface(g.display, g.surface);
        eglTerminate(g.display);
    }
    g.display = EGL_NO_DISPLAY;
    g.surface = EGL_NO_SURFACE;
    g.context = EGL_NO_CONTEXT;
}

static void handle_cmd(struct android_app *app, int32_t cmd) {
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            if (app->window && graphics_init(app->window)) graphics_draw();
            break;
        case APP_CMD_TERM_WINDOW:
            graphics_term();
            break;
        case APP_CMD_WINDOW_RESIZED:
            if (g.display != EGL_NO_DISPLAY) {
                eglQuerySurface(g.display, g.surface, EGL_WIDTH, &g.width);
                eglQuerySurface(g.display, g.surface, EGL_HEIGHT, &g.height);
                graphics_draw();
            }
            break;
        case APP_CMD_LOST_FOCUS:
            break;
        default:
            break;
    }
}

void android_main(struct android_app *app) {
    app_dummy();
    app->onAppCmd = handle_cmd;
    g.display = EGL_NO_DISPLAY;
    g.surface = EGL_NO_SURFACE;
    g.context = EGL_NO_CONTEXT;
    LOGI("Terminal native C runtime started");
    while (true) {
        int events;
        struct android_poll_source *source = NULL;
        int ident = ALooper_pollOnce(-1,
                                     NULL, &events, (void **)&source);
        if (ident == ALOOPER_POLL_ERROR) break;
        if (source) source->process(app, source);
        if (app->destroyRequested) break;
    }
    graphics_term();
}
