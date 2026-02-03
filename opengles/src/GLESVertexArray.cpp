#include "GLESVertexArray.h"

namespace core {
namespace opengles {

GLESVertexArray::GLESVertexArray() {
  glGenVertexArrays(1, &vao_id_);
  glGenBuffers(1, &ebo_id_);
}

GLESVertexArray::~GLESVertexArray() {
  glDeleteVertexArrays(1, &vao_id_);
  glDeleteBuffers(1, &ebo_id_);
}

void GLESVertexArray::SetElementData(const void* data, size_t size, GLenum usage) const {
  Bind();
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_id_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, usage);
}

// callsite shouldn't care about VBO and VAO binding
// We default to binding the VBO and VAO here for attribute setup
void GLESVertexArray::SetVertexAttribPointer(GLuint location, GLint size, GLenum type,
                                             GLboolean normalized, GLsizei stride,
                                             const void* pointer) const {
  if (vbo_id_ == 0) {
    throw std::runtime_error("Error: No VBO attached to VAO for setting vertex attribute pointer");
  }
  Bind();
  glBindBuffer(GL_ARRAY_BUFFER, vbo_id_);
  glVertexAttribPointer(location, size, type, normalized, stride, pointer);
  glEnableVertexAttribArray(location);
}

}  // namespace opengles
}  // namespace core