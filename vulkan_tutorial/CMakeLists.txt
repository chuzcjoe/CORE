if(NOT SLANGC_EXECUTABLE)
  find_program(SLANGC_EXECUTABLE NAMES slangc REQUIRED)
  message(STATUS "Found Slang compiler: ${SLANGC_EXECUTABLE}")
endif()

function (add_slang_shader_target TARGET)
  cmake_parse_arguments("SHADER" "" "" "SOURCES" ${ARGN})
  set(SHADERS_DIR ${CMAKE_CURRENT_LIST_DIR}/shaders)
  set(ENTRY_POINTS -entry vertMain -entry fragMain)

  set(ABS_SHADER_SOURCES)
  foreach(src ${SHADER_SOURCES})
    if(IS_ABSOLUTE ${src})
      list(APPEND ABS_SHADER_SOURCES ${src})
    else()
      list(APPEND ABS_SHADER_SOURCES ${CMAKE_CURRENT_LIST_DIR}/${src})
    endif()
  endforeach()

  add_custom_command(
          OUTPUT ${SHADERS_DIR}
          COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADERS_DIR}
  )
  add_custom_command(
          OUTPUT ${SHADERS_DIR}/slang.spv
          COMMAND ${SLANGC_EXECUTABLE} ${ABS_SHADER_SOURCES} -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name ${ENTRY_POINTS} -o ${SHADERS_DIR}/slang.spv
          DEPENDS ${SHADERS_DIR} ${ABS_SHADER_SOURCES}
          COMMENT "Compiling Slang Shaders"
          VERBATIM
  )
  add_custom_target(${TARGET} DEPENDS ${SHADERS_DIR}/slang.spv)
endfunction()

# Add example helper function
function(add_exmaple target)
    set(multiValueArgs SRCS DEPS SHADERS)
    cmake_parse_arguments(AE "" "" "${multiValueArgs}" ${ARGN})
    if(NOT AE_SRCS)
        set(AE_SRCS ${AE_UNPARSED_ARGUMENTS})
    endif()
    if(NOT AE_DEPS)
        set(AE_DEPS Vulkan::cppm glfw)
    endif()

    add_executable(${target} ${AE_SRCS})
    target_link_libraries(${target} PUBLIC ${AE_DEPS})
    set_target_properties(${target} PROPERTIES CXX_STANDARD 20)

    if(AE_SHADERS)
        add_slang_shader_target(${target}_shaders SOURCES ${AE_SHADERS})
        add_dependencies(${target} ${target}_shaders)
    endif()
endfunction()


# set up Vulkan C++ module
# For generators that support C++ modules (Ninja, VS 17.4+), use the module fileset.
# For others (e.g. Unix Makefiles), fall back to a normal header-based interface target.
if(CMAKE_GENERATOR MATCHES "Ninja" OR CMAKE_GENERATOR MATCHES "Visual Studio")
    add_library(VulkanCppModule)
    add_library(Vulkan::cppm ALIAS VulkanCppModule)

    target_compile_definitions(VulkanCppModule PUBLIC
            VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
            VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
    )

    target_include_directories(VulkanCppModule PRIVATE "${Vulkan_INCLUDE_DIR}")

    target_link_libraries(VulkanCppModule PUBLIC Vulkan::Vulkan)

    set_target_properties(VulkanCppModule PROPERTIES CXX_STANDARD 20)

    target_sources(VulkanCppModule
            PUBLIC
            FILE_SET cxx_modules TYPE CXX_MODULES
            BASE_DIRS "${Vulkan_INCLUDE_DIR}"
            FILES "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm"
    )
else()
    add_library(VulkanCppModule INTERFACE)
    add_library(Vulkan::cppm ALIAS VulkanCppModule)

    target_compile_definitions(VulkanCppModule INTERFACE
            VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
            VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
    )

    target_include_directories(VulkanCppModule INTERFACE "${Vulkan_INCLUDE_DIR}")

    target_link_libraries(VulkanCppModule INTERFACE Vulkan::Vulkan)
endif()


# examples
# add_exmaple(
#     01_base_code
#     SRCS ./01_base_code/main.cpp
#     DEPS Vulkan::Vulkan glfw
# )

add_exmaple(
    02_draw_triangle
    SRCS ./02_draw_triangle/main.cpp
    DEPS VulkanCppModule glfw
    SHADERS ./02_draw_triangle/triangle.slang
)
