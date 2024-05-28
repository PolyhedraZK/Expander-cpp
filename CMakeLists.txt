# Uncomment the next line if you want to force using clang
# SET (CMAKE_CXX_COMPILER "/usr/bin/clang++" CACHE STRING "C++ compiler" FORCE)

cmake_minimum_required(VERSION 3.16)
project(librapp)

# include MCL
include(FetchContent)
FetchContent_Declare(
        mcl
        # Specify the commit you depend on and update it regularly.
        URL https://github.com/herumi/mcl/archive/refs/tags/v1.92.zip
	DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(mcl)

set(CMAKE_CXX_STANDARD 20)
# Check the system processor and set compiler flags based on the architecture
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
    # Compiler flags for x86-64
    set(CMAKE_CXX_FLAGS "-march=native -faligned-new -flto -Wall -O3")
    set(CMAKE_CXX_FLAGS_DEBUG "-march=native -faligned-new -flto -Wall -g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "-march=native -faligned-new -flto -Wall -O3")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
    # Compiler flags for ARM architecture, assuming Apple M2
    set(CMAKE_CXX_FLAGS "-O3 -march=armv8.6-a -flto=auto -ffast-math -fvectorize -fslp-vectorize -funroll-loops -fomit-frame-pointer -fPIC -Wno-unused-variable -Wno-unused-but-set-variable")
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
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
        include_directories(/opt/homebrew/include /usr/local/include)
        link_directories(/opt/homebrew/lib /usr/local/lib)
        find_library(GMP_LIBRARY gmp PATHS /opt/homebrew/lib)
        find_library(MCL_LIBRARY mcl PATHS /usr/local/lib)
endif()

# add_subdirectory(src)
# we're not using Orion as a pure lib, it includes some of the code in the src folder actually
# add_subdirectory(lib/Orion)
add_subdirectory(lib/btc_sha256)
add_subdirectory(tests)
add_subdirectory(benchmarks)
