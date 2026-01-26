file(GLOB opengl_src "src/*.cpp")

add_library(opengl ${opengl_src})

target_include_directories(opengl
    PUBLIC
    include)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(opengl
        PUBLIC
        mat
        glm
        glfw
        gl_glad
        stb)
endif()