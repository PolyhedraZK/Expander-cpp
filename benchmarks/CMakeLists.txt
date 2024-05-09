cmake_minimum_required(VERSION 3.10)
project(benchmarks)
include_directories(../include)
set(CMAKE_CXX_STANDARD 20)

find_package(OpenSSL)

message (STATUS "Found OpenSSL version: ${OPENSSL_VERSION}")
message (STATUS "Location: ${OPENSSL_CRYPTO_LIBRARY}")

include_directories(../lib/btc_sha256)

add_executable(transcript_benchmark bench_transcript.cpp)
add_executable(keccak_benchmark bench_keccak.cpp)
target_link_libraries(keccak_benchmark pthread btc_sha256)
target_link_libraries(transcript_benchmark pthread btc_sha256)
