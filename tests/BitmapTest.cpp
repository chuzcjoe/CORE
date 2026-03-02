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
const std::string kDataPath = "./tests/data/studio.hdr";
const std::string kOutputPath = "./tmp/studio_cubmap.png";
#elif defined(__ANDROID__)
const std::string kDataPath = "/data/local/tmp/core/tests/data/studio.hdr";
const std::string kOutputPath = "/data/local/tmp/core/tests/data/studio_cubmap.png";
#endif

TEST(BitmapTest, test) {
  io::Bitmap bitmap(kDataPath, 3);

  if (bitmap.width() == 0 || bitmap.height() == 0 || bitmap.channels() == 0) {
    std::cerr << "Failed to load image: " << kDataPath << std::endl;
  }

  io::Bitmap cubemap = io::ConvertBitmapToVerticalCross(bitmap);
  cubemap.SaveToFile(kOutputPath);

  //   const bool write_status =
  //       stbi_write_png(kOutputPath.c_str(), width, height, channels, mat.data(), width *
  //       channels);

  //   stbi_image_free(img);

  //   EXPECT_EQ(write_status, true);
}

}  // namespace test
}  // namespace core