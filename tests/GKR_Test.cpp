#include <iostream>
#include <gtest/gtest.h>

#include "field/M31.hpp"
#include "field/bn254_fr.hpp"
#include "LinearGKR/gkr.hpp"
#include "LinearGKR/LinearGKR.hpp"

TEST(GKR_TEST, GKR_WITH_PC_TEST)
{
    using namespace gkr;
    using F = gkr::M31_field::VectorizedM31;
    using F_primitive = gkr::M31_field::M31;

    uint32 n_layers = 4;
    Circuit<F, F_primitive> circuit;
    for (int i = n_layers - 1; i >= 0; --i)
    {
        circuit.layers.emplace_back(CircuitLayer<F, F_primitive>::random(i + 1, i + 2, true));
    }
    circuit._extract_rnd_gates();

    Config default_config{};
    Prover<F, F_primitive> prover(default_config);
    prover.prepare_mem(circuit);
    auto t = prover.prove(circuit);
    auto claimed_v = std::get<0>(t);
    Proof<F> proof = std::get<1>(t);

    Verifier verifier(default_config);
    bool verified = verifier.verify(circuit, claimed_v, proof);
    EXPECT_TRUE(verified);
}

TEST(GKR_TEST, GKR_SAME_FIELD_TEST)
{
    using namespace gkr;
    using F = gkr::M31_field::M31;
    using F_primitive = gkr::M31_field::M31;

    uint32 n_layers = 4;
    Circuit<F, F_primitive> circuit;
    for (int i = n_layers - 1; i >= 0; --i)
    {
        circuit.layers.emplace_back(CircuitLayer<F, F_primitive>::random(i + 1, i + 2, true));
    }
    circuit._extract_rnd_gates();
    circuit.set_random_input();

    Config default_config{};
    Prover<F, F_primitive> prover(default_config);
    prover.prepare_mem(circuit);
    auto t = prover.prove(circuit);
    auto claimed_v = std::get<0>(t);
    Proof<F> proof = std::get<1>(t);

    Verifier verifier(default_config);
    bool verified = verifier.verify(circuit, claimed_v, proof);
    EXPECT_TRUE(verified);
}


TEST(GKR_TEST, GKR_BN254_TEST)
{
    using namespace gkr;
    using F = gkr::bn254fr::BN254_Fr;
    using F_primitive = gkr::bn254fr::BN254_Fr;

    bn254fr::init();

    uint32 n_layers = 4;
    Circuit<F, F_primitive> circuit;
    for (int i = n_layers - 1; i >= 0; --i)
    {
        circuit.layers.emplace_back(CircuitLayer<F, F_primitive>::random(i + 1, i + 2));
    }
    circuit.evaluate();

    Config default_config{};
    default_config.set_field(BN254);

    Prover<F, F_primitive> prover(default_config);
    prover.prepare_mem(circuit);
    auto t = prover.prove(circuit);

    auto claimed_v = std::get<0>(t);
    Proof<F> proof = std::get<1>(t);

    Verifier verifier(default_config);
    bool verified = verifier.verify(circuit, claimed_v, proof);
    EXPECT_TRUE(verified);
}

