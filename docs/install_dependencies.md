# Installing Build Dependencies on macOS

This document is written for AI coding agents preparing a machine to build CORE. The repository
supports two build modes:

- macOS host to macOS target: `./scripts/run.sh -t macos`
- macOS host to Android `arm64-v8a` target: `./scripts/run.sh -t arm64-v8a`

Run commands from the repository root unless a step says otherwise. Detect dependencies before
installing them, keep a compatible existing installation, and do not downgrade newer tools merely
to match the reference workstation. Exact SDK and Android package versions are pinned where the
build scripts depend on them.

Before running an interactive installer, accepting a license, using `sudo`, or modifying a shell
profile, obtain user approval unless it was already granted. Export environment variables in the
current shell by default; do not edit `~/.zshrc` or another profile without explicit permission.
Do not modify project source files or CI workflows while installing dependencies.

The versions below were observed on an Apple Silicon workstation on 2026-09-10. They are the
known-good reference versions for this guide.

## 1. Build macOS Locally on macOS

### Required tools and reference versions

| Dependency | Locally verified version | Purpose and version policy |
| --- | --- | --- |
| Apple Command Line Tools | 26.6.0.0.1781586589 | Provides the compiler, macOS SDK, `make`, Git, `curl`, `unzip`, and system frameworks. Full Xcode is not required by the current build. |
| macOS SDK | 26.5 | Provides Metal, Foundation, QuartzCore, Cocoa, OpenGL, and OpenCL frameworks. |
| Apple Clang | 21.0.0 (`clang-2100.1.1.101`) | C++20 compiler. |
| CMake | 3.29.5 | The repository declares CMake 3.10 as its minimum. Use 3.29.5 or a newer compatible version. |
| GNU Make | 3.81 | Build tool invoked by `scripts/run.sh`. |
| Git | 2.44.0 | Required to obtain the repository and initialize submodules. |
| Homebrew | 6.0.11 | Recommended package manager for CMake; it is not itself a build dependency. |
| LunarG Vulkan SDK | 1.4.350.0 | Provides Vulkan headers, loader, MoltenVK, tools, `glslc`, and `slangc`. The current CI workflow still installs 1.4.341.1. |
| `glslc` | shaderc 2026.2 | Required at CMake configure time for GLSL shader targets. The Vulkan SDK contains this version. |
| `slangc` | 2026.8 | Required at CMake configure time for Slang shader targets. The Vulkan SDK contains this version. |
| `xxd` | 2025-08-24 | Converts compiled Slang binaries into C headers for Vulkan tests. The system copy is sufficient. |

Third-party C++ libraries such as GLFW, GLM, GoogleTest, ImGui, stb, metal-cpp, and
tinyobjloader are stored under `external/` as repository content or Git submodules. Do not install
separate system copies of them.

### Install the Apple toolchain

First check the existing Command Line Tools installation:

```bash
xcode-select -p
xcrun --find clang++
xcrun --show-sdk-version
clang++ --version
make --version
```

If `xcode-select -p` or `xcrun --find clang++` fails, run the following command once:

```bash
xcode-select --install
```

This opens an Apple installer that requires user interaction. Stop and wait for the user to
complete it, then repeat the checks. Do not install full Xcode unless another task explicitly
requires it.

### Install Homebrew and CMake

Check for Homebrew and CMake first:

```bash
command -v brew
command -v cmake
cmake --version
```

