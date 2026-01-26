#include <EGL/eglplatform.h>
#include <glad/glad_egl.h>
#include <gtest/gtest.h>

namespace core {
namespace test {

TEST(EGL, test) {
  if (!gladLoadEGL()) {
    throw std::runtime_error("Failed to load EGL");
  }

  // 1. Get the default EGL display
  EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  ASSERT_NE(display, EGL_NO_DISPLAY);

  // 2. Initialize EGL
  EGLint major, minor;
  ASSERT_TRUE(eglInitialize(display, &major, &minor));

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
  ASSERT_TRUE(eglChooseConfig(display, config_attribs, &config, 1, &num_configs));
  ASSERT_GT(num_configs, 0);

  // 4. Create a Pbuffer surface
  const EGLint pbuffer_attribs[] = {
      EGL_WIDTH, 64, EGL_HEIGHT, 64, EGL_NONE,
  };
  EGLSurface surface = eglCreatePbufferSurface(display, config, pbuffer_attribs);
  ASSERT_NE(surface, EGL_NO_SURFACE);

  // 5. Create an EGL context
  const EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
  EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attribs);
  ASSERT_NE(context, EGL_NO_CONTEXT);

  // 6. Make the context current
  ASSERT_TRUE(eglMakeCurrent(display, surface, surface, context));

  // 7. Clean up
  eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  eglDestroyContext(display, context);
  eglDestroySurface(display, surface);
  eglTerminate(display);
}

}  // namespace test
}  // namespace core
