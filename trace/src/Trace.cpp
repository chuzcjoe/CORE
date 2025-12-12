#include "Trace.h"

#include <fstream>

namespace core {
namespace trace {

Trace::Trace(const std::string& trace_file) : trace_file_(trace_file) {}

void Trace::InitializeTracing() {
  perfetto::TracingInitArgs args;
  args.backends = perfetto::kInProcessBackend;
  perfetto::Tracing::Initialize(args);
  perfetto::TrackEvent::Register();
}

void Trace::StartTracing() {
  perfetto::TraceConfig cfg;
  cfg.add_buffers()->set_size_kb(1024);
  auto* ds_cfg = cfg.add_data_sources()->mutable_config();
  ds_cfg->set_name("track_event");
  perfetto::protos::gen::TrackEventConfig te_cfg;
  te_cfg.add_disabled_categories("*");
  te_cfg.add_enabled_categories("rendering");
  ds_cfg->set_track_event_config_raw(te_cfg.SerializeAsString());

  tracing_session_ = perfetto::Tracing::NewTrace();
  tracing_session_->Setup(cfg);
  tracing_session_->StartBlocking();
}

void Trace::SetTraceProcess(const std::string& process_name) {
  perfetto::ProcessTrack process_track = perfetto::ProcessTrack::Current();
  perfetto::protos::gen::TrackDescriptor desc = process_track.Serialize();
  desc.mutable_process()->set_process_name(process_name);
  perfetto::TrackEvent::SetTrackDescriptor(process_track, desc);
}

void Trace::StopTracing() {
  // Make sure the last event is closed for this example.
  perfetto::TrackEvent::Flush();

  // Stop tracing and read the trace data.
  tracing_session_->StopBlocking();
  std::vector<char> trace_data(tracing_session_->ReadTraceBlocking());

  // Write the result into a file.
  // Note: To save memory with longer traces, you can tell Perfetto to write
  // directly into a file by passing a file descriptor into Setup() above.
  std::ofstream output;
  output.open(trace_file_.c_str(), std::ios::out | std::ios::binary);
  output.write(&trace_data[0], std::streamsize(trace_data.size()));
  output.close();
  PERFETTO_LOG("Trace written in %s file.", trace_file_.c_str());
}

}  // namespace trace
}  // namespace core