include(${CMAKE_SOURCE_DIR}/cmake/vulkan_shaders.cmake)

add_vulkan_shader_target(vulkan_example_shaders
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/vulkan
    OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)

# only add glfw/draw triangle for macos
if(APPLE)
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
        LIBRARIES glfw gl_glad opengl stb imgui
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

    # 15. light demo using opengl
    build_example(gl_light_demo
        SOURCE_GLOBS "./opengl/GLLightDemo/*.cpp"
        PUBLIC_INCLUDES ./opengl/GLLightDemo
        LIBRARIES glfw gl_glad opengl stb imgui
        DEFINITIONS GL_SILENCE_DEPRECATION)

    # 16. cubemap demo using opengl
    build_example(gl_cubemap_demo
        SOURCE_GLOBS "./opengl/GLCubeMapDemo/*.cpp"
        LIBRARIES glfw gl_glad opengl stb imgui
        DEFINITIONS GL_SILENCE_DEPRECATION)

    # 17. mipmap demo using vulkan
    build_example(vk_mipmap_demo
        SOURCE_GLOBS "./vulkan/MipmapDemo/*.cpp"
        PUBLIC_INCLUDES ./vulkan/MipmapDemo
        PRIVATE_INCLUDES ${CMAKE_CURRENT_BINARY_DIR}/shaders
        DEPENDS vulkan_example_shaders
        LIBRARIES glfw vulkan stb tinyobjloader imgui)

    # 18. multi-sampling demo using vulkan
    build_example(vk_multi_sampling_demo
        SOURCE_GLOBS "./vulkan/MultiSamplingDemo/*.cpp"
        PUBLIC_INCLUDES ./vulkan/MultiSamplingDemo
        PRIVATE_INCLUDES ${CMAKE_CURRENT_BINARY_DIR}/shaders
        DEPENDS vulkan_example_shaders
        LIBRARIES glfw vulkan stb tinyobjloader imgui)

    # 19. cubemap demo using vulkan
    build_example(vk_cubemap_demo
        SOURCE_GLOBS "./vulkan/CubeMapDemo/*.cpp"
        PUBLIC_INCLUDES ./vulkan/CubeMapDemo
        PRIVATE_INCLUDES ${CMAKE_CURRENT_BINARY_DIR}/shaders
        DEPENDS vulkan_example_shaders
        LIBRARIES glfw vulkan stb tinyobjloader imgui io)

endif()
