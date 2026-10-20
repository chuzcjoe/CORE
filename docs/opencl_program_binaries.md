# OpenCL program binaries

`CLProgram` can export a built program and load it again without requiring the original
`.cl` file at load time. This supports applications that persist programs between runs or
package programs for a known device and driver. Source files and binaries are loaded
through checked `CLProgram::Create` factories.

## API

`CLContext::Create()` initializes the OpenCL loader. Build from source and obtain
owned binary bytes in a function returning `core::Status`:

```cpp
auto context_result = core::opencl::CLContext::Create();
if (!context_result.ok()) { return context_result.status(); }
auto context = std::move(*context_result);
auto source_result = core::opencl::CLProgram::Create(
    context.get(), std::string("tests/shaders/vec_add.cl"));
if (!source_result.ok()) { return source_result.status(); }
auto source = std::move(*source_result);
auto binary_result = source->GetBinary();
if (!binary_result.ok()) { return binary_result.status(); }
auto binary = std::move(*binary_result);
auto loaded_result = core::opencl::CLProgram::Create(
    context.get(), std::span<const unsigned char>(binary));
if (!loaded_result.ok()) { return loaded_result.status(); }
auto loaded = std::move(*loaded_result);
auto kernel_result = core::opencl::CLKernel::Create(loaded.get(), "vec_add");
if (!kernel_result.ok()) { return kernel_result.status(); }
auto kernel = std::move(*kernel_result);
```

The span is consumed during creation and need not remain alive afterward. The exported
vector is independent of the source program. The context must outlive its programs, kernels,
queues and buffers. Export supports the single-device programs created by `CLContext`.

Both factories accept an optional `const char* build_options`. Keep the default `nullptr`
when reloading to reuse the binary's original options. Supplying different options when
loading a binary has implementation-defined behavior under OpenCL. Binary loading checks both
the overall OpenCL error and the per-device binary status, then calls `clBuildProgram` to
produce an executable before kernels can be created. Build failures include the driver log
when available and release the program. Empty binary input or a missing context/device returns
`ErrorCode::kInvalidArgument`; driver rejection, missing loader symbols and export failures return
`ErrorCode::kBackend`. File failures return `ErrorCode::kIo`. Errors preserve native OpenCL
codes when available. There is no implicit source fallback. See [checked error handling](error_handling.md).

## Runnable example

Build with the supported entry point, then run these commands from the repository root:

```bash
./scripts/run.sh -t macos
./build/macos/examples/cl_program_binary_demo save tests/shaders/vec_add.cl tmp/vec_add.bin
./build/macos/examples/cl_program_binary_demo load tmp/vec_add.bin
```

`save` compiles the existing vector-add shader and writes its binary. `load` reads only the
binary file. Both paths dispatch the kernel and check all 257 output values. A file, driver
or output error returns exit status 1. The example expects the `vec_add(A, B, C)` signature;
the library API can load any compatible OpenCL program.

The same example is built for Android. After building and selecting an authorized device:

```bash
adb push build/arm64-v8a/examples/cl_program_binary_demo /data/local/tmp/core/
adb push tests/shaders/vec_add.cl /data/local/tmp/core/
adb shell chmod +x /data/local/tmp/core/cl_program_binary_demo
adb shell /data/local/tmp/core/cl_program_binary_demo save \
  /data/local/tmp/core/vec_add.cl /data/local/tmp/core/vec_add.bin
adb shell /data/local/tmp/core/cl_program_binary_demo load /data/local/tmp/core/vec_add.bin
```

Generate the binary separately on each target. The example does not transfer a macOS binary
to Android or automatically replace an incompatible binary.

## Compatibility and cache policy

OpenCL binaries may contain device code, implementation-specific intermediate representation,
or both. They are not a portable interchange format. Loading may still involve driver
compilation; this feature makes no measured startup-time guarantee. The API follows
[clCreateProgramWithBinary](https://registry.khronos.org/OpenCL/specs/unified/refpages/man/html/clCreateProgramWithBinary.html)
and [clGetProgramInfo](https://registry.khronos.org/OpenCL/specs/unified/refpages/man/html/clGetProgramInfo.html).

Applications own persistence and invalidation policy. A cache key should cover source and
included-file contents, build options, device/vendor identity, OpenCL implementation and
driver version. Invalidate entries when these change, preserve any required build options,
and rebuild from source when a binary is rejected. This API deliberately provides raw bytes
instead of choosing a cache directory, cache key or filesystem update policy for callers.

Tests exercise source-to-binary-to-kernel execution in a fresh context, preservation of a
compile-time macro, re-export of a loaded program, one-element and non-workgroup-sized inputs,
invalid input, invalid source build options, and successful use after rejected loads. No cross-driver
binary compatibility is promised.
