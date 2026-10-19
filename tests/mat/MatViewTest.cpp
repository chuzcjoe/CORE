#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "Mat.h"

namespace core {
namespace test {
namespace {

TEST(MatViewTest, AddressesPaddedMultichannelRows) {
  std::array<int, 24> pixels{};
  pixels.fill(-1);
  MatView<int, 3> image(pixels.data(), 3, 2, 8U);
  EXPECT_EQ(image.total(), 18);
  EXPECT_EQ(image.row_stride(), 8U);
  EXPECT_FALSE(image.is_contiguous());

  for (int row = 0; row < image.rows(); ++row) {
    for (int col = 0; col < image.cols(); ++col) {
      for (int channel = 0; channel < image.channels(); ++channel) {
        *image(row, col, channel) = 100 * row + 10 * col + channel;
      }
    }
  }
  const MatView<int, 3>& read_only = image;
  for (int row = 0; row < image.rows(); ++row) {
    for (int col = 0; col < image.cols(); ++col) {
      const std::size_t offset = static_cast<std::size_t>(row) * 8U +
                                 static_cast<std::size_t>(col) * 3U;
      EXPECT_EQ(image(row, col), pixels.data() + offset);
      for (int channel = 0; channel < image.channels(); ++channel) {
        const int expected = 100 * row + 10 * col + channel;
        EXPECT_EQ(pixels[offset + static_cast<std::size_t>(channel)], expected);
        EXPECT_EQ(*read_only(row, col, channel), expected);
      }
    }
    EXPECT_EQ(pixels[static_cast<std::size_t>(row) * 8U + 6U], -1);
    EXPECT_EQ(pixels[static_cast<std::size_t>(row) * 8U + 7U], -1);
  }
}

TEST(MatViewTest, NestedSubviewsAliasOnlyTheirRegion) {
  Mat<int, 2> image(4, 5);
  image.Fill(-1);
  MatView<int, 2> crop = image.Subview(1, 1, 3, 3);
  MatView<int, 2> nested = crop.Subview(1, 1, 2, 2);
  EXPECT_EQ(crop.data(), image(1, 1));
  EXPECT_EQ(nested.data(), image(2, 2));
  EXPECT_EQ(nested.row_stride(), image.row_stride());
  EXPECT_EQ(nested.total(), 8);
  EXPECT_FALSE(nested.is_contiguous());
  for (int row = 0; row < nested.rows(); ++row) {
    for (int col = 0; col < nested.cols(); ++col) {
      *nested(row, col, 1) = 42;
    }
  }
  for (int row = 0; row < image.rows(); ++row) {
    for (int col = 0; col < image.cols(); ++col) {
      EXPECT_EQ(*image(row, col, 0), -1);
      EXPECT_EQ(*image(row, col, 1),
                (row >= 2 && col >= 2 && col < 4) ? 42 : -1);
    }
  }

  const Mat<int, 2>& read_only = image;
  MatView<const int, 2> const_crop = read_only.Subview(2, 2, 2, 2);
  static_assert(std::is_same_v<decltype(*const_crop(0, 0)), const int&>);
  static_assert(std::is_same_v<decltype(read_only.Subview(0, 0, 1, 1)),
                               MatView<const int, 2>>);
  EXPECT_EQ(*const_crop.Subview(1, 1, 1, 1)(0, 0, 1), 42);
}

TEST(MatViewTest, ReportsLogicalContiguityAndEmptyRegions) {
  Mat<int, 1> image(3, 4);
  EXPECT_TRUE(image.is_contiguous());
  EXPECT_TRUE(image.Subview(1, 0, 2, 4).is_contiguous());
  EXPECT_TRUE(image.Subview(1, 1, 1, 2).is_contiguous());
  EXPECT_FALSE(image.Subview(0, 1, 2, 2).is_contiguous());
  const MatView<int, 1> empty = image.Subview(3, 4, 0, 0);
  EXPECT_EQ(empty.total(), 0);
  EXPECT_EQ(empty.data(), nullptr);
  EXPECT_TRUE(empty.is_contiguous());
  EXPECT_EQ(empty.row_stride(), image.row_stride());
  EXPECT_EQ(image.Subview(3, 0, 0, 4).data(), nullptr);
  EXPECT_EQ(image.Subview(0, 4, 3, 0).data(), nullptr);
  MatView<int, 1> default_view;
  EXPECT_EQ(default_view.row_stride(), 0U);
  EXPECT_TRUE(default_view.is_contiguous());
  EXPECT_EQ(default_view.Subview(0, 0, 0, 0).data(), nullptr);
}

TEST(MatViewTest, RejectsInvalidLayoutsAndRegions) {
  std::array<int, 16> pixels{};
  EXPECT_THROW((MatView<int, 2>(pixels.data(), 2, 3, 5U)), std::invalid_argument);
  EXPECT_THROW((MatView<int, 1>(pixels.data(), -1, 2)), std::invalid_argument);
  EXPECT_THROW((MatView<int, 1>(pixels.data(), 2, -1)), std::invalid_argument);
  EXPECT_THROW((MatView<int, 2>(pixels.data(), 1, std::numeric_limits<int>::max())),
               std::invalid_argument);
  EXPECT_THROW((MatView<int, 1>(pixels.data(), 2, std::numeric_limits<int>::max())),
               std::length_error);
  EXPECT_THROW((MatView<int, 1>(pixels.data(), 2, 1,
                               std::numeric_limits<std::size_t>::max())),
               std::length_error);
  MatView<int, 1> image(pixels.data(), 4, 4);
  EXPECT_THROW(static_cast<void>(image.Subview(-1, 0, 1, 1)), std::out_of_range);
  EXPECT_THROW(static_cast<void>(image.Subview(0, -1, 1, 1)), std::out_of_range);
  EXPECT_THROW(static_cast<void>(image.Subview(0, 0, -1, 1)), std::out_of_range);
  EXPECT_THROW(static_cast<void>(image.Subview(0, 0, 1, -1)), std::out_of_range);
  EXPECT_THROW(static_cast<void>(image.Subview(3, 3, 2, 1)), std::out_of_range);
  EXPECT_THROW(static_cast<void>(image.Subview(3, 3, 1, 2)), std::out_of_range);
  EXPECT_THROW(static_cast<void>(image.Subview(5, 0, 0, 0)), std::out_of_range);
  EXPECT_THROW(static_cast<void>(image.Subview(0, 5, 0, 0)), std::out_of_range);
  EXPECT_THROW(static_cast<void>(image.Subview(1, 1, std::numeric_limits<int>::max(), 1)),
               std::out_of_range);
  MatView<int, 1> unbacked(nullptr, 2, 2);
  EXPECT_THROW(static_cast<void>(unbacked.Subview(0, 0, 1, 1)), std::invalid_argument);
}

TEST(MatViewTest, OwningMatricesStayPackedAcrossCopyMoveAndClone) {
  Mat<int, 2> original(3, 4);
  original.Fill(7);
  Mat<int, 2> copied(original);
  Mat<int, 2> moved(std::move(copied));
  Mat<int, 2> assigned(1, 1);
  assigned = original;
  Mat<int, 2> move_assigned(1, 1);
  move_assigned = std::move(assigned);
  Mat<int, 2> cloned = original.clone();
  for (const Mat<int, 2>* matrix : {&moved, &move_assigned, &cloned}) {
    EXPECT_TRUE(matrix->is_contiguous());
    EXPECT_EQ(matrix->row_stride(), 8U);
    EXPECT_NE(matrix->data(), original.data());
    EXPECT_EQ(*matrix->Subview(2, 3, 1, 1)(0, 0, 1), 7);
  }
  EXPECT_EQ(copied.row_stride(), 0U);
  EXPECT_EQ(assigned.row_stride(), 0U);
  EXPECT_EQ(copied.total(), 0);
  EXPECT_EQ(assigned.total(), 0);
}

}  // namespace
}  // namespace test
}  // namespace core
