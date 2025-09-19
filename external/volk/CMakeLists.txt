set(volk_src volk.c)

add_library(volk ${volk_src})

target_include_directories(volk
    SYSTEM PUBLIC
    .)

target_link_libraries(volk
    PUBLIC
    vulkan-headers)