include(${CMAKE_SOURCE_DIR}/cmake/vulkan_shaders.cmake)

add_vulkan_shader_target(vulkan_shaders
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}
    OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)

                                
file(GLOB test_src "ComputeSum/*.cpp"
                   "ComputeGaussianBlur/*.cpp"
                   "Roofline/*.cpp"
                    "*.cpp")

add_executable(vulkan_tests ${test_src})

target_include_directories(vulkan_tests PUBLIC ComputeSum ComputeGaussianBlur Roofline
                                    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/shaders)

add_dependencies(vulkan_tests vulkan_shaders)

# gtest_main already links gtest PUBLIC, so gtest itself doesn't need to be
# listed separately here.
target_link_libraries(vulkan_tests PUBLIC gtest_main stb vulkan mat timer)
