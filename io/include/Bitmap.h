#pragma once

#include <string.h>

#include <vector>

#include "glm/glm.hpp"

enum class BitmapType { BitmapType_2D, BitmapType_Cube };

enum class BitmapFormat {
  BitmapFormat_Uint8,
  BitmapFormat_Float,
};

namespace core {
namespace io {

struct Bitmap {
  Bitmap() = default;

  Bitmap(int w, int h, int d, BitmapFormat fmt)
      : width(w), height(h), depth(d), format(fmt), pixel(w * h * d * GetBytesPerComponent(fmt)) {
    InitGetSetFuncs();
  }

  Bitmap(int w, int h, int depth, BitmapFormat fmt, const void* ptr)
      : width(w),
        height(h),
        depth(depth),
        format(fmt),
        pixel(w * h * depth * GetBytesPerComponent(fmt)) {
    InitGetSetFuncs();
    memcpy(pixel.data(), ptr, pixel.size());
  }

  int width = 0;
  int height = 0;
  int depth = 1;
  BitmapFormat format = BitmapFormat::BitmapFormat_Uint8;
  BitmapType type = BitmapType::BitmapType_2D;
  std::vector<uint8_t> pixel;

  static int GetBytesPerComponent(BitmapFormat fmt) {
    if (fmt == BitmapFormat::BitmapFormat_Uint8) return 1;
    if (fmt == BitmapFormat::BitmapFormat_Float) return 4;
    return 0;
  }

  void SetPixel(int x, int y, const glm::vec4& c) { (*this.*SetPixelFunc)(x, y, c); }

  glm::vec4 GetPixel(int x, int y) const { return ((*this.*GetPixelFunc)(x, y)); }

 private:
  using SetPixel_t = void (Bitmap::*)(int, int, const glm::vec4&);
  using GetPixel_t = glm::vec4 (Bitmap::*)(int, int) const;
  SetPixel_t SetPixelFunc;
  GetPixel_t GetPixelFunc;

  void InitGetSetFuncs() {
    switch (format) {
      case BitmapFormat::BitmapFormat_Uint8:
        SetPixelFunc = &Bitmap::SetPixelUnsignedByte;
        GetPixelFunc = &Bitmap::GetPixelUnsignedByte;
        break;
      case BitmapFormat::BitmapFormat_Float:
        SetPixelFunc = &Bitmap::SetPixelFloat;
        GetPixelFunc = &Bitmap::GetPixelFloat;
        break;
    }
  }

  void SetPixelFloat(int x, int y, const glm::vec4& c) {
    const int offset = depth * (y * width + x);
    float* data = reinterpret_cast<float*>(pixel.data());
    if (depth > 0) data[offset + 0] = c.x;
    if (depth > 1) data[offset + 1] = c.y;
    if (depth > 2) data[offset + 2] = c.z;
    if (depth > 3) data[offset + 3] = c.w;
  }

  glm::vec4 GetPixelFloat(int x, int y) const {
    const int offset = depth * (y * width + x);
    const float* data = reinterpret_cast<const float*>(pixel.data());
    return glm::vec4(depth > 0 ? data[offset + 0] : 0.0f, depth > 1 ? data[offset + 1] : 0.0f,
                     depth > 2 ? data[offset + 2] : 0.0f, depth > 3 ? data[offset + 3] : 0.0f);
  }

  void SetPixelUnsignedByte(int x, int y, const glm::vec4& c) {
    const int offset = depth * (y * width + x);
    if (depth > 0) pixel[offset + 0] = uint8_t(c.x * 255.0f);
    if (depth > 1) pixel[offset + 1] = uint8_t(c.y * 255.0f);
    if (depth > 2) pixel[offset + 2] = uint8_t(c.z * 255.0f);
    if (depth > 3) pixel[offset + 3] = uint8_t(c.w * 255.0f);
  }

  glm::vec4 GetPixelUnsignedByte(int x, int y) const {
    const int offset = depth * (y * width + x);
    return glm::vec4(depth > 0 ? float(pixel[offset + 0]) / 255.0f : 0.0f,
                     depth > 1 ? float(pixel[offset + 1]) / 255.0f : 0.0f,
                     depth > 2 ? float(pixel[offset + 2]) / 255.0f : 0.0f,
                     depth > 3 ? float(pixel[offset + 3]) / 255.0f : 0.0f);
  }
};

}  // namespace io
}  // namespace core
