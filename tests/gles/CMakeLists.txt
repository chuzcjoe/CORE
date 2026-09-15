add_executable(gles_tests
  EGLTest.cpp
  GLESTest.cpp)

target_link_libraries(gles_tests
  PRIVATE
  gtest_main
  gles_glad
  egl
  io
  log)
