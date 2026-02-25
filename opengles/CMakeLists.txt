file(GLOB opengles_src "src/*.cpp")

add_library(opengles ${opengles_src})

target_include_directories(opengles
    PUBLIC
    include)

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    target_link_libraries(opengles
        PUBLIC
        mat
        glm
        gles_glad
        stb)
endif()