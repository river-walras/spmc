cmake_minimum_required(VERSION 3.10)
project(spmc)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")

# Default to Release build
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Find required packages
find_package(Threads REQUIRED)

# Include directories
include_directories(src)

# Source files
set(SOURCES
    src/main.cpp
)

# Headers
set(HEADERS
    src/spmc.hpp
    src/statistic.hpp
)

# Create executable
add_executable(spmc_benchmark ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(spmc_benchmark Threads::Threads)

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(spmc_benchmark PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Installation
install(TARGETS spmc_benchmark DESTINATION bin)
install(FILES ${HEADERS} DESTINATION include/spmc)