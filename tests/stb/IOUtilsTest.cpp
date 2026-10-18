#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <limits>
#include <stdexcept>

#include "IOUtils.h"

namespace core {
namespace test {

TEST(IOUtilsTest, DirectCubemapMatchesTwoStepConversionForEveryComponentAndFormat) {
  for (const BitmapFormat format : {BitmapFormat::BitmapFormat_Uint8,
                                    BitmapFormat::BitmapFormat_Float}) {
    for (const int width : {4, 16, 32}) {
      for (int components = 1; components <= 4; ++components) {
        io::Bitmap input(width, width / 2, components, format);
        if (format == BitmapFormat::BitmapFormat_Uint8) {
          for (size_t index = 0; index < input.pixel.size(); ++index) {
            input.pixel[index] = static_cast<uint8_t>((index * 37) % 256);
          }
        } else {
          for (size_t index = 0; index < input.pixel.size() / sizeof(float); ++index) {
            const float value = static_cast<float>(index % 31) / 7.0f - 1.0f;
            std::memcpy(input.pixel.data() + index * sizeof(float), &value, sizeof(value));
          }
        }
        const io::Bitmap cross = io::ConvertBitmapToVerticalCross(input);
        const io::Bitmap reference = io::ConvertVerticalCrossToCubeMapFaces(cross);
        const io::Bitmap result = io::ConvertBitmapToCubeMapFaces(input);

        EXPECT_EQ(result.width, width / 4);
        EXPECT_EQ(result.height, width / 4);
        EXPECT_EQ(result.depth, components * 6);
        EXPECT_EQ(result.type, BitmapType::BitmapType_Cube);
        EXPECT_EQ(result.format, format);
        EXPECT_EQ(result.pixel, reference.pixel)
            << "width=" << width << " components=" << components;
      }
    }
  }
}

TEST(IOUtilsTest, DirectCubemapUsesExpectedAxisOrderAndPreservesHdrValues) {
  io::Bitmap input(4, 2, 1, BitmapFormat::BitmapFormat_Float);
  for (int y = 0; y < 2; ++y) {
    for (int x = 0; x < 4; ++x) {
      input.SetPixel(x, y, glm::vec4(static_cast<float>(y * 4 + x)));
    }
  }
  const io::Bitmap result = io::ConvertBitmapToCubeMapFaces(input);
  const std::array<float, 6> expected = {7.0f, 5.0f, 2.0f, 6.0f, 6.0f, 7.0f};
  for (size_t face = 0; face < expected.size(); ++face) {
    float value = 0.0f;
    std::memcpy(&value, result.pixel.data() + face * sizeof(float), sizeof(value));
    EXPECT_NEAR(value, expected[face], 0.00001f) << "face=" << face;
  }
}

TEST(IOUtilsTest, DirectCubemapRejectsInvalidMetadataAndIncompleteStorage) {
  EXPECT_THROW(io::ConvertBitmapToCubeMapFaces(io::Bitmap()), std::invalid_argument);
  io::Bitmap input(16, 8, 4, BitmapFormat::BitmapFormat_Uint8);
  input.type = BitmapType::BitmapType_Cube;
  EXPECT_THROW(io::ConvertBitmapToCubeMapFaces(input), std::invalid_argument);
  input.type = BitmapType::BitmapType_2D;
  input.width = 15;
  EXPECT_THROW(io::ConvertBitmapToCubeMapFaces(input), std::invalid_argument);
  input.width = 16;
  input.height = 7;
  EXPECT_THROW(io::ConvertBitmapToCubeMapFaces(input), std::invalid_argument);
  input.height = 8;
  input.depth = 5;
  EXPECT_THROW(io::ConvertBitmapToCubeMapFaces(input), std::invalid_argument);
  input.depth = 0;
  EXPECT_THROW(io::ConvertBitmapToCubeMapFaces(input), std::invalid_argument);
  input.depth = 4;
  input.format = static_cast<BitmapFormat>(99);
  EXPECT_THROW(io::ConvertBitmapToCubeMapFaces(input), std::invalid_argument);
  input.format = BitmapFormat::BitmapFormat_Uint8;
  input.pixel.pop_back();
  EXPECT_THROW(io::ConvertBitmapToCubeMapFaces(input), std::invalid_argument);
}

TEST(IOUtilsTest, DirectCubemapChecksAllocationLimitsBeforeAllocating) {
  io::Bitmap input;
  input.width = std::numeric_limits<int>::max() - 3;
  input.height = input.width / 2;
  input.depth = 4;
  input.format = BitmapFormat::BitmapFormat_Float;
  EXPECT_THROW(io::ConvertBitmapToCubeMapFaces(input), std::length_error);
}

}  // namespace test
}  // namespace core
