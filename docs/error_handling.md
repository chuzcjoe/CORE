# Checked error handling

First-party code reports recoverable failures through `core::Status` and
`core::Result<T>`, provided by the header-only `core_error` CMake target. Link that
target publicly when an exported header uses these types.

`Status()` means success. A failure contains an `ErrorCode`, a diagnostic message,
and, when available, the backend name and native numeric code. Inspect the category
or native code when choosing recovery behavior; messages are for diagnostics.

```cpp
core::Status Upload(core::vulkan::VulkanBuffer& source,
                    core::vulkan::VulkanBuffer& destination) {
  const core::Status status = source.CopyToBuffer(destination);
  if (!status.ok()) {
    return status;
  }
  return core::Status();
}
```

Use `Status` for operations without a value and `Result<T>` for value-producing
operations. Both types are `[[nodiscard]]`. Check `ok()` before dereferencing a
result; invalid access terminates with a diagnostic even in release builds.
Result construction is explicit, and a success status alone cannot construct a
result without a value. Move-only values, including resource owners, are supported.
Return failures to the caller and log once at the application boundary.

## Resource creation and lifetime

Fallible concrete resources expose `Create(...)`, returning
`Result<std::unique_ptr<T>>`. Constructors used by inheritance or resource members
only establish inert state or store configuration. Their `Init(...)` methods return
`Status`; callers must check initialization before using the object.

```cpp
auto context_result = core::opencl::CLContext::Create();
if (!context_result.ok()) {
  return context_result.status();
}
std::unique_ptr<core::opencl::CLContext> context = std::move(*context_result);
auto buffer_result = core::opencl::CLBuffer::Create(context.get(), 4096);
if (!buffer_result.ok()) {
  return buffer_result.status();
}
std::unique_ptr<core::opencl::CLBuffer> buffer = std::move(*buffer_result);
```

Initialization is single-use, including failed attempts. Recreate the object to
retry. Failed initialization leaves objects destructible and factories release
partially acquired resources before returning. Keep contexts/devices alive until
their dependent resources are destroyed. Runtime operations can have backend side
effects before reporting failure; only documented validation guarantees precede
all side effects.

Vulkan retains its staged lifecycle: `VulkanContext::Create(...)` creates an
instance, then `Init(surface)` selects and initializes a device after the caller
creates its surface. Compute contexts call `Init()` without a surface. Swapchains
borrow their surfaces; callers destroy surfaces after swapchains and before the
instance. Wait for outstanding GPU work before releasing resources on early exits.

## Thread pool

`ThreadPool::Create(count)` rejects zero workers. `submit(...)` returns
`Result<std::future<R>>`; a failed result means submission was rejected, while a
successful result contains the completion future. Shutdown drains accepted work
and rejects subsequent submissions. Tasks with recoverable failures return
`Status` or `Result<T>` themselves, producing futures of those types.

## Boundaries and verification

This refactor retains C++20 and compiler exception support. Allocation, thread
startup, futures, and third-party code can still raise standard-library or external
exceptions. CORE does not translate those failures or promise that its entire
process can never raise an exception. Operational methods are therefore not
blanket-marked `noexcept`.

Run `python3 scripts/check_no_exceptions.py` to check first-party C++ sources for
exception syntax and exception assertion macros. It ignores comments, literals,
generated shader headers, build outputs, and third-party code. Both platform CI
workflows run the check. `scripts/run.sh -t macos -r tests` includes the shared error
type tests, resource rejection/cleanup tests, and thread-pool result tests.
