find_package(CUDAToolkit REQUIRED)

add_executable(cuda_tests CudaTest.cpp)

set_source_files_properties(CudaTest.cpp PROPERTIES LANGUAGE CUDA)
set_target_properties(cuda_tests PROPERTIES
  CUDA_STANDARD 20
  CUDA_STANDARD_REQUIRED ON
  CUDA_EXTENSIONS OFF)

target_link_libraries(cuda_tests
  PRIVATE
  gtest_main
  CUDA::cudart)
