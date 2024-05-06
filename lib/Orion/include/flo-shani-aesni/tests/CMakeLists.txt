# The MIT License (MIT)
# Copyright (c) 2018 Armando Faz
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
# DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
# OR OTHER DEALINGS IN THE SOFTWARE.

cmake_minimum_required(VERSION 3.0)
enable_language(CXX)
set(CMAKE_CXX_STANDARD 98)

include_directories(../aes)
include_directories(../sha256)
include_directories(../prng)
include_directories(../cpuid)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O3 -pedantic")

# Locate GTest
find_package(GTest REQUIRED)
link_directories(${GTEST_LIBRARY_DIRS})

if(GTest_FOUND)
	include_directories(${GTEST_INCLUDE_DIRS})
	add_executable(tests_sha256 runTests.cpp test_sha256.cpp)
    add_dependencies(tests_sha256 flo-shani flo-prng)
    target_link_libraries(tests_sha256 ${GTEST_LIBRARIES} crypto flo-shani flo-prng)

    add_executable(tests_aes runTests.cpp test_aes.cpp)
    add_dependencies(tests_aes flo-aesni flo-prng)
	target_link_libraries(tests_aes ${GTEST_LIBRARIES} crypto flo-aesni flo-prng)
endif()
