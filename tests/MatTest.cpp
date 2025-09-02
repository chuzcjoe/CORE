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

TEST(MatTest2, test) {
  core::Mat<float, 3> mat(2, 2);
  mat.Fill(1.0f);
  *mat(0, 1) = 2.0f;

  core::Mat<float, 3> mat_copy = mat.clone();

  auto* ptr1 = mat.data();
  auto* ptr2 = mat_copy.data();
  for (int i = 0; i < mat.total(); ++i) {
    EXPECT_EQ(ptr1[i], ptr2[i]);
  }
}

}  // namespace test
}  // namespace core