cmake_minimum_required(VERSION 3.20)
project(CMake1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(CMake1
    app/main.cpp
    platform/Win32Window.cpp
 "gfx/Dx11Device.h" "gfx/Dx11Device.cpp")

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
