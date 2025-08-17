file(GLOB test_src "*.cpp")

add_executable(unit-test ${test_src})

target_include_directories(unit-test PUBLIC vulkan/include)

target_link_libraries(unit-test PUBLIC gtest)

