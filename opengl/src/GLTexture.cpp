#include "GLTexture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace core {
namespace opengl {

GLTexture::GLTexture(GLenum target, GLint wrap_type, GLint filter_type) {
  glGenTextures(1, &texture_id_);
  glBindTexture(target, texture_id_);

  // Set texture parameters
  glTexParameteri(target, GL_TEXTURE_WRAP_S, wrap_type);  // Set texture wrapping
  glTexParameteri(target, GL_TEXTURE_WRAP_T, wrap_type);
  glTexParameteri(target, GL_TEXTURE_MIN_FILTER, filter_type);  // Set texture filtering
  glTexParameteri(target, GL_TEXTURE_MAG_FILTER, filter_type);

  glBindTexture(target, 0);  // Unbind texture
}

GLTexture::~GLTexture() { glDeleteTextures(1, &texture_id_); }

void GLTexture::Load2DTextureFromFile(const char* file_path, GLenum format,
                                      [[maybe_unused]] bool flip_vertically) {
  stbi_set_flip_vertically_on_load(flip_vertically);
  unsigned char* data =
      stbi_load(file_path, &texture_width_, &texture_height_, &texture_channels_, 0);
  if (data) {
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glTexImage2D(GL_TEXTURE_2D, 0, format, texture_width_, texture_height_, 0, format,
                 GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);  // Unbind texture
  } else {
    throw std::runtime_error("Failed to load texture image");
  }
  stbi_image_free(data);
}

}  // namespace opengl
}  // namespace core