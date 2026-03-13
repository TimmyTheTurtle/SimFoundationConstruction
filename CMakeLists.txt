cmake_minimum_required(VERSION 3.20)
project(SimFoundation LANGUAGES CXX)

# normalise the application name into a variable for easy renames
set(APP_NAME ${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (MSVC)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
      "$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>")
endif()

# core simulation library ("sim")
add_library(sim STATIC
    src/sim/Sim.cpp
 "src/core/ScopedQpcTimer.cpp")
target_include_directories(sim PUBLIC ${CMAKE_SOURCE_DIR}/src)

add_library(app_support STATIC
    src/platform/Win32Window.cpp
    src/gfx/Dx11Device.cpp)
target_include_directories(app_support PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(app_support PUBLIC d3d11 dxgi)

add_executable(${APP_NAME}
    src/app/main.cpp)
target_include_directories(${APP_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(${APP_NAME} PRIVATE sim app_support d3dcompiler)

if (MSVC)
  target_compile_options(${APP_NAME} PRIVATE
    $<$<CONFIG:Debug>:/Zi /Od>
  )
  target_compile_options(sim PRIVATE
    $<$<CONFIG:Debug>:/Zi /Od>
  )
  target_compile_options(app_support PRIVATE
    $<$<CONFIG:Debug>:/Zi /Od>
  )
  target_link_options(${APP_NAME} PRIVATE
    $<$<CONFIG:Debug>:/DEBUG>
  )
endif()

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
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
function(simfoundation_configure_test_target target_name)
  target_include_directories(${target_name} PRIVATE ${CMAKE_SOURCE_DIR}/src)
  target_compile_definitions(${target_name} PRIVATE
      SIMFOUNDATION_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

  if (MSVC)
    target_compile_options(${target_name} PRIVATE
      $<$<CONFIG:Debug>:/Zi /Od>
    )
    target_link_options(${target_name} PRIVATE
      $<$<CONFIG:Debug>:/DEBUG>
    )
  endif()
endfunction()

add_executable(${APP_NAME}UnitTests
    tests/test_smoke.cpp
    tests/test_comptr_checklist.cpp
    tests/test_mapped_subresource.cpp
    tests/test_scoped_qpc_timer.cpp
    tests/test_unique_handle.cpp)
target_link_libraries(${APP_NAME}UnitTests PRIVATE GTest::gtest_main sim)
simfoundation_configure_test_target(${APP_NAME}UnitTests)

add_executable(${APP_NAME}IntegrationTests
    tests/test_dx11_device.cpp)
target_link_libraries(${APP_NAME}IntegrationTests PRIVATE GTest::gtest_main app_support)
simfoundation_configure_test_target(${APP_NAME}IntegrationTests)

# Optional: auto-discover individual TEST() cases too
include(GoogleTest)
gtest_discover_tests(${APP_NAME}UnitTests
    TEST_PREFIX "Unit."
    PROPERTIES LABELS "unit"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

gtest_discover_tests(${APP_NAME}IntegrationTests
    TEST_PREFIX "Integration."
    PROPERTIES LABELS "integration"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
