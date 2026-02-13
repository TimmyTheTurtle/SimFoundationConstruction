cmake_minimum_required(VERSION 3.20)
project(CMake1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(CMake1
    app/main.cpp
)

target_include_directories(CMake1 PRIVATE ${CMAKE_SOURCE_DIR})

if (MSVC)
  target_compile_options(CMake1 PRIVATE /W4 /permissive-)
endif()