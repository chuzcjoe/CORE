# CORE: Compute and Open Rendering Engine

**CORE** is a cross-platform C++ framework that abstracts away the boilerplate of modern GPU setup, letting developers focus on what matters. It combines high-performance GPU computing with powerful real-time rendering capabilities, offering a clean, unified interface for both compute workloads and graphics pipelines.

<p align="center">
  <img src="./logo/core.gif" alt="CORE" width="60%"/>
</p>

# 1. Graphics and Compute APIs

- [x] Vulkan
- [x] OpenGL
- [x] Metal
- [ ] OpenCL (WIP)
- [ ] OpenGLES (WIP)

# 2. Supporting OS

- [x] MacOS
- [x] Android arm64-v8a
- [ ] Windows (WIP)
- [ ] Linux (WIP)

# 3. Compile

## 3.1 Prerequisites

1. Download Vulkan from: https://vulkan.lunarg.com/. Select your OS and follow the install instructions.
2. Download Android NDK from: https://github.com/android/ndk/releases?page=1. Select your the NDK version that matches your OS(macos/windows/linux).

After downloading the NDK, set the environment variable **ANDROID_NDK_ROOT** pointing to your NDK directory. For example:
```
export ANDROID_NDK_ROOT=<path_to_ndk>/26.1.10909125
```

## 3.2 Host(MacOS) + Target(MacOS)
```
./run.sh -target macos
```

## 3.3 Host(MacOS) + Target(Android arm64-v8a)
```
./run.sh -target arm64-v8a
``` 

# 4. Compute
```
./run.sh [-target macos|arm64-v8a] [-test_module <name>] [-test_filter <Suite.Test>]
```

## Vulkan:
1. ./run.sh -target macos -test_module vulkan -test_filter ComputeSum.test (for macos)
2. ./run.sh -target arm64-v8a -test_module vulkan -test_filter ComputeSum.test (for arm64-v8a)


# 5. Graphics

Normally, you need to write ~1000 lines of code in Vulkan to draw a simple triangle. Using **CORE** APIs,
It only takes about **100** lines of code. See example in `examples/DrawTriangleDemo/main.cpp`

How to run this demo?
```
./run.sh -target macos
./build/macos/examples/vk_triangle_demo

```

