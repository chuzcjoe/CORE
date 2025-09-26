#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <iostream>

#include "GLProgram.h"
#include "GLUtils.h"
#include "GLVertexArray.h"

// settings
const unsigned int kWidth = 800;
const unsigned int kHeight = 600;

const char* vertex_shader_source =
    OPENGL_VERTEX_SHADER(layout(location = 0) in vec3 aPos; layout(location = 1) in vec3 aColor;
                         out vec3 outColor; void main() {
                           gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
                           ;
                           outColor = aColor;
                         });

const char* fragment_shader_source = OPENGL_FRAGMENT_SHADER(
    out vec4 FragColor; in vec3 outColor; void main() { FragColor = vec4(outColor, 1.0f); });

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

  float vertices[] = {
      0.5f,  0.5f,  0.0f, 1.0f, 0.0f, 0.0f,  // top right
      0.5f,  -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,  // bottom right
      -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f,  // bottom left
      -0.5f, 0.5f,  0.0f, 0.5f, 0.5f, 0.5f   // top left
  };
  unsigned int indices[] = {
      0, 1, 3,  // first Triangle
      1, 2, 3   // second Triangle
  };

  vao.Bind();
  vao.SetVertexData(vertices, sizeof(vertices), GL_STATIC_DRAW);
  vao.SetElementData(indices, sizeof(indices), GL_STATIC_DRAW);
  vao.SetVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
  vao.SetVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                             (void*)(3 * sizeof(float)));
  vao.Unbind();

  // render loop
  // -----------
  while (!glfwWindowShouldClose(window)) {
    // render
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // draw our first triangle
    program.Use();
    vao.Bind();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    vao.Unbind();

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}