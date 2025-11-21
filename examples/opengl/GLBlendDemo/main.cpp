// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// clang-format on

#include <iostream>
#include <map>

#include "GLCamera.h"
#include "GLProgram.h"
#include "GLTexture.h"
#include "GLUtils.h"
#include "GLVertexArray.h"
#include "GLVertexBuffer.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

void process_inputs(GLFWwindow* window);
void mouse_callback([[maybe_unused]] GLFWwindow* window, double xpos, double ypos);

// settings
const unsigned int kWidth = 1000;
const unsigned int kHeight = 1000;
const glm::vec3 kCameraPos = glm::vec3(0.0f, 1.0f, 3.0f);
const glm::vec3 kCameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
const glm::vec3 kCameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
const float kCameraSpeed = 0.03f;
const float kMouseSensitivity = 0.1f;
float laxt_x = kWidth / 2.0f;
float last_y = kHeight / 2.0f;
bool first_mouse = true;

std::unique_ptr<core::opengl::GLCamera> camera =
    std::make_unique<core::opengl::GLCamera>(kCameraPos, kCameraFront, kCameraUp, kCameraSpeed);

// clang-format off
const char* vertex_shader_source = OPENGL_VERTEX_SHADER(
  layout (location = 0) in vec3 aPos;
  layout (location = 1) in vec2 aTexCoords;
  out vec2 TexCoords;
  uniform mat4 model;
  uniform mat4 view;
  uniform mat4 projection;

  void main() {
    TexCoords = aTexCoords;    
    gl_Position = projection * view * model * vec4(aPos, 1.0);
  }
);

