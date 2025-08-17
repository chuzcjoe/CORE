set(GTEST_SOURCES src/gtest_main.cc src/gtest-all.cc)

add_library(gtest ${GTEST_SOURCES})

target_include_directories(gtest
    SYSTEM PUBLIC
    include
    .)