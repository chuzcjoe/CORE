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
    uniform int texture_type;
    float near = 0.1; 
    float far  = 100.0; 
  
    float LinearizeDepth(float depth) {
        float z = depth * 2.0 - 1.0; // back to NDC 
        return (2.0 * near * far) / (far + near - z * (far - near));	
    }

    void main() {
      if (texture_type == 0) {
        FragColor = texture(texture1, TexCoords);
      } else if (texture_type == 1) {
        FragColor = texture(texture2, TexCoords);
      } else {
        FragColor = vec4(0.0, 1.0, 0.0, 1.0);
      }
      // float depth = LinearizeDepth(gl_FragCoord.z) / far; // divide by far for demonstration
      // FragColor = vec4(vec3(depth), 1.0);
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

  GLFWwindow* window = glfwCreateWindow(kWidth, kHeight, "OpenGLDepthStencil", NULL, NULL);
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
  core::opengl::GLVertexBuffer cube_vbo;
  core::opengl::GLVertexBuffer plane_vbo;
  core::opengl::GLTexture texture(GL_TEXTURE_2D, GL_REPEAT, GL_LINEAR);
  texture.Load2DTextureFromFile("./examples/data/metal.png", GL_RGB, 0);
  texture.Load2DTextureFromFile("./examples/data/marble.jpg", GL_RGB, 1);

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

  // depth testing
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  // stencil testing
  glEnable(GL_STENCIL_TEST);
  glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

  program.Use();
  program.SetUniform1i("texture1", 0);
  program.SetUniform1i("texture2", 1);

  // Transformation matrices
  glm::mat4 model = glm::mat4(1.0f);
  glm::mat4 projection = glm::perspective(
      glm::radians(45.0f), static_cast<float>(kWidth) / static_cast<float>(kHeight), 0.1f, 100.0f);
  program.SetUniformMat4f("projection", projection);
  program.SetUniformMat4f("model", model);

  // render loops
  // -----------
  while (!glfwWindowShouldClose(window)) {
    process_inputs(window);
    // render
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    program.Use();
    auto view = camera->GetViewMatrix();
    program.SetUniformMat4f("view", view);

    // bind two textures
    texture.ActivateBind(GL_TEXTURE_2D, 0);
    texture.ActivateBind(GL_TEXTURE_2D, 1);

    model = glm::mat4(1.0f);
    // draw two cubes
    glStencilFunc(GL_ALWAYS, 1, 0xFF);  // all fragments should update the stencil buffer
    glStencilMask(0xFF);                // enable writing to the stencil buffer
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

    // draw two upscaled cubes to create a border effect
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);  // draw only where stencil is not 1
    glStencilMask(0x00);                  // disable writing to the stencil buffer
    float scale = 1.01f;
    cube_vao.Bind();
    program.SetUniform1i("texture_type", 2);
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-1.0f, 0.0f, -1.0f));
    model = glm::scale(model, glm::vec3(scale, scale, scale));
    program.SetUniformMat4f("model", model);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(2.0f, 0.0f, 0.0f));
    model = glm::scale(model, glm::vec3(scale, scale, scale));
    program.SetUniformMat4f("model", model);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    cube_vao.Unbind();

    // draw plane
    glStencilMask(0x00);  // disable writing to the stencil buffer
    plane_vao.Bind();
    program.SetUniform1i("texture_type", 1);
    model = glm::mat4(1.0f);
    program.SetUniformMat4f("model", model);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    plane_vao.Unbind();

    // make sure to stencil buffer will be reset in glClear()
    glStencilMask(0xFF);
    glStencilFunc(GL_ALWAYS, 0, 0xFF);

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