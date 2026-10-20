# Batched Vulkan buffer transfers

`VulkanBuffer::CopyBuffers` copies a sequence of buffer ranges using one command
buffer, one queue submission and one fence wait. It is useful when uploading
related vertex, index and instance buffers together. The Galaxy demo uses it for
its three initial uploads. The existing synchronous `CopyToBuffer` API is unchanged.

## Usage

Create sources with `VK_BUFFER_USAGE_TRANSFER_SRC_BIT` and destinations with
`VK_BUFFER_USAGE_TRANSFER_DST_BIT`, using the same initialized `VulkanContext`.
Fill or flush source memory before calling the function.

```cpp
std::array<core::vulkan::BatchBufferCopy, 2> uploads{};
uploads[0].source = &vertex_staging;
uploads[0].destination = &vertex_device;
uploads[1].source = &index_staging;
uploads[1].destination = &index_device;
core::vulkan::VulkanBuffer::CopyBuffers(uploads);
```

Each entry defaults to zero offsets and `VK_WHOLE_SIZE`, which copies the source's
remaining bytes. Set `source_offset`, `destination_offset` and `size` for partial
updates. Source and destination sizes may differ if the requested range fits both.
Offsets and sizes are in bytes. Zero-length copies and overlapping source/destination
ranges within one self-copy are rejected; non-overlapping self-copies are supported.

The entire batch is validated before recording. Missing or uninitialized buffers,
mixed contexts and incorrect usage flags throw `std::invalid_argument`; invalid
ranges throw `std::out_of_range`. An empty batch does nothing. A rejected batch
does not execute an earlier valid entry.

Copies execute in array order. Transfer barriers support dependencies such as
staging -> device -> readback within one batch and overlapping writes in separate
entries. The final barrier makes writes visible to later commands on the selected
context queue and to host reads after the fence wait. Mapping/invalidation rules
for non-coherent memory still apply. The existing `MapData` requires host-visible,
host-coherent buffers.

The call is synchronous: retain every buffer and its context until it returns,
and externally synchronize access to that context's queue. This API uses the
context's existing compute or graphics queue; cross-queue ownership transfers,
asynchronous lifetime management and image uploads are outside its scope.

## Repeated uploads

For streaming or recurring partial updates, keep a `VulkanBufferTransfer` instead
of creating submission resources on every `VulkanBuffer::CopyBuffers` call:

```cpp
#include "VulkanBufferTransfer.h"

core::vulkan::VulkanBufferTransfer transfer(context);
// On each update, fill staging memory and reuse the same session:
transfer.CopyBuffers(uploads);
```

The session owns one command pool, command buffer and fence. Each non-empty call
validates the entire batch, resets and records its command buffer, submits once,
and waits for its fence before returning. The initialized context must outlive
the session. Buffers and copy descriptors only need to survive the call; later
calls may use different buffers, ranges and batch lengths. Empty calls do nothing.
Validation exceptions leave the session reusable. Vulkan errors propagate;
recovery from device loss is outside this API's scope.

This is synchronous reuse: the existing copy order, barriers, range checks and
host visibility remain the same. The session is neither copyable nor movable.
Externally synchronize both the session and all access to its context queue.
It does not retain ownership of buffers or make concurrent queue use safe.
The existing static API delegates to a temporary session and remains available
for occasional uploads; no caller migration is required.

For N non-empty batches, retaining one session reduces command-pool creation,
command-buffer allocation and fence creation from N of each to one of each.
The region vector also retains capacity for the largest batch seen. This removes
repeated resource creation, rather than reducing the number of GPU copies,
submissions or waits. It retains these resources until session destruction.
A global cache would add shared mutable state and context-lifetime coupling;
an asynchronous uploader would require in-flight buffer ownership. Explicit
synchronous sessions avoid both while allowing callers to select reuse lifetime.

