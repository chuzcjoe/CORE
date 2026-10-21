# Vertical-cross cubemap extraction

`core::io::ConvertVerticalCrossToCubeMapFaces(cross)` extracts six packed faces
from the existing vertical-cross bitmap layout. The Vulkan `vk_cubemap_demo`
already calls this function before uploading its cubemap; existing callers
receive the faster copy path without changing their code.

```cpp
#include "IOUtils.h"

const core::io::Bitmap cube = core::io::ConvertVerticalCrossToCubeMapFaces(cross);
```

The returned face order is +X, -X, +Y, -Y, +Z, -Z. The -Z tile is rotated by
180 degrees. Output metadata retains the existing convention: `width` and
`height` describe one face, `depth` is six times the source component count,
and `type` is `BitmapType_Cube`. Uint8 and Float components are copied as raw
bytes, preserving their exact representation without conversion or resampling.
The input remains unchanged and the output owns its storage.

Five faces now use one `memcpy` per row instead of one per pixel. The -Z face
still reverses complete pixels within each reversed row. For face dimensions
W x H, the number of copy calls falls from `6*W*H` to `5*H + W*H`; the output
allocation and number of copied bytes stay the same. A 1024-square cube uses
1,053,696 copy calls instead of 6,291,456. Parallel scheduling and specialized
SIMD reversal would add complexity beyond this contiguous-row improvement.

The API and geometry are unchanged. Face dimensions use integer division
(`source.width/3`, `source.height/4`), including existing rectangular and
remainder behavior; -Z remains anchored to the source's bottom edge. Empty
face dimensions produce empty storage. Callers must still provide valid,
adequately sized bitmap storage. This change adds no malformed-input contract.

## Benchmark

After building with `./scripts/run.sh -t macos -r tests`, run:

```bash
./build/macos/tests/stb/io_cubemap_copy_benchmark
```

The same target builds for Android. After checking the authorized device and
setting `ANDROID_SERIAL`, push and execute it:

```bash
adb push build/arm64-v8a/tests/stb/io_cubemap_copy_benchmark /data/local/tmp/core/
adb shell chmod +x /data/local/tmp/core/io_cubemap_copy_benchmark
adb shell /data/local/tmp/core/io_cubemap_copy_benchmark
```

Each case uses deterministic, nonuniform source bytes, five warmups and 31
samples. A scalar coordinate oracle checks every output byte after every
conversion, outside the timer. Reported p10/median/p90 are sorted samples
3/15/27 (zero-based). Timing includes allocation, zero initialization and face
copying, and excludes source/oracle generation, output verification and output
destruction. Float input includes arbitrary bit patterns and is never interpreted.

On 2026-10-21, baseline `47d029bfe95e58a3c01a61b2b846ba7bd6731492` and
the row-copy implementation were built with the same benchmark and core flags
(`-O3 -DNDEBUG`, warnings as errors), toolchains and devices. Baseline measurements
were collected before modifying production code; saved baseline executables
were measured again after implementation. A final paired comparison also rebuilt
the baseline source with generated CMake flags and linked it to the exact final
benchmark object after benchmark-only formatting changes. To reproduce, build
this benchmark source/target against that baseline and the changed library in
separate checkouts using the same environment. Final paired medians in us:

| Face size | Components/storage | Apple M6 old -> row copy | Android old -> row copy |
| --- | --- | ---: | ---: |
| 64 | RGBA Uint8 | 161.67 -> 8.25 | 74.11 -> 26.35 |
| 256 | RGBA Uint8 | 1067.96 -> 125.17 | 1172.76 -> 458.07 |
| 1024 | RGBA Uint8 | 9489.17 -> 2362.46 | 18455.21 -> 8303.39 |
| 256 | RGB Uint8 | 503.33 -> 104.04 | 983.85 -> 287.50 |
| 256 | RGBA Float | 605.33 -> 194.92 | 1481.77 -> 1134.43 |
| 1024 | RGBA Float | 10574.21 -> 5199.25 | 27488.23 -> 23026.88 |

For 1024 RGBA Uint8, final paired p10/median/p90 in us:

- M6 baseline: 9443.12/9489.17/9626.88; row copy: 2315.08/2362.46/2449.96.
- Android baseline: 18264.17/18455.21/18651.41; row copy: 8136.25/8303.39/8489.11.

Earlier independent comparisons also improved 1024 RGBA Uint8: medians were
9451.21 -> 2359.42 and 9473.67 -> 2336.96 us on M6; 19792.97 -> 8399.69
and 18342.92 -> 8311.77 us on Android. Small-input timings varied substantially:
64 RGBA Uint8 row-copy medians ranged from 8.25 to 56.17 us on M6 and 26.30 to
82.66 us on Android, including an Android cross-run regression against the
74.17-us initial baseline. No universal small-input speedup is claimed. Power,
thermal state, CPU frequency and scheduling were uncontrolled.
Android is the physical 2210132C/nuwa, Android 13/API 33, built with existing
NDK 28.2.13676358/r28c for API 34. macOS used Apple Clang 21.0.0 / SDK 27.0.
No build ran concurrently with measurements. These are warmed CPU conversion
measurements on two devices, with unchanged storage use; no image-decode, GPU
upload, rendering/FPS, cold-start or process-memory improvement is claimed.
