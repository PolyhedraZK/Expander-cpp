#include <gtest/gtest.h>

#include "field/M31.hpp"
#include "LinearGKR/gkr.hpp"
#include <iostream>

TEST(GKR_TEST, GKR_CORRECTNESS_TEST)
{
    using namespace gkr;
    using F = gkr::M31_field::VectorizedM31;
    using F_primitive = gkr::M31_field::M31;

    uint32 n_layers = 4;
    Circuit<F, F_primitive> circuit;
    for (int i = n_layers - 1; i >= 0; --i)
    {
        circuit.layers.emplace_back(CircuitLayer<F, F_primitive>::random(i + 1, i + 2));
    }
    circuit.evaluate();

    GKRProof<F> proof;
    F claimed_value;
    GKRScratchPad<F, F_primitive> scratch_pad;
    scratch_pad.prepare(circuit);
    // GTest Print to see the claimed value
    srand(42);
    std::tie(proof, claimed_value) = gkr_prove<F, F_primitive>(circuit, scratch_pad);
    srand(42);
    bool verified = gkr_verify<F, F_primitive>(circuit, claimed_value, proof);    
    EXPECT_TRUE(verified);

    F non_zero = F::random();
    while (non_zero == F::zero())
    {
        non_zero = F::random();
    }
    srand(42);
    bool not_verified = gkr_verify<F, F_primitive>(circuit, claimed_value + non_zero, proof);
    EXPECT_FALSE(not_verified);
}


TEST(GKR_TEST, GKR_FROM_CIRCUIT_RAW_TEST)
{
    using namespace gkr;
    using F = gkr::M31_field::VectorizedM31;
    using F_primitive = gkr::M31_field::M31;

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
    GKRScratchPad<F, F_primitive> scratch_pad{};
    scratch_pad.prepare(circuit);
    GKRProof<F> proof;
    F claimed_value;
    srand(42);
    std::tie(proof, claimed_value) = gkr_prove<F, F_primitive>(circuit, scratch_pad);
    std::cout << "Proof Generated" << std::endl;
    srand(42);
    bool verified = gkr_verify<F, F_primitive>(circuit, claimed_value, proof);    
    EXPECT_TRUE(verified);

    F non_zero = F::random();
    while (non_zero == F::zero())
    {
        printf("wtf\n");
        non_zero = F::random();
    }
    bool not_verified = gkr_verify<F, F_primitive>(circuit, claimed_value + non_zero, proof);
    EXPECT_FALSE(not_verified);
}

TEST(GKR_TEST, GKR_CIRCUIT_LOADING_TEST)
{
    using namespace gkr;
    using F = gkr::M31_field::VectorizedM31;
    using F_primitive = gkr::M31_field::M31;    
    
    const char* filename = "../data/circuit8.txt";
    CircuitRaw<F_primitive> circuit_raw;
    ifstream fs(filename, ios::binary);
    fs >> circuit_raw;
    auto circuit_1 = Circuit<F, F_primitive>::from_circuit_raw(circuit_raw);

    const char* filename_mul = "../data/ExtractedCircuitMul.txt";
    const char* filename_add = "../data/ExtractedCircuitAdd.txt";
    auto circuit_2 = Circuit<F, F_primitive>::load_extracted_gates(filename_mul, filename_add);
    
    EXPECT_EQ(circuit_1.layers.size(), circuit_2.layers.size());
    for (uint32 i = 0; i < circuit_1.layers.size(); i++)
    {
        const CircuitLayer<F, F_primitive> &layer_1 = circuit_1.layers[i];
        const CircuitLayer<F, F_primitive> &layer_2 = circuit_2.layers[i];
        
        EXPECT_EQ(layer_1.mul.sparse_evals.size(), layer_2.mul.sparse_evals.size());
        for (size_t j = 0; j < layer_1.mul.sparse_evals.size(); j++)
        {
            const Gate<F_primitive, 2> &gate_1 = layer_1.mul.sparse_evals[j];
            const Gate<F_primitive, 2> &gate_2 = layer_2.mul.sparse_evals[j];
            EXPECT_EQ(gate_1.o_id, gate_2.o_id);
            EXPECT_EQ(gate_1.i_ids[0], gate_2.i_ids[0]);
            EXPECT_EQ(gate_1.i_ids[1], gate_2.i_ids[1]);
            EXPECT_EQ(gate_1.coef, gate_2.coef);
            
        }

        EXPECT_EQ(layer_1.add.sparse_evals.size(), layer_2.add.sparse_evals.size());
        for (size_t j = 0; j < layer_1.add.sparse_evals.size(); j++)
        {
            const Gate<F_primitive, 1> &gate_1 = layer_1.add.sparse_evals[j];
            const Gate<F_primitive, 1> &gate_2 = layer_2.add.sparse_evals[j];
            EXPECT_EQ(gate_1.o_id, gate_2.o_id);
            EXPECT_EQ(gate_1.i_ids[0], gate_2.i_ids[0]);
            EXPECT_EQ(gate_1.coef, gate_2.coef);
        }
    }
    
}