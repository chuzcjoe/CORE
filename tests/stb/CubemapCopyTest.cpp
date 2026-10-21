#include "IOUtils.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace core::test {
namespace {

// Build an atlas from the desired cube faces, rotating -Z into the bottom tile.
// Unused tiles and any extra edge pixels retain a sentinel value.
void CheckFaceExtraction(int face_width, int face_height, int channels,
                         BitmapFormat format, int extra_width = 0, int extra_height = 0) {
  io::Bitmap cross(3 * face_width + extra_width, 4 * face_height + extra_height,
                   channels, format);
  std::fill(cross.pixel.begin(), cross.pixel.end(), uint8_t{0xED});
  const int pixel_bytes = channels * io::Bitmap::GetBytesPerComponent(format);
  const std::size_t face_bytes = static_cast<std::size_t>(face_width) *
                                static_cast<std::size_t>(face_height) *
                                static_cast<std::size_t>(pixel_bytes);
  std::vector<uint8_t> expected(6U * face_bytes);

  struct Tile {
    int col;
    int row;
  };
  const Tile kTiles[] = {{.col = 2, .row = 1}, {.col = 0, .row = 1},
                         {.col = 1, .row = 0}, {.col = 1, .row = 2},
                         {.col = 1, .row = 1}, {.col = 1, .row = 3}};
  for (int face = 0; face < 6; ++face) {
    for (int row = 0; row < face_height; ++row) {
      for (int col = 0; col < face_width; ++col) {
        const int x = kTiles[face].col * face_width +
                      ((face == 5) ? face_width - col - 1 : col);
        const int y = (face == 5) ? cross.height - row - 1
                                  : kTiles[face].row * face_height + row;
        for (int byte = 0; byte < pixel_bytes; ++byte) {
          const uint8_t value = static_cast<uint8_t>(
              (face * 41 + row * 17 + col * 7 + byte * 53) % 256);
          const std::size_t src_offset =
              (static_cast<std::size_t>(y) * static_cast<std::size_t>(cross.width) +
               static_cast<std::size_t>(x)) * static_cast<std::size_t>(pixel_bytes) +
              static_cast<std::size_t>(byte);
          const std::size_t dst_offset = static_cast<std::size_t>(face) * face_bytes +
              (static_cast<std::size_t>(row) * static_cast<std::size_t>(face_width) +
               static_cast<std::size_t>(col)) * static_cast<std::size_t>(pixel_bytes) +
              static_cast<std::size_t>(byte);
          cross.pixel[src_offset] = value;
          expected[dst_offset] = value;
        }
      }
    }
  }
  const std::vector<uint8_t> original = cross.pixel;
  const io::Bitmap cube = io::ConvertVerticalCrossToCubeMapFaces(cross);
  EXPECT_EQ(cube.width, face_width);
  EXPECT_EQ(cube.height, face_height);
  EXPECT_EQ(cube.depth, 6 * channels);
  EXPECT_EQ(cube.type, BitmapType::BitmapType_Cube);
  EXPECT_EQ(cube.format, format);
  EXPECT_EQ(cube.pixel, expected);
  EXPECT_EQ(cross.pixel, original);
}

TEST(CubemapCopyTest, PreservesEveryFaceByteForAllStorageFormats) {
  for (const BitmapFormat format : {BitmapFormat::BitmapFormat_Uint8,
                                    BitmapFormat::BitmapFormat_Float}) {
    for (int channels = 1; channels <= 4; ++channels) {
      for (const int size : {1, 2, 17}) {
        SCOPED_TRACE(::testing::Message() << "size=" << size << " channels=" << channels
                                        << " format=" << static_cast<int>(format));
        CheckFaceExtraction(size, size, channels, format);
      }
    }
  }
}

TEST(CubemapCopyTest, PreservesRectangularAndTruncatedTileGeometry) {
  CheckFaceExtraction(7, 3, 3, BitmapFormat::BitmapFormat_Uint8);
  CheckFaceExtraction(3, 7, 4, BitmapFormat::BitmapFormat_Float);
  // Retain the existing integer-division geometry and bottom-anchored -Z tile.
  CheckFaceExtraction(7, 3, 3, BitmapFormat::BitmapFormat_Uint8, 2, 3);
}

TEST(CubemapCopyTest, EmptyFaceDimensionsProduceNoPixels) {
  for (const int width : {0, 2, 6}) {
    for (const int height : {0, 3, 8}) {
      if (width >= 3 && height >= 4) {
        continue;
      }
      const io::Bitmap cross(width, height, 4, BitmapFormat::BitmapFormat_Uint8);
      const io::Bitmap cube = io::ConvertVerticalCrossToCubeMapFaces(cross);
      EXPECT_EQ(cube.width, width / 3);
      EXPECT_EQ(cube.height, height / 4);
      EXPECT_TRUE(cube.pixel.empty());
    }
  }
}

}  // namespace
}  // namespace core::test
