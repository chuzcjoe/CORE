file(GLOB trace_src "src/*.cpp")

add_library(trace ${trace_src})

target_include_directories(trace
    PUBLIC
    include)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(trace
        PUBLIC
        perfetto)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
    target_link_libraries(trace
        PUBLIC
        perfetto
        log)
endif()