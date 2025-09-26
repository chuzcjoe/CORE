set(GLAD_SOURCES src/glad.c)

add_library(gl_glad ${GLAD_SOURCES})

target_include_directories(gl_glad
    SYSTEM PUBLIC
    include)