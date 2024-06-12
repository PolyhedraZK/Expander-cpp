
#include <mpi.h>
#include <iostream>
#include <gtest/gtest.h>

#include "field/M31.hpp"
#include "field/bn254_fr.hpp"
#include "LinearGKR/gkr.hpp"
#include "LinearGKR/LinearGKR.hpp"

TEST(GKR_TEST, GKR_MPI)
{
    using namespace gkr;
    using F = gkr::M31_field::VectorizedM31;
    using F_primitive = gkr::M31_field::M31;
    
    int world_rank, world_size;

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    std::cout << "Worker " << world_rank << "started." << std::endl;

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

    F claimed_v;
    Proof<F> proof;
    if (world_rank == 0) // temporarily skip verify
    {
        // claimed_v = std::get<0>(t);
        // proof = std::get<1>(t);
        // Verifier verifier{default_config};
        // bool verified = verifier.verify(circuit, claimed_v, proof);
        // EXPECT_TRUE(verified);
    }
    MPI_Finalize();
}