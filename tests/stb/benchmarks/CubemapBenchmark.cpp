#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <vector>

#include "IOUtils.h"

namespace {

void Benchmark(int width, BitmapFormat format) {
  core::io::Bitmap input(width, width / 2, 4, format);
  for (size_t index = 0; index < input.pixel.size(); ++index) {
    input.pixel[index] = static_cast<uint8_t>((index * 37) % 256);
  }
  if (format == BitmapFormat::BitmapFormat_Float) {
    for (size_t index = 0; index < input.pixel.size() / sizeof(float); ++index) {
      const float value = static_cast<float>(index % 31) / 7.0f - 1.0f;
      std::memcpy(input.pixel.data() + index * sizeof(float), &value, sizeof(value));
    }
  }
  core::io::Bitmap reference;
  size_t intermediate_bytes = 0;
  {
    const core::io::Bitmap cross = core::io::ConvertBitmapToVerticalCross(input);
    reference = core::io::ConvertVerticalCrossToCubeMapFaces(cross);
    intermediate_bytes = cross.pixel.size();
  }
  const auto run = [&](bool direct) {
    const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    core::io::Bitmap result;
    if (direct) {
      result = core::io::ConvertBitmapToCubeMapFaces(input);
    } else {
      const core::io::Bitmap intermediate = core::io::ConvertBitmapToVerticalCross(input);
      result = core::io::ConvertVerticalCrossToCubeMapFaces(intermediate);
    }
    const double elapsed =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    if (result.pixel != reference.pixel) {
      throw std::runtime_error("Cubemap benchmark pixel mismatch");
    }
    return elapsed;
  };
  constexpr int kWarmups = 2;
  constexpr int kSamples = 9;
  std::vector<double> legacy;
  std::vector<double> direct;
  for (int sample = -kWarmups; sample < kSamples; ++sample) {
    for (int turn = 0; turn < 2; ++turn) {
      const bool use_direct = ((sample + kWarmups + turn) % 2) == 0;
      const double elapsed = run(use_direct);
      if (sample >= 0) {
        (use_direct ? direct : legacy).push_back(elapsed);
      }
    }
  }
  std::sort(legacy.begin(), legacy.end());
  std::sort(direct.begin(), direct.end());
  std::printf("width=%d format=%s warmups=%d samples=%d "
              "legacy_ms[min,median,max]=[%.2f,%.2f,%.2f] "
              "direct_ms[min,median,max]=[%.2f,%.2f,%.2f] "
              "eliminated_intermediate_bytes=%zu output_bytes=%zu verified=yes\n",
              width, format == BitmapFormat::BitmapFormat_Uint8 ? "uint8" : "float",
              kWarmups, kSamples, legacy.front(), legacy[4], legacy.back(), direct.front(),
              direct[4], direct.back(), intermediate_bytes, reference.pixel.size());
}

}  // namespace

int main() {
  try {
    Benchmark(512, BitmapFormat::BitmapFormat_Uint8);
    Benchmark(1024, BitmapFormat::BitmapFormat_Uint8);
    Benchmark(1024, BitmapFormat::BitmapFormat_Float);
  } catch (const std::exception& error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
  }
  return 0;
}
