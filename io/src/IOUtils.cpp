#include "IOUtils.h"

#include <stb_image_resize2.h>

#include <algorithm>
#include <cmath>

namespace core {
namespace io {

Bitmap ConvertBitmapToVerticalCross(const Bitmap& src) {
  const int face_size = src.width() / 4;

  const int w = face_size * 3;
  const int h = face_size * 4;
  const int c = src.channels();

  Bitmap result = Bitmap(w, h, c);

  const glm::ivec2 kFaceOffsets[] = {glm::ivec2(face_size, face_size * 3),
                                     glm::ivec2(0, face_size),
                                     glm::ivec2(face_size, face_size),
                                     glm::ivec2(face_size * 2, face_size),
                                     glm::ivec2(face_size, 0),
                                     glm::ivec2(face_size, face_size * 2)};

  const int clamp_w = src.width() - 1;
  const int clamp_h = src.height() - 1;

  for (int face = 0; face != 6; ++face) {
    for (int i = 0; i != face_size; ++i) {
      for (int j = 0; j != face_size; ++j) {
        const glm::vec3 P = FaceCoordsToXYZ(i, j, face, face_size);
        const float R = std::hypot(P.x, P.y);
        const float theta = std::atan2(P.y, P.x);
        const float phi = std::atan2(P.z, R);
        //	float point source coordinates
        const float Uf = float(2.0f * face_size * (theta + M_PI) / M_PI);
        const float Vf = float(2.0f * face_size * (M_PI / 2.0f - phi) / M_PI);
        // 4-samples for bilinear interpolation
        const int U1 = std::clamp(int(std::floor(Uf)), 0, clamp_w);
        const int V1 = std::clamp(int(std::floor(Vf)), 0, clamp_h);
        const int U2 = std::clamp(U1 + 1, 0, clamp_w);
        const int V2 = std::clamp(V1 + 1, 0, clamp_h);
        // fractional part
        const float s = Uf - U1;
        const float t = Vf - V1;
        // fetch 3-samples
        const glm::vec3 A =
            glm::vec3(src.GetPixel(V1, U1, 0), src.GetPixel(V1, U1, 1), src.GetPixel(V1, U1, 2));
        const glm::vec3 B =
            glm::vec3(src.GetPixel(V1, U2, 0), src.GetPixel(V1, U2, 1), src.GetPixel(V1, U2, 2));
        const glm::vec3 C =
            glm::vec3(src.GetPixel(V2, U1, 0), src.GetPixel(V2, U1, 1), src.GetPixel(V2, U1, 2));
        // bilinear interpolation
        const glm::vec3 color = A * (1 - s) * (1 - t) + B * (s) * (1 - t) + C * (1 - s) * t;
        const int dst_col = i + kFaceOffsets[face].x;
        const int dst_row = j + kFaceOffsets[face].y;
        result.SetPixel(dst_row, dst_col, 0, uint8_t(color.r));
        result.SetPixel(dst_row, dst_col, 1, uint8_t(color.g));
        result.SetPixel(dst_row, dst_col, 2, uint8_t(color.b));
      }
    };
  }

  return result;
}

glm::vec3 FaceCoordsToXYZ(int i, int j, int face_id, int face_size) {
  const float A = 2.0f * float(i) / face_size;
  const float B = 2.0f * float(j) / face_size;

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
