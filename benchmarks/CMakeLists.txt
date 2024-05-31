cmake_minimum_required(VERSION 3.10)
project(benchmarks)
set(CMAKE_CXX_STANDARD 20)

include_directories(../include)
include_directories(../lib/btc_sha256)

add_executable(transcript_benchmark bench_transcript.cpp)
add_executable(keccak_benchmark bench_keccak.cpp)
add_executable(mcl_benchmark bench_mcl.cpp)
add_executable(kzg_benchmark bench_kzg.cpp)

target_link_libraries(keccak_benchmark pthread btc_sha256)
target_link_libraries(transcript_benchmark pthread btc_sha256)
target_link_libraries(mcl_benchmark mcl gmp)
target_link_libraries(kzg_benchmark mcl gmp)
