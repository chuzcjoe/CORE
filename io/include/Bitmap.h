#pragma once

#include <stb_image.h>
#include <stb_image_write.h>

#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace core {
namespace io {

class Bitmap {
 public:
  Bitmap() = default;
  Bitmap(const std::string& filename, const int channels);
  Bitmap(const int width, const int height, const int channels, const uint8_t* data);
  Bitmap(const int width, const int height, const int channels);

  ~Bitmap() = default;

  uint8_t GetPixel(const int row, const int col, const int channel) const;
  void SetPixel(const int row, const int col, const int channel, const uint8_t value);

  void SaveToFile(const std::string& filename) const;

  int width() const { return width_; }
  int height() const { return height_; }
  int channels() const { return channels_; }
  std::vector<uint8_t>& data() { return pixels_; }

 private:
  std::vector<uint8_t> pixels_;
  int width_ = 0;
  int height_ = 0;
  int channels_ = 0;
};

}  // namespace io
}  // namespace core
