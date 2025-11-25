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

// clang-format off
#define clGetPlatformIDs          CL_GET_FUN(core::opencl::__clGetPlatformIDs)
#define clGetDeviceIDs            CL_GET_FUN(core::opencl::__clGetDeviceIDs)
#define clCreateContext           CL_GET_FUN(core::opencl::__clCreateContext)
#define clCreateCommandQueue      CL_GET_FUN(core::opencl::__clCreateCommandQueue)
#define clCreateProgramWithSource CL_GET_FUN(core::opencl::__clCreateProgramWithSource)
#define clBuildProgram            CL_GET_FUN(core::opencl::__clBuildProgram)
#define clCreateKernel            CL_GET_FUN(core::opencl::__clCreateKernel)
#define clCreateBuffer            CL_GET_FUN(core::opencl::__clCreateBuffer)
#define clSetKernelArg            CL_GET_FUN(core::opencl::__clSetKernelArg)
#define clEnqueueNDRangeKernel    CL_GET_FUN(core::opencl::__clEnqueueNDRangeKernel)
#define clFinish                  CL_GET_FUN(core::opencl::__clFinish)
#define clEnqueueReadBuffer       CL_GET_FUN(core::opencl::__clEnqueueReadBuffer)
#define clEnqueueWriteBuffer      CL_GET_FUN(core::opencl::__clEnqueueWriteBuffer)
#define clReleaseMemObject        CL_GET_FUN(core::opencl::__clReleaseMemObject)
#define clReleaseKernel           CL_GET_FUN(core::opencl::__clReleaseKernel)
#define clReleaseProgram          CL_GET_FUN(core::opencl::__clReleaseProgram)
#define clReleaseCommandQueue     CL_GET_FUN(core::opencl::__clReleaseCommandQueue)
#define clReleaseContext          CL_GET_FUN(core::opencl::__clReleaseContext)
#define clGetProgramBuildInfo     CL_GET_FUN(core::opencl::__clGetProgramBuildInfo)
#define clGetDeviceInfo           CL_GET_FUN(core::opencl::__clGetDeviceInfo)
#define clGetPlatformInfo         CL_GET_FUN(core::opencl::__clGetPlatformInfo)
// clang-format on

