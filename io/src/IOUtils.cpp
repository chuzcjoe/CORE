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

  const int clamp_width = src.width - 1;
  const int clamp_height = src.height - 1;

  for (int face = 0; face != 6; face++) {
    for (int i = 0; i != face_size; i++) {
      for (int j = 0; j != face_size; j++) {
        const glm::vec3 P = FaceCoordsToXYZ(i, j, face, face_size);
        const float R = std::hypot(P.x, P.y);
        const float theta = std::atan2(P.y, P.x);
        const float phi = std::atan2(P.z, R);
        //	float point source coordinates
        const float Uf = float(2.0f * face_size * (theta + M_PI) / M_PI);
        const float Vf = float(2.0f * face_size * (M_PI / 2.0f - phi) / M_PI);
        // 4-samples for bilinear interpolation
        const int U1 = std::clamp(int(std::floor(Uf)), 0, clamp_width);
        const int V1 = std::clamp(int(std::floor(Vf)), 0, clamp_height);
        const int U2 = std::clamp(U1 + 1, 0, clamp_width);
        const int V2 = std::clamp(V1 + 1, 0, clamp_height);
        // fractional part
        const float s = Uf - U1;
        const float t = Vf - V1;
        // fetch 4-samples
        const glm::vec4 A = src.GetPixel(U1, V1);
        const glm::vec4 B = src.GetPixel(U2, V1);
        const glm::vec4 C = src.GetPixel(U1, V2);
        const glm::vec4 D = src.GetPixel(U2, V2);
        // bilinear interpolation
        const glm::vec4 color =
            A * (1 - s) * (1 - t) + B * (s) * (1 - t) + C * (1 - s) * t + D * (s) * (t);
        result.SetPixel(i + kFaceOffsets[face].x, j + kFaceOffsets[face].y, color);
      }
    };
  }

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
