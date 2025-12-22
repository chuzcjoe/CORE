#pragma once

#include <iostream>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

namespace core {
namespace vulkan {

enum class CameraMovement { FORWARD, BACKWARD, LEFT, RIGHT };

class VulkanCamera {
 public:
  VulkanCamera(glm::vec3 position, glm::vec3 front, glm::vec3 up, float move_speed = 2.5f);
  ~VulkanCamera();

  void ProcessKeyboard(CameraMovement direction);

  glm::mat4 GetViewMatrix() {
    view_matrix_ = glm::lookAt(camera_position, camera_position + camera_front, camera_up);
    return view_matrix_;
  }

  glm::vec3 camera_position;
  glm::vec3 camera_front;
  glm::vec3 camera_up;

  float yaw = -90.0f;
  float pitch = 0.0f;

 private:
  glm::mat4 view_matrix_;

  float move_speed_ = 2.5f;
};

}  // namespace vulkan
}  // namespace core