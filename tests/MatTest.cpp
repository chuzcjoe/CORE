#include <gtest/gtest.h>

#include "Mat.h"

namespace core {
namespace test {

TEST(MatTest, test) {
  core::Mat<float, 3> mat(2, 2);
  mat.Fill(1.0f);
  *mat(0, 1) = 2.0f;

  core::Mat<float, 3> mat_copy = mat;

  EXPECT_EQ(mat.rows(), 2);
  EXPECT_EQ(mat.cols(), 2);
  EXPECT_EQ(*mat(0, 0), 1.0f);
  EXPECT_EQ(*mat(0, 1), 2.0f);

  EXPECT_EQ(mat_copy.rows(), 2);
  EXPECT_EQ(mat_copy.cols(), 2);
  EXPECT_EQ(*mat_copy(0, 0), 1.0f);
  EXPECT_EQ(*mat_copy(0, 1), 2.0f);
}

}  // namespace test
}  // namespace core