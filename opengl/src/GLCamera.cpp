#include "GLCamera.h"

namespace core {
namespace opengl {

GLCamera::GLCamera(glm::vec3 position, glm::vec3 front, glm::vec3 up, float move_speed)
    : camera_position_(position), camera_front_(front), camera_up_(up), move_speed_(move_speed) {
  view_matrix_ = glm::lookAt(camera_position_, camera_position_ + camera_front_, camera_up_);
}

GLCamera::~GLCamera() {}

void GLCamera::ProcessKeyboard(CameraMovement direction) {
  if (direction == CameraMovement::FORWARD) camera_position_ += camera_front_ * move_speed_;
  if (direction == CameraMovement::BACKWARD) camera_position_ -= camera_front_ * move_speed_;
  if (direction == CameraMovement::LEFT)
    camera_position_ -= glm::normalize(glm::cross(camera_front_, camera_up_)) * move_speed_;
  if (direction == CameraMovement::RIGHT)
    camera_position_ += glm::normalize(glm::cross(camera_front_, camera_up_)) * move_speed_;
  view_matrix_ = glm::lookAt(camera_position_, camera_position_ + camera_front_, camera_up_);
}

}  // namespace opengl
}  // namespace core