#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <memory>

#include <stb_image.h>
#include <stb_image_write.h>

#include "Mat.h"

namespace core {
namespace test {
namespace {

TEST(MatViewImageTest, EncodesCroppedPaddedRowsWithoutPacking) {
#if defined(__ANDROID__)
  constexpr char kOutputPath[] = "/data/local/tmp/core/data/mat_crop.png";
#else
  constexpr char kOutputPath[] = "./tmp/mat_crop.png";
#endif
  std::array<unsigned char, 96> pixels{};
  MatView<unsigned char, 4> image(pixels.data(), 4, 5, 24U);
  for (int row = 0; row < image.rows(); ++row) {
    for (int col = 0; col < image.cols(); ++col) {
      for (int channel = 0; channel < image.channels(); ++channel) {
        *image(row, col, channel) = static_cast<unsigned char>(50 * row + 5 * col + channel);
      }
    }
  }
  const MatView<unsigned char, 4>& source = image;
  const MatView<const unsigned char, 4> crop = source.Subview(1, 2, 2, 3);
  ASSERT_FALSE(crop.is_contiguous());
  ASSERT_NE(stbi_write_png(kOutputPath, crop.cols(), crop.rows(), crop.channels(), crop.data(),
                           static_cast<int>(crop.row_stride())), 0);

  int width = 0;
  int height = 0;
  int channels = 0;
  const std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> decoded(
      stbi_load(kOutputPath, &width, &height, &channels, 4), &stbi_image_free);
  ASSERT_NE(decoded, nullptr);
  ASSERT_EQ(width, 3);
  ASSERT_EQ(height, 2);
  ASSERT_EQ(channels, 4);
  const MatView<const stbi_uc, 4> result(decoded.get(), height, width);
  for (int row = 0; row < height; ++row) {
    for (int col = 0; col < width; ++col) {
      for (int channel = 0; channel < channels; ++channel) {
        EXPECT_EQ(*result(row, col, channel), *source(row + 1, col + 2, channel));
      }
    }
  }
}

}  // namespace
}  // namespace test
}  // namespace core
