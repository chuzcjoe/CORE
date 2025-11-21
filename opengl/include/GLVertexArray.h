#pragma once

#include <glad/glad.h>

#include <iostream>

namespace core {
namespace opengl {

class GLVertexArray {
 public:
  GLVertexArray();
  ~GLVertexArray();

  void Bind() const;
  void Unbind() const;

  // void SetVertexData(const void* data, size_t size, GLenum usage = GL_STATIC_DRAW) const;

  void SetElementData(const void* data, size_t size, GLenum usage = GL_STATIC_DRAW) const;

  void SetVertexAttribPointer(GLuint location, GLint size, GLenum type, GLboolean normalized,
                              GLsizei stride, const void* pointer) const;

  void AttachVertexBuffer(unsigned int vbo_id) { vbo_id_ = vbo_id; }

 private:
  unsigned int vao_id_;
  unsigned int vbo_id_ = 0;  // Create from external, not owned, not responsible for deletion
  unsigned int ebo_id_ = 0;
};

}  // namespace opengl
}  // namespace core