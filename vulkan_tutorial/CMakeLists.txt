function(add_exmaple target)
    set(multiValueArgs SRCS DEPS)
    cmake_parse_arguments(AE "" "" "${multiValueArgs}" ${ARGN})
    if(NOT AE_SRCS)
        set(AE_SRCS ${AE_UNPARSED_ARGUMENTS})
    endif()
    if(NOT AE_DEPS)
        set(AE_DEPS Vulkan::Vulkan glfw)
    endif()
    add_executable(${target} ${AE_SRCS})
    target_link_libraries(${target} PUBLIC ${AE_DEPS})
    set_target_properties(${target} PROPERTIES CXX_STANDARD 20)
endfunction()

add_exmaple(
    01_base_code
    SRCS ./01_base_code/main.cpp
    DEPS Vulkan::Vulkan glfw
)
