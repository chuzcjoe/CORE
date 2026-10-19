# Strided matrix views and image regions

`core::MatView<T, C>` is a non-owning view of interleaved pixels with `C` channels.
It can address packed images, padded rows, and rectangular regions without
allocating or copying pixels. Include `Mat.h` and link the `mat` CMake target.

## Layout and indexing

The existing three-argument constructor still describes packed rows:

```cpp
core::MatView<float, 3> packed(pixels, height, width);
```

The fourth argument describes the distance between row starts in **T elements**,
including padding. Convert a byte pitch to elements only after checking that it
is divisible by `sizeof(T)` and that the pointer is appropriately aligned.

```cpp
// RGB floats: two pixels per row, followed by two padding elements.
std::array<float, 24> storage{};
core::MatView<float, 3> padded(storage.data(), 3, 2, 8U);
*padded(1, 1, 2) = 0.5f;  // storage[8 + 3 + 2]
```

`row_stride()` returns that distance. `total()` counts logical elements and
excludes padding. A single-row or full-width packed region is contiguous;
`is_contiguous()` identifies whether `data()[0..total())` covers the logical
elements without gaps. For other views, iterate by row/pixel, or copy each row
separately before calling an API that requires packed storage. Passing
`data()` and `total() * sizeof(T)` alone does not describe a strided image.

## Regions and constness

`Subview(row, col, rows, cols)` uses pixel coordinates and retains the parent's
row stride. Views can be nested; writes alias the original storage.

```cpp
core::Mat<float, 3> image(480, 640);
core::MatView<float, 3> region = image.Subview(20, 30, 100, 200);
*region(0, 0, 1) = 1.0f;  // changes image(20, 30, 1)

const core::Mat<float, 3>& read_only = image;
core::MatView<const float, 3> input = read_only.Subview(20, 30, 100, 200);
```

A const parent's subview has const elements even when the returned view is a
mutable variable. A mutable view can also be constructed over const storage by
using `MatView<const T, C>`.

Rows/columns must be nonnegative, stride must cover one logical row, the logical
element count must fit the existing `int total()` API, and the addressed span
must fit `ptrdiff_t` in bytes. Invalid geometry throws `std::invalid_argument` or
`std::length_error`; out-of-parent regions throw `std::out_of_range`. Empty
regions are allowed, have a null data pointer, and retain the parent's stride.
Nonempty subviews require nonnull backing storage. Pixel access still uses
assertions for index bounds, as before.

Views never extend storage lifetime or check its allocation capacity. The caller
must supply storage for `(rows - 1) * row_stride + cols * C` elements for a
nonempty view. A null pointer may describe an unbacked layout (also used while
`Mat` constructs its storage); do not index it. Destroying, moving or assigning
the owner, or releasing/unmapping external memory, can invalidate existing views.
Keep mapped GPU memory mapped for the entire lifetime of host accesses.

`Mat` continues to own packed storage, and its existing copy, move and `clone()`
operations retain that behavior. Views themselves copy only pointer/layout
metadata. Invalid or overflowing dimensions now fail before allocation/indexing
rather than relying on signed integer arithmetic.
The header's object layout gains stride metadata; rebuild compiled clients
instead of mixing old and new `MatView`/`Mat` definitions across a binary boundary.

## Runnable crop example

Build with `./scripts/run.sh -t macos`, then run from the repository root:

```bash
./build/macos/examples/mat_crop_demo input.png tmp/crop.png 30 20 200 100
```

Arguments are input path, output PNG path, x, y, width, height. The example loads
RGBA pixels using stb, takes a const subview, and passes its original row stride
directly to the PNG encoder. There is no separate crop pixel buffer or crop
copy; image decoding and PNG encoding still allocate their own working memory.
This is a feature/resource property, not a measured timing or process-RSS claim.
Output directories must already exist. Empty crops and invalid arguments return
a failure status with a diagnostic.

The same `mat_crop_demo` target builds for Android arm64-v8a. Push that executable
and an input image to an authorized device and use device paths for both files;
it needs no window, APK or device setting changes.
