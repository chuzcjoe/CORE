#include "GLFrameBuffer.h"

namespace core {
namespace opengl {

GLFrameBuffer::GLFrameBuffer(const bool use_render_buffer) {
  glGenFramebuffers(1, &fbo_id_);
  glGenTextures(1, &texture_id_);
  if (use_render_buffer) {
    glGenRenderbuffers(1, &rbo_id_);
  }
}

GLFrameBuffer::~GLFrameBuffer() {
  glDeleteFramebuffers(1, &fbo_id_);
  glDeleteTextures(1, &texture_id_);
  glDeleteRenderbuffers(1, &rbo_id_);
}

void GLFrameBuffer::AttachTexture2D(GLenum attachment, GLenum internal_format, GLsizei width,
                                    GLsizei height, GLenum format, GLenum type) const {
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_id_);
  glBindTexture(GL_TEXTURE_2D, texture_id_);
  glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, type, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture_id_, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLFrameBuffer::AttachRenderBuffer(GLenum internal_format, GLsizei width,
                                       GLsizei height) const {
  if (rbo_id_ == 0) {
    throw std::runtime_error("Renderbuffer not initialized");
  }
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_id_);
  glBindRenderbuffer(GL_RENDERBUFFER, rbo_id_);
  glRenderbufferStorage(GL_RENDERBUFFER, internal_format, width, height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo_id_);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

}  // namespace opengl
}  // namespace core