If Homebrew is missing, obtain approval and use the installer from
[brew.sh](https://brew.sh/). If CMake is missing or older than 3.10, install it:

```bash
brew install cmake
```

Homebrew generally installs the current CMake release rather than the locally verified 3.29.5.
A newer compatible CMake is acceptable.

### Install and activate the Vulkan SDK

Prefer an existing compatible SDK when `VULKAN_SDK` is set and all three commands below are
available:

```bash
test -n "$VULKAN_SDK"
command -v glslc
command -v slangc
command -v vulkaninfo
```

Otherwise, install the locally verified Vulkan SDK version. The archive URL was verified for
version 1.4.350.0. The LunarG installer uses `sudo`; obtain approval before executing it.

```bash
CORE_VULKAN_VERSION=1.4.350.0
CORE_VULKAN_ARCHIVE="vulkansdk-macos-${CORE_VULKAN_VERSION}.zip"
CORE_VULKAN_DOWNLOAD_DIR="$(mktemp -d /tmp/core-vulkan-sdk.XXXXXX)"

curl -fL \
  "https://sdk.lunarg.com/sdk/download/${CORE_VULKAN_VERSION}/mac/${CORE_VULKAN_ARCHIVE}" \
  -o "${CORE_VULKAN_DOWNLOAD_DIR}/${CORE_VULKAN_ARCHIVE}"
unzip -q "${CORE_VULKAN_DOWNLOAD_DIR}/${CORE_VULKAN_ARCHIVE}" \
  -d "${CORE_VULKAN_DOWNLOAD_DIR}"

sudo "${CORE_VULKAN_DOWNLOAD_DIR}/vulkansdk-macOS-${CORE_VULKAN_VERSION}.app/Contents/MacOS/vulkansdk-macOS-${CORE_VULKAN_VERSION}" \
  --root "${HOME}/VulkanSDK/${CORE_VULKAN_VERSION}" \
  --accept-licenses \
  --default-answer \
  --confirm-command \
  install com.lunarg.vulkan.core
```

The `--accept-licenses` flag must only be used after the user has authorized license acceptance.
Activate the SDK in every shell used for configuration or compilation:

```bash
CORE_VULKAN_VERSION=1.4.350.0
source "${HOME}/VulkanSDK/${CORE_VULKAN_VERSION}/setup-env.sh"
```

Verify the active SDK and shader compilers:

```bash
test -n "$VULKAN_SDK"
test -f "$VULKAN_SDK/include/vulkan/vulkan.h"
glslc --version
slangc -version
xxd -v
vulkaninfo --summary
```

`vulkaninfo` may require an interactive macOS session with MoltenVK available. A `vulkaninfo`
runtime failure does not necessarily mean shader compilation is unavailable, but missing Vulkan
headers, `glslc`, or `slangc` is a hard dependency failure.

### Initialize repository dependencies and build

Initialize submodules without replacing local changes:

```bash
git submodule sync --recursive
git submodule update --init --recursive
```

Confirm that all required tools resolve, then build:

```bash
command -v cmake make clang++ glslc slangc xxd
./scripts/run.sh -t macos
```

Success means the command exits with status 0 and produces build outputs under `build/macos/`.
The script recreates the repository's `tmp/` directory each time it runs.

## 2. Cross-compile Android on macOS

Android compilation needs the Apple host toolchain, CMake, Git, Make, `glslc`, `slangc`, `xxd`,
and initialized submodules from section 1. It additionally needs Java and Android SDK packages.
The Android NDK supplies the Vulkan headers and `libvulkan.so` used by the Android target.

### Required tools and reference versions

| Dependency | Locally verified version | Purpose and version policy |
| --- | --- | --- |
| OpenJDK | 17.0.14 | Runs modern Android command-line tools and supplies `keytool` for APK packaging. Use JDK 17. |
| Android Command-line Tools | Install the current release | Bootstrap tool only; use its `sdkmanager` to install the exact packages below. |
| Android SDK Platform | API 34, revision 3 | Required by `scripts/run.sh` through `ANDROID_PLATFORM=android-34`. |
| Android NDK | 26.1.10909125 (r26b) | Exact version used locally, documented in `AGENTS.md`, and installed by CI. Includes Android Clang 17.0.2. |
| Android Build Tools | 34.0.0 | Required by `scripts/build_android_apps.sh` for `aapt` and `apksigner`. |
| Android SDK Platform | API 33, revision 3 | Required only by `scripts/build_android_apps.sh`, which packages against `android-33/android.jar`. |
| Android Platform Tools | 35.0.1 | Provides `adb`; required to run tests or install an APK, but not for compilation alone. |

The reference workstation also contains legacy Android SDK Tools 26.1.1 at
`$ANDROID_SDK_ROOT/tools/bin/sdkmanager`. Do not use that legacy executable: it fails with Java 17.
Use `cmdline-tools/latest/bin/sdkmanager` instead.

### Install host-side prerequisites

Complete these steps from section 1 before continuing:

1. Install Apple Command Line Tools.
2. Install CMake.
3. Install and activate Vulkan SDK 1.4.350.0 so `glslc` and `slangc` are on `PATH`.
4. Initialize all Git submodules.

For an Android-only build, the macOS Vulkan runtime is not linked into Android binaries, but the
host shader compiler executables are still required while CMake configures shader targets.

### Install Java and Android Command-line Tools

Install JDK 17 if `java -version` does not report version 17:

```bash
brew install openjdk@17
export JAVA_HOME="$(brew --prefix openjdk@17)/libexec/openjdk.jdk/Contents/Home"
export PATH="${JAVA_HOME}/bin:${PATH}"
java -version
keytool -help >/dev/null
```

Install the current Android Command-line Tools if a modern `sdkmanager` is not already available:

```bash
brew install --cask android-commandlinetools
```

Set the SDK location and resolve the modern `sdkmanager`. Do not select
`$ANDROID_SDK_ROOT/tools/bin/sdkmanager` as a fallback.

```bash
export ANDROID_SDK_ROOT="${HOME}/Library/Android/sdk"
export ANDROID_HOME="${ANDROID_SDK_ROOT}"

CORE_SDKMANAGER="$(brew --prefix)/share/android-commandlinetools/cmdline-tools/latest/bin/sdkmanager"
if test ! -x "$CORE_SDKMANAGER"; then
  CORE_SDKMANAGER="${ANDROID_SDK_ROOT}/cmdline-tools/latest/bin/sdkmanager"
fi
test -x "$CORE_SDKMANAGER"
```

### Accept licenses and install pinned Android packages

License acceptance is interactive. Obtain user approval, then run:

```bash
"$CORE_SDKMANAGER" --sdk_root="$ANDROID_SDK_ROOT" --licenses
```

Install the exact packages used by the repository:

```bash
"$CORE_SDKMANAGER" --sdk_root="$ANDROID_SDK_ROOT" \
  "platform-tools" \
  "platforms;android-33" \
  "platforms;android-34" \
  "build-tools;34.0.0" \
  "ndk;26.1.10909125"
```

Export the NDK variable expected by `scripts/run.sh`. `ANDROID_NDK_HOME` is not a substitute;
the script directly reads `ANDROID_NDK_ROOT`.

```bash
export ANDROID_NDK_ROOT="${ANDROID_SDK_ROOT}/ndk/26.1.10909125"
```

### Verify the Android toolchain and build

Run all checks before invoking the build:

```bash
test -f "$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake"
test -f "$ANDROID_SDK_ROOT/platforms/android-34/android.jar"
test -f "$ANDROID_SDK_ROOT/platforms/android-33/android.jar"
test -x "$ANDROID_SDK_ROOT/build-tools/34.0.0/aapt"
test -x "$ANDROID_SDK_ROOT/build-tools/34.0.0/apksigner"
command -v cmake make glslc slangc xxd java
```

Build all enabled Android targets:

```bash
./scripts/run.sh -t arm64-v8a
```

Success means the command exits with status 0 and produces Android `arm64-v8a` outputs under
`build/arm64-v8a/`.

An Android device is not needed to compile. It is needed for the following commands:

```bash
adb devices
./scripts/run.sh -t arm64-v8a -r tests
./scripts/build_android_apps.sh -app BasicApp
```

Before running device commands, require `adb devices` to show exactly one authorized target or
ask the user which device to use. Never install an APK or run tests on an unknown device.

For upstream installation details, see the
[Android `sdkmanager` documentation](https://developer.android.com/tools/sdkmanager) and the
[LunarG Vulkan SDK documentation](https://vulkan.lunarg.com/sdk/home).
