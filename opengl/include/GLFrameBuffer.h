#pragma once

#include <glad/glad.h>

#include <iostream>

namespace core {
namespace opengl {

class GLFrameBuffer {
 public:
  GLFrameBuffer(const bool use_render_buffer = false);
  ~GLFrameBuffer();

  void Bind() const { glBindFramebuffer(GL_FRAMEBUFFER, fbo_id_); };
  void Unbind() const { glBindFramebuffer(GL_FRAMEBUFFER, 0); };

  GLint GetTexutureID() const { return texture_id_; };

  void AttachTexture2D(GLenum attachment, GLenum internal_format, GLsizei width, GLsizei height,
                       GLenum format, GLenum type) const;

  void AttachRenderBuffer(GLenum internal_format, GLsizei width, GLsizei height) const;

  bool IsComplete() const {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_id_);
    bool complete = (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return complete;
  };

 private:
  unsigned int fbo_id_;
  unsigned int rbo_id_;
  unsigned int texture_id_;
};

}  // namespace opengl
}  // namespace core