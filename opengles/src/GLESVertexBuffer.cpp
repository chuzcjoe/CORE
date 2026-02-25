#include "GLESVertexBuffer.h"

#include <utility>

namespace core {
namespace opengles {

GLESVertexBuffer::GLESVertexBuffer(GLenum target) : target_(target) {
  glGenBuffers(1, &buffer_id_);
}

GLESVertexBuffer::~GLESVertexBuffer() {
  if (owns_buffer_ && buffer_id_ != 0) {
    glDeleteBuffers(1, &buffer_id_);
  }
}

void GLESVertexBuffer::SetData(const void* data, std::size_t size, GLenum usage) const {
  Bind();
  glBufferData(target_, size, data, usage);
}

void GLESVertexBuffer::Release() { owns_buffer_ = false; }

}  // namespace opengles
}  // namespace core