namespace core {
namespace opencl {

int cl_init();

// --------------------------------------------------------------------------------------------------------------------------------

typedef cl_int (*PFN_CLGETPLATFORMIDS)(cl_uint /* num_entries */, cl_platform_id* /* platforms */,
                                       cl_uint* /* num_platforms */) CL_API_SUFFIX__VERSION_1_0;

typedef cl_int (*PFN_CLGETDEVICEIDS)(cl_platform_id /* platform */,
                                     cl_device_type /* device_type */, cl_uint /* num_entries */,
                                     cl_device_id* /* devices */,
                                     cl_uint* /* num_devices */) CL_API_SUFFIX__VERSION_1_0;

typedef cl_context (*PFN_CLCREATECONTEXT)(
    const cl_context_properties* /* properties */, cl_uint /* num_devices */,
    const cl_device_id* /* devices */,
    void (* /* pfn_notify */)(const char*, const void*, size_t, void*), void* /* user_data */,
    cl_int* /* errcode_ret */) CL_API_SUFFIX__VERSION_1_0;

typedef cl_command_queue (*PFN_CLCREATECOMMANDQUEUE)(
    cl_context /* context */, cl_device_id /* device */,
    cl_command_queue_properties /* properties */,
    cl_int* /* errcode_ret */) CL_API_SUFFIX__VERSION_1_0;

typedef cl_program (*PFN_CLCREATEPROGRAMWITHSOURCE)(
    cl_context /* context */, cl_uint /* count */, const char** /* strings */,
    const size_t* /* lengths */, cl_int* /* errcode_ret */) CL_API_SUFFIX__VERSION_1_0;

typedef cl_int (*PFN_CLBUILDPROGRAM)(cl_program /* program */, cl_uint /* num_devices */,
                                     const cl_device_id* /* device_list */,
                                     const char* /* options */,
                                     void(CL_CALLBACK* /* pfn_notify */)(cl_program /* program */,
                                                                         void* /* user_data */),
                                     void* /* user_data */) CL_API_SUFFIX__VERSION_1_0;

typedef cl_kernel (*PFN_CLCREATEKERNEL)(cl_program /* program */, const char* /* kernel_name */,
                                        cl_int* /* errcode_ret */) CL_API_SUFFIX__VERSION_1_0;

typedef cl_mem (*PFN_CLCREATEBUFFER)(cl_context /* context */, cl_mem_flags /* flags */,
                                     size_t /* size */, void* /* host_ptr */,
                                     cl_int* /* errcode_ret */) CL_API_SUFFIX__VERSION_1_0;

typedef cl_int (*PFN_CLSETKERNELARG)(cl_kernel /* kernel */, cl_uint /* arg_index */,
                                     size_t /* arg_size */,
                                     const void* /* arg_value */) CL_API_SUFFIX__VERSION_1_0;

typedef cl_int (*PFN_CLENQUEUENDRANGEKERNEL)(
    cl_command_queue /* command_queue */, cl_kernel /* kernel */, cl_uint /* work_dim */,
    const size_t* /* global_work_offset */, const size_t* /* global_work_size */,
    const size_t* /* local_work_size */, cl_uint /* num_events_in_wait_list */,
    const cl_event* /* event_wait_list */, cl_event* /* event */) CL_API_SUFFIX__VERSION_1_0;

typedef cl_int (*PFN_CLFINISH)(cl_command_queue /* command_queue */) CL_API_SUFFIX__VERSION_1_0;

typedef cl_int (*PFN_CLENQUEUEREADBUFFER)(cl_command_queue /* command_queue */, cl_mem /* buffer */,
                                          cl_bool /* blocking_read */, size_t /* offset */,
                                          size_t /* cb */, void* /* ptr */,
                                          cl_uint /* num_events_in_wait_list */,
                                          const cl_event* /* event_wait_list */,
                                          cl_event* /* event */) CL_API_SUFFIX__VERSION_1_0;

typedef cl_int (*PFN_CLENQUEUEWRITEBUFFER)(cl_command_queue /* command_queue */,
                                           cl_mem /* buffer */, cl_bool /* blocking_write */,
                                           size_t /* offset */, size_t /* cb */,
                                           const void* /* ptr */,
                                           cl_uint /* num_events_in_wait_list */,
                                           const cl_event* /* event_wait_list */,
                                           cl_event* /* event */) CL_API_SUFFIX__VERSION_1_0;

typedef cl_int (*PFN_CLGETPROGRAMBUILDINFO)(
    cl_program /* program */, cl_device_id /* device */, cl_program_build_info /* param_name */,
    size_t /* param_value_size */, void* /* param_value */,
    size_t* /* param_value_size_ret */) CL_API_SUFFIX__VERSION_1_0;

typedef cl_int (*PFN_CLRELEASEMEMOBJECT)(cl_mem /* memobj */) CL_API_SUFFIX__VERSION_1_0;

typedef cl_int (*PFN_CLRELEASEKERNEL)(cl_kernel /* kernel */) CL_API_SUFFIX__VERSION_1_0;

typedef cl_int (*PFN_CLRELEASEPROGRAM)(cl_program /* program */) CL_API_SUFFIX__VERSION_1_0;

typedef cl_int (*PFN_CLRELEASECOMMANDQUEUE)(cl_command_queue /* command_queue */)
    CL_API_SUFFIX__VERSION_1_0;

typedef cl_int (*PFN_CLRELEASECONTEXT)(cl_context /* context */) CL_API_SUFFIX__VERSION_1_0;

typedef cl_int (*PFN_CLGETDEVICEINFO)(
    cl_device_id /* device */, cl_device_info /* param_name */, size_t /* param_value_size */,
    void* /* param_value */, size_t* /* param_value_size_ret */) CL_API_SUFFIX__VERSION_1_0;

typedef cl_int (*PFN_CLGETPLATFORMINFO)(
    cl_platform_id /* platform */, cl_platform_info /* param_name */, size_t /* param_value_size */,
    void* /* param_value */, size_t* /* param_value_size_ret */) CL_API_SUFFIX__VERSION_1_0;

// clang-format off
extern PFN_CLGETPLATFORMIDS          __clGetPlatformIDs;
extern PFN_CLGETDEVICEIDS            __clGetDeviceIDs;
extern PFN_CLCREATECONTEXT           __clCreateContext;
extern PFN_CLCREATECOMMANDQUEUE      __clCreateCommandQueue;
extern PFN_CLCREATEPROGRAMWITHSOURCE __clCreateProgramWithSource;
extern PFN_CLBUILDPROGRAM            __clBuildProgram;
extern PFN_CLCREATEKERNEL            __clCreateKernel;
extern PFN_CLCREATEBUFFER            __clCreateBuffer;
extern PFN_CLSETKERNELARG            __clSetKernelArg;
extern PFN_CLENQUEUENDRANGEKERNEL    __clEnqueueNDRangeKernel;
extern PFN_CLFINISH                  __clFinish;
extern PFN_CLENQUEUEREADBUFFER       __clEnqueueReadBuffer;
extern PFN_CLENQUEUEWRITEBUFFER      __clEnqueueWriteBuffer;
extern PFN_CLGETPROGRAMBUILDINFO     __clGetProgramBuildInfo;
extern PFN_CLRELEASEMEMOBJECT        __clReleaseMemObject;
extern PFN_CLRELEASEKERNEL           __clReleaseKernel;
extern PFN_CLRELEASEPROGRAM          __clReleaseProgram;
extern PFN_CLRELEASECOMMANDQUEUE     __clReleaseCommandQueue;
extern PFN_CLRELEASECONTEXT          __clReleaseContext;
extern PFN_CLGETDEVICEINFO           __clGetDeviceInfo;
extern PFN_CLGETPLATFORMINFO        __clGetPlatformInfo;
// clang-format on

}  // namespace opencl
}  // namespace core
