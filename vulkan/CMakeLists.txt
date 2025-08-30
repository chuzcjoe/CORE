file(GLOB vulkan_src "src/*.cpp")

add_library(vulkan ${vulkan_src})

target_include_directories(vulkan
    PUBLIC
    include)

target_link_libraries(vulkan
    PUBLIC
    Vulkan::Vulkan
    mat
    timer)

add_subdirectory(tests)