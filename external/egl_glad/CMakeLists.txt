set(GLAD_SOURCES src/glad_egl.c)

add_library(egl_glad ${GLAD_SOURCES})

target_include_directories(egl_glad
    SYSTEM PUBLIC
    include)