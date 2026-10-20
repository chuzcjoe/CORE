add_executable(error_tests StatusTest.cpp)
target_link_libraries(error_tests PRIVATE gtest_main core_error)
