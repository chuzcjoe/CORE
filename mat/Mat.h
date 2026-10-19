#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <limits>
#include <random>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace core {

template <typename T, int C>
class MatView {
 public:
  static_assert(C > 0, "MatView requires at least one channel");

  MatView() = default;
  MatView(T* data, int rows, int cols)
      : MatView(data, rows, cols, PackedRowSize(cols)) {}

  // Stride is in T elements, including any padding between rows. The caller
  // owns the storage and must keep the entire addressed range alive.
  MatView(T* data, int rows, int cols, std::size_t row_stride)
      : data_(data), rows_(rows), cols_(cols), row_stride_(row_stride) {
    const std::size_t row_size = PackedRowSize(cols);
    if (rows < 0 || row_stride < row_size) {
      throw std::invalid_argument("MatView: invalid dimensions or row stride");
    }
    if (rows > 0 && row_size > static_cast<std::size_t>(std::numeric_limits<int>::max()) /
                                   static_cast<std::size_t>(rows)) {
      throw std::length_error("MatView: element count exceeds int range");
    }
    const std::size_t max_elements =
        static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max()) / sizeof(T);
    if (row_size > max_elements ||
        (rows > 1 && row_stride > (max_elements - row_size) /
                                     static_cast<std::size_t>(rows - 1))) {
      throw std::length_error("MatView: addressed storage is too large");
    }
  }

  T* data() { return data_; }
  const T* data() const { return data_; }
  int rows() const { return rows_; }
  int cols() const { return cols_; }
  int channels() const { return C; }
  // Logical elements only; padding is excluded. data()[0..total()) is a packed
  // matrix only when is_contiguous() is true.
  int total() const { return rows_ * cols_ * C; }
  std::size_t row_stride() const { return row_stride_; }
  bool is_contiguous() const {
    return rows_ <= 1 || cols_ == 0 || row_stride_ == static_cast<std::size_t>(cols_ * C);
  }

  [[nodiscard]] MatView<T, C> Subview(int row, int col, int rows, int cols) {
    ValidateSubview(row, col, rows, cols);
    T* start = (rows == 0 || cols == 0) ? nullptr : (*this)(row, col);
    return MatView<T, C>(start, rows, cols, row_stride_);
  }

  [[nodiscard]] MatView<const T, C> Subview(int row, int col, int rows, int cols) const {
    ValidateSubview(row, col, rows, cols);
    const T* start = (rows == 0 || cols == 0) ? nullptr : (*this)(row, col);
    return MatView<const T, C>(start, rows, cols, row_stride_);
  }

  T* operator()(int r, int c) {
    assert(r >= 0 && r < rows_ && c >= 0 && c < cols_);
    return data_ + static_cast<std::size_t>(r) * row_stride_ +
           static_cast<std::size_t>(c * C);
  }

  const T* operator()(int r, int c) const {
    assert(r >= 0 && r < rows_ && c >= 0 && c < cols_);
    return data_ + static_cast<std::size_t>(r) * row_stride_ +
           static_cast<std::size_t>(c * C);
  }

  T* operator()(int row, int col, int ch) {
    assert(row >= 0 && row < rows_ && col >= 0 && col < cols_ && ch >= 0 && ch < C);
    return (*this)(row, col) + ch;
  }

  const T* operator()(int row, int col, int ch) const {
    assert(row >= 0 && row < rows_ && col >= 0 && col < cols_ && ch >= 0 && ch < C);
    return (*this)(row, col) + ch;
  }

 private:
  static std::size_t PackedRowSize(int cols) {
    if (cols < 0 || cols > std::numeric_limits<int>::max() / C) {
      throw std::invalid_argument("MatView: invalid column count");
    }
    return static_cast<std::size_t>(cols * C);
  }

  void ValidateSubview(int row, int col, int rows, int cols) const {
    if (row < 0 || col < 0 || rows < 0 || cols < 0 || row > rows_ || col > cols_ ||
        rows > rows_ - row || cols > cols_ - col) {
      throw std::out_of_range("MatView: subview is out of bounds");
    }
    if (rows > 0 && cols > 0 && data_ == nullptr) {
      throw std::invalid_argument("MatView: subview requires backing storage");
    }
  }

 protected:
  T* data_ = nullptr;
  int rows_ = 0;
  int cols_ = 0;
  std::size_t row_stride_ = 0;
};

template <typename T, int C>
class Mat : public MatView<T, C> {
 public:
  Mat(int rows, int cols)
      : MatView<T, C>(nullptr, rows, cols),
        storage_(static_cast<std::size_t>(this->total())) {
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
    other.row_stride_ = 0;
    other.data_ = nullptr;
  }

  Mat& operator=(const Mat& other) {
    if (this == &other) return *this;
    storage_ = other.storage_;
    this->rows_ = other.rows();
    this->cols_ = other.cols();
    this->row_stride_ = other.row_stride();
    this->data_ = storage_.data();
    return *this;
  }

  Mat& operator=(Mat&& other) noexcept {
    if (this == &other) return *this;
    storage_ = std::move(other.storage_);
    this->rows_ = other.rows();
    this->cols_ = other.cols();
    this->row_stride_ = other.row_stride();
    this->data_ = storage_.data();
    other.rows_ = 0;
    other.cols_ = 0;
    other.row_stride_ = 0;
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

  void Random() {
    std::random_device rd;
    std::mt19937 gen(rd());
    if constexpr (std::is_floating_point_v<T>) {
      std::uniform_real_distribution<T> dist(static_cast<T>(0), static_cast<T>(1));
      for (auto& v : storage_) v = dist(gen);
    } else if constexpr (std::is_integral_v<T>) {
      using U = std::conditional_t<std::is_signed_v<T>, long long, unsigned long long>;
      std::uniform_int_distribution<U> dist(static_cast<U>(0),
                                            static_cast<U>(std::numeric_limits<T>::max()));
      for (auto& v : storage_) v = static_cast<T>(dist(gen));
    } else {
      // For non-arithmetic types, default-initialize
      for (auto& v : storage_) v = T{};
    }
  }

 private:
  std::vector<T> storage_;
};

}  // namespace core
