#pragma once

#include<mpi.h>
#include<vector>
#include "scratch_pad.hpp"
#include "sumcheck.hpp"
#include "circuit/circuit.hpp"
#include <map>
#include "configuration/config.hpp"
#include <chrono>

inline int ceil(int a, int b)
{
    if (a % b == 0)
    {
        return a / b;
    }
    return a / b + 1;
}

namespace gkr
{


template<typename F, typename F_primitive>
std::tuple<F, 
    std::vector<F_primitive>, std::vector<F_primitive>, 
    std::vector<F_primitive>, std::vector<F_primitive>>
gkr_prove(
    const Circuit<F, F_primitive> &circuit, 
    GKRScratchPad<F, F_primitive> *scratch_pad,
    Transcript<F, F_primitive> &transcript,
    const Config &config
)
{
    int world_rank, world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    uint32 lg_world_size = log_2(world_size);

    uint32 n_layers = circuit.layers.size();
    uint32 lg_output_size = circuit.layers.back().nb_output_vars; 

    std::vector<F_primitive> mpi_combine_coefs(world_size);
    std::vector<F_primitive> rz1(lg_output_size, F_primitive::zero()), 
        rz2(lg_output_size, F_primitive::zero()), rw1(lg_world_size), rw2(lg_world_size);

    if (world_rank == 0)
    {
        for (uint32 i = 0; i < lg_output_size; i++)
        {
            rz1[i] = transcript.challenge_f();
        }
        
        for (uint32 i = 0; i < lg_world_size; i++)
        {
            rw1[i] = transcript.challenge_f();
        }
        _eq_evals_at_primitive(rw1, F_primitive::one(), mpi_combine_coefs.data());      
    }

    MPI_Bcast(rz1.data(), rz1.size() * sizeof(F_primitive), MPI_CHAR, 0, MPI_COMM_WORLD);
    MPI_Bcast(rw1.data(), rw1.size() * sizeof(F_primitive), MPI_CHAR, 0, MPI_COMM_WORLD);

    F_primitive alpha = F_primitive::one(), beta = F_primitive::zero();

    F claimed_v_local = eval_multilinear(circuit.layers.back().output_layer_vals.evals, rz1);
    F claimed_v_global;
    _mpi_rnd_combine(claimed_v_local, claimed_v_global, mpi_combine_coefs);

    for (int i = n_layers - 1; i >= 0; i--)
    {
        if (world_rank == 0)
        {
            std::cout << "Sumchecking at layer " << i << std::endl;
        }

        // SIDE EFFECT: rz1, rz2, rw1 and rw2 will be updated in this function
        sumcheck_prove_gkr_layer<F, F_primitive>(circuit.layers[i], rz1, rz2, rw1, rw2, alpha, beta, transcript, scratch_pad, config);
        
        // prepare alpha and beta for next layer
        if (i != 0)
        {
            if (world_rank == 0)
            {
                alpha = transcript.challenge_f();
                beta = transcript.challenge_f();
            }
            MPI_Bcast(&alpha, sizeof(F_primitive), MPI_CHAR, 0, MPI_COMM_WORLD);
            MPI_Bcast(&beta, sizeof(F_primitive), MPI_CHAR, 0, MPI_COMM_WORLD);
        }
    }
    return {claimed_v_global, rz1, rz2, rw1, rw2};
}

template<typename F, typename F_primitive>
std::tuple<bool, 
std::vector<F_primitive>, std::vector<F_primitive>, 
std::vector<F_primitive>, std::vector<F_primitive>, F, F> 
gkr_verify(
    const Circuit<F, F_primitive>& circuit,
    const std::vector<F>& claimed_v,
    Transcript<F, F_primitive>& transcript,
    Proof<F> &proof,
    const Config &config
)
{
    uint32 world_size = config.mpi_world_size;

    uint32 lg_world_size = log_2(world_size);
    uint32 lg_output_size = circuit.layers.back().nb_output_vars; 
    uint32 n_layers = circuit.layers.size();
    std::vector<F_primitive> rz1, rz2, rw1, rw2;
    std::vector<F_primitive> mpi_combine_coefs(world_size);    
    
    for (uint32 i = 0; i < lg_output_size; i++)
    {
        for(int j = 0; j < config.get_num_repetitions(); j++)
        {
            rz1[j].emplace_back(transcript.challenge_f());
            rz2[j].emplace_back(F_primitive::zero());
        }
    }

    for (uint32 i = 0; i < lg_world_size; i++)
    {
        rw1.emplace_back(transcript.challenge_f());
        rw2.emplace_back(F_primitive::zero());
    }
    _eq_evals_at_primitive(rw1, F_primitive::one(), mpi_combine_coefs.data());
    
    F_primitive alpha = F_primitive::one(), beta = F_primitive::zero();
    auto claimed_v1 = claimed_v;
    std::vector<F> claimed_v2;
    for(int j = 0; j < config.get_num_repetitions(); j++)
    {
        claimed_v2.emplace_back(F::zero());
    }

    bool verified = true;
    for (int i = n_layers - 1; i >= 0; i--)
    {
        verified &= sumcheck_verify_gkr_layer(circuit.layers[i], rz1, rz2, rw1, rw2, claimed_v1, claimed_v2, alpha, beta, proof, transcript, config);

        alpha = transcript.challenge_f();
        beta = transcript.challenge_f();
    }
    return {verified, rz1, rz2, rw1, rw2, claimed_v1, claimed_v2};
}

}