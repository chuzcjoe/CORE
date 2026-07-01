set(GTEST_SOURCES
    googletest/googletest/src/gtest_main.cc
    googletest/googletest/src/gtest-all.cc)

add_library(gtest ${GTEST_SOURCES})

target_include_directories(gtest
    SYSTEM PUBLIC
    googletest/googletest/include
    googletest/googletest)
