add_executable(stb_tests StbTest.cpp IOUtilsTest.cpp)

target_link_libraries(stb_tests
  PRIVATE
  gtest_main
  io
  mat
  stb)

add_executable(io_cubemap_benchmark benchmarks/CubemapBenchmark.cpp)
target_link_libraries(io_cubemap_benchmark PRIVATE io)
