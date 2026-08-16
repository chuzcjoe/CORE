add_subdirectory(external)
add_subdirectory(vulkan)
add_subdirectory(opencl)
add_subdirectory(mat)
add_subdirectory(timer)

if (ENABLE_TRACE)
  message(STATUS "Tracing is enabled")
  add_subdirectory(trace)
endif()

if (APPLE)
  add_subdirectory(opengl)
  add_subdirectory(metal)
elseif(ANDROID)
  add_subdirectory(egl)
endif()

add_subdirectory(io)
add_subdirectory(threadpool)

# do not build tests/examples when this is included as a submodule
if(CMAKE_SOURCE_DIR STREQUAL CORE_SOURCE_DIR)
  add_subdirectory(tests)
  add_subdirectory(examples)
endif()
