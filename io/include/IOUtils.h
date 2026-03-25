#pragma once

#include <algorithm>
#include <cmath>
#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include "Bitmap.h"

namespace core {
namespace io {

glm::vec3 FaceCoordsToXYZ(int i, int j, int face_id, int face_size);

Bitmap ConvertBitmapToVerticalCross(const Bitmap& src);

Bitmap ConvertVerticalCrossToCubeMapFaces(const Bitmap& src);

}  // namespace io
}  // namespace core
