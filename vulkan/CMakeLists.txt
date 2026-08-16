file(GLOB vulkan_src "src/*.cpp")

add_library(vulkan ${vulkan_src})

target_include_directories(vulkan
    PUBLIC
    include)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
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

# do not build tests/examples when this is included as a submodule
if(CMAKE_SOURCE_DIR STREQUAL CORE_SOURCE_DIR)
    add_subdirectory(tests)
endif()
