add_library(vulkan-headers INTERFACE)

target_include_directories(vulkan-headers
    SYSTEM INTERFACE
    include
    )