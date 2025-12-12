#include <gtest/gtest.h>

#include <chrono>
#include <iostream>
#include <thread>

#include "Trace.h"

void DrawPlayer(int player_number) {
  TRACE_EVENT("rendering", "DrawPlayer", "player_number", player_number);
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

void DrawGame() {
  TRACE_EVENT_BEGIN("rendering", "DrawGame");
  DrawPlayer(1);
  DrawPlayer(2);
  TRACE_EVENT_END("rendering");

  // Record the rendering framerate as a counter sample.
  TRACE_COUNTER("rendering", "Framerate", 120);
}

TEST(Perfetto, test1) {
  core::trace::Trace trace("test1.perf");
  trace.InitializeTracing();
  trace.StartTracing();

  // Give a custom name for the traced process.
  perfetto::ProcessTrack process_track = perfetto::ProcessTrack::Current();
  perfetto::protos::gen::TrackDescriptor desc = process_track.Serialize();
  desc.mutable_process()->set_process_name("test1_process");
  perfetto::TrackEvent::SetTrackDescriptor(process_track, desc);

  TRACE_EVENT_INSTANT("rendering", "Event1");

  // Sleep to simulate a long computation.
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Simulate some work that emits trace events.
  DrawGame();

  // Sleep to simulate a long computation.
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  TRACE_EVENT_INSTANT("rendering", "Event2");

  trace.StopTracing();
}