cmake_minimum_required(VERSION 3.10)
# test
enable_testing()
find_package(OpenSSL)
set(CMAKE_CXX_STANDARD 20)

include(FetchContent)
FetchContent_Declare(
        googletest
        # Specify the commit you depend on and update it regularly.
        URL https://github.com/google/googletest/archive/5376968f6948923e2411081fd9372e71a59d8e77.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
include(GoogleTest)

# executables
add_executable(ff ff.cpp)
# add_executable(mimc mimc.cpp)
# add_executable(orion_pc orion_pc.cpp)
# add_executable(roots_of_unity roots_of_unity.cpp)
# add_executable(fft fft.cpp)
# add_executable(poly poly.cpp)
# add_executable(kzg kzg.cpp)
add_executable(merkle_tree merkle_tree.cpp)
# add_executable(fri fri.cpp)
add_executable(sumcheck sumcheck.cpp)
add_executable(gkr GKR_Test.cpp)
add_executable(transcript transcript.cpp)

# links
target_link_libraries(ff gtest_main gtest pthread)
# target_link_libraries(mimc gtest_main gtest pthread XKCP OpenSSL::Crypto)
# target_link_libraries(orion_pc gtest_main gtest pthread orion)
# target_link_libraries(fft gtest_main gtest pthread gmp gmpxx libff.a libzm.a)
# target_link_libraries(poly gtest_main gtest pthread gmp gmpxx libff.a libzm.a)
# target_link_libraries(kzg gtest_main gtest pthread libff.a libzm.a gmp gmpxx)
target_link_libraries(merkle_tree gtest_main gtest pthread OpenSSL::Crypto)
# target_link_libraries(fri gtest_main gtest pthread libff.a libzm.a gmp gmpxx XKCP)
target_link_libraries(sumcheck gtest_main gtest pthread OpenSSL::Crypto btc_sha256)
target_link_libraries(gkr gtest_main gtest pthread OpenSSL::Crypto btc_sha256)
target_link_libraries(transcript gtest_main gtest pthread OpenSSL::Crypto btc_sha256)


gtest_discover_tests(ff)
# gtest_discover_tests(mimc)
# gtest_discover_tests(orion_pc)
# gtest_discover_tests(fft)
# gtest_discover_tests(poly)
# gtest_discover_tests(kzg)
gtest_discover_tests(merkle_tree)
# gtest_discover_tests(fri)
gtest_discover_tests(sumcheck)
gtest_discover_tests(gkr)
gtest_discover_tests(transcript)
