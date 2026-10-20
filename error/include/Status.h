#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <string>
#include <utility>

namespace core {

enum class ErrorCode {
  kOk,
  kInvalidArgument,
  kInvalidState,
  kNotFound,
  kUnsupported,
  kOutOfRange,
  kIo,
  kBackend,
};

class [[nodiscard]] Status {
 public:
  Status() = default;
  explicit Status(ErrorCode code, std::string message, std::string backend = {},
                  std::optional<std::int64_t> native_code = std::nullopt)
      : code_(code),
        message_(std::move(message)),
        backend_(std::move(backend)),
        native_code_(native_code) {}

  bool ok() const { return code_ == ErrorCode::kOk; }
  ErrorCode code() const { return code_; }
  const std::string& message() const { return message_; }
  const std::string& backend() const { return backend_; }
  std::optional<std::int64_t> native_code() const { return native_code_; }

 private:
  ErrorCode code_ = ErrorCode::kOk;
  std::string message_;
  std::string backend_;
  std::optional<std::int64_t> native_code_;
};

template <typename T>
class [[nodiscard]] Result {
 public:
  explicit Result(T value) : value_(std::move(value)) {}
  explicit Result(Status status) : status_(std::move(status)) {
    if (status_.ok()) {
      std::fputs("Result requires a value or a failure status\n", stderr);
      std::abort();
    }
  }

  bool ok() const { return value_.has_value(); }
  const Status& status() const { return status_; }

  T& operator*() & { return Value(); }
  const T& operator*() const& { return Value(); }
  T&& operator*() && { return std::move(Value()); }
  T* operator->() { return &Value(); }
  const T* operator->() const { return &Value(); }

 private:
  T& Value() {
    CheckValue();
    return *value_;
  }
  const T& Value() const {
    CheckValue();
    return *value_;
  }
  void CheckValue() const {
    if (!ok()) {
      std::fprintf(stderr, "Accessing failed Result: %s\n",
                   status_.message().c_str());
      std::abort();
    }
  }

  std::optional<T> value_;
  Status status_;
};

}  // namespace core
