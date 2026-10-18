#include "IOUtils.h"

#include <limits>
#include <stdexcept>

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

namespace {

glm::vec4 SamplePanorama(const Bitmap& src, int i, int j, int face, int face_size) {
  const int clamp_width = src.width - 1;
  const int clamp_height = src.height - 1;
  const glm::vec3 direction = FaceCoordsToXYZ(i, j, face, face_size);
  const float radius = std::hypot(direction.x, direction.y);
  const float theta = std::atan2(direction.y, direction.x);
  const float phi = std::atan2(direction.z, radius);
  // Preserve the existing projection and bilinear sampling arithmetic.
  const float source_x =
      static_cast<float>(2.0f * static_cast<float>(face_size) * (theta + M_PI) / M_PI);
  const float source_y =
      static_cast<float>(2.0f * static_cast<float>(face_size) * (M_PI / 2.0f - phi) / M_PI);
  const int x1 = std::clamp(static_cast<int>(std::floor(source_x)), 0, clamp_width);
  const int y1 = std::clamp(static_cast<int>(std::floor(source_y)), 0, clamp_height);
  const int x2 = std::clamp(x1 + 1, 0, clamp_width);
  const int y2 = std::clamp(y1 + 1, 0, clamp_height);
  const float s = source_x - static_cast<float>(x1);
  const float t = source_y - static_cast<float>(y1);
  const glm::vec4 a = src.GetPixel(x1, y1);
  const glm::vec4 b = src.GetPixel(x2, y1);
  const glm::vec4 c = src.GetPixel(x1, y2);
  const glm::vec4 d = src.GetPixel(x2, y2);
  const glm::vec4 color =
      a * (1.0f - s) * (1.0f - t) + b * s * (1.0f - t) + c * (1.0f - s) * t + d * s * t;
  return color;
}

}  // namespace

Bitmap ConvertBitmapToVerticalCross(const Bitmap& src) {
  if (src.type != BitmapType::BitmapType_2D) return Bitmap();

  const int face_size = src.width / 4;

  const int w = face_size * 3;
  const int h = face_size * 4;

  Bitmap result(w, h, src.depth, src.format);

  const glm::ivec2 kFaceOffsets[] = {glm::ivec2(face_size, face_size * 3),
                                     glm::ivec2(0, face_size),
                                     glm::ivec2(face_size, face_size),
                                     glm::ivec2(face_size * 2, face_size),
                                     glm::ivec2(face_size, 0),
                                     glm::ivec2(face_size, face_size * 2)};

  for (int face = 0; face != 6; face++) {
    for (int i = 0; i != face_size; i++) {
      for (int j = 0; j != face_size; j++) {
        const glm::vec4 color = SamplePanorama(src, i, j, face, face_size);
        result.SetPixel(i + kFaceOffsets[face].x, j + kFaceOffsets[face].y, color);
      }
    };
  }

  return result;
}

Bitmap ConvertBitmapToCubeMapFaces(const Bitmap& src) {
  if (src.type != BitmapType::BitmapType_2D || src.width < 4 || src.width % 4 != 0 ||
      src.height != src.width / 2 || src.depth < 1 || src.depth > 4 ||
      (src.format != BitmapFormat::BitmapFormat_Uint8 &&
       src.format != BitmapFormat::BitmapFormat_Float)) {
    throw std::invalid_argument("Cubemap conversion requires a valid 2:1 panorama");
  }
  const size_t width = static_cast<size_t>(src.width);
  const size_t height = static_cast<size_t>(src.height);
  const size_t pixel_bytes = static_cast<size_t>(src.depth) *
                             static_cast<size_t>(Bitmap::GetBytesPerComponent(src.format));
  const int face_size = src.width / 4;
  // Bitmap currently calculates allocation sizes using int arithmetic.
  const size_t max_pixels = static_cast<size_t>(std::numeric_limits<int>::max()) / pixel_bytes;
  if (static_cast<size_t>(face_size) > max_pixels / 6 / static_cast<size_t>(face_size)) {
    throw std::length_error("Cubemap exceeds the supported bitmap size");
  }
  if (width > std::numeric_limits<size_t>::max() / height / pixel_bytes ||
      src.pixel.size() < width * height * pixel_bytes) {
    throw std::invalid_argument("Panorama pixel storage is incomplete");
  }

  // Fill six rows of faces as an ordinary 2D bitmap so SetPixel uses the
  // source component stride. Publish the existing cube metadata after filling.
  Bitmap result(face_size, face_size * 6, src.depth, src.format);
  const int face_order[] = {5, 1, 4, 0, 2, 3};
  for (int face = 0; face < 6; ++face) {
    for (int y = 0; y < face_size; ++y) {
      for (int x = 0; x < face_size; ++x) {
        const glm::vec4 color = SamplePanorama(src, x, y, face, face_size);
        const int output_x = face == 0 ? face_size - x - 1 : x;
        const int output_y = face == 0 ? face_size - y - 1 : y;
        result.SetPixel(output_x, face_order[face] * face_size + output_y, color);
      }
    }
  }
  result.height = face_size;
  result.depth = src.depth * 6;
  result.type = BitmapType::BitmapType_Cube;
  return result;
}

Bitmap ConvertVerticalCrossToCubeMapFaces(const Bitmap& src) {
  const int face_width = src.width / 3;
  const int face_height = src.height / 4;

  Bitmap cubemap(face_width, face_height, 6 * src.depth, src.format);
  cubemap.type = BitmapType::BitmapType_Cube;

  const uint8_t* src_ptr = src.pixel.data();
  uint8_t* dst_ptr = cubemap.pixel.data();

  /*
      ------
      | +Y |
   ----------------
   | -X | +Z | +X |
   ----------------
      | -Y |
      ------
      | -Z |
      ------
  */

  const int pixel_size = src.depth * Bitmap::GetBytesPerComponent(src.format);

  for (int face = 0; face != 6; ++face) {
    for (int j = 0; j != face_height; ++j) {
      for (int i = 0; i != face_width; ++i) {
        int x = 0;
        int y = 0;

        switch (face) {
          // CUBE_MAP_POSITIVE_X
          case 0:
            x = 2 * face_width + i;
            y = 1 * face_height + j;
            break;

          // CUBE_MAP_NEGATIVE_X
          case 1:
            x = i;
            y = face_height + j;
            break;

          // CUBE_MAP_POSITIVE_Y
          case 2:
            x = 1 * face_width + i;
            y = j;
            break;

          // CUBE_MAP_NEGATIVE_Y
          case 3:
            x = 1 * face_width + i;
            y = 2 * face_height + j;
            break;

          // CUBE_MAP_POSITIVE_Z
          case 4:
            x = face_width + i;
            y = face_height + j;
            break;

          // CUBE_MAP_NEGATIVE_Z
          case 5:
            x = 2 * face_width - (i + 1);
            y = src.height - (j + 1);
            break;
        }

        memcpy(dst_ptr, src_ptr + (y * src.width + x) * pixel_size, pixel_size);
        dst_ptr += pixel_size;
      }
    }
  }

  return cubemap;
}

}  // namespace io
}  // namespace core
