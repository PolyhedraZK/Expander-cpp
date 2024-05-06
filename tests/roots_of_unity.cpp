#include<iostream>
#include<cassert>
#include "field/M31.hpp"

int main() {
    using namespace gkr;
    using namespace gkr::M31_field;
    using F = M31;

    // the field has size p^2 where p = 2^31 - 1
    // the order of any element is p^2 - 1 = 2^32 (2^30 - 1)
    // thus the max order is 32
    int max_order = 32;
    Scalar e = (1 << 30) - 1;
    int64 field_size = static_cast<int64>(mod) * mod;
    F root_of_unity;

    srand(1234);
    while (true) 
    {
        F f = F::random(); 
        root_of_unity = f.exp(e);
        assert(root_of_unity.exp(Scalar(1) << 32) == F::one());
        // test primitive, can avoid this step if f is a generator
        if (root_of_unity.exp(Scalar(1) << 31) != F::one()) 
        {
            break;
        }
    }
    std::cout << root_of_unity.x << std::endl;
}