#include "GLVertexBuffer.h"

#include <utility>

namespace core {
namespace opengl {

GLVertexBuffer::GLVertexBuffer(GLenum target) : target_(target) { glGenBuffers(1, &buffer_id_); }

GLVertexBuffer::~GLVertexBuffer() {
  if (owns_buffer_ && buffer_id_ != 0) {
    glDeleteBuffers(1, &buffer_id_);
  }
}

void GLVertexBuffer::SetData(const void* data, std::size_t size, GLenum usage) const {
  Bind();
  glBufferData(target_, size, data, usage);
}

void GLVertexBuffer::Release() { owns_buffer_ = false; }

}  // namespace opengl
}  // namespace core
