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

// Project a 2:1 panorama directly into +X, -X, +Y, -Y, +Z, -Z face storage.
// The face size is src.width / 4; each source pixel has 1-4 components.
// Reject invalid dimensions, unsupported formats and incomplete pixel storage.
Bitmap ConvertBitmapToCubeMapFaces(const Bitmap& src);

Bitmap ConvertVerticalCrossToCubeMapFaces(const Bitmap& src);

}  // namespace io
}  // namespace core
