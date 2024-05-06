
#include <iostream>
#include <gtest/gtest.h>

#include "field/bn254_fr.hpp"
#include "poly_commit/fri.hpp"

TEST(FRI_TESTS, FRI_LDT_TEST)
{
    using namespace gkr;
    using F = bn254_fr::BN254_Fr;
    bn254_fr::init();

    uint32 degree = 1024;
    UniPoly<F> poly = UniPoly<F>::random(degree);

    Transcript<F> prover_transcript;
    gkr::fri::FriLDTProof<F> proof =  fri::fri_ldt_prove(poly, prover_transcript);

    Transcript<F> verifier_transcript_1;
    bool verified = fri::fri_ldt_verify(proof, verifier_transcript_1, degree);
    EXPECT_TRUE(verified);

    Transcript<F> verifier_transcript_2;
    bool not_verified = fri::fri_ldt_verify(proof, verifier_transcript_2, degree >> 1);
    EXPECT_FALSE(not_verified);
}