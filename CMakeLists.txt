cmake_minimum_required(VERSION 3.20)
project(CMake1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(CMake1
    app/main.cpp
    platform/Win32Window.cpp
    gfx/Dx11Device.cpp
    gfx/Dx11Device.h
)

target_include_directories(CMake1 PRIVATE ${CMAKE_SOURCE_DIR})

if (MSVC)
  target_compile_options(CMake1 PRIVATE
    $<$<CONFIG:Debug>:/Zi /Od>
  )
  target_link_options(CMake1 PRIVATE
    $<$<CONFIG:Debug>:/DEBUG>
  )
endif()

target_link_libraries(CMake1 PRIVATE d3d11 dxgi d3dcompiler)

# ---------------------------
# Unit tests (CTest + GoogleTest)
# ---------------------------
include(CTest)
set(BUILD_TESTING ON CACHE BOOL "Build tests" FORCE)
enable_testing()
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
add_executable(CMake1Tests
    tests/test_smoke.cpp
)
target_include_directories(CMake1Tests PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(CMake1Tests PRIVATE GTest::gtest_main)

# **Hard guarantee**: at least one test exists for CTest/VS to discover
add_test(NAME SmokeRuns COMMAND CMake1Tests)

# Optional: auto-discover individual TEST() cases too
include(GoogleTest)
gtest_discover_tests(CMake1Tests)
