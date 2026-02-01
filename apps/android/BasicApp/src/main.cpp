#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android/log.h>
#include <android/native_activity.h>
#include <android/native_window.h>

#include <atomic>
#include <chrono>
#include <thread>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "NativeClear", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "NativeClear", __VA_ARGS__)

struct App {
  ANativeWindow* window = nullptr;

  EGLDisplay display = EGL_NO_DISPLAY;
  EGLSurface surface = EGL_NO_SURFACE;
  EGLContext context = EGL_NO_CONTEXT;

  std::thread renderThread;
  std::atomic<bool> running{false};
};

/* ================= EGL ================= */

static void init_egl(App& app) {
  app.display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  eglInitialize(app.display, nullptr, nullptr);

  const EGLint configAttribs[] = {EGL_RENDERABLE_TYPE,
                                  EGL_OPENGL_ES3_BIT,
                                  EGL_SURFACE_TYPE,
                                  EGL_WINDOW_BIT,
                                  EGL_RED_SIZE,
                                  8,
                                  EGL_GREEN_SIZE,
                                  8,
                                  EGL_BLUE_SIZE,
                                  8,
                                  EGL_ALPHA_SIZE,
                                  8,
                                  EGL_NONE};

  EGLConfig config;
  EGLint numConfigs;
  eglChooseConfig(app.display, configAttribs, &config, 1, &numConfigs);

  const EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};

  app.context = eglCreateContext(app.display, config, EGL_NO_CONTEXT, contextAttribs);

  app.surface = eglCreateWindowSurface(app.display, config, app.window, nullptr);

  eglMakeCurrent(app.display, app.surface, app.surface, app.context);

  LOGI("EGL initialized");
}

static void destroy_egl(App& app) {
  eglMakeCurrent(app.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

  if (app.context != EGL_NO_CONTEXT) eglDestroyContext(app.display, app.context);
  if (app.surface != EGL_NO_SURFACE) eglDestroySurface(app.display, app.surface);

  eglTerminate(app.display);

  app.display = EGL_NO_DISPLAY;
  app.surface = EGL_NO_SURFACE;
  app.context = EGL_NO_CONTEXT;

  LOGI("EGL destroyed");
}

/* ================= Render ================= */

static void render_loop(App& app) {
  init_egl(app);

  while (app.running.load()) {
    int w = ANativeWindow_getWidth(app.window);
    int h = ANativeWindow_getHeight(app.window);

    glViewport(0, 0, w, h);
    glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    eglSwapBuffers(app.display, app.surface);

    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }

  destroy_egl(app);
}

/* ================= NativeActivity callbacks ================= */

static void onNativeWindowCreated(ANativeActivity* activity, ANativeWindow* window) {
  LOGI("Native window created");

  auto* app = static_cast<App*>(activity->instance);
  app->window = window;
  app->running = true;

  app->renderThread = std::thread(render_loop, std::ref(*app));
}

static void onNativeWindowDestroyed(ANativeActivity* activity, ANativeWindow*) {
  LOGI("Native window destroyed");

  auto* app = static_cast<App*>(activity->instance);
  app->running = false;

  if (app->renderThread.joinable()) app->renderThread.join();

  app->window = nullptr;
}

/* ================= Entry ================= */

extern "C" void ANativeActivity_onCreate(ANativeActivity* activity, void*, size_t) {
  LOGI("NativeActivity onCreate");

  auto* app = new App();
  activity->instance = app;

  activity->callbacks->onNativeWindowCreated = onNativeWindowCreated;
  activity->callbacks->onNativeWindowDestroyed = onNativeWindowDestroyed;
}
