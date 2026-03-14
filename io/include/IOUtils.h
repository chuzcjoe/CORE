#pragma once

#include <algorithm>
#include <cmath>
#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include "Bitmap.h"

namespace core {
namespace io {

glm::vec3 FaceCoordsToXYZ(int i, int j, int face_id, int face_size);

template <typename T, int Channels>
Bitmap<T, Channels> ConvertBitmapToVerticalCross(const Bitmap<T, Channels>& src) {
  static_assert(Channels >= 3, "ConvertBitmapToVerticalCross requires at least 3 channels");

  constexpr float kPi = 3.14159265358979323846f;
  constexpr float kHalfPi = kPi * 0.5f;
  constexpr float kInvPi = 1.0f / kPi;
  constexpr float kInvTwoPi = 0.5f / kPi;

  const int face_size = src.width() / 4;
  const int width = face_size * 3;
  const int height = face_size * 4;

  Bitmap<T, Channels> result(width, height);

  const glm::ivec2 kFaceOffsets[] = {glm::ivec2(face_size, face_size * 3),
                                     glm::ivec2(0, face_size),
                                     glm::ivec2(face_size, face_size),
                                     glm::ivec2(face_size * 2, face_size),
                                     glm::ivec2(face_size, 0),
                                     glm::ivec2(face_size, face_size * 2)};

  const int src_w = src.width();
  const int src_h = src.height();
  const int clamp_h = src_h - 1;
  const float src_wf = static_cast<float>(src_w);
  const float src_hf = static_cast<float>(src_h);

  for (int face = 0; face != 6; ++face) {
    for (int i = 0; i != face_size; ++i) {
      for (int j = 0; j != face_size; ++j) {
        const glm::vec3 point = FaceCoordsToXYZ(i, j, face, face_size);
        const float radius = std::hypot(point.x, point.y);
        const float theta = std::atan2(point.y, point.x);
        const float phi = std::atan2(point.z, radius);
        const float uf = (theta + kPi) * kInvTwoPi * src_wf;
        const float vf = (kHalfPi - phi) * kInvPi * src_hf;

        int u0 = static_cast<int>(std::floor(uf));
        const float s = uf - static_cast<float>(u0);
        u0 %= src_w;
        if (u0 < 0) u0 += src_w;
        int u1 = u0 + 1;
        if (u1 == src_w) u1 = 0;

        const int v0 = std::clamp(static_cast<int>(std::floor(vf)), 0, clamp_h);
        const int v1 = std::min(v0 + 1, clamp_h);
        const float t = vf - static_cast<float>(v0);

        const float inv_s = 1.0f - s;
        const float inv_t = 1.0f - t;
        const int dst_col = i + kFaceOffsets[face].x;
        const int dst_row = j + kFaceOffsets[face].y;
        for (int channel = 0; channel < Channels; ++channel) {
          const float a = static_cast<float>(src.GetPixel(v0, u0, channel));
          const float b = static_cast<float>(src.GetPixel(v0, u1, channel));
          const float c = static_cast<float>(src.GetPixel(v1, u0, channel));
          const float d = static_cast<float>(src.GetPixel(v1, u1, channel));
          const float color = a * inv_s * inv_t + b * s * inv_t + c * inv_s * t + d * s * t;
          result.SetPixel(dst_row, dst_col, channel, static_cast<T>(color));
        }
      }
    }
  }

  return result;
}

}  // namespace io
}  // namespace core
