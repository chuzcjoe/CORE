#pragma once

#include <glad/glad.h>

#include <iostream>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

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

  void SetUniform1i(const std::string& name, int value) const {
    glUniform1i(glGetUniformLocation(program_id_, name.c_str()), value);
  }

  void SetUniformMat4f(const std::string& name, glm::mat4& mat) const {
    glUniformMatrix4fv(glGetUniformLocation(program_id_, name.c_str()), 1, GL_FALSE,
                       glm::value_ptr(mat));
  }

 private:
  unsigned int program_id_;
};

}  // namespace opengl
}  // namespace core