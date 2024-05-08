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
    Config config{};

    uint32 nb_output_vars = 2, nb_input_vars = 3;
    CircuitLayer<F, F_primitive> layer = CircuitLayer<F, F_primitive>::random(nb_output_vars, nb_input_vars);
    Circuit<F, F_primitive> circuit;
    circuit.layers.emplace_back(layer);

    std::vector<F> output = layer.evaluate();

    std::vector<std::vector<F_primitive>> rz1, rz2;
    rz1.resize(config.get_num_repetitions());
    rz2.resize(config.get_num_repetitions());
    for (uint32 i = 0; i < nb_output_vars; i++)
    {
        for(int j = 0; j < config.get_num_repetitions(); j++)
        {
            rz1[j].emplace_back(F_primitive::random());
            rz2[j].emplace_back(F_primitive::random());
        }
    }
    std::vector<F> claim_v1, claim_v2;
    for(int i = 0; i < config.get_num_repetitions(); i++)
    {
        claim_v1.push_back(eval_multilinear(output, rz1[i]));
        claim_v2.push_back(eval_multilinear(output, rz2[i]));
    }
    F_primitive alpha = F_primitive::random(), beta = F_primitive::random();

    gkr::Transcript<F, F_primitive> prover_transcript;
    gkr::GKRScratchPad<F, F_primitive> *scratch_pad = new gkr::GKRScratchPad<F, F_primitive>[config.get_num_repetitions()];
    for (int i = 0; i < config.get_num_repetitions(); i++)
    {
        scratch_pad[i].prepare(circuit);
    }
    gkr::Timing timer;
    sumcheck_prove_gkr_layer(layer, rz1, rz2, alpha, beta, prover_transcript, scratch_pad, timer, config);
    Proof<F> &proof = prover_transcript.proof;

    gkr::Transcript<F, F_primitive> verifier_transcript;
    bool verified = std::get<0>(sumcheck_verify_gkr_layer(layer, rz1, rz2, claim_v1, claim_v2, alpha, beta, proof, verifier_transcript, config));
    EXPECT_TRUE(verified);

    proof.reset();
    gkr::Transcript<F, F_primitive> verifier_transcript_fail;
    F non_zero = F::random();
    while (non_zero == F::zero())
    {
        non_zero = F::random();
    }
    for(int i = 0; i < config.get_num_repetitions(); i++)
    {
        claim_v1[i] += non_zero;
    }
    bool not_verified = std::get<0>(sumcheck_verify_gkr_layer(layer, rz1, rz2, claim_v1, claim_v2, alpha, beta, proof, verifier_transcript_fail, config));
    EXPECT_FALSE(not_verified);
}