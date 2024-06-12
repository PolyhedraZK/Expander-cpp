#pragma once

#include<cassert>
#include<mpi.h>
#include<numeric>

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
void sumcheck_prove_multilinear(
    const std::vector<F> &evals,
    std::vector<F_primitive> &rz,
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

    rz = helper.rx; 
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
void sumcheck_prove_gkr_layer(
    const CircuitLayer<F, F_primitive>& poly,
    std::vector<F_primitive>& rz1,
    std::vector<F_primitive>& rz2,
    std::vector<F_primitive>& r_mpi,
    std::vector<F_primitive>& mpi_combine_coefs,
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

    // v_next(rx), v_next(ry), v(ry) \cdot h(ry)
    F vx_claim, vy_claim, reduced_claim;
    std::vector<F> vx_claims(world_size), vy_claims(world_size), reduced_claims(world_size);

    SumcheckGKRHelper<F, F_primitive> helper; 
    helper.prepare(poly, rz1, rz2, alpha, beta, scratch_pad);

    for (uint32 i_var = 0; i_var < (2 * poly.nb_input_vars); i_var++)
    {
        std::vector<F> evals = helper.poly_evals_at(i_var, 2);
        std::vector<F> evals_global(3);
        
        _mpi_rnd_combine(evals[0], evals_global[0], mpi_combine_coefs);
        _mpi_rnd_combine(evals[1], evals_global[1], mpi_combine_coefs);
        _mpi_rnd_combine(evals[2], evals_global[2], mpi_combine_coefs);

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
            vx_claim = helper.vx_claim();
        }
    }

    vy_claim = helper.vy_claim();
    reduced_claim = helper.reduced_claim();

    MPI_Gather(&vx_claim, sizeof(F), MPI_CHAR, vx_claims.data(), sizeof(F), MPI_CHAR, 0, MPI_COMM_WORLD);
    MPI_Gather(&vy_claim, sizeof(F), MPI_CHAR, vy_claims.data(), sizeof(F), MPI_CHAR, 0, MPI_COMM_WORLD);
    MPI_Gather(&reduced_claim, sizeof(F), MPI_CHAR, reduced_claims.data(), sizeof(F), MPI_CHAR, 0, MPI_COMM_WORLD);

    if(world_rank == 0)
    {
        assert(reduced_claims.size() == world_size);
        for (uint32 i = 0; i < reduced_claims.size(); i++)
        {
            reduced_claims[i] = reduced_claims[i] * mpi_combine_coefs[i];
        }
        sumcheck_prove_multilinear(reduced_claims, r_mpi, transcript, scratch_pad);
        _eq_evals_at_primitive(r_mpi, F_primitive::one(), mpi_combine_coefs.data());

        F vx_claim_global = F::zero(), vy_claim_global = F::zero();
        for (uint32 i = 0; i < world_size; i++)
        {
            vx_claim_global += vx_claims[i] * mpi_combine_coefs[i];
            vy_claim_global += vy_claims[i] * mpi_combine_coefs[i];
        }
        transcript.append_f(vx_claim_global);
        transcript.append_f(vy_claim_global);
    }
    
    rz1 = helper.rx;
    rz2 = helper.ry;
}

template<typename F, typename F_primitive>
bool sumcheck_verify_gkr_layer(
    const CircuitLayer<F, F_primitive>& poly,
    std::vector<F_primitive> &rz1,
    std::vector<F_primitive> &rz2,
    std::vector<F_primitive> &r_mpi,
    std::vector<F_primitive> &mpi_combine_coefs,
    F& claimed_v1,
    F& claimed_v2,
    const F_primitive& alpha,
    const F_primitive& beta,
    Proof<F>& proof,
    Transcript<F, F_primitive>& transcript,
    const Config &config
)
{
    uint32 nb_vars = poly.nb_input_vars;
    F claimed_v = claimed_v1 * alpha + claimed_v2 * beta;
    
    F summation_of_mpi_combine_coefs = std::accumulate(mpi_combine_coefs.begin(), mpi_combine_coefs.end(), F_primitive::zero());
    F cst = eval_sparse_circuit_connect_poly(poly.cst, rz1, rz2, alpha, beta, std::vector<std::vector<F_primitive>>{});
    claimed_v -= summation_of_mpi_combine_coefs * cst;

    // first parse the proof
    std::vector<std::vector<F>> low_degree_polys(2 * nb_vars);
    for(uint32 i_var = 0; i_var < (2 * nb_vars); i_var++)
    {
        std::vector<F> &low_degree_evals = low_degree_polys[i_var];
        low_degree_evals.clear();

        // evals at 0 1 and 2
        low_degree_evals.emplace_back(proof.get_next_and_step());
        low_degree_evals.emplace_back(proof.get_next_and_step());
        low_degree_evals.emplace_back(proof.get_next_and_step());
    }
    F vx_claim = proof.get_next_and_step();
    F vy_claim = proof.get_next_and_step();

    // start verification
    std::vector<F_primitive> rx, ry;
    std::vector<F_primitive> *rs = &rx;
    bool verified = true;
    for (uint32 i_var = 0; i_var < (2 * nb_vars); i_var++)
    {

        const std::vector<F> &low_degree_evals = low_degree_polys[i_var];

        transcript.append_f(low_degree_evals[0]);
        transcript.append_f(low_degree_evals[1]);
        transcript.append_f(low_degree_evals[2]);
        F_primitive r = transcript.challenge_f();

        rs->emplace_back(r);
        verified &= (low_degree_evals[0] + low_degree_evals[1]) == claimed_v;
        claimed_v = degree_2_eval(low_degree_evals, r);

        if (i_var == nb_vars - 1)
        {
            claimed_v -= vx_claim * eval_sparse_circuit_connect_poly(poly.add, rz1, rz2, alpha, beta, {rx});
            rs = &ry;
        }
    }
    
    verified &= sum == vx_claim * vy_claim * eval_sparse_circuit_connect_poly(poly.mul, rz1, rz2, alpha, beta, {rx, ry});
    
    transcript.append_f(vx_claim);
    transcript.append_f(vy_claim);
        
    // side effect and return:
    rz1 = rx;
    rz2 = ry;
    claimed_v1 = vx_claim;
    claimed_v2 = vy_claim;
    
    return verified;
}


}