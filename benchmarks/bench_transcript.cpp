#include <mutex>

#include "field/M31.hpp"
#include "fiat_shamir/transcript.hpp"

int main()
{
    using namespace gkr;
    using F = M31_field::VectorizedM31;
    using F_primitive = M31_field::M31;
    Transcript<F, F_primitive> transcript;

    uint32 test_size = 1e5;
    std::vector<F> input;
    input.reserve(test_size);
    for (uint32 i = 0; i < test_size; i++)
    {
        input.emplace_back(F::random());
    }
    
    auto t0 = std::chrono::high_resolution_clock::now();
    for (uint32 i = 0; i < test_size; i++)
    {
        transcript.append_f(input[i]);        
    }
    F_primitive f = transcript.challenge_f();

    auto t1 = std::chrono::high_resolution_clock::now();
    float running_time = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    std::cout << "Running time for " << test_size << " append is " << running_time << " milli seconds" << std::endl;
    std::cout << "Output the result to avoid being optimized out: " << f.x << std::endl;
}