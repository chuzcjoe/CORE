# Compile shaders
set(GLSLC ${Vulkan_GLSLC_EXECUTABLE})

file(GLOB_RECURSE shader_src "vulkan/*/*.comp" 
                                "vulkan/*/*.vert" 
                                "vulkan/*/*.frag")

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
    # 1. glfw demo
    add_executable(glfw_demo ./vulkan/GLFWWindowDemo/glfw.cpp)
    target_link_libraries(glfw_demo PRIVATE glfw)

    # 2. draw triangle demo using vulkan
    file(GLOB vk_triangle_src "./vulkan/DrawTriangleDemo/*.cpp")
    add_executable(vk_triangle_demo ${vk_triangle_src})
    target_include_directories(vk_triangle_demo PUBLIC ./vulkan/DrawTriangleDemo
                                                PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/shaders)
    add_dependencies(vk_triangle_demo vulkan_example_shaders)
    target_link_libraries(vk_triangle_demo PRIVATE glfw vulkan)

    # 3. draw texture demo using vulkan
    file(GLOB vk_texture_src "./vulkan/DrawTextureDemo/*.cpp")
    add_executable(vk_texture_demo ${vk_texture_src})
    target_include_directories(vk_texture_demo PUBLIC ./vulkan/DrawTextureDemo
                                                PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/shaders)
    add_dependencies(vk_texture_demo vulkan_example_shaders)
    target_link_libraries(vk_texture_demo PRIVATE glfw vulkan stb)

    # 4. draw triangle demo using opengl
    file(GLOB gl_triangle_src "./opengl/GLTriangleDemo/*.cpp")
    add_executable(gl_triangle_demo ${gl_triangle_src})
    target_include_directories(gl_triangle_demo PUBLIC ./opengl/GLTriangleDemo)
    target_link_libraries(gl_triangle_demo PRIVATE glfw gl_glad opengl)
    target_compile_definitions(gl_triangle_demo PRIVATE GL_SILENCE_DEPRECATION) # silence macos deprecation warning

    # 4. draw texture demo using opengl
    file(GLOB gl_texture_src "./opengl/GLTextureDemo/*.cpp")
    add_executable(gl_texture_demo ${gl_texture_src})
    target_include_directories(gl_texture_demo PUBLIC ./opengl/GLTextureDemo)
    target_link_libraries(gl_texture_demo PRIVATE glfw gl_glad opengl stb)
    target_compile_definitions(gl_texture_demo PRIVATE GL_SILENCE_DEPRECATION) # silence macos deprecation warning

    # 5. camera movement demo using opengl
    file(GLOB gl_camera_src "./opengl/GLCameraDemo/*.cpp")
    add_executable(gl_camera_demo ${gl_camera_src})
    target_include_directories(gl_camera_demo PUBLIC ./opengl/GLCameraDemo)
    target_link_libraries(gl_camera_demo PRIVATE glfw gl_glad opengl stb)
    target_compile_definitions(gl_camera_demo PRIVATE GL_SILENCE_DEPRECATION) # silence macos deprecation warning
endif()
