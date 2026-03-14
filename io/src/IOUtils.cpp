#include "IOUtils.h"

namespace core {
namespace io {

glm::vec3 FaceCoordsToXYZ(int i, int j, int face_id, int face_size) {
  const float A = 2.0f * (static_cast<float>(i) + 0.5f) / static_cast<float>(face_size);
  const float B = 2.0f * (static_cast<float>(j) + 0.5f) / static_cast<float>(face_size);

  if (face_id == 0) return glm::vec3(-1.0f, A - 1.0f, B - 1.0f);
  if (face_id == 1) return glm::vec3(A - 1.0f, -1.0f, 1.0f - B);
  if (face_id == 2) return glm::vec3(1.0f, A - 1.0f, 1.0f - B);
  if (face_id == 3) return glm::vec3(1.0f - A, 1.0f, 1.0f - B);
  if (face_id == 4) return glm::vec3(B - 1.0f, A - 1.0f, 1.0f);
  if (face_id == 5) return glm::vec3(1.0f - B, A - 1.0f, -1.0f);

  return glm::vec3();
}

}  // namespace io
}  // namespace core
