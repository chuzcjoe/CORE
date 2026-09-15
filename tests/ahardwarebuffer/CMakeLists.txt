add_executable(ahardwarebuffer_tests AHardwareBufferTest.cpp)

target_link_libraries(ahardwarebuffer_tests
  PRIVATE
  gtest_main
  io
  stb
  android
  log)
