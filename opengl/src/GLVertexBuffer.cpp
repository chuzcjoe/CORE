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

GLVertexBuffer::GLVertexBuffer(GLVertexBuffer&& other) noexcept { *this = std::move(other); }

GLVertexBuffer& GLVertexBuffer::operator=(GLVertexBuffer&& other) noexcept {
  if (this == &other) {
    return *this;
  }

  if (owns_buffer_ && buffer_id_ != 0) {
    glDeleteBuffers(1, &buffer_id_);
  }

  buffer_id_ = other.buffer_id_;
  target_ = other.target_;
  owns_buffer_ = other.owns_buffer_;

  other.buffer_id_ = 0;
  other.owns_buffer_ = false;

  return *this;
}

void GLVertexBuffer::SetData(const void* data, std::size_t size, GLenum usage) const {
  Bind();
  glBufferData(target_, size, data, usage);
}

void GLVertexBuffer::Release() { owns_buffer_ = false; }

}  // namespace opengl
}  // namespace core
