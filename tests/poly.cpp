#include <iostream>
#include <gtest/gtest.h>

#include "field/M31.hpp"
#include "poly_commit/poly.hpp"

TEST(POLY_TESTS, POLY_QUOTIENT_TEST)
{
    using namespace gkr;
    using F = M31_field::M31;
    
    // x^3 - 3x^2 + 3x - 1
    UniPoly<F> p;
    p.coefs.emplace_back(-F::one()); // -1
    p.coefs.emplace_back(F(3)); // +3x 
    p.coefs.emplace_back(-F(3)); // -3x^2
    p.coefs.emplace_back(F::one()); // x^3 

    UniPoly<F> q = p.quotient(F::one()); // x - 1

    std::cout << q.coefs[0].x << std::endl; // 1
    std::cout << q.coefs[1].x << std::endl; // -2x
    std::cout << q.coefs[2].x << std::endl; // x^2

}
