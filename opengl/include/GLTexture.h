#pragma once

#include <glad/glad.h>

#include <iostream>
#include <unordered_map>

namespace core {
namespace opengl {

class GLTexture {
 public:
  GLTexture() = delete;
  GLTexture(GLenum target, GLint wrap_type, GLint filter_type);
  ~GLTexture();

  void ActivateBind(GLenum target, int texture_uint) {
    if (texture_ids_.find(texture_uint) == texture_ids_.end()) {
      throw std::runtime_error("Texture unit not found");
    }
    // printf("Activate and bind texture unit %d with ID %u\n", texture_uint,
    // texture_ids_[texture_uint]);
    glActiveTexture(GL_TEXTURE0 + texture_uint);
    glBindTexture(target, texture_ids_[texture_uint]);
  }

  void Load2DTextureFromFile(const char* file_path, GLenum format, int texture_unit,
                             [[maybe_unused]] GLint wrap_type = GL_REPEAT,
                             [[maybe_unused]] bool flip_vertically = true);

 private:
  void ReadImageData(const char* file_path, GLuint texture_id, GLenum format);

  std::unordered_map<int, GLuint> texture_ids_;
};

}  // namespace opengl
}  // namespace core
