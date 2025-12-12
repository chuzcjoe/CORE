#pragma once

#include "TraceCategory.h"

namespace core {
namespace trace {

class Trace {
 public:
  Trace(const std::string& trace_file);

  void InitializeTracing();
  void StartTracing();
  void StopTracing();

 private:
  std::unique_ptr<perfetto::TracingSession> tracing_session_;
  std::string trace_file_;
};

}  // namespace trace
}  // namespace core