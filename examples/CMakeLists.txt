# only add glfw for macos
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    add_executable(glfw-demo glfw.cpp)
    target_link_libraries(glfw-demo PRIVATE glfw)
endif()
