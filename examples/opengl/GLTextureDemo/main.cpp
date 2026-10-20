#include <memory>

#include "Status.h"

// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// clang-format on

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>

#include <iostream>

#include "GLProgram.h"
#include "GLTexture.h"
#include "GLUtils.h"
#include "GLVertexArray.h"
#include "GLVertexBuffer.h"

// settings
const unsigned int kWidth = 800;
const unsigned int kHeight = 600;

// clang-format off
const char* vertex_shader_source = OPENGL_VERTEX_SHADER(
    layout(location = 0) in vec3 aPos;
    layout(location = 1) in vec2 aTexCoord;
    out vec2 TexCoord;
    void main() {
        gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
        TexCoord = aTexCoord;
    }
);

const char* fragment_shader_source = OPENGL_FRAGMENT_SHADER(
    out vec4 FragColor;
    in vec2 TexCoord;
    uniform float alpha;
    uniform sampler2D texture1;
    void main() {
        vec4 tmp = texture(texture1, TexCoord);
        FragColor = vec4(tmp.r, tmp.g, tmp.b, alpha);
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

  // imgui setup
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void)io;

  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");

  auto program_result = core::opengl::GLProgram::Create(vertex_shader_source,
                                                        fragment_shader_source);
  if (!program_result.ok()) {
    return program_result.status();
  }
  std::unique_ptr<core::opengl::GLProgram> program_owner =
      std::move(*program_result);
  core::opengl::GLProgram& program = *program_owner;
  core::opengl::GLVertexArray vao;
  core::opengl::GLVertexBuffer vbo;
  core::opengl::GLTexture texture(GL_TEXTURE_2D);
  {
    const core::Status status =
        texture.Load2DTextureFromFile("./examples/data/core.png", GL_RGB, 0);
    if (!status.ok()) {
      return status;
    }
  }

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

  // Setup VBO
  vbo.SetData(vertices, sizeof(vertices), GL_STATIC_DRAW);

  // Setup VAO
  vao.AttachVertexBuffer(vbo.id());
  vao.SetElementData(indices, sizeof(indices), GL_STATIC_DRAW);
  {
    const core::Status status = vao.SetVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    if (!status.ok()) {
      return status;
    }
  }
  {
    const core::Status status =
        vao.SetVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                                   (void*)(3 * sizeof(float)));
    if (!status.ok()) {
      return status;
    }
  }

  float alpha = 1.0f;

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBlendEquation(GL_FUNC_ADD);

  // render loop
  // -----------
  while (!glfwWindowShouldClose(window)) {
    // imgui
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // UI to control transparency
    ImGui::Begin("Transparency Control");
    ImGui::SliderFloat("Alpha", &alpha, 0.0f, 1.0f);
    ImGui::End();

    // render
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // texture
    {
      const core::Status status = texture.ActivateBind(GL_TEXTURE_2D, 0);
      if (!status.ok()) {
        return status;
      }
    }

    // draw our first triangle
    program.Use();
    program.SetUniform1f("alpha", alpha);

    vao.Bind();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    vao.Unbind();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

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
