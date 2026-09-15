add_executable(threadpool_tests ThreadPoolTest.cpp)

target_link_libraries(threadpool_tests
  PRIVATE
  gtest_main
  threadpool
  timer)
