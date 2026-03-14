#pragma once

#include <stb_image.h>
#include <stb_image_write.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace core {
namespace io {

namespace detail {

inline std::string BitmapFileExtension(const std::string& filename) {
  const size_t dot_pos = filename.find_last_of('.');
  if (dot_pos == std::string::npos) return "";
  std::string extension = filename.substr(dot_pos + 1);
  for (char& ch : extension) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return extension;
}

template <typename T>
inline constexpr bool kIsSupportedBitmapType =
    std::is_same_v<T, float> || std::is_same_v<T, uint8_t>;

inline int WriteSDRBitmap(const std::string& filename, const int width, const int height,
                          const int channels, const uint8_t* data) {
  const std::string extension = BitmapFileExtension(filename);
  if (extension == "bmp" || extension == "jpg" || extension == "jpeg") {
    throw std::runtime_error("Unsupported bitmap file format for SDR data: " + extension);
  }
  return stbi_write_png(filename.c_str(), width, height, channels, data, width * channels);
}

}  // namespace detail

template <typename T, int Channels>
class Bitmap {
 public:
  static_assert(Channels > 0, "Bitmap channel count must be positive");
  static_assert(detail::kIsSupportedBitmapType<T>,
                "Bitmap supports only float and uint8_t pixel data");
  static_assert(std::is_trivially_copyable_v<T>, "Bitmap pixel data must be trivially copyable");

  Bitmap() = default;

  explicit Bitmap(const std::string& filename) {
    if (filename.empty()) {
      throw std::invalid_argument("Bitmap filename cannot be empty");
    }

    T* data = nullptr;
    if constexpr (std::is_same_v<T, float>) {
      data = stbi_loadf(filename.c_str(), &width_, &height_, nullptr, Channels);
    } else {
      data = stbi_load(filename.c_str(), &width_, &height_, nullptr, Channels);
    }

    if (data == nullptr) {
      throw std::runtime_error("Failed to load bitmap");
    }

    const size_t pixel_count = PixelCount(width_, height_);
    pixels_.resize(pixel_count);
    std::memcpy(pixels_.data(), data, pixel_count * sizeof(T));
    stbi_image_free(data);
  }

  Bitmap(const int width, const int height, const T* data) : width_(width), height_(height) {
    if (data == nullptr) {
      throw std::invalid_argument("Bitmap source data cannot be null");
    }

    const size_t pixel_count = PixelCount(width_, height_);
    pixels_.resize(pixel_count);
    std::memcpy(pixels_.data(), data, pixel_count * sizeof(T));
  }

  Bitmap(const int width, const int height) : width_(width), height_(height) {
    pixels_.resize(PixelCount(width_, height_), T{0});
  }

  ~Bitmap() = default;

  T GetPixel(const int row, const int col, const int channel) const {
    return pixels_[PixelIndex(row, col, channel)];
  }

  void SetPixel(const int row, const int col, const int channel, const T value) {
    pixels_[PixelIndex(row, col, channel)] = value;
  }

  void SaveToFile(const std::string& filename) const {
    if (filename.empty()) {
      throw std::invalid_argument("Bitmap filename cannot be empty");
    }

    int write_status = 0;
    const std::string extension = detail::BitmapFileExtension(filename);
    if constexpr (std::is_same_v<T, float>) {
      if (extension == "hdr") {
        write_status = stbi_write_hdr(filename.c_str(), width_, height_, Channels, pixels_.data());
      } else {
        std::vector<uint8_t> ldr_pixels(pixels_.size());
        for (size_t index = 0; index < pixels_.size(); ++index) {
          const float clamped = std::clamp(pixels_[index], 0.0f, 1.0f);
          ldr_pixels[index] = static_cast<uint8_t>(clamped * 255.0f + 0.5f);
        }
        write_status =
            detail::WriteSDRBitmap(filename, width_, height_, Channels, ldr_pixels.data());
      }
    } else {
      if (extension == "hdr") {
        std::vector<float> hdr_pixels(pixels_.size());
        for (size_t index = 0; index < pixels_.size(); ++index) {
          hdr_pixels[index] = static_cast<float>(pixels_[index]) / 255.0f;
        }
        write_status =
            stbi_write_hdr(filename.c_str(), width_, height_, Channels, hdr_pixels.data());
      } else {
        write_status = detail::WriteSDRBitmap(filename, width_, height_, Channels, pixels_.data());
      }
    }

    if (write_status == 0) {
      throw std::runtime_error("Failed to save bitmap");
    }
  }

  int width() const { return width_; }
  int height() const { return height_; }
  int channels() const { return Channels; }
  std::vector<T>& data() { return pixels_; }
  const std::vector<T>& data() const { return pixels_; }

 private:
  static size_t PixelCount(const int width, const int height) {
    return static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(Channels);
  }

  int PixelIndex(const int row, const int col, const int channel) const {
    return (row * width_ + col) * Channels + channel;
  }

  std::vector<T> pixels_;
  int width_ = 0;
  int height_ = 0;
};

}  // namespace io
}  // namespace core
