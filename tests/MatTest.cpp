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

TEST(MatTest3, test) {
  std::vector<float> data = {1.0f, 2.0f, 3.0f, 4.0f};
  core::MatView<float, 1> mat_view(data.data(), 2, 2);

  for (int i = 0; i < mat_view.total(); ++i) {
    mat_view.data()[i] += 1.0f;
    printf("mat_view.data()[%d] = %f\n", i, mat_view.data()[i]);
    printf("data[%d] = %f\n", i, data[i]);
  }

  for (int i = 0; i < mat_view.total(); ++i) {
    EXPECT_EQ(data[i], mat_view.data()[i]);
  }
}

}  // namespace test
}  // namespace core