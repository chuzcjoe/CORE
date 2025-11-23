#pragma once

#include <CL/cl.h>
#include <stdlib.h>

// #undef clGetPlatformIDs

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <windows.h>

typedef HMODULE CORE_DYNLIB_HANDLE;

#define CORE_DYNLIB_OPEN LoadLibrary
#define CORE_DYNLIB_CLOSE FreeLibrary
#define CORE_DYNLIB_IMPORT GetProcAddress
#else
#include <dlfcn.h>

typedef void* CORE_DYNLIB_HANDLE;

#define CORE_DYNLIB_OPEN(path) dlopen(path, RTLD_NOW | RTLD_GLOBAL)
#define CORE_DYNLIB_CLOSE dlclose
#define CORE_DYNLIB_IMPORT dlsym
#endif

#define CL_GET_FUN(x) x

#define CL_SUCCESS 0               // success
#define CL_ERROR_OPEN_FAILED -1    // fail to open the OpenCL dynamic library
#define CL_ERROR_ATEXIT_FAILED -2  // fail to register atexit handler

#define clGetPlatformIDs CL_GET_FUN(core::opencl::__clGetPlatformIDs)

namespace core {
namespace opencl {

int cl_init();

// --------------------------------------------------------------------------------------------------------------------------------

typedef cl_int (*PFN_CLGETPLATFORMIDS)(cl_uint /* num_entries */, cl_platform_id* /* platforms */,
                                       cl_uint* /* num_platforms */) CL_API_SUFFIX__VERSION_1_0;

extern PFN_CLGETPLATFORMIDS __clGetPlatformIDs;

}  // namespace opencl
}  // namespace core