TEST(GKR_TEST, GKR_FROM_CIRCUIT_RAW_TEST)
{
    using namespace gkr;
    using F = gkr::M31_field::VectorizedM31;
    using F_primitive = gkr::M31_field::M31;
    Config config{};

    const char* filename = "../data/circuit8.txt";
    CircuitRaw<F_primitive> circuit_raw;
    ifstream fs(filename, ios::binary);
    fs >> circuit_raw;
    auto circuit = Circuit<F, F_primitive>::from_circuit_raw(circuit_raw);

    std::cout << "Number Mul Gate: " << circuit.nb_mul_gates() << std::endl;
    std::cout << "Number Add Gate: " << circuit.nb_add_gates() << std::endl;
    circuit.set_random_input();
    circuit.evaluate();
    std::cout << "Circuit Evaluated" << std::endl;
    GKRScratchPad<F, F_primitive> scratch_pad;
    scratch_pad.prepare(circuit);

    Transcript<F, F_primitive> prover_transcript;
    auto t = gkr_prove<F, F_primitive>(circuit, scratch_pad, prover_transcript, config);
    auto claimed_value = std::get<0>(t);
    std::cout << "Proof Generated" << std::endl;
   
    Proof<F> &proof = prover_transcript.proof;
    Transcript<F, F_primitive> verifier_transcript;
    bool verified = std::get<0>(gkr_verify<F, F_primitive>(circuit, claimed_value, verifier_transcript, proof, config));
    EXPECT_TRUE(verified);

    F non_zero = F::random();
    while (non_zero == F::zero())
    {
        non_zero = F::random();
    }
    claimed_value += non_zero;

    proof.reset();
    Transcript<F, F_primitive> verifier_transcript_fail;
    
    bool not_verified = std::get<0>(gkr_verify<F, F_primitive>(circuit, claimed_value, verifier_transcript_fail, proof, config));
    EXPECT_FALSE(not_verified);
}

// TEST(GKR_TEST, GKR_CIRCUIT_LOADING_TEST)
// {
//     using namespace gkr;
//     using F = gkr::M31_field::VectorizedM31;
//     using F_primitive = gkr::M31_field::M31;    
    
//     const char* filename = "../data/circuit8.txt";
//     CircuitRaw<F_primitive> circuit_raw;
//     ifstream fs(filename, ios::binary);
//     fs >> circuit_raw;
//     auto circuit_1 = Circuit<F, F_primitive>::from_circuit_raw(circuit_raw);

//     const char* filename_mul = "../data/ExtractedCircuitMul.txt";
//     const char* filename_add = "../data/ExtractedCircuitAdd.txt";
//     auto circuit_2 = Circuit<F, F_primitive>::load_extracted_gates(filename_mul, filename_add);
    
//     EXPECT_EQ(circuit_1.layers.size(), circuit_2.layers.size());
//     for (uint32 i = 0; i < circuit_1.layers.size(); i++)
//     {
//         const CircuitLayer<F, F_primitive> &layer_1 = circuit_1.layers[i];
//         const CircuitLayer<F, F_primitive> &layer_2 = circuit_2.layers[i];
        
//         EXPECT_EQ(layer_1.mul.sparse_evals.size(), layer_2.mul.sparse_evals.size());
//         for (size_t j = 0; j < layer_1.mul.sparse_evals.size(); j++)
//         {
//             const Gate<F_primitive, 2> &gate_1 = layer_1.mul.sparse_evals[j];
//             const Gate<F_primitive, 2> &gate_2 = layer_2.mul.sparse_evals[j];
//             EXPECT_EQ(gate_1.o_id, gate_2.o_id);
//             EXPECT_EQ(gate_1.i_ids[0], gate_2.i_ids[0]);
//             EXPECT_EQ(gate_1.i_ids[1], gate_2.i_ids[1]);
//             EXPECT_EQ(gate_1.coef, gate_2.coef);
            
//         }

//         EXPECT_EQ(layer_1.add.sparse_evals.size(), layer_2.add.sparse_evals.size());
//         for (size_t j = 0; j < layer_1.add.sparse_evals.size(); j++)
//         {
//             const Gate<F_primitive, 1> &gate_1 = layer_1.add.sparse_evals[j];
//             const Gate<F_primitive, 1> &gate_2 = layer_2.add.sparse_evals[j];
//             EXPECT_EQ(gate_1.o_id, gate_2.o_id);
//             EXPECT_EQ(gate_1.i_ids[0], gate_2.i_ids[0]);
//             EXPECT_EQ(gate_1.coef, gate_2.coef);
//         }
//     }
    
// }
