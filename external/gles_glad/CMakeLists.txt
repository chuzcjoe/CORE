set(GLAD_SOURCES src/glad.c)

add_library(gles_glad ${GLAD_SOURCES})

target_include_directories(gles_glad
    SYSTEM PUBLIC
    include)