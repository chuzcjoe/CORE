#include "CLLoader.h"

#include <stdio.h>
// Include extension error code if available for better diagnostics
#include <CL/cl_ext.h>

namespace core {
namespace opencl {

static CORE_DYNLIB_HANDLE module = nullptr;

// clang-format off
// Function pointers
PFN_CLGETPLATFORMIDS          __clGetPlatformIDs          = nullptr;
PFN_CLGETDEVICEIDS            __clGetDeviceIDs            = nullptr;
PFN_CLCREATECONTEXT           __clCreateContext           = nullptr;
PFN_CLCREATECOMMANDQUEUE      __clCreateCommandQueue      = nullptr;
PFN_CLCREATEPROGRAMWITHSOURCE __clCreateProgramWithSource = nullptr;
PFN_CLBUILDPROGRAM            __clBuildProgram            = nullptr;
PFN_CLCREATEKERNEL            __clCreateKernel            = nullptr;
PFN_CLCREATEBUFFER            __clCreateBuffer            = nullptr;
PFN_CLSETKERNELARG            __clSetKernelArg            = nullptr;
PFN_CLENQUEUENDRANGEKERNEL    __clEnqueueNDRangeKernel    = nullptr;
PFN_CLFINISH                  __clFinish                  = nullptr;
PFN_CLENQUEUEREADBUFFER       __clEnqueueReadBuffer       = nullptr;
PFN_CLENQUEUEWRITEBUFFER      __clEnqueueWriteBuffer      = nullptr;
PFN_CLENQUEUEMAPBUFFER        __clEnqueueMapBuffer        = nullptr;
PFN_CLENQUEUEUNMAPMEMOBJECT   __clEnqueueUnmapMemObject   = nullptr;
PFN_CLRELEASEMEMOBJECT        __clReleaseMemObject        = nullptr;
PFN_CLRELEASEKERNEL           __clReleaseKernel           = nullptr;
PFN_CLRELEASEPROGRAM          __clReleaseProgram          = nullptr;
PFN_CLRELEASECOMMANDQUEUE     __clReleaseCommandQueue     = nullptr;
PFN_CLRELEASECONTEXT          __clReleaseContext          = nullptr;
PFN_CLGETPROGRAMBUILDINFO     __clGetProgramBuildInfo     = nullptr;
PFN_CLGETDEVICEINFO           __clGetDeviceInfo           = nullptr;
PFN_CLGETPLATFORMINFO         __clGetPlatformInfo         = nullptr;
PFN_CLWAITFOREVENTS           __clWaitForEvents           = nullptr;
PFN_CLGETEVENTPROFILINGINFO   __clGetEventProfilingInfo   = nullptr;
PFN_CLRELEASEEVENT            __clReleaseEvent            = nullptr;
// clang-format on

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

  // clang-format off
  // Load functions
  __clGetPlatformIDs          = (PFN_CLGETPLATFORMIDS )CORE_DYNLIB_IMPORT(module, "clGetPlatformIDs");
  __clGetDeviceIDs            = (PFN_CLGETDEVICEIDS)CORE_DYNLIB_IMPORT(module, "clGetDeviceIDs");
  __clCreateContext           = (PFN_CLCREATECONTEXT)CORE_DYNLIB_IMPORT(module, "clCreateContext");
  __clCreateCommandQueue      = (PFN_CLCREATECOMMANDQUEUE)CORE_DYNLIB_IMPORT(module, "clCreateCommandQueue");
  __clCreateProgramWithSource = (PFN_CLCREATEPROGRAMWITHSOURCE)CORE_DYNLIB_IMPORT(module, "clCreateProgramWithSource");
  __clBuildProgram            = (PFN_CLBUILDPROGRAM)CORE_DYNLIB_IMPORT(module, "clBuildProgram");
  __clCreateKernel            = (PFN_CLCREATEKERNEL)CORE_DYNLIB_IMPORT(module, "clCreateKernel");
  __clCreateBuffer            = (PFN_CLCREATEBUFFER)CORE_DYNLIB_IMPORT(module, "clCreateBuffer");
  __clSetKernelArg            = (PFN_CLSETKERNELARG)CORE_DYNLIB_IMPORT(module, "clSetKernelArg");
  __clEnqueueNDRangeKernel    = (PFN_CLENQUEUENDRANGEKERNEL)CORE_DYNLIB_IMPORT(module, "clEnqueueNDRangeKernel");
  __clFinish                  = (PFN_CLFINISH)CORE_DYNLIB_IMPORT(module, "clFinish");
  __clEnqueueReadBuffer       = (PFN_CLENQUEUEREADBUFFER)CORE_DYNLIB_IMPORT(module, "clEnqueueReadBuffer");
  __clEnqueueWriteBuffer      = (PFN_CLENQUEUEWRITEBUFFER)CORE_DYNLIB_IMPORT(module, "clEnqueueWriteBuffer");
  __clEnqueueMapBuffer        = (PFN_CLENQUEUEMAPBUFFER)CORE_DYNLIB_IMPORT(module, "clEnqueueMapBuffer");
  __clEnqueueUnmapMemObject   = (PFN_CLENQUEUEUNMAPMEMOBJECT)CORE_DYNLIB_IMPORT(module, "clEnqueueUnmapMemObject");
  __clReleaseMemObject        = (PFN_CLRELEASEMEMOBJECT)CORE_DYNLIB_IMPORT(module, "clReleaseMemObject");
  __clReleaseKernel           = (PFN_CLRELEASEKERNEL)CORE_DYNLIB_IMPORT(module, "clReleaseKernel");
  __clReleaseProgram          = (PFN_CLRELEASEPROGRAM)CORE_DYNLIB_IMPORT(module, "clReleaseProgram");
  __clReleaseCommandQueue     = (PFN_CLRELEASECOMMANDQUEUE)CORE_DYNLIB_IMPORT(module, "clReleaseCommandQueue");
  __clReleaseContext          = (PFN_CLRELEASECONTEXT)CORE_DYNLIB_IMPORT(module, "clReleaseContext");
  __clGetProgramBuildInfo     = (PFN_CLGETPROGRAMBUILDINFO)CORE_DYNLIB_IMPORT(module, "clGetProgramBuildInfo");
  __clGetDeviceInfo           = (PFN_CLGETDEVICEINFO)CORE_DYNLIB_IMPORT(module, "clGetDeviceInfo");
  __clGetPlatformInfo         = (PFN_CLGETPLATFORMINFO)CORE_DYNLIB_IMPORT(module, "clGetPlatformInfo");
  __clWaitForEvents           = (PFN_CLWAITFOREVENTS)CORE_DYNLIB_IMPORT(module, "clWaitForEvents");
  __clGetEventProfilingInfo   = (PFN_CLGETEVENTPROFILINGINFO)CORE_DYNLIB_IMPORT(module, "clGetEventProfilingInfo");
  __clReleaseEvent            = (PFN_CLRELEASEEVENT)CORE_DYNLIB_IMPORT(module, "clReleaseEvent");
  // clang-format on

  printf("OpenCL library loaded successfully.\n");

  return CL_SUCCESS;
}

}  // namespace opencl
}  // namespace core
