#pragma once

#include <glad/glad_egl.h>

namespace core {
namespace egl {

class Context {
 public:
  Context(const int width, const int height);
  ~Context();

  EGLContext context;
  EGLDisplay display;
  EGLSurface surface;
};

}  // namespace egl
}  // namespace core
