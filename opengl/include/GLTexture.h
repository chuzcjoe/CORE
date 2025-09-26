#pragma once

#include <glad/glad.h>

#include <iostream>

namespace core {
namespace opengl {

class GLTexture {
 public:
  GLTexture() = delete;
  GLTexture(GLenum target, GLint wrap_type, GLint filter_type);
  ~GLTexture();

  void Bind(GLenum target) const { glBindTexture(target, texture_id_); }

  void Load2DTextureFromFile(const char* file_path, GLenum format,
                             [[maybe_unused]] bool flip_vertically = true);

 private:
  GLuint texture_id_;
  int texture_width_;
  int texture_height_;
  int texture_channels_;
};

}  // namespace opengl
}  // namespace core
