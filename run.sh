#! /bin/bash
set -e
set -x

usage() {
  cat <<EOF
Usage: $0 [-target macos|arm64-v8a] [-test_module <name>] [-test_filter <Suite.Test>]

Examples:
  $0 -target macos -test_module vulkan -test_filter ComputeGaussianBlur.test

Environment:
  ANDROID_NDK_ROOT (or NDK_ROOT) required when -target android
EOF
}

target=macos
test_module=""
test_filter=""

# Parse args
while [[ $# -gt 0 ]]; do
  case "$1" in
    -h|--help) usage; exit 0 ;;
    -target|--target) target="$2"; shift 2 ;;
    -test_module|--test_module) test_module="$2"; shift 2 ;;
    -test_filter|--test_filter) test_filter="$2"; shift 2 ;;
    *) echo "Unknown arg: $1"; usage; exit 1 ;;
  esac
done

if [ "$target" != "macos" ] && [ "$target" != "arm64-v8a" ] ; then
    echo "target must be macos, arm64-v8a"
    exit 1
fi

# save tmp data
rm -rf ./tmp
mkdir -p ./tmp

rm -rf build/$target
mkdir -p build/$target
cd build/$target

cmake_options=(-DCMAKE_BUILD_TYPE=Debug)

if [ "$target" = "arm64-v8a" ] ; then
    cmake_options+=(-DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake
                    -DANDROID_ABI=$target
                    -DANDROID_PLATFORM=android-34)
fi

cmake "${cmake_options[@]}" ../..
make -j10

cd ../..

if [ "$target" = "macos" ] ; then
    if [ "$test_module" = "vulkan" ]; then
        echo "run vulkan tests"
        if [ -z "$test_filter" ]; then
            # test_filter is empty → run all tests
            ./build/$target/vulkan/tests/vulkan_tests
        else
            # test_filter is not empty → apply filter
            ./build/$target/vulkan/tests/vulkan_tests --gtest_filter="$test_filter"
        fi
    fi

    if [ "$test_module" = "tests" ]; then
        echo "run tests"
        if [ -z "$test_filter" ]; then
            # test_filter is empty → run all tests
            ./build/$target/tests/core-tests
        else
            # test_filter is not empty → apply filter
            ./build/$target/tests/core-tests --gtest_filter="$test_filter"
        fi
    fi
    exit 1
fi

device_path="/data/local/tmp/core"
if [ "$target" = "arm64-v8a" ]; then
    if [ "$test_module" = "vulkan" ]; then
      echo "run vulkan test"
      adb push ./build/$target/vulkan/tests/vulkan_tests $device_path
      adb shell chmod +X $device_path/vulkan_tests
      if [ -z "$test_filter" ]; then
        # test_filter is empty → run all tests
        adb shell $device_path/vulkan_tests
      else
        # test_filter is not empty → apply filter
        adb shell $device_path/vulkan_tests --gtest_filter="$test_filter"
      fi
    fi

    if [ "$test_module" = "tests" ]; then
      echo "run tests"
      adb push ./build/$target/tests/core-tests $device_path
      adb push ./tests/data $device_path
      adb shell chmod +X $device_path/core-tests
      if [ -z "$test_filter" ]; then
        # test_filter is empty → run all tests
        adb shell $device_path/core-tests
      else
        # test_filter is not empty → apply filter
        adb shell $device_path/core-tests --gtest_filter="$test_filter"
      fi

      # pull results from device
      adb pull $device_path/data ./tmp
    fi
fi
