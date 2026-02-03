#pragma once

#include <glad/glad.h>

#include <cstddef>

namespace core {
namespace opengles {

// RAII wrapper around an OpenGL vertex buffer object (VBO).
class GLESVertexBuffer {
 public:
  explicit GLESVertexBuffer(GLenum target = GL_ARRAY_BUFFER);
  ~GLESVertexBuffer();

  void Bind() const { glBindBuffer(target_, buffer_id_); }
  void Unbind() const { glBindBuffer(target_, 0); }

  void SetData(const void* data, std::size_t size, GLenum usage = GL_STATIC_DRAW) const;

  unsigned int id() const { return buffer_id_; }
  GLenum target() const { return target_; }

  // Releases ownership without deleting the underlying GL buffer. Useful when the
  // buffer lifetime is managed elsewhere after moving it out of this class.
  void Release();

 private:
  unsigned int buffer_id_ = 0;
  GLenum target_ = GL_ARRAY_BUFFER;
  bool owns_buffer_ = true;
};

}  // namespace opengles
}  // namespace core
