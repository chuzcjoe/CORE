file(GLOB metal_src "src/*.cpp")

# Avoid colliding with external/metal_cpp target named "metal".
add_library(core_metal ${metal_src})

target_include_directories(core_metal
    PUBLIC
    include)

if(APPLE)
    # Link against the metal-cpp headers (INTERFACE target named "metal")
    # and required Apple frameworks so dependents get them transitively.
    find_library(METAL_FRAMEWORK Metal)
    find_library(FOUNDATION_FRAMEWORK Foundation)
    find_library(QUARTZCORE_FRAMEWORK QuartzCore)
    find_library(COCOA_FRAMEWORK Cocoa)

    target_link_libraries(core_metal
        PUBLIC
        metal_cpp         # external/metal_cpp INTERFACE target
        ${METAL_FRAMEWORK}
        ${FOUNDATION_FRAMEWORK}
        ${QUARTZCORE_FRAMEWORK}
        ${COCOA_FRAMEWORK}
        mat
        glm
        glfw
        stb)
endif()
