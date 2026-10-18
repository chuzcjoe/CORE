# Direct panorama to cubemap conversion

`core::io::ConvertBitmapToCubeMapFaces` projects an equirectangular panorama
directly into the six face buffers used by the existing cubemap upload path.
It avoids constructing the full vertical-cross image and copying its faces into
another allocation. The two existing conversion functions remain available.

## API and layout

```cpp
#include "IOUtils.h"

// panorama is a populated 2D Bitmap with a width divisible by four.
const core::io::Bitmap cube = core::io::ConvertBitmapToCubeMapFaces(panorama);
```

Input must have a 2:1 aspect ratio, width at least four, 1-4 components, and
`BitmapFormat_Uint8` or `BitmapFormat_Float` storage. Invalid metadata or truncated
pixel storage throws `std::invalid_argument`. Output exceeding Bitmap's current
integer allocation limit throws `std::length_error` before allocation.

Each face is `panorama.width / 4` square. Pixel bytes are packed by face in
`+X, -X, +Y, -Y, +Z, -Z` order. The -Z face retains the existing 180-degree
rotation. Bilinear sampling, clamping and component values match the existing
panorama -> vertical cross -> cube conversion, including Float values outside
the normalized [0, 1] range.

Output metadata follows the existing cube Bitmap convention: `width` and `height`
are one face's dimensions, `type` is `BitmapType_Cube`, and `depth` is six times
the source component count. Faces are consecutive planes, rather than pixels
with six times as many interleaved components. Use `cube.pixel` with a per-face
stride of `cube.width * cube.height * source_components * bytes_per_component`.
The 2D `GetPixel`/`SetPixel` methods are not face-indexed accessors.

## Vulkan example

The macOS cubemap example accepts an optional image path:

```bash
./build/macos/examples/vk_cubemap_demo ./panorama.png
```

It detects a compatible 2:1 image and uses the direct projection. Existing
vertical-cross inputs and the default image path keep their previous behavior.
The example's existing `stbi_load` path loads 8-bit RGBA; Float support belongs
to the conversion API. Assets must be supplied separately as described in the
repository README.

## Memory and benchmark

For face size N and B bytes per pixel, the old pipeline simultaneously holds
12*N*N*B vertical-cross bytes and 6*N*N*B output bytes. The direct pipeline
allocates only the 6*N*N*B output. A 1024x512 RGBA Float panorama therefore avoids
a 12 MiB intermediate pixel buffer (18 MiB -> 6 MiB conversion pixel storage).
These figures exclude the input image, allocator overhead/caching and benchmark
reference images; they are not process RSS measurements.

After a normal build, run:

```bash
./build/macos/tests/stb/io_cubemap_benchmark
# Select one authorized physical device before using adb.
adb push ./build/arm64-v8a/tests/stb/io_cubemap_benchmark /data/local/tmp/core/
adb shell chmod +x /data/local/tmp/core/io_cubemap_benchmark
adb shell /data/local/tmp/core/io_cubemap_benchmark
```

The benchmark alternates the old and direct paths, warms each twice, and records
nine samples per path. It reports min/median/max wall time and checks every output
byte against the two-step result outside the timed region. Timings include output
and intermediate allocation/conversion; input creation and reference checking are
excluded. No timing threshold is used as a unit-test assertion.

Observed medians on 2026-10-18 using the standard core Debug build configuration:

| Platform | Panorama | Old (ms) | Direct (ms) | Intermediate eliminated |
| --- | --- | ---: | ---: | ---: |
| macOS | 512x256 RGBA Uint8 | 2.60 | 2.44 | 768 KiB |
| macOS | 1024x512 RGBA Uint8 | 7.57 | 6.89 | 3 MiB |
| macOS | 1024x512 RGBA Float | 6.66 | 5.92 | 12 MiB |
| Android 2210132C | 512x256 RGBA Uint8 | 7.52 | 7.13 | 768 KiB |
| Android 2210132C | 1024x512 RGBA Uint8 | 30.02 | 28.50 | 3 MiB |
| Android 2210132C | 1024x512 RGBA Float | 30.84 | 28.57 | 12 MiB |

Android used the connected Android 13 / API 33 device and NDK 28.2.13676358.
CPU load and power states affect timing; the eliminated image allocation is the
primary benefit. The new function still performs the same projection work.
