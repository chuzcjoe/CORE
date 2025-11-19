// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// clang-format on

#include <iostream>

#include "GLCamera.h"
#include "GLProgram.h"
#include "GLTexture.h"
#include "GLUtils.h"
#include "GLVertexArray.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

void process_inputs(GLFWwindow* window);
void mouse_callback([[maybe_unused]] GLFWwindow* window, double xpos, double ypos);

// settings
const unsigned int kWidth = 1000;
const unsigned int kHeight = 1000;
const glm::vec3 kCameraPos = glm::vec3(0.0f, 2.0f, 6.0f);
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

// Object shaders
const char* obj_vertex_shader_source = OPENGL_VERTEX_SHADER(
    layout(location = 0) in vec3 aPos; 
    layout(location = 1) in vec3 aNormal;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    out vec3 Normal;
    out vec3 FragPos;

    void main() {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        FragPos = vec3(model * vec4(aPos, 1.0)); // get fragment position in world space
        Normal = aNormal;
    }
);

const char* obj_fragment_shader_source = OPENGL_FRAGMENT_SHADER(
    in vec3 Normal;
    in vec3 FragPos;
    out vec4 FragColor; 
    uniform vec3 objectColor;
    uniform vec3 lightColor;
    uniform vec3 lightPos;

    void main() {
        float ambientStrength = 0.1;

        // ambient
        vec3 ambient = ambientStrength * lightColor;
        
        // diffuse
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lightColor;

        // final color
        vec3 result = (ambient + diffuse) * objectColor;
        FragColor = vec4(result, 1.0f);
    }
);

// Light shaders
const char* light_vertex_shader_source = OPENGL_VERTEX_SHADER(
    layout(location = 0) in vec3 aPos; 
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    void main() {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
    }
);

const char* light_fragment_shader_source = OPENGL_FRAGMENT_SHADER(
    out vec4 FragColor; 
    void main() { 
        FragColor = vec4(1.0f);
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

  GLFWwindow* window = glfwCreateWindow(kWidth, kHeight, "OpenGLCamera", NULL, NULL);
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
  core::opengl::GLProgram obj_program(obj_vertex_shader_source, obj_fragment_shader_source);
  core::opengl::GLProgram light_program(light_vertex_shader_source, light_fragment_shader_source);
  core::opengl::GLVertexArray obj_vao;
  core::opengl::GLVertexArray light_vao;

  // clang-format off
  float vertices[] = {
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 

    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
  };
  // clang-format on

  // setup object cube
  obj_vao.Bind();
  obj_vao.SetVertexData(vertices, sizeof(vertices), GL_STATIC_DRAW);
  // position attribute
  obj_vao.SetVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
  // normal attribute
  obj_vao.SetVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                                 (void*)(3 * sizeof(float)));
  obj_vao.Unbind();

  // setup light cube
  light_vao.Bind();
  light_vao.SetVertexData(vertices, sizeof(vertices), GL_STATIC_DRAW);
  light_vao.SetVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
  light_vao.Unbind();

  glEnable(GL_DEPTH_TEST);

  // Transformation matrices
  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model, glm::vec3(1.2f, 1.0f, 2.0f));
  model = glm::scale(model, glm::vec3(0.2f));
  glm::mat4 projection = glm::perspective(
      glm::radians(45.0f), static_cast<float>(kWidth) / static_cast<float>(kHeight), 0.1f, 100.0f);

  light_program.Use();
  light_program.SetUniformMat4f("projection", projection);
  light_program.SetUniformMat4f("model", model);

  model = glm::mat4(1.0f);
  obj_program.Use();
  obj_program.SetUniformMat4f("projection", projection);
  obj_program.SetUniformMat4f("model", model);
  obj_program.SetUniform3f("lightPos", 1.2f, 1.0f, 2.0f);

  // render loop
  // -----------
  while (!glfwWindowShouldClose(window)) {
    process_inputs(window);
    // render
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // draw light
    light_program.Use();
    auto view = camera->GetViewMatrix();
    light_program.SetUniformMat4f("view", view);
    light_vao.Bind();
    glDrawArrays(GL_TRIANGLES, 0, 36);

    // draw obj
    obj_program.Use();
    obj_program.SetUniform3f("objectColor", 1.0f, 0.5f, 0.31f);
    obj_program.SetUniform3f("lightColor", 1.0f, 1.0f, 1.0f);
    obj_program.SetUniformMat4f("view", view);
    obj_vao.Bind();
    glDrawArrays(GL_TRIANGLES, 0, 36);

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