cmake_minimum_required(VERSION 3.20)
project(SimFoundation LANGUAGES CXX)

# normalise the application name into a variable for easy renames
set(APP_NAME ${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${APP_NAME}
    src/app/main.cpp
    src/platform/Win32Window.cpp
    src/gfx/Dx11Device.cpp
)

target_include_directories(${APP_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)

# core simulation library ("sim")
add_library(sim STATIC
    src/sim/Sim.cpp
)
target_include_directories(sim PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_compile_features(sim PUBLIC cxx_std_20)

target_link_libraries(${APP_NAME} PRIVATE sim)

if (MSVC)
  target_compile_options(${APP_NAME} PRIVATE
    $<$<CONFIG:Debug>:/Zi /Od>
  )
  target_link_options(${APP_NAME} PRIVATE
    $<$<CONFIG:Debug>:/DEBUG>
  )
endif()

target_link_libraries(${APP_NAME} PRIVATE d3d11 dxgi d3dcompiler)

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
add_executable(${APP_NAME}Tests
    tests/test_smoke.cpp
)
target_include_directories(${APP_NAME}Tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(${APP_NAME}Tests PRIVATE GTest::gtest_main sim)

# **Hard guarantee**: at least one test exists for CTest/VS to discover
# use the target file generator so CTest gets the real executable path
add_test(NAME SmokeRuns COMMAND $<TARGET_FILE:${APP_NAME}Tests>)

# Optional: auto-discover individual TEST() cases too
include(GoogleTest)
# Ensure the test executable is built before trying to discover individual GTest cases.
# This helps CTest/IDE discover tests even if the binary hasn't been built yet.
gtest_discover_tests(${APP_NAME}Tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    BUILD_COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target ${APP_NAME}Tests
)