Command-buffer reset happens after the preceding fence wait, as required by the
[Vulkan command-buffer lifecycle](https://docs.vulkan.org/spec/latest/chapters/cmdbuffers.html#commandbuffers-lifecycle).
No buffers remain in flight after a successful call.

Run the headless streaming example from the repository root:

```bash
./build/macos/examples/vk_stream_upload_demo
adb push ./build/arm64-v8a/examples/vk_stream_upload_demo /data/local/tmp/core/
adb shell chmod +x /data/local/tmp/core/vk_stream_upload_demo
adb shell /data/local/tmp/core/vk_stream_upload_demo
```

It uploads 60 distinct 16 KiB frames through device-local memory and checks every
readback word, reusing one session and the same three buffers throughout.

### Reuse benchmark

Run `build/macos/tests/vulkan/vulkan_repeated_transfer_benchmark`, or push and run
the corresponding `build/arm64-v8a/tests/vulkan/` executable on the selected device.
It compares a fresh session per batch (the existing static API) with one retained
session, using the same buffers, barriers, data and toolchain. Both paths get five
warmups and 31 samples in alternating order. Each call transfers new data; every
output word is verified after every call, outside timing.

The benchmark reports wall time and calling-thread CPU time, each with
p10/median/p90 in microseconds. Timed work includes range validation, recording,
submission and waiting; the fresh-session path includes resource creation and
destruction. Retained-session setup, buffer allocation, filling and readback are
excluded. CPU time excludes blocked time and driver work on other threads; it is
not total process CPU time or GPU time. Both contexts request validation layers;
actual layer availability depends on the installed runtime. No timing assertion
is used in unit tests.

Observed on 2026-10-20 with `run.sh` core builds (actual `-O3 -DNDEBUG` flags),
Vulkan SDK 1.4.350.0 / MoltenVK 1.4.1 on macOS, and existing NDK 28.2.13676358
on physical 2210132C / Android 13 / API 33. First invocation medians in us:

| GPU | Batch | Fresh wall | Reused wall | Fresh caller CPU | Reused caller CPU |
| --- | --- | ---: | ---: | ---: | ---: |
| Apple M6 | 1 x 4 KiB | 644.42 | 607.04 | 48.75 | 28.58 |
| Apple M6 | 3 x 4 KiB | 411.71 | 360.21 | 66.75 | 44.92 |
| Apple M6 | 16 x 64 KiB | 295.17 | 278.67 | 64.00 | 52.58 |
| Apple M6 | 16 x 1 MiB | 418.38 | 414.08 | 58.54 | 48.17 |
| Adreno 740 | 1 x 4 KiB | 1768.70 | 941.98 | 1144.22 | 317.45 |
| Adreno 740 | 3 x 4 KiB | 1898.65 | 1030.83 | 1230.83 | 345.26 |
| Adreno 740 | 16 x 64 KiB | 3155.83 | 2132.19 | 1648.07 | 617.45 |
| Adreno 740 | 16 x 1 MiB | 4957.71 | 4540.42 | 963.28 | 397.34 |

A second complete invocation confirmed the caller CPU reduction on both devices.
For 3 x 4 KiB, second-run medians were 27.92 -> 18.46 us CPU and
190.08 -> 179.42 us wall on macOS; 1254.06 -> 358.80 us CPU and
1928.07 -> 1021.88 us wall on Android. Android small-batch wall-time ranges were
well separated; macOS wall-time ranges overlapped, and its 1 x 4 KiB median
regressed in the second invocation. Large-batch wall-time ranges also overlapped
on Android. These results support reduced caller CPU overhead and resource churn,
not a general macOS latency improvement, GPU bandwidth or rendering FPS claim.
CPU frequency, thermal state, scheduling and driver worker threads were not
controlled. Every output word passed in both invocations on both platforms.

## Benchmark

After the normal build, run from the repository root:

```bash
./build/macos/tests/vulkan/vulkan_transfer_benchmark
# Android: use one explicitly selected, authorized physical device.
adb push ./build/arm64-v8a/tests/vulkan/vulkan_transfer_benchmark /data/local/tmp/core/
adb shell chmod +x /data/local/tmp/core/vulkan_transfer_benchmark
adb shell /data/local/tmp/core/vulkan_transfer_benchmark
```

The executable compares the unchanged per-buffer `CopyToBuffer` loop with the
batch API on the same buffers/device. Each case warms up both paths five times,
then collects 31 samples per path in alternating order and reports p10/median/p90.
It verifies every output word for both paths outside the timed interval.
Wall time includes command recording, pool/fence allocation, submission and waiting;
buffer allocation, filling and readback verification are excluded. Validation is
requested through the normal context configuration; its availability depends on
the installed runtime. No timing threshold is used as a unit-test assertion.

Observed on 2026-10-17, Debug build, Vulkan SDK 1.4.350.0 on macOS and existing
NDK 28.2.13676358 on Android 13 / API 33 (2210132C):

| GPU | Workload | Legacy median (us) | Batch median (us) | Ratio |
| --- | --- | ---: | ---: | ---: |
| Apple M6 / MoltenVK | 3 x 4 KiB | 1466.71 | 508.04 | 2.89x |
| Apple M6 / MoltenVK | 16 x 64 KiB | 4146.25 | 529.46 | 7.83x |
| Apple M6 / MoltenVK | 16 x 1 MiB | 2402.17 | 414.00 | 5.80x |
| Adreno 740 | 3 x 4 KiB | 3686.04 | 1263.18 | 2.92x |
| Adreno 740 | 16 x 64 KiB | 26469.95 | 2146.04 | 12.33x |
| Adreno 740 | 16 x 1 MiB | 31011.93 | 5910.62 | 5.25x |

These are end-to-end transfer-call measurements, not GPU bandwidth or frame-rate
claims. They include the reduction from N submissions/waits/pool allocations to
one of each. Different drivers, validation settings, workloads and power states
can change the results. Buffers already remain allocated in both paths; this does
not measure a general GPU memory allocator or rendering startup as a whole.
