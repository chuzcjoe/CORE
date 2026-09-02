if(ENABLE_EXTERNAL)
  add_subdirectory(external)
endif()

if(ENABLE_VULKAN)
  add_subdirectory(vulkan)
endif()

if(ENABLE_OPENCL)
  add_subdirectory(opencl)
endif()

if(ENABLE_MAT)
  add_subdirectory(mat)
endif()

if(ENABLE_TIMER)
  add_subdirectory(timer)
endif()

if(ENABLE_TRACE)
  message(STATUS "Tracing is enabled")
  add_subdirectory(trace)
endif()

if(APPLE AND ENABLE_OPENGL)
  add_subdirectory(opengl)
endif()

if(APPLE AND ENABLE_METAL)
  add_subdirectory(metal)
endif()

if(ANDROID AND ENABLE_EGL)
  add_subdirectory(egl)
endif()

if(ENABLE_IO)
  add_subdirectory(io)
endif()

if(ENABLE_THREADPOOL)
  add_subdirectory(threadpool)
endif()

if(ENABLE_TESTS)
  add_subdirectory(tests)
endif()

if(ENABLE_EXAMPLES)
  add_subdirectory(examples)
endif()