const char* fragment_shader_source = OPENGL_FRAGMENT_SHADER(
    out vec4 FragColor;
    in vec2 TexCoords;
    uniform sampler2D texture1;
    uniform sampler2D texture2;
    uniform sampler2D texture3;
    uniform sampler2D texture4;
    uniform int texture_type;

    void main() {
      if (texture_type == 0) {
        FragColor = texture(texture1, TexCoords);
      } else if (texture_type == 1) {
        FragColor = texture(texture2, TexCoords);
      } else if (texture_type == 2) { // grass
        FragColor = texture(texture3, TexCoords);
        // discard transparent fragments
        if (FragColor.a < 0.1) {
          discard;
        }    
      } else if (texture_type == 3) { // window
        FragColor = texture(texture4, TexCoords);
      }
      else {
        FragColor = vec4(0.0, 1.0, 0.0, 1.0);
      }
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

  GLFWwindow* window = glfwCreateWindow(kWidth, kHeight, "OpenGLBlend", NULL, NULL);
  if (window == nullptr) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetCursorPosCallback(window, mouse_callback);

  // glad: load all OpenGL function pointers
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    throw std::runtime_error("Failed to initialize GLAD");
  }

  // Use GL after glad is initialized
  core::opengl::GLProgram program(vertex_shader_source, fragment_shader_source);
  core::opengl::GLVertexArray cube_vao;
  core::opengl::GLVertexArray plane_vao;
  core::opengl::GLVertexArray transparent_vao;
  core::opengl::GLVertexBuffer cube_vbo;
  core::opengl::GLVertexBuffer plane_vbo;
  core::opengl::GLVertexBuffer transparent_vbo;
  core::opengl::GLTexture texture(GL_TEXTURE_2D, GL_REPEAT, GL_LINEAR);
  texture.Load2DTextureFromFile("./examples/opengl/GLBlendDemo/metal.png", GL_RGB, 0);
  texture.Load2DTextureFromFile("./examples/opengl/GLBlendDemo/marble.jpg", GL_RGB, 1);
  texture.Load2DTextureFromFile("./examples/opengl/GLBlendDemo/grass.png", GL_RGBA, 2,
                                GL_CLAMP_TO_EDGE, false);
  texture.Load2DTextureFromFile("./examples/opengl/GLBlendDemo/window.png", GL_RGBA, 3,
                                GL_CLAMP_TO_EDGE, false);

  // clang-format off
  float cube_vertices[] = {
        // positions          // texture Coords
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };
    
  float plane_vertices[] = {
        // positions          // texture Coords (note we set these higher than 1 (together with GL_REPEAT as texture wrapping mode). this will cause the floor texture to repeat)
         5.0f, -0.5f - 0.01f,  5.0f,  2.0f, 0.0f,
        -5.0f, -0.5f - 0.01f,  5.0f,  0.0f, 0.0f,
        -5.0f, -0.5f - 0.01f, -5.0f,  0.0f, 2.0f, // add a small offset to the floor to prevent z-fighting

         5.0f, -0.5f - 0.01f,  5.0f,  2.0f, 0.0f,
        -5.0f, -0.5f - 0.01f, -5.0f,  0.0f, 2.0f,
         5.0f, -0.5f - 0.01f, -5.0f,  2.0f, 2.0f								
  };

  float transparent_vertices[] = {
        // positions         // texture Coords
        0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
        0.0f, -0.5f,  0.0f,  0.0f,  1.0f,
        1.0f, -0.5f,  0.0f,  1.0f,  1.0f,

        0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
        1.0f, -0.5f,  0.0f,  1.0f,  1.0f,
        1.0f,  0.5f,  0.0f,  1.0f,  0.0f
  };
  // clang-format on

  // config cube vao
  cube_vbo.SetData(cube_vertices, sizeof(cube_vertices), GL_STATIC_DRAW);
  cube_vao.AttachVertexBuffer(cube_vbo.id());
  cube_vao.SetVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
  cube_vao.SetVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                                  (void*)(3 * sizeof(float)));

  // config plane vao
  plane_vbo.SetData(plane_vertices, sizeof(plane_vertices), GL_STATIC_DRAW);
  plane_vao.AttachVertexBuffer(plane_vbo.id());
  plane_vao.SetVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
  plane_vao.SetVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                                   (void*)(3 * sizeof(float)));
  plane_vao.Unbind();

  // config transparent vao
  transparent_vbo.SetData(transparent_vertices, sizeof(transparent_vertices), GL_STATIC_DRAW);
  transparent_vao.AttachVertexBuffer(transparent_vbo.id());
  transparent_vao.SetVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
  transparent_vao.SetVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                                         (void*)(3 * sizeof(float)));
  transparent_vao.Unbind();

  // depth testing
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  // blending
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBlendEquation(GL_FUNC_ADD);  // default

  program.Use();
  program.SetUniform1i("texture1", 0);
  program.SetUniform1i("texture2", 1);
  program.SetUniform1i("texture3", 2);
  program.SetUniform1i("texture4", 3);

  // Transformation matrices
  glm::mat4 model = glm::mat4(1.0f);
  glm::mat4 projection = glm::perspective(
      glm::radians(45.0f), static_cast<float>(kWidth) / static_cast<float>(kHeight), 0.1f, 100.0f);
  program.SetUniformMat4f("projection", projection);
  program.SetUniformMat4f("model", model);

  std::vector<glm::vec3> vegetation;
  vegetation.push_back(glm::vec3(-1.5f, 0.0f, -0.48f));
  vegetation.push_back(glm::vec3(1.5f, 0.0f, 0.51f));
  vegetation.push_back(glm::vec3(0.0f, 0.0f, 0.7f));
  vegetation.push_back(glm::vec3(-0.3f, 0.0f, -2.3f));
  vegetation.push_back(glm::vec3(0.5f, 0.0f, -0.6f));

  std::vector<glm::vec3> window_positions;
  window_positions.push_back(glm::vec3(-1.5f, 0.0f, -0.3f));
  window_positions.push_back(glm::vec3(1.5f, 0.0f, 0.6f));
  window_positions.push_back(glm::vec3(0.0f, 0.0f, 0.8f));

  // sort the transparent windows before rendering
  std::map<float, glm::vec3> sorted;
  for (unsigned int i = 0; i < window_positions.size(); i++) {
    float distance = glm::length(camera->camera_position - window_positions[i]);
    sorted[distance] = window_positions[i];
  }

  // render loops
  // -----------
  while (!glfwWindowShouldClose(window)) {
    process_inputs(window);
    // render
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    program.Use();
    auto view = camera->GetViewMatrix();
    program.SetUniformMat4f("view", view);

    // bind two textures
    texture.ActivateBind(GL_TEXTURE_2D, 0);
    texture.ActivateBind(GL_TEXTURE_2D, 1);
    texture.ActivateBind(GL_TEXTURE_2D, 2);
    texture.ActivateBind(GL_TEXTURE_2D, 3);

    model = glm::mat4(1.0f);
    // draw two cubes
    cube_vao.Bind();
    program.SetUniform1i("texture_type", 0);
    model = glm::translate(model, glm::vec3(-1.0f, 0.0f, -1.0f));
    program.SetUniformMat4f("model", model);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(2.0f, 0.0f, 0.0f));
    program.SetUniformMat4f("model", model);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    cube_vao.Unbind();

    // draw plane
    plane_vao.Bind();
    program.SetUniform1i("texture_type", 1);
    model = glm::mat4(1.0f);
    program.SetUniformMat4f("model", model);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    plane_vao.Unbind();

    // draw grass and window
    transparent_vao.Bind();
    program.SetUniform1i("texture_type", 2);
    for (unsigned int i = 0; i < vegetation.size(); i++) {
      model = glm::mat4(1.0f);
      model = glm::translate(model, vegetation[i]);
      program.SetUniformMat4f("model", model);
      glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    program.SetUniform1i("texture_type", 3);
    for (auto it = sorted.rbegin(); it != sorted.rend(); ++it) {
      model = glm::mat4(1.0f);
      model = glm::translate(model, it->second);
      program.SetUniformMat4f("model", model);
      glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    transparent_vao.Unbind();

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}

void process_inputs(GLFWwindow* window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    camera->ProcessKeyboard(core::opengl::CameraMovement::FORWARD);
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    camera->ProcessKeyboard(core::opengl::CameraMovement::BACKWARD);
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    camera->ProcessKeyboard(core::opengl::CameraMovement::LEFT);
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    camera->ProcessKeyboard(core::opengl::CameraMovement::RIGHT);
}

void mouse_callback([[maybe_unused]] GLFWwindow* window, double xpos, double ypos) {
  if (first_mouse) {
    laxt_x = xpos;
    last_y = ypos;
    first_mouse = false;
  }
  float xoffset = xpos - laxt_x;
  float yoffset = last_y - ypos;  // reversed since y-coordinates go from bottom to top
  laxt_x = xpos;
  last_y = ypos;

  xoffset *= kMouseSensitivity;
  yoffset *= kMouseSensitivity;

  camera->yaw += xoffset;
  camera->pitch += yoffset;

  if (camera->pitch > 89.0f) camera->pitch = 89.0f;
  if (camera->pitch < -89.0f) camera->pitch = -89.0f;

  glm::vec3 front;
  front.x = std::cos(glm::radians(camera->yaw)) * std::cos(glm::radians(camera->pitch));
  front.y = std::sin(glm::radians(camera->pitch));
  front.z = std::sin(glm::radians(camera->yaw)) * std::cos(glm::radians(camera->pitch));
  camera->camera_front = glm::normalize(front);
}