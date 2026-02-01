#!/bin/bash
set -e

while [ "$#" -gt 0 ]; do
  case "$1" in
    -app)
      APP_NAME="$2"
      shift 2
      ;;
    *)
      echo "Unknown option: $1"
      exit 1
      ;;
  esac
done

if [ -z "$APP_NAME" ]; then
  echo "Usage: $0 -app <AppName>"
  echo "  <AppName> is the name of the directory in apps/android/"
  echo "Example: $0 -app BasicApp"
  exit 1
fi

APK_NAME="core_${APP_NAME}.apk"
MANIFEST_PATH="apps/android/$APP_NAME/AndroidManifest.xml"

if [ ! -f "$MANIFEST_PATH" ]; then
    echo "Error: AndroidManifest.xml not found for app '$APP_NAME' at $MANIFEST_PATH"
    exit 1
fi

LIB_NAME=$(grep 'android:value' "$MANIFEST_PATH" | sed -e 's/.*android:value="//' -e 's/".*//')
SO_NAME="lib${LIB_NAME}.so"

if [ ! -f "debug.keystore" ]; then
    keytool -genkeypair \
      -keystore debug.keystore \
      -storepass android \
      -keypass android \
      -alias androiddebugkey \
      -keyalg RSA \
      -keysize 2048 \
      -validity 10000 \
      -dname "CN=Android Debug,O=Android,C=US"
fi

mkdir -p build_apps
cd build_apps

cmake ../apps/android/$APP_NAME \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-34

make -j10

$ANDROID_SDK_ROOT/build-tools/34.0.0/aapt package -f \
  -M ../apps/android/$APP_NAME/AndroidManifest.xml \
  -I $ANDROID_SDK_ROOT/platforms/android-33/android.jar \
  -F $APK_NAME

mkdir -p lib/arm64-v8a
cp $SO_NAME lib/arm64-v8a/
$ANDROID_SDK_ROOT/build-tools/34.0.0/aapt add $APK_NAME lib/arm64-v8a/$SO_NAME

$ANDROID_SDK_ROOT/build-tools/34.0.0/apksigner sign \
  --ks ../debug.keystore \
  --ks-pass pass:android \
  $APK_NAME
adb install -r $APK_NAME
