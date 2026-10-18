add_executable(opencl_tests CLTest.cpp CLProgramBinaryTest.cpp)

target_link_libraries(opencl_tests
  PRIVATE
  gtest_main
  mat
  opencl)
