#pragma once

#include <glad/glad.h>

#include <iostream>

#include "Status.h"

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

  Status AttachRenderBuffer(GLenum internal_format, GLsizei width,
                            GLsizei height) const;

  bool IsComplete() const {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_id_);
    bool complete = (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return complete;
  };

 private:
  unsigned int fbo_id_ = 0;
  unsigned int rbo_id_ = 0;
  unsigned int texture_id_ = 0;
};

}  // namespace opengl
}  // namespace core
