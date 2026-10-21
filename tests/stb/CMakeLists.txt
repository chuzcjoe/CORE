add_executable(stb_tests StbTest.cpp CubemapCopyTest.cpp)

target_link_libraries(stb_tests
  PRIVATE
  gtest_main
  io
  mat
  stb)

add_executable(io_cubemap_copy_benchmark benchmarks/CubemapCopyBenchmark.cpp)
target_link_libraries(io_cubemap_copy_benchmark PRIVATE io)
