#include "GLVertexArray.h"

namespace core {
namespace opengl {

GLVertexArray::GLVertexArray() {
  glGenVertexArrays(1, &vao_id_);
  glGenBuffers(1, &vbo_id_);
  glGenBuffers(1, &ebo_id_);
}

GLVertexArray::~GLVertexArray() {
  glDeleteVertexArrays(1, &vao_id_);
  glDeleteBuffers(1, &vbo_id_);
  glDeleteBuffers(1, &ebo_id_);
}

void GLVertexArray::Bind() const { glBindVertexArray(vao_id_); }

void GLVertexArray::Unbind() const { glBindVertexArray(0); }

void GLVertexArray::SetVertexData(const void* data, size_t size, GLenum usage) const {
  glBindBuffer(GL_ARRAY_BUFFER, vbo_id_);
  glBufferData(GL_ARRAY_BUFFER, size, data, usage);
}

void GLVertexArray::SetElementData(const void* data, size_t size, GLenum usage) const {
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_id_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, usage);
}

void GLVertexArray::SetVertexAttribPointer(GLuint location, GLint size, GLenum type,
                                           GLboolean normalized, GLsizei stride,
                                           const void* pointer) const {
  glVertexAttribPointer(location, size, type, normalized, stride, pointer);
  glEnableVertexAttribArray(location);
}

}  // namespace opengl
}  // namespace core