function(add_vulkan_shader_target target_name)
    set(options)
    set(oneValueArgs SOURCE_DIR OUTPUT_DIR)
    set(multiValueArgs)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ARG_SOURCE_DIR)
        set(ARG_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR})
    endif()

    if(NOT ARG_OUTPUT_DIR)
        set(ARG_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)
    endif()

    # Find glsls shaders
    file(GLOB_RECURSE glsl_shader_src
        "${ARG_SOURCE_DIR}/*.comp"
        "${ARG_SOURCE_DIR}/*.vert"
        "${ARG_SOURCE_DIR}/*.frag")

    # Find Slang shaders
    file(GLOB_RECURSE slang_shader_src "${ARG_SOURCE_DIR}/*.slang")

    # Check for required shader compilers
    if(glsl_shader_src AND NOT GLSLC_EXECUTABLE)
        find_program(GLSLC_EXECUTABLE NAMES glslc REQUIRED)
        message(STATUS "Found glslc executable: ${GLSLC_EXECUTABLE}")
    endif()

    if(slang_shader_src AND NOT SLANGC_EXECUTABLE)
        find_program(SLANGC_EXECUTABLE NAMES slangc REQUIRED)
        message(STATUS "Found Slang compiler: ${SLANGC_EXECUTABLE}")
    endif()

    if(slang_shader_src AND NOT XXD_EXECUTABLE)
        find_program(XXD_EXECUTABLE NAMES xxd REQUIRED)
        message(STATUS "Found xxd executable: ${XXD_EXECUTABLE}")
    endif()

    set(shader_spv_files)

    foreach(glsl_shader ${glsl_shader_src})
        get_filename_component(shader_name ${glsl_shader} NAME)
        set(shader_spv "${ARG_OUTPUT_DIR}/${shader_name}.spv")
        add_custom_command(
            OUTPUT ${shader_spv}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${ARG_OUTPUT_DIR}
            COMMAND ${GLSLC_EXECUTABLE} --target-env=vulkan1.1 -mfmt=c ${glsl_shader} -o ${shader_spv}
            DEPENDS ${glsl_shader}
            COMMENT "Compiling shader ${shader_name} to SPIR-V"
            VERBATIM
        )
        list(APPEND shader_spv_files ${shader_spv})
    endforeach()

    foreach(slang_shader ${slang_shader_src})
        get_filename_component(shader_name ${slang_shader} NAME)
        set(shader_spv "${ARG_OUTPUT_DIR}/${shader_name}.spv")
        set(shader_header "${shader_spv}.h")
        string(MAKE_C_IDENTIFIER "${shader_name}_spv" shader_spv_identifier)
        add_custom_command(
            OUTPUT ${shader_spv} ${shader_header}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${ARG_OUTPUT_DIR}
            COMMAND ${SLANGC_EXECUTABLE} ${slang_shader} -target spirv -profile spirv_1_4 -entry main -stage compute -emit-spirv-directly -fvk-use-entrypoint-name -o ${shader_spv}
            COMMAND ${XXD_EXECUTABLE} -i -n ${shader_spv_identifier} ${shader_spv} ${shader_header}
            DEPENDS ${slang_shader}
            COMMENT "Compiling Slang shader ${shader_name} to embedded SPIR-V"
            VERBATIM
        )
        list(APPEND shader_spv_files ${shader_header})
    endforeach()

    add_custom_target(${target_name} DEPENDS ${shader_spv_files})
endfunction()
