# only add glfw for macos
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_WIN32   OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_COCOA   ON  CACHE BOOL "" FORCE)
    set(GLFW_BUILD_X11     OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)

    # glfw ships its own CMakeLists.txt that already defines a `glfw` target,
    # so the submodule is used as-is.
    add_subdirectory(glfw)

    # metal-cpp (submodule: external/metal_cpp) is a header-only library that
    # ships no CMakeLists.txt upstream, so the `metal_cpp` INTERFACE target is
    # defined here directly. Headers are included as e.g. "Metal/Metal.hpp",
    # so the submodule root is the include directory.
    add_library(metal_cpp INTERFACE)
    target_include_directories(metal_cpp
        SYSTEM INTERFACE
        metal_cpp)
    # metal-cpp intentionally uses GNU anonymous structs/unions; with -Werror
    # enabled, Clang/AppleClang needs these specific diagnostics disabled.
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(metal_cpp
            INTERFACE
            -Wno-gnu-anonymous-struct
            -Wno-nested-anon-types)
    endif()

    # Dear ImGui (submodule: external/imgui) ships no CMakeLists.txt upstream,
    # so the `imgui` target is defined here directly against its sources.
    file(GLOB IMGUI_SOURCES "imgui/*.cpp" "imgui/backends/imgui_impl_glfw.cpp"
                                          "imgui/backends/imgui_impl_opengl3.cpp"
                                          "imgui/backends/imgui_impl_vulkan.cpp")
    add_library(imgui ${IMGUI_SOURCES})
    target_include_directories(imgui
        SYSTEM PUBLIC
        imgui
        imgui/backends)
    target_link_libraries(imgui
        PUBLIC
        Vulkan::Vulkan
        glfw)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_subdirectory(gl_glad)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
    message("Adding GLES and EGL GLAD subdirectories")
    add_subdirectory(gles_glad)
    add_subdirectory(egl_glad)
endif()

add_library(gtest gtest/googletest/src/gtest-all.cc)
target_include_directories(gtest
    SYSTEM PUBLIC
    gtest/googletest/include
    gtest/googletest)

add_library(gtest_main gtest/googletest/src/gtest_main.cc)
target_link_libraries(gtest_main PUBLIC gtest)

add_library(stb INTERFACE)
target_include_directories(stb
    SYSTEM INTERFACE
    stb)

set(GLM_BUILD_LIBRARY OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(glm)

add_library(tinyobjloader INTERFACE)
target_include_directories(tinyobjloader
    SYSTEM INTERFACE
    tinyobjloader)

add_subdirectory(OpenCL-Headers)
add_library(opencl-headers ALIAS Headers)
target_compile_definitions(Headers INTERFACE CL_TARGET_OPENCL_VERSION=120)

if (ENABLE_TRACE)
  add_subdirectory(perfetto)
  # Perfetto's generated amalgamation contains an internal name shadow that
  # must not fail this project's default -Wshadow -Werror build.
  target_compile_options(perfetto PRIVATE -Wno-error=shadow)
endif()
