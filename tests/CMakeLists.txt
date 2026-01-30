file(GLOB test_src "*.cpp")
# Exclude MetalTest.cpp on non-Apple platforms
if (NOT APPLE)
    list(REMOVE_ITEM test_src "${CMAKE_CURRENT_SOURCE_DIR}/MetalTest.cpp")
endif()

# Exclude EGLTest.cpp and AHardwareBufferTest.cpp on non-Android platforms
if (NOT ANDROID)
    list(REMOVE_ITEM test_src "${CMAKE_CURRENT_SOURCE_DIR}/EGLTest.cpp")
    list(REMOVE_ITEM test_src "${CMAKE_CURRENT_SOURCE_DIR}/AHardwareBufferTest.cpp")
    list(REMOVE_ITEM test_src "${CMAKE_CURRENT_SOURCE_DIR}/GLESTest.cpp")
endif()

add_executable(core-tests ${test_src})

target_include_directories(core-tests PUBLIC vulkan/include)

target_link_libraries(core-tests PUBLIC gtest stb vulkan mat threadpool opencl mat)
if (ENABLE_TRACE)
    target_link_libraries(core-tests PUBLIC trace)
    target_compile_definitions(core-tests PUBLIC CORE_ENABLE_TRACE)
endif()

if (APPLE)
    find_library(METAL_FRAMEWORK Metal)
    find_library(FOUNDATION_FRAMEWORK Foundation)
    find_library(QUARTZCORE_FRAMEWORK QuartzCore)
    target_link_libraries(core-tests PUBLIC 
            glfw
            opencl
            metal_cpp
            ${METAL_FRAMEWORK}
            ${FOUNDATION_FRAMEWORK}
            ${QUARTZCORE_FRAMEWORK})
elseif(ANDROID)
    target_link_libraries(core-tests PUBLIC
            gles_glad
            io
            egl
            log)
endif()
