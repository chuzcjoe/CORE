// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// clang-format on

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

#include "GLProgram.h"
#include "GLTexture.h"
#include "GLUtils.h"
#include "GLVertexArray.h"

// settings
const unsigned int kWidth = 600;
const unsigned int kHeight = 600;

// clang-format off
const char* vertex_shader_source = OPENGL_VERTEX_SHADER(
    layout(location = 0) in vec3 aPos; 
    layout(location = 1) in vec2 aTexCoord;
    uniform mat4 transform;
    out vec2 TexCoord; 
    void main() {
        gl_Position = transform * vec4(aPos.x, aPos.y, aPos.z, 1.0);
        TexCoord = aTexCoord;
    }
);

const char* fragment_shader_source = OPENGL_FRAGMENT_SHADER(
    out vec4 FragColor; 
    in vec2 TexCoord;
    uniform sampler2D texture1;
    void main() { 
        FragColor = texture(texture1, TexCoord);
    }
);
// clang-format on

int main() {
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  GLFWwindow* window = glfwCreateWindow(kWidth, kHeight, "OpenGLTriangle", NULL, NULL);
  if (window == nullptr) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);

  // glad: load all OpenGL function pointers
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  // Use GL after glad is initialized
  core::opengl::GLProgram program(vertex_shader_source, fragment_shader_source);
  core::opengl::GLVertexArray vao;
  core::opengl::GLTexture texture(GL_TEXTURE_2D, GL_REPEAT, GL_LINEAR);
  texture.Load2DTextureFromFile("./examples/opengl/GLTextureDemo/core.png", GL_RGB, true);

  float vertices[] = {
      0.5f,  0.5f,  0.0f, 1.0f, 1.0f,  // top right
      0.5f,  -0.5f, 0.0f, 1.0f, 0.0f,  // bottom right
      -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,  // bottom left
      -0.5f, 0.5f,  0.0f, 0.0f, 1.0f   // top left
  };
  unsigned int indices[] = {
      0, 1, 3,  // first Triangle
      1, 2, 3   // second Triangle
  };

  vao.Bind();
  vao.SetVertexData(vertices, sizeof(vertices), GL_STATIC_DRAW);
  vao.SetElementData(indices, sizeof(indices), GL_STATIC_DRAW);
  vao.SetVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
  vao.SetVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                             (void*)(3 * sizeof(float)));
  vao.Unbind();

  glm::mat4 trans = glm::mat4(1.0f);

  // render loop
  // -----------
  while (!glfwWindowShouldClose(window)) {
    // render
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // texture
    texture.Bind(GL_TEXTURE_2D);

    trans = glm::mat4(1.0f);  // reset to identity matrix each frame
    trans = glm::translate(trans, glm::vec3(0.5f, 0.0f, 0.0f));
    trans = glm::rotate(trans, static_cast<float>(glfwGetTime()), glm::vec3(0.0f, 0.0f, 1.0f));
    trans = glm::scale(trans, glm::vec3(0.5f, 0.5f, 0.5f));

    // draw our first triangle
    program.Use();
    program.SetUniformMat4f("transform", trans);

    vao.Bind();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    vao.Unbind();

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}