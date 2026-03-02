#pragma once

#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include "Bitmap.h"

namespace core {
namespace io {

Bitmap ConvertBitmapToVerticalCross(const Bitmap& src);

glm::vec3 FaceCoordsToXYZ(int i, int j, int face_id, int face_size);

}  // namespace io
}  // namespace core
