add_executable(bitmap_tests BitmapTest.cpp)

target_link_libraries(bitmap_tests
  PRIVATE
  gtest_main
  io
  stb)
