cmake_minimum_required(VERSION 3.16)
project(csopesy_mo3 LANGUAGES CXX)

# C++17, standard library only (see CLAUDE.md section 2).
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

find_package(Threads REQUIRED)

# ---------------------------------------------------------------------------
# Marquee console (the deliverable). Entry point: src/main.cpp
# ---------------------------------------------------------------------------
set(MO3_CORE_SOURCES
    src/platform.cpp
    src/console.cpp
    src/marquee.cpp
    src/input.cpp
    src/interpreter.cpp
)

add_executable(csopesy_mo3 src/main.cpp ${MO3_CORE_SOURCES})
target_include_directories(csopesy_mo3 PRIVATE src)
target_link_libraries(csopesy_mo3 PRIVATE Threads::Threads)

# Warnings are treated as bugs in this repo: keep them loud on every compiler.
function(mo3_apply_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
        target_compile_definitions(${target} PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()
mo3_apply_warnings(csopesy_mo3)

# Visual Studio: run/debug from the repo root so config.txt is found directly.
set_target_properties(csopesy_mo3 PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

# ---------------------------------------------------------------------------
# Unit tests for the command interpreter (no framework, plain asserts).
#   cmake --build build && ctest --test-dir build
# ---------------------------------------------------------------------------
option(MO3_BUILD_TESTS "Build the interpreter unit tests" ON)
if(MO3_BUILD_TESTS)
    enable_testing()
    add_executable(test_interpreter tests/test_interpreter.cpp src/interpreter.cpp src/console.cpp src/platform.cpp)
    target_include_directories(test_interpreter PRIVATE src)
    target_link_libraries(test_interpreter PRIVATE Threads::Threads)
    mo3_apply_warnings(test_interpreter)
    add_test(NAME interpreter COMMAND test_interpreter)
endif()
