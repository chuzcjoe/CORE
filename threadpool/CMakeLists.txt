add_library(threadpool INTERFACE)

target_include_directories(threadpool
    INTERFACE
    .)
target_link_libraries(threadpool INTERFACE core_error)
