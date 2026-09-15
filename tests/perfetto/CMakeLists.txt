add_executable(perfetto_tests PerfettoTest.cpp)

target_compile_definitions(perfetto_tests PRIVATE CORE_ENABLE_TRACE)

target_link_libraries(perfetto_tests
  PRIVATE
  gtest_main
  trace)
