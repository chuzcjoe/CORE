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
    build_example(glfw_demo
        SOURCE_GLOBS "./vulkan/GLFWWindowDemo/*.cpp"
        LIBRARIES glfw)

    # 2. draw triangle demo using vulkan
    build_example(vk_triangle_demo
        SOURCE_GLOBS "./vulkan/DrawTriangleDemo/*.cpp"
        PUBLIC_INCLUDES ./vulkan/DrawTriangleDemo
        PRIVATE_INCLUDES ${CMAKE_CURRENT_BINARY_DIR}/shaders
        DEPENDS vulkan_example_shaders
        LIBRARIES glfw vulkan)

    # 3. draw texture demo using vulkan
    build_example(vk_texture_demo
        SOURCE_GLOBS "./vulkan/DrawTextureDemo/*.cpp"
        PUBLIC_INCLUDES ./vulkan/DrawTextureDemo
        PRIVATE_INCLUDES ${CMAKE_CURRENT_BINARY_DIR}/shaders
        DEPENDS vulkan_example_shaders
        LIBRARIES glfw vulkan stb)

    # 4. draw triangle demo using opengl
    build_example(gl_triangle_demo
        SOURCE_GLOBS "./opengl/GLTriangleDemo/*.cpp"
        PUBLIC_INCLUDES ./opengl/GLTriangleDemo
        LIBRARIES glfw gl_glad opengl
        DEFINITIONS GL_SILENCE_DEPRECATION)

    # 5. draw texture demo using opengl
    build_example(gl_texture_demo
        SOURCE_GLOBS "./opengl/GLTextureDemo/*.cpp"
        PUBLIC_INCLUDES ./opengl/GLTextureDemo
        LIBRARIES glfw gl_glad opengl stb
        DEFINITIONS GL_SILENCE_DEPRECATION)

    # 6. camera movement demo using opengl
    build_example(gl_camera_demo
        SOURCE_GLOBS "./opengl/GLCameraDemo/*.cpp"
        PUBLIC_INCLUDES ./opengl/GLCameraDemo
        LIBRARIES glfw gl_glad opengl stb
        DEFINITIONS GL_SILENCE_DEPRECATION)

    # 7. draw texture demo using vulkan
    build_example(vk_depth_demo
        SOURCE_GLOBS "./vulkan/DepthTestingDemo/*.cpp"
        PUBLIC_INCLUDES ./vulkan/DepthTestingDemo
        PRIVATE_INCLUDES ${CMAKE_CURRENT_BINARY_DIR}/shaders
        DEPENDS vulkan_example_shaders
        LIBRARIES glfw vulkan stb)

    # 8. load model demo using vulkan
    build_example(vk_model_demo
        SOURCE_GLOBS "./vulkan/LoadModelDemo/*.cpp"
        PUBLIC_INCLUDES ./vulkan/LoadModelDemo
        PRIVATE_INCLUDES ${CMAKE_CURRENT_BINARY_DIR}/shaders
        DEPENDS vulkan_example_shaders
        LIBRARIES glfw vulkan stb tinyobjloader)

    # 9. Implement a rasterizer using compute shader + barycentric algo
    build_example(vk_rasterize_demo
        SOURCE_GLOBS "./vulkan/ComputeBarycentricRasterizer/*.cpp"
        PUBLIC_INCLUDES ./vulkan/ComputeBarycentricRasterizer
        PRIVATE_INCLUDES ${CMAKE_CURRENT_BINARY_DIR}/shaders
        DEPENDS vulkan_example_shaders
        LIBRARIES glfw vulkan stb)

    # 10. depth and stencil testing demo using opengl
    build_example(gl_depth_stencil_demo
        SOURCE_GLOBS "./opengl/GLDepthStencilDemo/*.cpp"
        PUBLIC_INCLUDES ./opengl/GLDepthStencilDemo
        LIBRARIES glfw gl_glad opengl stb
        DEFINITIONS GL_SILENCE_DEPRECATION)

    # 11. blend demo using opengl
    build_example(gl_blend_demo
        SOURCE_GLOBS "./opengl/GLBlendDemo/*.cpp"
        PUBLIC_INCLUDES ./opengl/GLBlendDemo
        LIBRARIES glfw gl_glad opengl stb
        DEFINITIONS GL_SILENCE_DEPRECATION)

    # 12. framebuffer demo using opengl
    build_example(gl_framebuffer_demo
        SOURCE_GLOBS "./opengl/GLFrameBufferDemo/*.cpp"
        PUBLIC_INCLUDES ./opengl/GLFrameBufferDemo
        LIBRARIES glfw gl_glad opengl stb
        DEFINITIONS GL_SILENCE_DEPRECATION)

    # 13. metal triangle demo
    build_example(metal_triangle_demo
        SOURCE_GLOBS "./metal/MetalTriangleDemo/main.cpp"
        LIBRARIES core_metal glfw)
    
    # 14. metal texture demo
    build_example(metal_texture_demo
        SOURCE_GLOBS "./metal/MetalTextureDemo/main.cpp"
        LIBRARIES core_metal glfw stb)
endif()
