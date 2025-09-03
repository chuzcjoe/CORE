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

    add_subdirectory(glfw)
endif()

add_subdirectory(gtest)
add_subdirectory(stb)