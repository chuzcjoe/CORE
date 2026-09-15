if(ENABLE_MAT)
  add_subdirectory(mat)
endif()

if(ENABLE_THREADPOOL AND ENABLE_TIMER)
  add_subdirectory(threadpool)
endif()

if(ENABLE_IO AND ENABLE_MAT)
  add_subdirectory(stb)
  add_subdirectory(bitmap)
endif()

if(ENABLE_OPENCL AND ENABLE_MAT)
  add_subdirectory(opencl)
endif()

if(ENABLE_VULKAN AND ENABLE_MAT AND ENABLE_TIMER)
  add_subdirectory(vulkan)
endif()

if(APPLE AND ENABLE_METAL)
  add_subdirectory(metal)
endif()

if(ENABLE_TRACE)
  add_subdirectory(perfetto)
endif()

if(ANDROID AND ENABLE_EGL AND ENABLE_IO)
  add_subdirectory(gles)
endif()

if(ANDROID AND ENABLE_IO)
  add_subdirectory(ahardwarebuffer)
endif()
