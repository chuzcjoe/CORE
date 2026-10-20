#include <memory>

#include "Status.h"

// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// clang-format on

#include <iostream>

#include "GLProgram.h"
#include "GLUtils.h"
#include "GLVertexArray.h"
#include "GLVertexBuffer.h"

// settings
const unsigned int kWidth = 800;
const unsigned int kHeight = 600;

// clang-format off
const char* vertex_shader_source = OPENGL_VERTEX_SHADER(
    layout(location = 0) in vec3 aPos;
    layout(location = 1) in vec3 aColor;
    out vec3 outColor;
    void main() {
        gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
        outColor = aColor;
    }
);

const char* fragment_shader_source = OPENGL_FRAGMENT_SHADER(
    out vec4 FragColor;
    in vec3 outColor;
    uniform float alpha_weight;
    void main() {
        FragColor = vec4(outColor, alpha_weight);
    }
);
// clang-format on

core::Status RunApplication() {
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
    return core::Status(core::ErrorCode::kBackend, "Application failed");
  }
  const auto close_window = [](GLFWwindow* value) {
    glfwDestroyWindow(value);
    glfwTerminate();
  };
  const std::unique_ptr<GLFWwindow, decltype(close_window)> window_owner(
      window, close_window);

  glfwMakeContextCurrent(window);

  // glad: load all OpenGL function pointers
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return core::Status(core::ErrorCode::kBackend, "Application failed");
  }

  auto program_result = core::opengl::GLProgram::Create(vertex_shader_source,
                                                        fragment_shader_source);
  if (!program_result.ok()) {
    return program_result.status();
  }
  std::unique_ptr<core::opengl::GLProgram> program_owner =
      std::move(*program_result);
  core::opengl::GLProgram& program = *program_owner;
  core::opengl::GLVertexArray vao;
  core::opengl::GLVertexBuffer vbo(GL_ARRAY_BUFFER);

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

  // Setup VBO (handle vbo binding internally)
  vbo.SetData(vertices, sizeof(vertices), GL_STATIC_DRAW);

  // Setup VAO (handle vao binding internally)
  vao.AttachVertexBuffer(vbo.id());
  vao.SetElementData(indices, sizeof(indices), GL_STATIC_DRAW);
  {
    const core::Status status = vao.SetVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    if (!status.ok()) {
      return status;
    }
  }
  {
    const core::Status status =
        vao.SetVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                                   (void*)(3 * sizeof(float)));
    if (!status.ok()) {
      return status;
    }
  }

  // Enable blending for transparency
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // render loop
  // -----------
  while (!glfwWindowShouldClose(window)) {
    // render
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // draw our first triangle
    program.Use();

    // update the uniform color
    float time_value = glfwGetTime();
    float alpha = std::sin(time_value) / 2.0f + 0.5f;
    printf("alpha: %f\n", alpha);
    program.SetUniform1f("alpha_weight", alpha);

    vao.Bind();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    vao.Unbind();

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return core::Status();
}
int main() {
  const core::Status status = RunApplication();
  if (!status.ok()) {
    std::fprintf(stderr, "%s\n", status.message().c_str());
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
