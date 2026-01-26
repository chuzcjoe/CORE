file(GLOB egl_src "egl/*.cpp")

add_library(egl ${egl_src})

target_include_directories(egl
    PUBLIC
    .)

target_link_libraries(egl
    PUBLIC
    egl_glad)