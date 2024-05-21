
#include <iostream>
#include <gtest/gtest.h>
#include <libff/algebra/curves/bn128/bn128_fields.hpp>

#include "field/bn254_fr.hpp"
#include "poly_commit/poly.hpp"
#include "poly_commit/kzg.hpp"

TEST(KZG_TESTS, KZG_SETUP_TEST)
{
    using namespace gkr;
    using F = bn254fr::BN254_Fr;

    bn254fr::init();

    uint32 degree = 1024;
    UniPoly<F> poly = UniPoly<F>::random(degree);
    F x = F::random();

    kzg::KZGSetup setup = kzg::setup(F::random(), degree);
    kzg::KZGCommitment commitment = kzg::commit(poly, setup);
    kzg::KZGOpening opening = kzg::open(poly, x, setup);
    EXPECT_EQ(opening.v, poly.eval_at(x));
    bool verified = kzg::verify(poly, x, setup, commitment, opening);
    EXPECT_TRUE(verified);

    opening.v += F::random();
    bool not_verified = kzg::verify(poly, x, setup, commitment, opening);
    EXPECT_FALSE(not_verified);
}