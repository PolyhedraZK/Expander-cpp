#include <mutex>
#include <iostream>
#include <mcl/bn256.hpp>
#include "field/bn254_fr.hpp"

int main() 
{
    using namespace mcl::bn;
    initPairing(mcl::BN_SNARK1);
    
    u_int32_t test_size = 10000000;

    Fr a = 123, b = 456;
    auto t0 = std::chrono::high_resolution_clock::now();
    for (u_int32_t i = 0; i < test_size; i++)
    {
        Fr::mul(a, a, b);
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    float running_time_mcl = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    std::cout << "Running time for original " << test_size << " mul is " << running_time_mcl << " ms" << std::endl;
    
    using F = gkr::bn254fr::BN254_Fr;
    F c = 123, d = 456;
    auto t2 = std::chrono::high_resolution_clock::now();
    for (u_int32_t i = 0; i < test_size; i++)
    {
        c = c * d;
    }
    auto t3 = std::chrono::high_resolution_clock::now();
    float running_time_wrapped = std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count();
    std::cout << "Running time for wrappeed " << test_size << " mul is " << running_time_wrapped << " ms" << std::endl;

    std::cout << "Result equal? " << (a == c.mcl_data) << std::endl;
}