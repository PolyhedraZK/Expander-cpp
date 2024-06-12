#pragma once

#include<cassert>
#include<mpi.h>

#include "mpi_utils.hpp"
#include "utils/types.hpp"
#include "fiat_shamir/transcript.hpp"
#include "utils/myutil.hpp"
#include "poly_commit/poly.hpp"
#include "sumcheck_helper.hpp"
#include "sumcheck_verifier_utils.hpp"
#include "configuration/config.hpp"
#include <map>
namespace gkr
{

template<typename F>
struct SumcheckProof
{
    std::vector<std::vector<F>> low_degree_poly_evals;
};

template<typename F, typename F_primitive>
std::vector<F_primitive> sumcheck_prove_multilinear(
    const std::vector<F> &evals,
    Transcript<F, F_primitive> &transcript,
    GKRScratchPad<F, F_primitive> &scratch_pad
)
{
    uint32 nb_vars = log_2(evals.size());
    SumcheckMultiLinearHelper<F, F_primitive> helper;
    helper.prepare(nb_vars, scratch_pad.v_evals, evals.data());
    
    for (uint32 i_var = 0; i_var < nb_vars; i_var++)
    {
        std::vector<F> evals = helper.poly_eval_at(i_var);
        transcript.append_f(evals[0]);
        transcript.append_f(evals[1]);
        F_primitive r = transcript.challenge_f();
        helper.receive_challenge(i_var, r);
    }

    return helper.rs; 
}

template<typename F, typename F_primitive>
bool sumcheck_verify_multilinear(
    const MultiLinearPoly<F>& poly,
    const F& claimed_sum,
    const SumcheckProof<F>& proof,
    Transcript<F, F_primitive>& transcript
)
{
    uint32 nb_vars = poly.nb_vars;

    F sum = claimed_sum;
    bool verified = true;
    for (uint32 i_var = 0; i_var < nb_vars; i_var++)
    {
        const std::vector<F>& low_degree_evals = proof.low_degree_poly_evals[i_var];
        assert(low_degree_evals.size() == 2);

        transcript.append_f(low_degree_evals[0]);
        transcript.append_f(low_degree_evals[1]);
        auto r = transcript.challenge_f();

        verified &= (low_degree_evals[0] + low_degree_evals[1]) == sum;
        sum = low_degree_evals[0] + (low_degree_evals[1] - low_degree_evals[0]) * r;
    }

    return verified;
}

template<typename F, typename F_primitive>
std::tuple<std::vector<F_primitive>, std::vector<F_primitive>, std::vector<F_primitive>> sumcheck_prove_gkr_layer(
    const CircuitLayer<F, F_primitive>& poly,
    const std::vector<F_primitive>& rz1,
    const std::vector<F_primitive>& rz2,
    const std::vector<F_primitive>& rand_coefs,
    const F_primitive& alpha,
    const F_primitive& beta,
    Transcript<F, F_primitive>& transcript,
    GKRScratchPad<F, F_primitive> &scratch_pad,
    const Config &config
)
{
    int world_rank, world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    std::vector<F> vx_claims(world_size), vy_claims(world_size);

    SumcheckGKRHelper<F, F_primitive> helper; 
    helper.prepare(poly, rz1, rz2, alpha, beta, scratch_pad);

    for (uint32 i_var = 0; i_var < (2 * poly.nb_input_vars); i_var++)
    {
        std::vector<F> evals = helper.poly_evals_at(i_var, 2);
        std::vector<F> evals_global(3);
        
        _mpi_rnd_combine(evals[0], evals_global[0], rand_coefs);
        _mpi_rnd_combine(evals[1], evals_global[1], rand_coefs);
        _mpi_rnd_combine(evals[2], evals_global[2], rand_coefs);

        F_primitive r;
        if (world_rank == 0)
        {
            transcript.append_f(evals_global[0]);
            transcript.append_f(evals_global[1]);
            transcript.append_f(evals_global[2]);
            r = transcript.challenge_f();
        }
        MPI_Bcast(&r, sizeof(F_primitive), MPI_CHAR, 0, MPI_COMM_WORLD);

        helper.receive_challenge(i_var, r);
        if (i_var == poly.nb_input_vars - 1)
        {
            F vx_claim = helper.vx_claim();
            if (world_rank == 0)
            {
                MPI_Gather(&vx_claim, sizeof(F), MPI_CHAR, vx_claims.data(), sizeof(F), MPI_CHAR, 0, MPI_COMM_WORLD);
            }
        }
    }

    F vy_claim = helper.vy_claim();
    MPI_Gather(&vy_claim, sizeof(F), MPI_CHAR, vy_claims.data(), sizeof(F), MPI_CHAR, 0, MPI_COMM_WORLD);

    F vy_claim_global;
    std::vector<F_primitive> r_world;
    if(world_rank == 0)
    {
        vy_claim_global = F::zero();
        for (uint32 i = 0; i < world_size; i++)
        {
            vy_claim_global += vy_claims[i] * rand_coefs[i];
        }
        transcript.append_f(vy_claim_global);
        r_world = sumcheck_prove_multilinear(vy_claims, transcript, scratch_pad);
    }
    
    return {helper.rx, helper.ry, r_world};
}

template<typename F, typename F_primitive>
std::tuple<bool, std::vector<F_primitive>, std::vector<F_primitive>, F, F > sumcheck_verify_gkr_layer(
    const CircuitLayer<F, F_primitive>& poly,
    const std::vector<F_primitive>& rz1,
    const std::vector<F_primitive>& rz2,
    const F& claimed_v1,
    const F& claimed_v2,
    const F_primitive& alpha,
    const F_primitive& beta,
    Proof<F>& proof,
    Transcript<F, F_primitive>& transcript,
    const Config &config
)
{
    uint32 nb_vars = poly.nb_input_vars;
    F sum = claimed_v1 * alpha + claimed_v2 * beta -
        eval_sparse_circuit_connect_poly(poly.cst, rz1, rz2, alpha, beta, std::vector<std::vector<F_primitive>>{});
    
    std::vector<F_primitive> rx, ry;
    std::vector<F_primitive> *rs = &rx;
    F vx_claim;
    bool verified = true;
    for (uint32 i_var = 0; i_var < (2 * nb_vars); i_var++)
    {

        const std::vector<F> low_degree_evals = {proof.get_next_and_step(), proof.get_next_and_step(), proof.get_next_and_step()};

        transcript.append_f(low_degree_evals[0]);
        transcript.append_f(low_degree_evals[1]);
        transcript.append_f(low_degree_evals[2]);
        auto r = transcript.challenge_f();

        rs->emplace_back(r);
        verified &= (low_degree_evals[0] + low_degree_evals[1]) == sum;
        sum = degree_2_eval(low_degree_evals, r);

        if (i_var == nb_vars - 1)
        {
            vx_claim = proof.get_next_and_step();
            sum -= vx_claim * eval_sparse_circuit_connect_poly(poly.add, rz1, rz2, alpha, beta, {rx});
            transcript.append_f(vx_claim);
            rs = &ry;
        }
    }
    
    F vy_claim = proof.get_next_and_step();
    verified &= sum == vx_claim * vy_claim * eval_sparse_circuit_connect_poly(poly.mul, rz1, rz2, alpha, beta, {rx, ry});
    transcript.append_f(vy_claim);
    
    return {verified, rx, ry, vx_claim, vy_claim};
}


}