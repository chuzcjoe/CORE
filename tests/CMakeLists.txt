file(GLOB test_src "*.cpp")

add_executable(core-tests ${test_src})

target_include_directories(core-tests PUBLIC vulkan/include)

target_link_libraries(core-tests PUBLIC gtest stb vulkan mat)

