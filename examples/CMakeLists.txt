# Compile shaders
set(GLSLC ${Vulkan_GLSLC_EXECUTABLE})

file(GLOB_RECURSE shader_src "*/*.comp" 
                                "*/*.vert" 
                                "*/*.frag")

foreach(shader ${shader_src})
    get_filename_component(shader_name ${shader} NAME)
    set(shader_spv "${CMAKE_CURRENT_BINARY_DIR}/shaders/${shader_name}.spv")
    add_custom_command(
        OUTPUT ${shader_spv}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
        COMMAND ${GLSLC} --target-env=vulkan1.1 -mfmt=c ${shader} -o ${shader_spv}
        DEPENDS ${shader}
        COMMENT "Compiling shader ${shader_name} to SPIR-V"
        VERBATIM
    )
    list(APPEND shader_spv_files ${shader_spv})
endforeach(shader)

add_custom_target(vulkan_example_shaders DEPENDS ${shader_spv_files})

# only add glfw/draw triangle for macos
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # glfw demo
    add_executable(glfw_demo ./GLFWWindowDemo/glfw.cpp)
    target_link_libraries(glfw_demo PRIVATE glfw)

    # draw triangle demo using vulkan
    file(GLOB vk_triangle_src "DrawTriangleDemo/*.cpp")
    add_executable(vk_triangle_demo ${vk_triangle_src})
    target_include_directories(vk_triangle_demo PUBLIC ./DrawTriangleDemo
                                                PRIVATE PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/shaders)
    add_dependencies(vk_triangle_demo vulkan_example_shaders)
    target_link_libraries(vk_triangle_demo PRIVATE glfw vulkan)
endif()
