
#include <iostream>
#include <gtest/gtest.h>
#include <libff/algebra/curves/bn128/bn128_fields.hpp>

#include "field/bn254_fr.hpp"
#include "poly_commit/poly.hpp"
#include "poly_commit/kzg.hpp"
#include "poly_commit/bi-kzg/bi-kzg.hpp"

TEST(KZG_TESTS, KZG_TEST)
{
    using namespace gkr;
    using F = bn254fr::BN254_Fr;

    bn254fr::init();

    uint32 degree = 1024;
    UniPoly<F> poly = UniPoly<F>::random(degree);
    F x = F::random();

    KZGSetup setup_ = setup(F::random(), degree);
    KZGCommitment commitment = commit(poly, setup_);
    KZGOpening opening = open(poly, x, setup_);
    EXPECT_EQ(opening.v, poly.eval_at(x));
    bool verified = verify(poly, x, setup_, commitment, opening);
    EXPECT_TRUE(verified);

    opening.v += F::random();
    bool not_verified = verify(poly, x, setup_, commitment, opening);
    EXPECT_FALSE(not_verified);
}

TEST(KZG_TESTS, BI_KZG_TEST)
{
    using namespace gkr;
    using F = bn254fr::BN254_Fr;

    bn254fr::init();

    uint32 deg = 8;
    BivariatePoly<F> poly{deg, deg};
    poly.random_init();
    std::cout << poly.deg_u << " " << poly.deg_v << std::endl;
    std::cout << poly.evals->size() << std::endl;
    F x = F::random(), y = F::random();

    BiKZG bi_kzg;
    BiKZGSetup setup;
    bi_kzg.setup_gen(setup, deg, deg, F::random(), F::random());
    
    BiKZGCommitment commitment;
    bi_kzg.commit(commitment, poly, setup);

    BiKZGOpening opening;
    bi_kzg.open(opening, poly, x, y, setup);

    bool verified = bi_kzg.verify(setup, commitment, x, y, opening);    
    EXPECT_TRUE(verified);

    opening.v += F::random();
    bool not_verified = bi_kzg.verify(setup, commitment, x, y, opening);
    EXPECT_FALSE(not_verified);
}