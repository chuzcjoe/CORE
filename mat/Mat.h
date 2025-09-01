#pragma once

#include <cassert>
#include <vector>

namespace core {

template <typename T, int C>
class Mat {
 public:
  Mat(int rows, int cols) : rows_(rows), cols_(cols), ch_(C) { data_.resize(rows * cols * C); }
  ~Mat() = default;

  Mat(const Mat& rhs) {
    rows_ = rhs.rows_;
    cols_ = rhs.cols_;
    data_ = rhs.data_;
    data_.resize(rows_ * cols_ * C);
    memcpy(data_.data(), rhs.data_.data(), sizeof(T) * rows_ * cols_ * C);
  }

  Mat(Mat&& rhs) {
    rows_ = rhs.rows_;
    cols_ = rhs.cols_;
    data_ = std::move(rhs.data_);
    rhs.rows_ = 0;
    rhs.cols_ = 0;
    rhs.data_.clear();
  }

  Mat& operator=(const Mat& rhs) {
    if (this != &rhs) {
      rows_ = rhs.rows_;
      cols_ = rhs.cols_;
      data_ = rhs.data_;
      data_.resize(rows_ * cols_ * C);
      memcpy(data_.data(), rhs.data_.data(), sizeof(T) * rows_ * cols_ * C);
    }
    return *this;
  }

  T* data() { return data_.data(); }
  int rows() const { return rows_; }
  int cols() const { return cols_; }
  int total() const { return rows_ * cols_ * C; }

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
