file(GLOB IMGUI_SOURCES "*.cpp" "backends/imgui_impl_glfw.cpp" 
                                "backends/imgui_impl_opengl3.cpp" 
                                "backends/imgui_impl_vulkan.cpp")

add_library(imgui ${IMGUI_SOURCES})

target_include_directories(imgui
    SYSTEM PUBLIC
    .
    ./backends)

target_link_libraries(imgui
    PUBLIC
    Vulkan::Vulkan
    glfw)