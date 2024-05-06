#include <gtest/gtest.h>

#include "LinearGKR/gkr.hpp"
#include "LinearGKR/sumcheck.hpp"
#include "field/M31.hpp"
#include "circuit/circuit.hpp"


TEST(SUMCHECK_TEST, SUMCHECK_GKR_LAYER)
{
    using namespace gkr;
    using F = M31_field::VectorizedM31;
    using F_primitive = M31_field::M31;

    uint32 nb_output_vars = 2, nb_input_vars = 3;
    CircuitLayer<F, F_primitive> layer = CircuitLayer<F, F_primitive>::random(nb_output_vars, nb_input_vars);
    Circuit<F, F_primitive> circuit;
    circuit.layers.emplace_back(layer);

    std::vector<F> output = layer.evaluate();

    std::vector<F_primitive> rz1, rz2;
    for (uint32 i = 0; i < nb_output_vars; i++)
    {
        rz1.emplace_back(F_primitive::random());
        rz2.emplace_back(F_primitive::random());
    }
    F claim_v1 = eval_multilinear(output, rz1);
    F claim_v2 = eval_multilinear(output, rz2);
    F_primitive alpha = F_primitive::random(), beta = F_primitive::random();

    gkr::Transcript<F, F_primitive> prover_transcript;
    gkr::GKRScratchPad<F, F_primitive> scratch_pad;
    scratch_pad.prepare(circuit);
    gkr::Timing timer;
    srand(123);
    GKRLayerProof<F> proof = std::get<0>(sumcheck_prove_gkr_layer(layer, rz1, rz2, alpha, beta, prover_transcript, scratch_pad, timer));

    gkr::Transcript<F, F_primitive> verifier_transcript;
    srand(123);
    bool verified = std::get<0>(sumcheck_verify_gkr_layer(layer, rz1, rz2, claim_v1, claim_v2, alpha, beta, proof, verifier_transcript));
    EXPECT_TRUE(verified);

    gkr::Transcript<F, F_primitive> verifier_transcript_fail;
    F non_zero = F::random();
    while (non_zero == F::zero())
    {
        non_zero = F::random();
    }
    srand(123);
    bool not_verified = std::get<0>(sumcheck_verify_gkr_layer(layer, rz1, rz2, claim_v1 + non_zero, claim_v2, alpha, beta, proof, verifier_transcript_fail));
    EXPECT_FALSE(not_verified);
}