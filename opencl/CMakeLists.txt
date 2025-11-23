file(GLOB opencl_src "src/*.cpp")

add_library(opencl ${opencl_src})

target_include_directories(opencl
    PUBLIC
    include)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(opencl opencl-headers dl)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
    target_link_libraries(opencl opencl-headers dl)
endif()