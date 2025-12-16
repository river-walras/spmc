cmake_minimum_required(VERSION 3.15)
project(msgbus VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard (C++17 required for std::variant)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags for optimization
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")

# Default to Release build
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Find required packages
find_package(Threads REQUIRED)
find_package(Python REQUIRED COMPONENTS Interpreter Development.Module)
find_package(pybind11 CONFIG REQUIRED)

# Python extension module
pybind11_add_module(_core MODULE msgbus/python_bindings.cpp)

# Include directories for the module
target_include_directories(_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/msgbus
)

# Link libraries
target_link_libraries(_core PRIVATE Threads::Threads)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Installation - scikit-build-core handles the installation to msgbus/
install(TARGETS _core LIBRARY DESTINATION msgbus)

# Optional: Build benchmark executable (not installed with pip)
option(BUILD_BENCHMARK "Build C++ benchmark executable" ON)
if(BUILD_BENCHMARK)
    add_executable(spmc_benchmark benchmarks/main.cpp)
    target_include_directories(spmc_benchmark PRIVATE msgbus)
    target_link_libraries(spmc_benchmark PRIVATE Threads::Threads)

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(spmc_benchmark PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()