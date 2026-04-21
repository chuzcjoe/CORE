#include <gtest/gtest.h>
#include <stb_image.h>
#include <stb_image_write.h>

#include <filesystem>
#include <string>

#include "Bitmap.h"
#include "IOUtils.h"
#include "Mat.h"

namespace core {
namespace test {

// More images can be downloaded from
// https://commons.wikimedia.org/wiki/Category:360%C2%B0_panoramas_with_equirectangular_projection

#if defined(__APPLE__)
const std::string kDataPath =
    (std::filesystem::path(__FILE__).parent_path() / "data" / "street.jpg").string();
const std::string kOutputPath = "./tmp/street_vertical_cross.png";
#elif defined(__ANDROID__)
const std::string kDataPath = "/data/local/tmp/core/tests/data/street.jpg";
const std::string kOutputPath = "/data/local/tmp/street_vertical_cross.png";
#endif

TEST(BitmapTest, VerticalCross) {
  int width = 0;
  int height = 0;
  int channels = 0;
  unsigned char* img = stbi_load(kDataPath.c_str(), &width, &height, &channels, 4);
  ASSERT_NE(img, nullptr) << "Failed to load image: " << kDataPath;

  io::Bitmap in(width, height, 4, BitmapFormat::BitmapFormat_Uint8, img);
  io::Bitmap out = io::ConvertBitmapToVerticalCross(in);
  stbi_image_free(img);

  const bool write_status = stbi_write_png(kOutputPath.c_str(), out.width, out.height, out.depth,
                                           out.pixel.data(), out.width * out.depth);
  EXPECT_TRUE(write_status);
}

}  // namespace test
}  // namespace core
