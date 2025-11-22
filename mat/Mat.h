#pragma once

#include <algorithm>
#include <cassert>
#include <cstring>
#include <type_traits>
#include <vector>

namespace core {

template <typename T, int C>
class MatView {
 public:
  MatView() = default;
  MatView(T* data, int rows, int cols) : data_(data), rows_(rows), cols_(cols) {}

  T* data() { return data_; }
  const T* data() const { return data_; }
  int rows() const { return rows_; }
  int cols() const { return cols_; }
  int channels() const { return C; }
  int total() const { return rows_ * cols_ * C; }

  T* operator()(int r, int c) {
    assert(r >= 0 && r < rows_ && c >= 0 && c < cols_);
    return data_ + (r * cols_ + c) * C;
  }

  const T* operator()(int r, int c) const {
    assert(r >= 0 && r < rows_ && c >= 0 && c < cols_);
    return data_ + (r * cols_ + c) * C;
  }

  T* operator()(int row, int col, int ch) {
    assert(row >= 0 && row < rows_ && col >= 0 && col < cols_ && ch >= 0 && ch < C);
    return data_ + (row * cols_ + col) * C + ch;
  }

  const T* operator()(int row, int col, int ch) const {
    assert(row >= 0 && row < rows_ && col >= 0 && col < cols_ && ch >= 0 && ch < C);
    return data_ + (row * cols_ + col) * C + ch;
  }

 protected:
  T* data_ = nullptr;
  int rows_ = 0;
  int cols_ = 0;
};

template <typename T, int C>
class Mat : public MatView<T, C> {
 public:
  Mat(int rows, int cols) : MatView<T, C>(nullptr, rows, cols), storage_(rows * cols * C) {
    this->data_ = storage_.data();
  }

  Mat(const Mat& other)
      : MatView<T, C>(nullptr, other.rows(), other.cols_), storage_(other.storage_) {
    this->rows_ = other.rows();
    this->cols_ = other.cols();
    this->data_ = storage_.data();
  }

  Mat(Mat&& other) noexcept
      : MatView<T, C>(nullptr, other.rows(), other.cols_), storage_(std::move(other.storage_)) {
    this->rows_ = other.rows();
    this->cols_ = other.cols();
    this->data_ = storage_.data();
    other.rows_ = 0;
    other.cols_ = 0;
    other.data_ = nullptr;
  }

  Mat& operator=(const Mat& other) {
    if (this == &other) return *this;
    storage_ = other.storage_;
    this->rows_ = other.rows();
    this->cols_ = other.cols();
    this->data_ = storage_.data();
    return *this;
  }

  Mat& operator=(Mat&& other) noexcept {
    if (this == &other) return *this;
    storage_ = std::move(other.storage_);
    this->rows_ = other.rows();
    this->cols_ = other.cols();
    this->data_ = storage_.data();
    other.rows_ = 0;
    other.cols_ = 0;
    other.data_ = nullptr;
    return *this;
  }

  ~Mat() = default;

  [[nodiscard]] Mat clone() const {
    Mat out(this->rows_, this->cols_);

    if constexpr (std::is_trivially_copyable_v<T>) {
      std::memcpy(out.storage_.data(), storage_.data(), out.total() * sizeof(T));
    } else {
      std::copy(storage_.begin(), storage_.end(), out.storage_.begin());
    }
    return out;
  }

  void Fill(const T value) { std::fill(storage_.begin(), storage_.end(), value); }

 private:
  std::vector<T> storage_;
};

}  // namespace core
