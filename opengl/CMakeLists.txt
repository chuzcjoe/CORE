file(GLOB opengl_src "src/*.cpp")

add_library(opengl ${opengl_src})

target_include_directories(opengl
    PUBLIC
    include)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # No glfw here: this module never includes/calls GLFW itself, so
    # linking it was pure duplication for every demo that also links
    # glfw directly for its own window creation.
    target_link_libraries(opengl
        PUBLIC
        mat
        glm
        gl_glad
        stb)
endif()