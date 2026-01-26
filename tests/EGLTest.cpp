#include <EGL/eglplatform.h>
#include <glad/glad_egl.h>
#include <gtest/gtest.h>

#include "egl/Context.h"

namespace core {
namespace test {

TEST(EGL, test) {
  if (!gladLoadEGL()) {
    throw std::runtime_error("Failed to load EGL");
  }

  core::egl::Context egl_context(3, 3);
  EXPECT_NE(egl_context.context, nullptr);
  EXPECT_NE(egl_context.display, nullptr);
  EXPECT_NE(egl_context.surface, nullptr);
}

}  // namespace test
}  // namespace core
