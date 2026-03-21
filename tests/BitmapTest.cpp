#include <gtest/gtest.h>
#include <stb_image.h>
#include <stb_image_write.h>

#include <string>

#include "Bitmap.h"
#include "IOUtils.h"
#include "Mat.h"

namespace core {
namespace test {

#if defined(__APPLE__)
const std::string kDataPath = "./tests/data/piazza_bologni_1k.hdr";
const std::string kOutputPath = "./tmp/piazza_bologni_1k_cubmap.hdr";
#elif defined(__ANDROID__)
const std::string kDataPath = "/data/local/tmp/core/tests/data/piazza_bologni_1k.hdr";
const std::string kOutputPath = "/data/local/tmp/core/tests/data/piazza_bologni_1k_cubmap.hdr";
#endif

TEST(BitmapTest, test) {
  int width, height;
  const float* img = stbi_loadf(kDataPath.c_str(), &width, &height, nullptr, 4);
  io::Bitmap in(width, height, 4, BitmapFormat::BitmapFormat_Float, img);
  io::Bitmap out = io::ConvertBitmapToVerticalCross(in);
  stbi_image_free((void*)img);

  stbi_write_hdr(kOutputPath.c_str(), out.width, out.height, out.depth,
                 (const float*)out.pixel.data());

  //   const bool write_status =
  //       stbi_write_png(kOutputPath.c_str(), width, height, channels, mat.data(), width *
  //       channels);

  //   stbi_image_free(img);

  //   EXPECT_EQ(write_status, true);
}

}  // namespace test
}  // namespace core
