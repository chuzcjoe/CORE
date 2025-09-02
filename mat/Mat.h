#pragma once

#include <cassert>
#include <vector>

namespace core {

template <typename T, int C>
class Mat {
 public:
  Mat(int rows, int cols) : rows_(rows), cols_(cols), ch_(C) { data_.resize(rows * cols * C); }

  Mat(const Mat&) = default;
  Mat(Mat&&) noexcept = default;
  Mat& operator=(const Mat&) = default;
  Mat& operator=(Mat&&) noexcept = default;
  ~Mat() = default;

  T* data() { return data_.data(); }
  int rows() const { return rows_; }
  int cols() const { return cols_; }
  int total() const { return rows_ * cols_ * C; }

  [[nodiscard]] Mat clone() const {
    Mat out(rows_, cols_);

    if constexpr (std::is_trivially_copyable_v<T>) {
      std::memcpy(out.data_.data(), data_.data(), out.total() * sizeof(T));
    } else {
      std::copy(data_.begin(), data_.end(), out.data_.begin());
    }
    return out;
  }

  T* operator()(int r, int c) {
    assert(r >= 0 && r < rows_ && c >= 0 && c < cols_);
    return data_.data() + (r * cols_ + c) * C;
  }

  const T* operator()(int r, int c) const {
    assert(r >= 0 && r < rows_ && c >= 0 && c < cols_);
    return data_.data() + (r * cols_ + c) * C;
  }

  T* operator()(int row, int col, int ch) {
    assert(r >= 0 && r < rows_ && c >= 0 && c < cols_ && ch >= 0 && ch < ch_);
    return data_.data() + (row * cols_ + col) * C + ch;
  }

  const T* operator()(int row, int col, int ch) const {
    assert(r >= 0 && r < rows_ && c >= 0 && c < cols_ && ch >= 0 && ch < ch_);
    return data_.data() + (row * cols_ + col) * C + ch;
  }

  void Fill(const T value) { std::fill(data_.begin(), data_.end(), value); }

 private:
  int rows_;
  int cols_;
  int ch_;
  std::vector<T> data_;
};

}  // namespace core
