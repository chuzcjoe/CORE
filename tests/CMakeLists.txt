file(GLOB test_src "*.cpp")

add_executable(core-tests ${test_src})

target_include_directories(core-tests PUBLIC vulkan/include)

target_link_libraries(core-tests PUBLIC gtest stb vulkan mat threadpool)

if (APPLE)
    find_library(METAL_FRAMEWORK Metal)
    find_library(FOUNDATION_FRAMEWORK Foundation)
    find_library(QUARTZCORE_FRAMEWORK QuartzCore)
    target_link_libraries(core-tests PUBLIC 
            glfw 
            metal_cpp
            ${METAL_FRAMEWORK}
            ${FOUNDATION_FRAMEWORK}
            ${QUARTZCORE_FRAMEWORK})
endif()

