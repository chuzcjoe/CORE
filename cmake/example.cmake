function(build_example target_name)
    set(options)
    set(oneValueArgs)
    set(multiValueArgs SOURCES SOURCE_GLOBS PUBLIC_INCLUDES PRIVATE_INCLUDES DEPENDS LIBRARIES DEFINITIONS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(resolved_sources ${ARG_SOURCES})
    foreach(_pattern IN LISTS ARG_SOURCE_GLOBS)
        file(GLOB _expanded "${_pattern}")
        list(APPEND resolved_sources ${_expanded})
    endforeach()

    if(NOT resolved_sources)
        message(FATAL_ERROR "build_example(${target_name}) needs SOURCES or SOURCE_GLOBS")
    endif()

    add_executable(${target_name} ${resolved_sources})

    if(ARG_PUBLIC_INCLUDES)
        target_include_directories(${target_name} PUBLIC ${ARG_PUBLIC_INCLUDES})
    endif()

    if(ARG_PRIVATE_INCLUDES)
        target_include_directories(${target_name} PRIVATE ${ARG_PRIVATE_INCLUDES})
    endif()

    if(ARG_DEPENDS)
        add_dependencies(${target_name} ${ARG_DEPENDS})
    endif()

    if(ARG_LIBRARIES)
        target_link_libraries(${target_name} PRIVATE ${ARG_LIBRARIES})
    endif()

    if(ARG_DEFINITIONS)
        target_compile_definitions(${target_name} PRIVATE ${ARG_DEFINITIONS})
    endif()
endfunction()