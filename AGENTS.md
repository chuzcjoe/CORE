# Development Guidelines

This file provides repository-specific guidance for coding agents working on CORE. Run all
commands from the repository root unless stated otherwise.

## Build Commands

CORE uses CMake and requires C++20. Initialize submodules before the first build:

Before installing or changing build dependencies, read `docs/install_dependencies.md` and follow
the detection, permission, installation, and verification steps for the requested target.

```bash
git submodule update --init --recursive
```

The supported build entry point is `scripts/run.sh`:

```bash
# Build all enabled targets for macOS.
./scripts/run.sh -t macos

# Build all enabled targets for Android arm64-v8a.
export ANDROID_NDK_ROOT=/path/to/android-ndk/26.1.10909125
./scripts/run.sh -t arm64-v8a

# Build and run the general test suite on macOS.
./scripts/run.sh -t macos -r tests

# Build and run the Vulkan test suite on macOS.
./scripts/run.sh -t macos -r vulkan
```

Android test execution uses `adb` and requires a connected device:

```bash
./scripts/run.sh -t arm64-v8a -r tests
./scripts/run.sh -t arm64-v8a -r vulkan
```

Run a macOS example from the repository root so relative asset paths resolve correctly:

```bash
./build/macos/examples/vk_triangle_demo
```

The `core` build variant enables `-Wall`, `-Wextra`, `-Wpedantic`, and `-Werror`. Treat compiler
warnings as build failures. CI builds both macOS and Android; Android currently uses NDK
26.1.10909125, ABI `arm64-v8a`, and API level 34.

## Code Structure

```text
CORE/
├── CMakeLists.txt          # C++ standard, build options, and top-level configuration
├── cmake/                  # Module selection and CMake helpers for examples and shaders
├── vulkan/                 # Vulkan abstraction library and Vulkan-specific tests
│   ├── include/            # Public Vulkan headers
│   ├── src/                # Vulkan implementations
│   └── tests/              # Compute, rendering, and performance tests
├── opencl/                 # Dynamically loaded OpenCL abstraction
├── metal/                  # Apple Metal abstraction
├── opengl/                 # Desktop OpenGL abstraction
├── opengles/               # OpenGL ES work in progress
├── egl/                    # Android EGL support
├── io/                     # Bitmap, texture, and file I/O helpers
├── mat/                    # Math utilities
├── timer/                  # Timing utilities
├── trace/                  # Optional Perfetto-backed tracing
├── threadpool/             # Header-only thread pool
├── tests/                  # Cross-module GoogleTest suite
├── examples/               # Vulkan, OpenGL, and Metal demos plus their assets/shaders
├── apps/android/           # Native Android application projects
├── vulkan_tutorial/        # Standalone Vulkan tutorial programs
├── external/               # Third-party dependencies, mostly Git submodules
└── scripts/                # Build, run, and Android packaging scripts
```

The top-level build includes modules through `cmake/core.cmake` and `ENABLE_*` options. Most
libraries keep public declarations in `<module>/include/` and implementations in
`<module>/src/`. General integration tests belong in `tests/`; Vulkan-only tests belong in
`vulkan/tests/`. Platform-specific sources and targets are selected with CMake conditions such
as `APPLE` and `ANDROID`.

## Development Guidelines

### C++ Code Style

- All C++ code must comply with the
  [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).
- Naming, indentation, header include order, comment style, and related conventions must follow
  that guide.

### Simplicity

- Complete the task with the minimum amount of code changes.
- Do not add features, refactors, or abstractions beyond the scope of the task.
- Do not design for hypothetical future requirements.
- Avoid unnecessary helper functions or wrappers.

### Minimal Blast Radius

- Touch only what is necessary. Clean up only changes introduced by the current task.
- Only modify the files and code strictly necessary to complete the current task.
- Do not opportunistically refactor, reformat, or optimize unrelated code.
- Only clean up temporary code or debug leftovers introduced by the current task; do not clean
  up pre-existing code unrelated to the task.

### Explicit Style

- Write syntax that makes types, conversions, and intent clear without relying on hidden behavior.
- Initialize C++ `struct` and `union` aggregates with explicit member names. Use C++20 designated
  initializers in declaration order, including for nested aggregates; avoid positional member
  initialization and unexplained nested braces.
- For `struct` aggregates, initialize all fields or use `{}` followed by explicit member assignments
  to avoid missing-field initializer warnings. For `union` aggregates, explicitly select only the
  intended active member (for example, `.color = {.float32 = {...}}` for `VkClearValue`).
- Use explicit types when `auto` would obscure the type or its signedness, precision, or ownership.
- Avoid implicit narrowing, signed/unsigned mixing, and implicit user-defined conversions. Use
  named C++ casts for intentional conversions, and validate ranges before potentially lossy casts.
- Mark constructors callable with one argument and conversion operators `explicit` unless implicit
  conversion is an intentional, documented part of the API.
- Use `nullptr` for null pointers and explicit pointer and numeric comparisons in conditions.
- Use braces for control-flow bodies and parentheses when mixed operators could obscure grouping.
- Keep side effects separate when combining them would obscure evaluation order or intent; avoid
  clever expressions and ambiguous overloads.
