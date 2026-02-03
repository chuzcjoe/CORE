#include "GLESTexture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace core {
namespace opengles {

GLESTexture::GLESTexture(GLenum target) {
  // Generate default texture ID for texure unit 0
  GLuint texture_id;
  glGenTextures(1, &texture_id);
  glBindTexture(target, texture_id);

  texture_ids_[0] = texture_id;
  glBindTexture(target, 0);  // Unbind texture
}

GLESTexture::~GLESTexture() {
  for (const auto& texture : texture_ids_) {
    GLuint texture_id = texture.second;
    glDeleteTextures(1, &texture_id);
  }
}

void GLESTexture::Load2DTextureFromFile(const char* file_path, GLenum format, int texture_unit,
                                        GLint wrap_type, GLint filter_type, bool flip_vertically) {
  stbi_set_flip_vertically_on_load(flip_vertically);

  // If texture for the texture unit already exists, update it
  if (texture_ids_.find(texture_unit) != texture_ids_.end()) {
    ReadImageData(file_path, texture_ids_[texture_unit], format);
    return;
  }

  GLuint texture_id;
  glGenTextures(1, &texture_id);
  glBindTexture(GL_TEXTURE_2D, texture_id);
  // Set texture parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_type);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_type);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter_type);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter_type);
  ReadImageData(file_path, texture_id, format);

  texture_ids_[texture_unit] = texture_id;
}

void GLTexture::ReadImageData(const char* file_path, GLuint texture_id, GLenum format) {
  int width, height, channels;
  unsigned char* data = stbi_load(file_path, &width, &height, &channels, 0);
  if (data) {
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);  // Unbind texture
  } else {
    throw std::runtime_error("Failed to load texture image");
  }
  stbi_image_free(data);
}

}  // namespace opengles
}  // namespace core