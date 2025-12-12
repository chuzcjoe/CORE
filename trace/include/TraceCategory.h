#pragma once

#include <perfetto.h>

PERFETTO_DEFINE_CATEGORIES(
    perfetto::Category("rendering").SetDescription("Rendering and graphics events"),
    perfetto::Category("network.debug").SetTags("debug").SetDescription("Verbose network events"),
    perfetto::Category("audio.latency")
        .SetTags("verbose")
        .SetDescription("Detailed audio latency metrics"));