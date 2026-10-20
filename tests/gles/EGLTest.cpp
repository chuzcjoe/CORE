#include <EGL/eglplatform.h>
#include <glad/glad_egl.h>
#include <gtest/gtest.h>

#include "Status.h"
#include "egl/Context.h"

namespace core {
namespace test {

TEST(EGL, test) {
  if (!gladLoadEGL()) {
    FAIL() << core::Status(core::ErrorCode::kBackend, "Failed to load EGL")
                  .message();
  }

  core::egl::Context egl_context(3, 3);
  EXPECT_NE(egl_context.context, nullptr);
  EXPECT_NE(egl_context.display, nullptr);
  EXPECT_NE(egl_context.surface, nullptr);
}

}  // namespace test
}  // namespace core
