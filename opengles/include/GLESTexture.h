#pragma once

#include <glad/glad.h>

#include <iostream>
#include <unordered_map>

#include "Status.h"

namespace core {
namespace opengles {

class GLESTexture {
 public:
  GLESTexture() = delete;
  GLESTexture(GLenum target);
  ~GLESTexture();

  // Activate and bind normal 2D texture
  Status ActivateBind(GLenum target, int texture_uint) {
    if (texture_ids_.find(texture_uint) == texture_ids_.end()) {
      return Status(ErrorCode::kBackend, "Texture unit not found");
    }
    glActiveTexture(GL_TEXTURE0 + texture_uint);
    glBindTexture(target, texture_ids_[texture_uint]);

    return Status();
  }

  // Bind cubemap texture
  Status BindCubeMap(int texture_unit) {
    if (texture_ids_.find(texture_unit) == texture_ids_.end()) {
      return Status(ErrorCode::kBackend, "Texture unit not found");
    }
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture_ids_[texture_unit]);

    return Status();
  }

  Status Load2DTextureFromFile(const char* file_path, GLenum format,
                               int texture_unit, GLint wrap_type = GL_REPEAT,
                               GLint filter_type = GL_LINEAR,
                               bool flip_vertically = true);

 private:
  Status ReadImageData(const char* file_path, GLuint texture_id, GLenum format);

  std::unordered_map<int, GLuint> texture_ids_;
};

}  // namespace opengles
}  // namespace core
