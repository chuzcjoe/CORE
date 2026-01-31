#include "TextureIO.h"

#include <stb_image.h>
#include <stb_image_write.h>

#include <stdexcept>

#include "Mat.h"

namespace core {
namespace io {

void WriteTextureToFile(const GLint texture, const int width, const int height,
                        const std::string& file_path) {
  if (width <= 0 || height <= 0 || file_path.empty()) {
    throw std::invalid_argument("Invalid write texture to file parameters");
  }

  core::Mat<unsigned char, 4> pixels(height, width);
  GLuint fbo = 0;
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

  const GLenum fb_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (fb_status != GL_FRAMEBUFFER_COMPLETE) {
    throw std::runtime_error("Framebuffer incomplete for texture readback");
  }

  glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
  glDeleteFramebuffers(1, &fbo);

  stbi_flip_vertically_on_write(1);
  stbi_write_png_compression_level = 0;
  int write_status = stbi_write_png(file_path.c_str(), width, height, 4, pixels.data(), width * 4);
  if (!write_status) {
    throw std::runtime_error("Failed to write texture to file");
  } else {
    printf("Texture written to file: %s\n", file_path.c_str());
  }
}

}  // namespace io
}  // namespace core
