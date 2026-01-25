#include <android/hardware_buffer.h>
#include <android/log.h>
#include <gtest/gtest.h>

// skip #define STB_IMAGE_IMPLEMENTATION since StbTest.cpp already includes it
#include <stb_image.h>

// skip #define STB_IMAGE_WRITE_IMPLEMENTATION since StbTest.cpp already includes it
#include <stb_image_write.h>

void write_func(void* context, void* data, int size) { fwrite(data, 1, size, (FILE*)context); }

namespace core {
namespace tests {

TEST(AHardwareBufferTest, test) {
  const int width = 256;
  const int height = 256;
  const int channels = 4;

  AHardwareBuffer* ahb = nullptr;
  AHardwareBuffer_Desc desc = {};
  desc.width = width;
  desc.height = height;
  desc.layers = 1;
  desc.format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM;
  desc.usage = AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN | AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN;

  if (AHardwareBuffer_allocate(&desc, &ahb) != 0) {
    throw std::runtime_error("Failed to allocate AHardwareBuffer");
  }

  void* data = nullptr;
  if (AHardwareBuffer_lock(ahb, AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN, -1, nullptr, &data) != 0) {
    AHardwareBuffer_release(ahb);
    throw std::runtime_error("Failed to lock AHardwareBuffer");
  }

  uint8_t* pixels = (uint8_t*)data;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      int index = (y * width + x) * channels;
      pixels[index] = 0;        // R
      pixels[index + 1] = 255;  // G
      pixels[index + 2] = 0;    // B
      pixels[index + 3] = 255;  // A
    }
  }

  AHardwareBuffer_unlock(ahb, nullptr);

  const char* filename = "/data/local/tmp/core/green_image.png";
  const bool write_status =
      stbi_write_png(filename, width, height, channels, data, width * channels);
  AHardwareBuffer_release(ahb);

  EXPECT_TRUE(write_status);
}

}  // namespace tests
}  // namespace core
