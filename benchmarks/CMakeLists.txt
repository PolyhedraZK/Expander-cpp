cmake_minimum_required(VERSION 3.10)
project(benchmarks)
set(CMAKE_CXX_STANDARD 20)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
	include_directories(/opt/homebrew/include /usr/local/include)
	link_directories(/opt/homebrew/lib /usr/local/lib)
	find_library(GMP_LIBRARY gmp PATHS /opt/homebrew/lib)
	find_library(MCL_LIBRARY mcl PATHS /usr/local/lib)
endif()

include_directories(../lib)
include_directories(../include)
include_directories(../lib/btc_sha256)

add_executable(transcript_benchmark bench_transcript.cpp)
add_executable(keccak_benchmark bench_keccak.cpp)
add_executable(mcl_benchmark bench_mcl.cpp)

target_link_libraries(keccak_benchmark pthread btc_sha256)
target_link_libraries(transcript_benchmark pthread btc_sha256)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
	target_link_libraries(mcl_benchmark ${GMP_LIBRARY} ${MCL_LIBRARY})
else()
	target_link_libraries(mcl_benchmark gmp mcl)
endif()

