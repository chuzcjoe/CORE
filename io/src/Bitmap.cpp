#include "Bitmap.h"

namespace core {
namespace io {

Bitmap::Bitmap(const std::string& filename, const int channels) {
  if (filename.empty()) {
    throw std::invalid_argument("Bitmap filename cannot be empty");
  }
  if (channels <= 0) {
    throw std::invalid_argument("Bitmap channel count must be positive");
  }

  unsigned char* data = stbi_load(filename.c_str(), &width_, &height_, &channels_, channels);

  if (data == nullptr) {
    throw std::runtime_error("Failed to load bitmap");
  }

  printf("Loaded bitmap from %s, width=%d height=%d channels=%d\n", filename.c_str(), width_,
         height_, channels_);

  const size_t pixel_count = static_cast<size_t>(width_ * height_ * channels_);
  pixels_.resize(pixel_count);
  std::memcpy(pixels_.data(), data, pixel_count);
  stbi_image_free(data);
}

Bitmap::Bitmap(const int width, const int height, const int channels, const uint8_t* data)
    : width_(width), height_(height), channels_(channels) {
  const size_t pixel_count = static_cast<size_t>(width_ * height_ * channels_);
  if (data == nullptr) {
    throw std::invalid_argument("Bitmap source data cannot be null");
  }
  pixels_.resize(pixel_count);
  std::memcpy(pixels_.data(), data, pixel_count);
}

Bitmap::Bitmap(const int width, const int height, const int channels)
    : width_(width), height_(height), channels_(channels) {
  pixels_.resize(static_cast<size_t>(width_ * height_ * channels_), uint8_t{0});
}

uint8_t Bitmap::GetPixel(const int row, const int col, const int channel) const {
  if (row < 0 || row >= height_ || col < 0 || col >= width_ || channel < 0 ||
      channel >= channels_) {
    throw std::out_of_range(
        "Get Bitmap pixel coordinates out of range, row=" + std::to_string(row) +
        " col=" + std::to_string(col) + " channel=" + std::to_string(channel));
  }
  return pixels_[(row * width_ + col) * channels_ + channel];
}

void Bitmap::SetPixel(const int row, const int col, const int channel, const uint8_t value) {
  if (row < 0 || row >= height_ || col < 0 || col >= width_ || channel < 0 ||
      channel >= channels_) {
    throw std::out_of_range(
        "Set Bitmap pixel coordinates out of range, row=" + std::to_string(row) +
        " col=" + std::to_string(col) + " channel=" + std::to_string(channel));
  }
  pixels_[(row * width_ + col) * channels_ + channel] = value;
}

void Bitmap::SaveToFile(const std::string& filename) const {
  if (filename.empty()) {
    throw std::invalid_argument("Bitmap filename cannot be empty");
  }
  if (stbi_write_png(filename.c_str(), width_, height_, channels_, pixels_.data(),
                     width_ * channels_) == 0) {
    throw std::runtime_error("Failed to save bitmap");
  }
  printf("Saved bitmap to %s\n", filename.c_str());
}

}  // namespace io
}  // namespace core
