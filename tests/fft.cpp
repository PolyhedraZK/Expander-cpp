#include <gtest/gtest.h>

#include "field/utils.hpp"
#include "field/mersenne_extension.h"
#include "field/bn254_fr.hpp"
#include "poly_commit/poly.hpp"

TEST(FFT_TESTS, FFT_TESTS) 
{
    using namespace gkr;
    using F = bn254_fr::BN254_Fr;
    using Scalar = bn254_fr::Scalar;
    bn254_fr::init();

    uint32 order = 6; // 64
    UniPoly<F> poly = UniPoly<F>::random(1 << order);
    F pow = F::one();

    F root = get_primitive_root_of_unity<F>(order);

    std::vector<F> evals = ntt<F>(poly.coefs);
    for (uint32 i = 0; i < (1 << order); i++)
    {
        EXPECT_EQ(evals[i], poly.eval_at(pow));
        pow *= root;
    }

    std::vector<F> coefs = intt<F>(evals);
    for (uint32 i = 0; i < (1 << order); i++)
    {
        EXPECT_EQ(coefs[i], poly.coefs[i]);
    }

}