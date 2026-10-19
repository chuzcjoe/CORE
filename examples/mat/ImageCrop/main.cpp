#include <charconv>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <system_error>

#include <stb_image.h>
#include <stb_image_write.h>

#include "Mat.h"

namespace {

int ParseInt(std::string_view text) {
  int value = 0;
  const std::from_chars_result result =
      std::from_chars(text.data(), text.data() + text.size(), value);
  if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) {
    throw std::invalid_argument("Crop coordinates and dimensions must be integers");
  }
  return value;
}

}  // namespace

int main(int argc, char* argv[]) {
  if (argc != 7) {
    std::cerr << "Usage: " << argv[0] << " input output.png x y width height\n";
    return EXIT_FAILURE;
  }

  try {
    const int x = ParseInt(argv[3]);
    const int y = ParseInt(argv[4]);
    const int crop_width = ParseInt(argv[5]);
    const int crop_height = ParseInt(argv[6]);
    if (crop_width <= 0 || crop_height <= 0) {
      throw std::invalid_argument("Crop dimensions must be positive");
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    const std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> pixels(
        stbi_load(argv[1], &width, &height, &channels, 4), &stbi_image_free);
    if (pixels == nullptr) {
      throw std::runtime_error("Failed to load input image");
    }

    const core::MatView<stbi_uc, 4> image(pixels.get(), height, width);
    const core::MatView<const stbi_uc, 4> crop =
        image.Subview(y, x, crop_height, crop_width);
    // The encoder reads each row at the original image stride; no packed crop
    // allocation or pixel copy is needed before encoding.
    if (crop.row_stride() > static_cast<std::size_t>(std::numeric_limits<int>::max()) ||
        stbi_write_png(argv[2], crop.cols(), crop.rows(), crop.channels(), crop.data(),
                       static_cast<int>(crop.row_stride())) == 0) {
      throw std::runtime_error("Failed to write cropped PNG");
    }
    std::cout << "Wrote " << crop.cols() << 'x' << crop.rows()
              << " crop using source row stride " << crop.row_stride() << " bytes\n";
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
