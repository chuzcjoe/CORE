#include "IOUtils.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <vector>

namespace {

using core::io::Bitmap;

// Scalar coordinate oracle, independent of the production copy granularity.
std::vector<uint8_t> ExpectedPixels(const Bitmap& cross) {
  const int width = cross.width / 3;
  const int height = cross.height / 4;
  const std::size_t pixel_bytes = static_cast<std::size_t>(cross.depth) *
      static_cast<std::size_t>(Bitmap::GetBytesPerComponent(cross.format));
  const std::size_t face_pixels = static_cast<std::size_t>(width) *
                                  static_cast<std::size_t>(height);
  std::vector<uint8_t> expected(6U * face_pixels * pixel_bytes);
  for (std::size_t pixel = 0; pixel < 6U * face_pixels; ++pixel) {
    const int face = static_cast<int>(pixel / face_pixels);
    const int row = static_cast<int>((pixel % face_pixels) / static_cast<std::size_t>(width));
    const int col = static_cast<int>(pixel % static_cast<std::size_t>(width));
    int x = col;
    int y = row;
    switch (face) {
      case 0:
        x += 2 * width;
        y += height;
        break;
      case 1:
        y += height;
        break;
      case 2:
        x += width;
        break;
      case 3:
        x += width;
        y += 2 * height;
        break;
      case 4:
        x += width;
        y += height;
        break;
      case 5:
        x = 2 * width - col - 1;
        y = cross.height - row - 1;
        break;
    }
    const std::size_t src_pixel = static_cast<std::size_t>(y) *
        static_cast<std::size_t>(cross.width) + static_cast<std::size_t>(x);
    std::memcpy(expected.data() + pixel * pixel_bytes,
                cross.pixel.data() + src_pixel * pixel_bytes, pixel_bytes);
  }
  return expected;
}

bool Measure(int size, int channels, BitmapFormat format) {
  Bitmap cross(3 * size, 4 * size, channels, format);
  uint32_t state = 0x12345678U;
  for (uint8_t& byte : cross.pixel) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    byte = static_cast<uint8_t>(state & 0xFFU);
  }
  const std::vector<uint8_t> expected = ExpectedPixels(cross);
  constexpr int kWarmups = 5;
  constexpr int kSamples = 31;
  std::vector<double> samples;
  samples.reserve(kSamples);
  for (int iteration = -kWarmups; iteration < kSamples; ++iteration) {
    const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    const Bitmap cube = core::io::ConvertVerticalCrossToCubeMapFaces(cross);
    const std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    if (cube.pixel != expected || cube.width != size || cube.height != size ||
        cube.depth != 6 * channels || cube.format != format ||
        cube.type != BitmapType::BitmapType_Cube) {
      std::cerr << "Incorrect cube output\n";
      return false;
    }
    if (iteration >= 0) {
      samples.push_back(std::chrono::duration<double, std::micro>(end - start).count());
    }
  }
  std::sort(samples.begin(), samples.end());
  std::cout << "face=" << size << " channels=" << channels << " format="
            << ((format == BitmapFormat::BitmapFormat_Uint8) ? "uint8" : "float")
            << " bytes=" << expected.size() << " warmups=" << kWarmups
            << " samples=" << samples.size() << std::fixed << std::setprecision(2)
            << " p10_us=" << samples[3] << " median_us=" << samples[15]
            << " p90_us=" << samples[27] << " all_bytes_verified=true\n";
  return true;
}

}  // namespace

int main() {
  for (const int size : {64, 256, 1024}) {
    if (!Measure(size, 4, BitmapFormat::BitmapFormat_Uint8)) {
      return EXIT_FAILURE;
    }
  }
  if (!Measure(256, 3, BitmapFormat::BitmapFormat_Uint8) ||
      !Measure(256, 4, BitmapFormat::BitmapFormat_Float) ||
      !Measure(1024, 4, BitmapFormat::BitmapFormat_Float)) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
