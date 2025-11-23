#include "CLLoader.h"

#include <stdio.h>
// Include extension error code if available for better diagnostics
#include <CL/cl_ext.h>

namespace core {
namespace opencl {

static CORE_DYNLIB_HANDLE module = nullptr;

// Function pointers
PFN_CLGETPLATFORMIDS __clGetPlatformIDs = nullptr;

static void *dynamic_library_open_find(const char **paths) {
  int i = 0;
  while (paths[i] != nullptr) {
    CORE_DYNLIB_HANDLE lib = CORE_DYNLIB_OPEN(paths[i]);
    if (lib != nullptr) {
      printf("Successfully opened library: %s\n", paths[i]);
      return lib;
    }
    ++i;
  }
  return nullptr;
}

static void cl_exit(void) {
  if (module != nullptr) {
    //  Ignore errors
    CORE_DYNLIB_CLOSE(module);
    module = nullptr;
  }
}

int cl_init() {
#ifdef _WIN32
  const char *paths[] = {"OpenCL.dll", NULL};
#elif defined(__APPLE__)
  const char *paths[] = {"/Library/Frameworks/OpenCL.framework/OpenCL", NULL};
#else
  const char *paths[] = {"libOpenCL.so", "libOpenCL.so.0", "libOpenCL.so.1", "libOpenCL.so.2",
                         NULL};
#endif

  int error = 0;

  //  Check if already initialized
  if (module != nullptr) {
    return CL_SUCCESS;
  }

  //  Load library
  module = dynamic_library_open_find(paths);

  //  Check for errors
  if (module == nullptr) {
    return CL_ERROR_OPEN_FAILED;
  }

  //  Set unloading
  error = atexit(cl_exit);

  if (error) {
    //  Failure queuing atexit, shutdown with error
    CORE_DYNLIB_CLOSE(module);
    module = nullptr;
    return CL_ERROR_ATEXIT_FAILED;
  }

  // Load functions
  __clGetPlatformIDs = (PFN_CLGETPLATFORMIDS)CORE_DYNLIB_IMPORT(module, "clGetPlatformIDs");

  printf("OpenCL library loaded successfully.\n");

  return CL_SUCCESS;
}

}  // namespace opencl
}  // namespace core
