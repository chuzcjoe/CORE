add_executable(opencl_tests CLTest.cpp)

target_link_libraries(opencl_tests
  PRIVATE
  gtest_main
  mat
  opencl)
