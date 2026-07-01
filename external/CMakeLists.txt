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
    add_subdirectory(metal_cpp)

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

# googletest (submodule: external/gtest) ships a CMakeLists.txt of its own,
# but add_subdirectory()'ing it also pulls in its own
# `target_compile_features(gtest PUBLIC cxx_std_17)` declaration, which
# propagates a "must be at least C++17" requirement to every consumer.
# CMake resolves that against this project's own C++20 requirement correctly
# today, but only because CMAKE_CXX_STANDARD is forced at the root
# CMakeLists.txt; to avoid relying on that resolution at all, bypass
# googletest's build system entirely and compile just the two source files
# this project needs, same as imgui/stb/tinyobjloader above. `gtest_main`
# mirrors upstream's shape (a separate target providing main(), linking
# `gtest` PUBLIC) so nothing downstream needs to change.
add_library(gtest gtest/googletest/src/gtest-all.cc)
target_include_directories(gtest
    SYSTEM PUBLIC
    gtest/googletest/include
    gtest/googletest)

add_library(gtest_main gtest/googletest/src/gtest_main.cc)
target_link_libraries(gtest_main PUBLIC gtest)

# stb (submodule: external/stb) ships no CMakeLists.txt upstream, so the
# `stb` target is defined here directly; only a handful of its headers are
# used (stb_image.h, stb_image_write.h, stb_image_resize2.h).
add_library(stb INTERFACE)
target_include_directories(stb
    SYSTEM INTERFACE
    stb)

# glm (submodule: external/glm) ships a compatible CMakeLists.txt that
# already defines a `glm` target; force header-only mode so it stays a
# lightweight INTERFACE library instead of compiling an (empty) archive.
set(GLM_BUILD_LIBRARY OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(glm)

# tinyobjloader (submodule: external/tinyobjloader) ships its own
# CMakeLists.txt, but it compiles tiny_obj_loader.cc into a separate library;
# this project instead uses the header-only mode
# (TINYOBJLOADER_IMPLEMENTATION), so the `tinyobjloader` target is defined
# here directly against the header only.
add_library(tinyobjloader INTERFACE)
target_include_directories(tinyobjloader
    SYSTEM INTERFACE
    tinyobjloader)

# OpenCL-Headers (submodule: external/OpenCL-Headers) ships a compatible
# CMakeLists.txt, but its real target is named `Headers` (aliased as
# `OpenCL::Headers`); alias it to the `opencl-headers` name used elsewhere
# in this project.
add_subdirectory(OpenCL-Headers)
add_library(opencl-headers ALIAS Headers)
# Pin the API level cl.h compiles against; without this it defaults to the
# newest version (3.1) and emits a #pragma message on every translation
# unit that includes it. The opencl/ module only calls pre-2.0 entry points
# (e.g. clCreateCommandQueue) and Apple's runtime reports device version 1.2,
# so target that explicitly.
target_compile_definitions(Headers INTERFACE CL_TARGET_OPENCL_VERSION=120)

if (ENABLE_TRACE)
  add_subdirectory(perfetto)
endif()
