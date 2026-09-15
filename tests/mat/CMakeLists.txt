add_executable(mat_tests MatTest.cpp)

target_link_libraries(mat_tests
  PRIVATE
  gtest_main
  mat)
