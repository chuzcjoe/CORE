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

add_custom_target(vulkan_shaders DEPENDS ${shader_spv_files})

                                
file(GLOB test_src "ComputeSum/*.cpp" 
                    "*.cpp")

add_executable(vulkan_tests ${test_src})

target_include_directories(vulkan_tests PUBLIC ComputeSum
                                    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/shaders)

add_dependencies(vulkan_tests vulkan_shaders)

target_link_libraries(vulkan_tests PUBLIC gtest vulkan mat timer)