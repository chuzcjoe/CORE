include(${CMAKE_SOURCE_DIR}/cmake/vulkan_shaders.cmake)

add_vulkan_shader_target(vulkan_shaders
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}
  OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)

file(GLOB test_src
  "ComputeSum/*.cpp"
  "ComputeGaussianBlur/*.cpp"
  "Roofline/*.cpp"
  "*.cpp")

add_executable(vulkan_tests ${test_src})

target_include_directories(vulkan_tests
  PRIVATE
  ComputeSum
  ComputeGaussianBlur
  Roofline
  ${CMAKE_CURRENT_BINARY_DIR}/shaders)

add_dependencies(vulkan_tests vulkan_shaders)

target_link_libraries(vulkan_tests
  PRIVATE
  gtest_main
  mat
  stb
  timer
  vulkan)
