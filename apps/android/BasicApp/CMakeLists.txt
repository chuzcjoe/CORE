cmake_minimum_required(VERSION 3.10)
project(native_gles_app)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(native_app SHARED
    src/main.cpp
)

find_library(log-lib log)
find_library(android-lib android)
find_library(egl-lib EGL)
find_library(glesv3-lib GLESv3)

target_link_libraries(native_app
    ${log-lib}
    ${android-lib}
    ${egl-lib}
    ${glesv3-lib}
)