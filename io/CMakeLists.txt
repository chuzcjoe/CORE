file(GLOB io_src "src/*.cpp")

add_library(io ${io_src})

target_include_directories(io
    PUBLIC
    include)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(io
        PUBLIC
        mat
        stb
        gl_glad)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
    target_link_libraries(io
        PUBLIC
        mat
        stb
        gles_glad)
endif()