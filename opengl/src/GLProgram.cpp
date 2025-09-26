#include "GLProgram.h"

namespace core {
namespace opengl {

GLProgram::GLProgram(const char* vertex_shader_source, const char* fragment_shader_source) {
  // Vertex Shader
  GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vertex_shader_source, nullptr);
  glCompileShader(vertex_shader);

  // Check for compile errors
  GLint success;
  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char info_log[512];
    glGetShaderInfoLog(vertex_shader, 512, nullptr, info_log);
    throw std::runtime_error(std::string("Vertex shader compilation failed: ") + info_log);
  }

  // Fragment Shader
  GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fragment_shader_source, nullptr);
  glCompileShader(fragment_shader);

  // Check for compile errors
  glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char info_log[512];
    glGetShaderInfoLog(fragment_shader, 512, nullptr, info_log);
    throw std::runtime_error(std::string("Fragment shader compilation failed: ") + info_log);
  }

  // Shader Program
  program_id_ = glCreateProgram();
  glAttachShader(program_id_, vertex_shader);
  glAttachShader(program_id_, fragment_shader);
  glLinkProgram(program_id_);

  // Check for linking errors
  glGetProgramiv(program_id_, GL_LINK_STATUS, &success);
  if (!success) {
    char info_log[512];
    glGetProgramInfoLog(program_id_, 512, nullptr, info_log);
    throw std::runtime_error(std::string("Shader program linking failed: ") + info_log);
  }

  // Clean up shaders as they're linked into our program now and no longer necessary
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);
}

GLProgram::~GLProgram() { glDeleteProgram(program_id_); }

void GLProgram::Use() const { glUseProgram(program_id_); }

}  // namespace opengl
}  // namespace core