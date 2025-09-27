#pragma once

#include <glad/glad.h>

#include <iostream>

namespace core {
namespace opengl {

class GLProgram {
 public:
  GLProgram(const char* vertex_shader_source, const char* fragment_shader_source);
  ~GLProgram();

  void Use() const;

  unsigned int GetProgramID() const { return program_id_; };

  void SetUniform1f(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(program_id_, name.c_str()), value);
  }

 private:
  unsigned int program_id_;
};

}  // namespace opengl
}  // namespace core