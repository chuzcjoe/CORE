find_library(EGL_LIBRARY EGL)
message("EGL path: ${EGL_LIBRARY}")

set(egl_glad_src src/glad_egl.c)

add_library(egl_glad ${egl_glad_src})

target_include_directories(egl_glad
    SYSTEM PUBLIC
    include)

target_link_libraries(egl_glad
    PUBLIC
    ${EGL_LIBRARY}
    android)