add_library(perfetto STATIC perfetto.cc)
target_include_directories(perfetto
    SYSTEM PUBLIC
    .)