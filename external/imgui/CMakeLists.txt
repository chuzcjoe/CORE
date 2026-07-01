file(GLOB IMGUI_SOURCES "imgui/*.cpp" "imgui/backends/imgui_impl_glfw.cpp"
                                      "imgui/backends/imgui_impl_opengl3.cpp"
                                      "imgui/backends/imgui_impl_vulkan.cpp")

add_library(imgui ${IMGUI_SOURCES})

target_include_directories(imgui
    SYSTEM PUBLIC
    imgui
    imgui/backends)

target_link_libraries(imgui
    PUBLIC
    Vulkan::Vulkan
    glfw)
