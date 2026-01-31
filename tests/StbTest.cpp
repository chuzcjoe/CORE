#include <gtest/gtest.h>
#include <stb_image.h>
#include <stb_image_write.h>

#include <string>

#include "Mat.h"

namespace core {
namespace test {

#if defined(__APPLE__)
const std::string kDataPath = "./tests/data/core.png";
const std::string kOutputPath = "./tmp/core_write.png";
#elif defined(__ANDROID__)
const std::string kDataPath = "/data/local/tmp/core/tests/data/core.png";
const std::string kOutputPath = "/data/local/tmp/core/tests/data/core_write.png";
#endif

TEST(StbTest, test) {
  int width;
  int height;
  int channels;
  unsigned char* img = stbi_load(kDataPath.c_str(), &width, &height, &channels, 3);

  if (!img) {
    std::cerr << "Failed to load image: " << kDataPath << std::endl;
  }

  std::cout << "Loaded image " << width << "x" << height << " channels=" << channels << std::endl;

  core::Mat<uint8_t, 3> mat(height, width);

  uint8_t* mat_ptr = mat.data();
  for (int i = 0; i < width * height * channels; ++i) {
    float val = static_cast<float>(img[i]);
    mat_ptr[i] = static_cast<uint8_t>(std::clamp(val * 1.2f, 0.0f, 255.0f));
  }

  const bool write_status =
      stbi_write_png(kOutputPath.c_str(), width, height, channels, mat.data(), width * channels);

  stbi_image_free(img);

  EXPECT_EQ(write_status, true);
}

}  // namespace test
}  // namespace core