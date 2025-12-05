// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// clang-format on

#include <iostream>
#include <map>

#include "GLCamera.h"
#include "GLFrameBuffer.h"
#include "GLProgram.h"
#include "GLTexture.h"
#include "GLUtils.h"
#include "GLVertexArray.h"
#include "GLVertexBuffer.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

void framebuffer_size_callback([[maybe_unused]] GLFWwindow* window, int width, int height);
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

const char* screen_vertex_shader_source = OPENGL_VERTEX_SHADER(
  layout (location = 0) in vec2 aPos;
  layout (location = 1) in vec2 aTexCoords;
  out vec2 TexCoords;
  void main() {
      gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0); 
      TexCoords = aTexCoords;
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
      } else {
        FragColor = vec4(0.0, 1.0, 0.0, 1.0);
      }
    }
);

const char* screen_fragment_shader_source = OPENGL_FRAGMENT_SHADER(
  out vec4 FragColor;
  in vec2 TexCoords;
  uniform sampler2D screenTexture;
  const float offset = 1.0 / 300.0;  
  void main() {
      vec2 offsets[9] = vec2[](
          vec2(-offset,  offset), // top-left
          vec2( 0.0f,    offset), // top-center
          vec2( offset,  offset), // top-right
          vec2(-offset,  0.0f),   // center-left
          vec2( 0.0f,    0.0f),   // center-center
          vec2( offset,  0.0f),   // center-right
          vec2(-offset, -offset), // bottom-left
          vec2( 0.0f,   -offset), // bottom-center
          vec2( offset, -offset)  // bottom-right    
      );

      float kernel[9] = float[](
          -1, -1, -1,
          -1,  9, -1,
          -1, -1, -1
      );
      
      vec3 sampleTex[9];
      for(int i = 0; i < 9; i++) {
          sampleTex[i] = vec3(texture(screenTexture, TexCoords.st + offsets[i]));
      }
      vec3 col = vec3(0.0);
      for(int i = 0; i < 9; i++) {
        col += sampleTex[i] * kernel[i];
      }
      FragColor = vec4(col, 1.0);
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

  GLFWwindow* window = glfwCreateWindow(kWidth, kHeight, "OpenGLFrameBuffer", NULL, NULL);
  if (window == nullptr) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetCursorPosCallback(window, mouse_callback);

  // glad: load all OpenGL function pointers
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    throw std::runtime_error("Failed to initialize GLAD");
  }

  // Use GL after glad is initialized
  core::opengl::GLProgram program(vertex_shader_source, fragment_shader_source);
  core::opengl::GLProgram screen_program(screen_vertex_shader_source,
                                         screen_fragment_shader_source);
  core::opengl::GLVertexArray cube_vao;
  core::opengl::GLVertexArray plane_vao;
  core::opengl::GLVertexArray transparent_vao;
  core::opengl::GLVertexArray screen_vao;
  core::opengl::GLVertexBuffer cube_vbo;
  core::opengl::GLVertexBuffer plane_vbo;
  core::opengl::GLVertexBuffer transparent_vbo;
  core::opengl::GLVertexBuffer screen_vbo;
  core::opengl::GLTexture texture(GL_TEXTURE_2D);
  core::opengl::GLFrameBuffer framebuffer(true);  // use renderbuffer

  texture.Load2DTextureFromFile("./examples/data/metal.png", GL_RGB, 0);
  texture.Load2DTextureFromFile("./examples/data/marble.jpg", GL_RGB, 1);
  texture.Load2DTextureFromFile("./examples/data/grass.png", GL_RGBA, 2, GL_CLAMP_TO_EDGE, false);

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

  float quad_vertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
        // positions   // texCoords
        -0.3f,  1.0f,  0.0f, 1.0f,
        -0.3f,  0.5f,  0.0f, 0.0f,
         0.3f,  0.5f,  1.0f, 0.0f,

        -0.3f,  1.0f,  0.0f, 1.0f,
         0.3f,  0.5f,  1.0f, 0.0f,
         0.3f,  1.0f,  1.0f, 1.0f
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

  // config transparent vao
  transparent_vbo.SetData(transparent_vertices, sizeof(transparent_vertices), GL_STATIC_DRAW);
  transparent_vao.AttachVertexBuffer(transparent_vbo.id());
  transparent_vao.SetVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
  transparent_vao.SetVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                                         (void*)(3 * sizeof(float)));

  // config screen vao
  screen_vbo.SetData(quad_vertices, sizeof(quad_vertices), GL_STATIC_DRAW);
  screen_vao.AttachVertexBuffer(screen_vbo.id());
  screen_vao.SetVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
  screen_vao.SetVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                                    (void*)(2 * sizeof(float)));

  // config framebuffer
  framebuffer.AttachTexture2D(GL_COLOR_ATTACHMENT0, GL_RGB, kWidth, kHeight, GL_RGB,
                              GL_UNSIGNED_BYTE);
  framebuffer.AttachRenderBuffer(GL_DEPTH24_STENCIL8, kWidth, kHeight);
  if (!framebuffer.IsComplete()) {
    std::cout << "ERROR: Framebuffer is not complete!" << std::endl;
  } else {
    std::cout << "Framebuffer is complete!" << std::endl;
  }

  // blending
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBlendEquation(GL_FUNC_ADD);  // default

  program.Use();
  program.SetUniform1i("texture1", 0);
  program.SetUniform1i("texture2", 1);
  program.SetUniform1i("texture3", 2);

  screen_program.Use();
  screen_program.SetUniform1i("screenTexture", 0);

  // Transformation matrices
  program.Use();
  glm::mat4 model = glm::mat4(1.0f);
  glm::mat4 projection = glm::perspective(
      glm::radians(45.0f), static_cast<float>(kWidth) / static_cast<float>(kHeight), 0.1f, 100.0f);
  program.SetUniformMat4f("projection", projection);

  std::vector<glm::vec3> vegetation;
  vegetation.push_back(glm::vec3(-1.5f, 0.0f, -0.48f));
  vegetation.push_back(glm::vec3(1.5f, 0.0f, 0.51f));
  vegetation.push_back(glm::vec3(0.0f, 0.0f, 0.7f));
  vegetation.push_back(glm::vec3(-0.3f, 0.0f, -2.3f));
  vegetation.push_back(glm::vec3(0.5f, 0.0f, -0.6f));

  // render loops
  // -----------
  while (!glfwWindowShouldClose(window)) {
    process_inputs(window);

    // 1. first render pass
    // bind to the created framebuffer
    framebuffer.Bind();
    glEnable(GL_DEPTH_TEST);

    // clear framebuffer's content
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    program.Use();
    auto view = camera->GetViewMatrix();
    program.SetUniformMat4f("view", view);

    // bind textures
    texture.ActivateBind(GL_TEXTURE_2D, 0);
    texture.ActivateBind(GL_TEXTURE_2D, 1);
    texture.ActivateBind(GL_TEXTURE_2D, 2);

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

    // draw grass
    transparent_vao.Bind();
    program.SetUniform1i("texture_type", 2);
    for (unsigned int i = 0; i < vegetation.size(); i++) {
      model = glm::mat4(1.0f);
      model = glm::translate(model, vegetation[i]);
      program.SetUniformMat4f("model", model);
      glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    transparent_vao.Unbind();

    // 2. second render pass
    // bind the default framebuffer and draw a quad with the attached framebuffer color texture
    framebuffer.Unbind();
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    model = glm::mat4(1.0f);
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
    plane_vao.Bind();
    program.SetUniform1i("texture_type", 1);
    model = glm::mat4(1.0f);
    program.SetUniformMat4f("model", model);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    plane_vao.Unbind();
    transparent_vao.Bind();
    program.SetUniform1i("texture_type", 2);
    for (unsigned int i = 0; i < vegetation.size(); i++) {
      model = glm::mat4(1.0f);
      model = glm::translate(model, vegetation[i]);
      program.SetUniformMat4f("model", model);
      glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    transparent_vao.Unbind();

    glDisable(GL_DEPTH_TEST);  // disable depth test so screen-space quad isn't discarded due to
                               // depth test.
    screen_program.Use();
    screen_vao.Bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, framebuffer.GetTexutureID());
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}

void framebuffer_size_callback([[maybe_unused]] GLFWwindow* window, int width, int height) {
  glViewport(0, 0, width, height);
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