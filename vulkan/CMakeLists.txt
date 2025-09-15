file(GLOB vulkan_src "src/*.cpp")

add_library(vulkan ${vulkan_src})

target_include_directories(vulkan
    PUBLIC
    include)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_link_libraries(vulkan
        PUBLIC
        Vulkan::Vulkan
        mat
        timer
        glm
        glfw)
else()
    target_link_libraries(vulkan
        PUBLIC
        Vulkan::Vulkan
        mat
        timer
        glm)
endif()

add_subdirectory(tests)