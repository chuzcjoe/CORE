#include "VulkanCamera.h"

namespace core {
namespace vulkan {

VulkanCamera::VulkanCamera(glm::vec3 position, glm::vec3 front, glm::vec3 up, float move_speed)
    : camera_position(position), camera_front(front), camera_up(up), move_speed_(move_speed) {
  // Make sure front is normalized
  camera_front = glm::normalize(camera_front);
  glm::vec3 f = camera_front;

  // Derive yaw and pitch from the front vector (Y-up convention)
  yaw = glm::degrees(glm::atan(f.z, f.x));  // atan2(z, x)
  pitch = glm::degrees(glm::asin(f.y));     // y is “vertical” here
  view_matrix_ = glm::lookAt(camera_position, camera_position + camera_front, camera_up);
}

VulkanCamera::~VulkanCamera() {}

void VulkanCamera::ProcessKeyboard(CameraMovement direction) {
  if (direction == CameraMovement::FORWARD) camera_position += camera_front * move_speed_;
  if (direction == CameraMovement::BACKWARD) camera_position -= camera_front * move_speed_;
  if (direction == CameraMovement::LEFT)
    camera_position -= glm::normalize(glm::cross(camera_front, camera_up)) * move_speed_;
  if (direction == CameraMovement::RIGHT)
    camera_position += glm::normalize(glm::cross(camera_front, camera_up)) * move_speed_;
  view_matrix_ = glm::lookAt(camera_position, camera_position + camera_front, camera_up);
}

}  // namespace vulkan
}  // namespace core