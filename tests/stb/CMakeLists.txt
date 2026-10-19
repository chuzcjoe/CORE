add_executable(stb_tests StbTest.cpp MatViewImageTest.cpp)

target_link_libraries(stb_tests
  PRIVATE
  gtest_main
  io
  mat
  stb)
