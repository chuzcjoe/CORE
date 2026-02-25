#pragma once

#include <glad/glad.h>

#include <iostream>
#include <unordered_map>

namespace core {
namespace opengles {

class GLESTexture {
 public:
  GLESTexture() = delete;
  GLESTexture(GLenum target);
  ~GLESTexture();

  // Activate and bind normal 2D texture
  void ActivateBind(GLenum target, int texture_uint) {
    if (texture_ids_.find(texture_uint) == texture_ids_.end()) {
      throw std::runtime_error("Texture unit not found");
    }
    glActiveTexture(GL_TEXTURE0 + texture_uint);
    glBindTexture(target, texture_ids_[texture_uint]);
  }

  // Bind cubemap texture
  void BindCubeMap(int texture_unit) {
    if (texture_ids_.find(texture_unit) == texture_ids_.end()) {
      throw std::runtime_error("Texture unit not found");
    }
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture_ids_[texture_unit]);
  }

  void Load2DTextureFromFile(const char* file_path, GLenum format, int texture_unit,
                             GLint wrap_type = GL_REPEAT, GLint filter_type = GL_LINEAR,
                             bool flip_vertically = true);

 private:
  void ReadImageData(const char* file_path, GLuint texture_id, GLenum format);

  std::unordered_map<int, GLuint> texture_ids_;
};

}  // namespace opengles
}  // namespace core
