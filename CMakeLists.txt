# Uncomment the next line if you want to force using clang
# SET (CMAKE_CXX_COMPILER "/usr/bin/clang++" CACHE STRING "C++ compiler" FORCE)

cmake_minimum_required(VERSION 3.16)
project(librapp)

set(CMAKE_CXX_STANDARD 20)
# Check the system processor and set compiler flags based on the architecture
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
    # Compiler flags for x86-64
    set(CMAKE_CXX_FLAGS "-march=native -mavx512f -mavx512vl -faligned-new -flto -Wall -funroll-loops -O3")
    set(CMAKE_CXX_FLAGS_DEBUG "-march=native -mavx512f -mavx512vl -faligned-new -flto -Wall -g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "-march=native -mavx512f -mavx512vl -faligned-new -flto -Wall -O3")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
    # Compiler flags for ARM architecture, assuming Apple M2
    set(CMAKE_CXX_FLAGS "-O3 -march=armv8.6-a -mcpu=apple-m2 -flto=auto -ffast-math -fvectorize -fslp-vectorize -funroll-loops -fomit-frame-pointer -fPIC -Wno-unused-variable -Wno-unused-but-set-variable")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    # Compiler flags for ARM architecture
    set(CMAKE_CXX_FLAGS "-O3 -march=armv8.6-a -mcpu=cortex-a76 -flto=auto -ffast-math -fvectorize -fslp-vectorize -funroll-loops -fomit-frame-pointer -fPIC -Wno-unused-variable -Wno-unused-but-set-variable")
else()
    # omit an error
    message(FATAL_ERROR "Unsupported architecture " ${CMAKE_SYSTEM_PROCESSOR})
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(include)
include_directories(lib)

# add_subdirectory(src)
# we're not using Orion as a pure lib, it includes some of the code in the src folder actually
# add_subdirectory(lib/Orion)
add_subdirectory(lib/btc_sha256)
add_subdirectory(tests)
add_subdirectory(benchmarks)
