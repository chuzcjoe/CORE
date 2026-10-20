#include "TextureIO.h"

#include <stb_image.h>
#include <stb_image_write.h>

#include "Mat.h"
#include "Status.h"

namespace core {
namespace io {

Status WriteTextureToFile(const GLint texture, const int width,
                          const int height, const std::string& file_path) {
  if (width <= 0 || height <= 0 || file_path.empty()) {
    return Status(ErrorCode::kInvalidArgument,
                  "Invalid write texture to file parameters");
  }

  core::Mat<unsigned char, 4> pixels(height, width);
  GLuint fbo = 0;
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

  const GLenum fb_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (fb_status != GL_FRAMEBUFFER_COMPLETE) {
    glDeleteFramebuffers(1, &fbo);
    return Status(ErrorCode::kBackend,
                  "Framebuffer incomplete for texture readback");
  }

  glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
  glDeleteFramebuffers(1, &fbo);

  stbi_flip_vertically_on_write(1);
  stbi_write_png_compression_level = 0;
  int write_status = stbi_write_png(file_path.c_str(), width, height, 4, pixels.data(), width * 4);
  if (!write_status) {
    return Status(ErrorCode::kBackend, "Failed to write texture to file");
  } else {
    printf("Texture written to file: %s\n", file_path.c_str());
  }

  return Status();
}

}  // namespace io
}  // namespace core
