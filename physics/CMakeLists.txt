cmake_minimum_required(VERSION 3.16)
project(delta-physics CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Platform detection ────────────────────────────────────────────────────────
if(WIN32)
    add_compile_definitions(PLATFORM_WINDOWS=1 PLATFORM_LINUX=0 PLATFORM_MACOS=0)
elseif(UNIX AND NOT APPLE)
    add_compile_definitions(PLATFORM_WINDOWS=0 PLATFORM_LINUX=1 PLATFORM_MACOS=0)
elseif(APPLE)
    add_compile_definitions(PLATFORM_WINDOWS=0 PLATFORM_LINUX=0 PLATFORM_MACOS=1)
endif()

# ── MSVC shims / compiler flags ───────────────────────────────────────────────
if(MSVC)
    add_compile_definitions(COMPILER_MSVC=1)
    add_compile_options(/W3)
else()
    add_compile_definitions(COMPILER_MSVC=0)
    # Suppress warnings expected in cross-platform migration
    add_compile_options(
        -Wall -Wextra -Wno-unused-parameter
        -Wno-missing-field-initializers
    )
    # SIMD: __m128 layout — use intrinsics, not MSVC member names
    add_compile_options(-msse4.1)
endif()

# ── Find SDL2 (required on Linux / macOS) ────────────────────────────────────
if(NOT WIN32)
    find_package(SDL2 REQUIRED)
    find_package(OpenGL REQUIRED)

    # Some distros expose SDL2 via pkg-config only
    if(NOT SDL2_FOUND)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(SDL2 REQUIRED sdl2)
    endif()
endif()

# ── delta-physics library (original structure) ────────────────────────────────
add_library(delta-physics
    include/collision_detector.h
    include/collision_geometry.h
    include/collision_object.h
    include/collision_primitives.h
    include/delta_core.h
    include/delta_physics.h
    include/expanding_spring.h
    include/force_generator.h
    include/grid_partition_system.h
    include/hinge_link.h
    include/ledge_link.h
    include/mass_spring_system.h
    include/particle.h
    include/particle_system.h
    include/rigid_body.h
    include/rigid_body_link.h
    include/rigid_body_system.h
    include/spring_link.h

    src/collision_detector.cpp
    src/collision_geometry.cpp
    src/collision_object.cpp
    src/collision_primitives.cpp
    src/expanding_spring.cpp
    src/force_generator.cpp
    src/grid_partition_system.cpp
    src/hinge_link.cpp
    src/ledge_link.cpp
    src/mass_spring_system.cpp
    src/particle.cpp
    src/particle_system.cpp
    src/rigid_body.cpp
    src/rigid_body_link.cpp
    src/rigid_body_system.cpp
    src/spring_link.cpp
)

target_include_directories(delta-physics
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# ── Cross-platform dependencies ──────────────────────────────────────────────
if(NOT WIN32)
    target_link_libraries(delta-physics
        PUBLIC
            pthread
    )
    # Linux: link against rt for clock_gettime on older glibc
    if(UNIX AND NOT APPLE)
        target_link_libraries(delta-physics PUBLIC rt)
    endif()
endif()

# ── Install instructions (optional) ──────────────────────────────────────────
install(TARGETS delta-physics
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
install(DIRECTORY include/ DESTINATION include/delta-physics)