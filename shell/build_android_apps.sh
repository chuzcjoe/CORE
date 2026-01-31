keytool -genkeypair \
  -keystore debug.keystore \
  -storepass android \
  -keypass android \
  -alias androiddebugkey \
  -keyalg RSA \
  -keysize 2048 \
  -validity 10000 \
  -dname "CN=Android Debug,O=Android,C=US"

rm -rf build_apps
mkdir build_apps
cd build_apps
mv ../debug.keystore .
cmake ../apps/android/BasicApp \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-34

make -j10

$ANDROID_SDK_ROOT/build-tools/34.0.0/aapt package -f \
  -M ../apps/android/BasicApp/AndroidManifest.xml \
  -I $ANDROID_SDK_ROOT/platforms/android-33/android.jar \
  -F core_basic_app.apk

$ANDROID_SDK_ROOT/build-tools/34.0.0/aapt add core_basic_app.apk libnative_app.so

$ANDROID_SDK_ROOT/build-tools/34.0.0/apksigner sign \
  --ks debug.keystore \
  --ks-pass pass:android \
  core_basic_app.apk
adb install core_basic_app.apk