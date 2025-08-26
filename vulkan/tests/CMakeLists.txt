file(GLOB test_src "src/*.cpp" "*.cpp")
add_executable(vulkan_tests ${test_src})
target_include_directories(vulkan_tests PUBLIC include)
target_link_libraries(vulkan_tests PUBLIC gtest vulkan mat)