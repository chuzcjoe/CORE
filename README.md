# CORE: Compute and Open Rendering Engine

[![CI](https://github.com/chuzcjoe/core/actions/workflows/macos.yml/badge.svg)](https://github.com/chuzcjoe/core/actions/workflows/macos.yml)
[![CI](https://github.com/chuzcjoe/core/actions/workflows/android.yml/badge.svg)](https://github.com/chuzcjoe/core/actions/workflows/android.yml)

CORE is a C++20 framework for GPU computing and rendering. It simplifies working with
Vulkan, OpenCL, Metal, and OpenGL, with examples and tests for compute and graphics workloads.
It provides higher-level abstractions over these GPU APIs to reduce boilerplate and make
compute and rendering tasks easier to implement.

To get started with CORE, see [CORE quickstart guide](https://core-computings.github.io/)

## 1. Supported Platforms

- **macOS**: build and run locally.
- **Android (arm64-v8a)**: cross-compile on macOS and run on an Android device.

## 2. Install Dependencies

Ask your AI agent to follow [docs/install_dependencies.md](docs/install_dependencies.md).
For example:

> Read `docs/install_dependencies.md` and prepare this machine to build CORE for macOS
> (or Android arm64-v8a). Check existing tools first, install missing dependencies,
> configure the environment, and verify the setup.

## 3. Build and Run

Run all commands from the repository root in the shell configured in step 2.
Initialize submodules before the first build:

```bash
git submodule update --init --recursive
```

Before running the demos in `examples/`, sync their required data (such as models and
textures). These data files are not stored in this repository; they are maintained separately
in [core_data](https://github.com/chuzcjoe/core_data). Run the sync script from the repository
root to download the data into the corresponding local directories.

```bash
./scripts/sync_data.sh
```

The script requires Git LFS and SSH access to `git@github.com:chuzcjoe/core_data.git`.
Run it again whenever you need to update the local data.

### macOS

```bash
# Build libraries, examples, and tests.
./scripts/run.sh -t macos

# Run the Vulkan triangle example.
./build/macos/examples/vk_triangle_demo

# Build and run the general or Vulkan test suite.
./scripts/run.sh -t macos -r tests
./scripts/run.sh -t macos -r vulkan
```

More examples are available in [examples/](examples/); macOS executables are built under
`build/macos/examples/`.

### Android

```bash
./scripts/run.sh -t arm64-v8a
```

To run tests, connect an Android device with USB debugging enabled and authorize it for `adb`:

```bash
adb devices
./scripts/run.sh -t arm64-v8a -r tests
./scripts/run.sh -t arm64-v8a -r vulkan
```
