#include "GLVertexArray.h"

namespace core {
namespace opengl {

GLVertexArray::GLVertexArray() {
  glGenVertexArrays(1, &vao_id_);
  glGenBuffers(1, &ebo_id_);
}

GLVertexArray::~GLVertexArray() {
  glDeleteVertexArrays(1, &vao_id_);
  glDeleteBuffers(1, &ebo_id_);
}

void GLVertexArray::Bind() const { glBindVertexArray(vao_id_); }

void GLVertexArray::Unbind() const { glBindVertexArray(0); }

// void GLVertexArray::SetVertexData(const void* data, size_t size, GLenum usage) const {
//   glBindBuffer(GL_ARRAY_BUFFER, vbo_id_);
//   glBufferData(GL_ARRAY_BUFFER, size, data, usage);
// }

void GLVertexArray::SetElementData(const void* data, size_t size, GLenum usage) const {
  Bind();
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_id_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, usage);
}

// callsite shouldn't care about VBO and VAO binding
// We default to binding the VBO and VAO here for attribute setup
void GLVertexArray::SetVertexAttribPointer(GLuint location, GLint size, GLenum type,
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

}  // namespace opengl
}  // namespace core