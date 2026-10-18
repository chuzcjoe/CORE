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
std::array<core::vulkan::BufferCopy, 2> uploads{};
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
