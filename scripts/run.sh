#! /bin/bash
set -e
set -x

usage() {
  cat <<EOF
Usage: $0 [-t macos|arm64-v8a] [-r vulkan|tests] [-enable_trace 0|1]

Examples:
  $0 -t macos -r vulkan
  $0 -t arm64-v8a -r tests

Environment:
  ANDROID_NDK_ROOT required when -t arm64-v8a
EOF
}

target=macos
run_module=""
enable_trace=0 # Disable tracing by default

device_path="/data/local/tmp/core"

# Parse args
while [[ $# -gt 0 ]]; do
  case "$1" in
    -h|--help) usage; exit 0 ;;
    -t|--target) target="$2"; shift 2 ;;
    -r|--run) run_module="$2"; shift 2 ;;
    -enable_trace|--enable_trace) enable_trace="$2"; shift 2 ;;
    *) echo "Unknown arg: $1"; usage; exit 1 ;;
  esac
done

if [ "$target" != "macos" ] && [ "$target" != "arm64-v8a" ] ; then
    echo "target must be macos, arm64-v8a"
    exit 1
fi

if [ -n "$run_module" ] && [ "$run_module" != "vulkan" ] && [ "$run_module" != "tests" ]; then
    echo "module must be vulkan or tests"
    exit 1
fi

# save tmp data
rm -rf ./tmp
mkdir -p ./tmp

rm -rf build/$target
mkdir -p build/$target
cd build/$target

cmake_options=(-DCMAKE_BUILD_TYPE=Debug
               -DBUILD_VARIANT=core)
if [ "$enable_trace" = "1" ]; then
    cmake_options+=(-DENABLE_TRACE=1)
fi

if [ "$target" = "arm64-v8a" ] ; then
    cmake_options+=(-DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake
                    -DANDROID_ABI=$target
                    -DANDROID_PLATFORM=android-34
                    -DPIPELINE_CACHE_DIR="$device_path")
elif [ "$target" = "macos" ]; then
  project_root="$(cd ../.. && pwd)"
  echo "project root: $project_root"
  cmake_options+=(-DPIPELINE_CACHE_DIR="$project_root")
fi

cmake "${cmake_options[@]}" ../..
make -j10

cd ../..

if [ "$target" = "macos" ]; then
    if [ "$run_module" = "vulkan" ]; then
        echo "run vulkan tests"
        ./build/"$target"/vulkan/tests/vulkan_tests
    elif [ "$run_module" = "tests" ]; then
        echo "run tests"
        ./build/"$target"/tests/core-tests
    fi
elif [ "$target" = "arm64-v8a" ]; then
    if [ "$run_module" = "vulkan" ]; then
        echo "run vulkan tests"
        adb push ./build/"$target"/vulkan/tests/vulkan_tests "$device_path"
        adb shell chmod +x "$device_path/vulkan_tests"
        adb shell "$device_path/vulkan_tests"
    elif [ "$run_module" = "tests" ]; then
        echo "run tests"
        adb shell mkdir -p "$device_path/tests"
        adb push ./build/"$target"/tests/core-tests "$device_path"
        adb push ./tests/data "$device_path/tests"
        adb push ./tests/shaders "$device_path/tests"
        adb shell chmod +x "$device_path/core-tests"
        adb shell "$device_path/core-tests"

        # Pull results from device.
        adb pull "$device_path/data" ./tmp
    fi
fi
