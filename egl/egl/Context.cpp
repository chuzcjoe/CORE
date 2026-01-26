#include "Context.h"

namespace core {
namespace egl {

Context::Context(const int width, const int height) {
  // 1. Get the default EGL display
  display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

  // 2. Initialize EGL
  EGLint major, minor;
  eglInitialize(display, &major, &minor);

  // 3. Choose an EGL config
  const EGLint config_attribs[] = {EGL_SURFACE_TYPE,
                                   EGL_PBUFFER_BIT,
                                   EGL_BLUE_SIZE,
                                   8,
                                   EGL_GREEN_SIZE,
                                   8,
                                   EGL_RED_SIZE,
                                   8,
                                   EGL_DEPTH_SIZE,
                                   8,
                                   EGL_RENDERABLE_TYPE,
                                   EGL_OPENGL_ES2_BIT,
                                   EGL_NONE};
  EGLConfig config;
  EGLint num_configs;
  eglChooseConfig(display, config_attribs, &config, 1, &num_configs);

  // 4. Create a Pbuffer surface
  const EGLint pbuffer_attribs[] = {
      EGL_WIDTH, width, EGL_HEIGHT, height, EGL_NONE,
  };
  surface = eglCreatePbufferSurface(display, config, pbuffer_attribs);

  // 5. Create an EGL context
  const EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
  context = eglCreateContext(display, config, nullptr, context_attribs);

  // 6. Make the context current
  eglMakeCurrent(display, surface, surface, context);
}

Context::~Context() {
  eglDestroySurface(display, surface);
  eglDestroyContext(display, context);
  eglTerminate(display);
}

}  // namespace egl
}  // namespace core
