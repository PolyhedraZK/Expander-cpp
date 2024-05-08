cmake_minimum_required(VERSION 3.10)
project(benchmarks)
include_directories(../include)
set(CMAKE_CXX_STANDARD 20)

message(STATUS "Detected Processor: ${CMAKE_HOST_SYSTEM_PROCESSOR}")

# Check the system processor and set compiler flags based on the architecture
if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "x86_64")
    # Compiler flags for x86-64
    set(CMAKE_CXX_FLAGS "-march=native -mavx512f -mavx512vl -faligned-new -fPIC -flto -Wall -O3")
    set(CMAKE_CXX_FLAGS_DEBUG "-march=native -mavx512f -mavx512vl -faligned-new -flto -Wall -g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "-march=native -mavx512f -mavx512vl -faligned-new -flto -Wall -O3")
elseif(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm64")
    # Compiler flags for ARM architecture, assuming Apple M2
    set(CMAKE_CXX_FLAGS "-O3 -march=armv8.6-a -mcpu=apple-m2 -flto -ffast-math -fvectorize -fslp-vectorize -funroll-loops -fomit-frame-pointer -fPIC -Wno-unused-variable -Wno-unused-but-set-variable")
endif()
find_package(OpenSSL)

message (STATUS "Found OpenSSL version: ${OPENSSL_VERSION}")
message (STATUS "Location: ${OPENSSL_CRYPTO_LIBRARY}")

include_directories(../lib/btc_sha256)

add_executable(transcript_benchmark bench_transcript.cpp)
add_executable(keccak_benchmark bench_keccak.cpp)
target_link_libraries(keccak_benchmark pthread btc_sha256)
target_link_libraries(transcript_benchmark pthread btc_sha256)
