#include <cstdlib>
#include <iostream>

#include <stb_image.h>
#include <stb_image_write.h>

#include "Bitmap.h"
#include "IOUtils.h"

namespace {

#if defined(__ANDROID__)
constexpr char kDataPath[] = "/data/local/tmp/core/tests/data/street.jpg";
constexpr char kOutputPath[] = "/data/local/tmp/street_vertical_cross.png";
#else
constexpr char kDataPath[] = "./tests/data/street.jpg";
constexpr char kOutputPath[] = "./tmp/street_vertical_cross.png";
#endif

}  // namespace

int main() {
  int width = 0;
  int height = 0;
  int channels = 0;
  unsigned char* image = stbi_load(kDataPath, &width, &height, &channels, 4);
  if (image == nullptr) {
    std::cerr << "Failed to load image: " << kDataPath << '\n';
    return EXIT_FAILURE;
  }

  const core::io::Bitmap input(width, height, 4, BitmapFormat::BitmapFormat_Uint8, image);
  const core::io::Bitmap output = core::io::ConvertBitmapToVerticalCross(input);
  stbi_image_free(image);

  const int write_status =
      stbi_write_png(kOutputPath, output.width, output.height, output.depth, output.pixel.data(),
                     output.width * output.depth);
  if (write_status == 0) {
    std::cerr << "Failed to write image: " << kOutputPath << '\n';
    return EXIT_FAILURE;
  }

  std::cout << "Wrote image: " << kOutputPath << '\n';
  return EXIT_SUCCESS;
}
