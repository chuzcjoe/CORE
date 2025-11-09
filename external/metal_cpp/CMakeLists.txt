add_library(metal_cpp INTERFACE)

target_include_directories(metal_cpp
    INTERFACE
    .
    )

# Suppress Clang diagnostics for GNU anonymous structs/unions used by metal-cpp.
# These headers intentionally use anonymous aggregates; with -Werror enabled,
# Clang/AppleClang will fail without disabling these specific warnings.
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(metal_cpp
        INTERFACE
        -Wno-gnu-anonymous-struct
        -Wno-nested-anon-types
    )
endif()
