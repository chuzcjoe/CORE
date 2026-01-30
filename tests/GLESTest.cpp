#include <glad/glad.h>
#include <gtest/gtest.h>

#include <cstdint>

#include "egl/Context.h"

namespace core {
namespace test {

TEST(GLES, test) {
  // Initialize EGL context
  core::egl::Context egl_context(3, 3);

  // Load GLES
  int gles_load = gladLoadGLES2Loader((GLADloadproc)eglGetProcAddress);
  EXPECT_NE(gles_load, 0);

  constexpr int kWidth = 3;
  constexpr int kHeight = 3;

  GLuint fbo = 0;
  GLuint color_tex = 0;
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);

  glGenTextures(1, &color_tex);
  glBindTexture(GL_TEXTURE_2D, color_tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kWidth, kHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_tex, 0);
  ASSERT_EQ(glCheckFramebufferStatus(GL_FRAMEBUFFER), GL_FRAMEBUFFER_COMPLETE);

  const auto compile_shader = [](GLenum type, const char* source) -> GLuint {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok != GL_TRUE) {
      glDeleteShader(shader);
      return 0;
    }
    return shader;
  };

  const char* kVertexShaderSource =
      "attribute vec2 aPos;\n"
      "void main() { gl_Position = vec4(aPos, 0.0, 1.0); }\n";
  const char* kFragmentShaderSource =
      "precision mediump float;\n"
      "void main() { gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0); }\n";

  GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, kVertexShaderSource);
  ASSERT_NE(vertex_shader, 0u);
  GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, kFragmentShaderSource);
  ASSERT_NE(fragment_shader, 0u);

  GLuint program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glBindAttribLocation(program, 0, "aPos");
  glLinkProgram(program);
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  GLint linked = GL_FALSE;
  glGetProgramiv(program, GL_LINK_STATUS, &linked);
  ASSERT_EQ(linked, GL_TRUE);

  const GLfloat vertices[] = {
      -1.0f, -1.0f,  //
      3.0f,  -1.0f,  //
      -1.0f, 3.0f,   //
  };

  GLuint vbo = 0;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glViewport(0, 0, kWidth, kHeight);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  glUseProgram(program);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);
  glDrawArrays(GL_TRIANGLES, 0, 3);

  std::uint8_t pixel[4] = {0, 0, 0, 0};
  glReadPixels(1, 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
  EXPECT_GE(pixel[0], 250);
  EXPECT_LE(pixel[1], 5);
  EXPECT_LE(pixel[2], 5);
  EXPECT_GE(pixel[3], 250);

  glDeleteBuffers(1, &vbo);
  glDeleteProgram(program);
  glDeleteTextures(1, &color_tex);
  glDeleteFramebuffers(1, &fbo);
}

}  // namespace test
}  // namespace core
