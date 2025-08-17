file(GLOB test_src "*.cpp")

add_executable(core-unit-test ${test_src})

target_include_directories(core-unit-test PUBLIC vulkan/include)

target_link_libraries(core-unit-test PUBLIC gtest)

