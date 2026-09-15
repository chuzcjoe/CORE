add_executable(metal_tests MetalTest.cpp)

find_library(METAL_FRAMEWORK Metal)
find_library(FOUNDATION_FRAMEWORK Foundation)
find_library(QUARTZCORE_FRAMEWORK QuartzCore)

target_link_libraries(metal_tests
  PRIVATE
  gtest_main
  metal_cpp
  ${METAL_FRAMEWORK}
  ${FOUNDATION_FRAMEWORK}
  ${QUARTZCORE_FRAMEWORK})
