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
    uint32 nb_vars,
    std::vector<F_primitive> &rz,
    const F& claimed_sum,
    const Proof<F>& proof,
    Transcript<F, F_primitive>& transcript
)
{
    rz.clear();
    F sum = claimed_sum;
    bool verified = true;
    for (uint32 i_var = 0; i_var < nb_vars; i_var++)
    {
        const std::vector<F> low_degree_evals = {proof.get_next_and_step(), proof.get_next_and_step()};

        transcript.append_f(low_degree_evals[0]);
        transcript.append_f(low_degree_evals[1]);
        F_primitive r = transcript.challenge_f();
        rz.emplace_back(r);

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
    std::vector<F_primitive>& rw1,
    std::vector<F_primitive>& rw2,
    const F_primitive& alpha,
    const F_primitive& beta,
    Transcript<F, F_primitive>& transcript,
    GKRScratchPad<F, F_primitive> &scratch_pad,
    const Config &config
)
{
    uint32 world_rank, world_size;
    MPI_Comm_size(MPI_COMM_WORLD, reinterpret_cast<int*>(&world_size));
    MPI_Comm_rank(MPI_COMM_WORLD, reinterpret_cast<int*>(&world_rank));
    uint32 lg_world_size = log_2(world_size);

    // v_next(rx), v_next(ry), v(ry) \cdot h(ry)
    F vx_claim, vy_claim, reduced_claim;

    SumcheckGKRHelper<F, F_primitive> helper; 
    helper.prepare(poly, rz1, rz2, rw1, rw2, alpha, beta, scratch_pad);

    for (uint32 i_var = 0; i_var < (2 * (poly.nb_input_vars + lg_world_size)); i_var++)
    {
        std::vector<F> evals = helper.poly_evals_at(i_var, 2);
        
        F_primitive r;
        if (world_rank == 0)
        {
            transcript.append_f(evals[0]);
            transcript.append_f(evals[1]);
            transcript.append_f(evals[2]);
            r = transcript.challenge_f();
        }
        MPI_Bcast(&r, sizeof(F_primitive), MPI_CHAR, 0, MPI_COMM_WORLD);

        helper.receive_challenge(i_var, r);
        if (i_var == poly.nb_input_vars + lg_world_size - 1)
        {
            if (world_rank == 0)
            {
                vx_claim = helper.vx_claim();
                transcript.append_f(vx_claim);
            }
        }
    }

    if(world_rank == 0)
    {
        vy_claim = helper.vy_claim();
        transcript.append_f(vy_claim);
    }
    
    rz1 = helper.rx;
    rz2 = helper.ry;
    rw1 = helper.rwx;
    rw2 = helper.rwy;    
}

template<typename F, typename F_primitive>
bool sumcheck_verify_gkr_layer(
    const CircuitLayer<F, F_primitive>& poly,
    std::vector<F_primitive> &rz1,
    std::vector<F_primitive> &rz2,
    std::vector<F_primitive> &rw1,
    std::vector<F_primitive> &rw2,
    F& claimed_v1,
    F& claimed_v2,
    const F_primitive& alpha,
    const F_primitive& beta,
    Proof<F>& proof,
    Transcript<F, F_primitive>& transcript,
    const Config &config
)
{
    uint32 lg_world_size = log_2(config.mpi_world_size);
    uint32 nb_vars = poly.nb_input_vars;

    F claimed_v = claimed_v1 * alpha + claimed_v2 * beta;
    F cst = eval_sparse_circuit_connect_poly(poly.cst, rz1, rz2, rw1, rw2, alpha, beta, {}, {}, {});
    claimed_v -= cst;

    // start verification
    std::vector<F_primitive> rx, ry, rwx, rwy;
    std::vector<F_primitive> *rs = &rx;
    F vx_claim;
    bool verified = true;
    std::vector<F> low_degree_evals;
    for (uint32 i_var = 0; i_var < (2 * (nb_vars + lg_world_size)); i_var++)
    {
        low_degree_evals = {proof.get_next_and_step(), proof.get_next_and_step(), proof.get_next_and_step()};

        transcript.append_f(low_degree_evals[0]);
        transcript.append_f(low_degree_evals[1]);
        transcript.append_f(low_degree_evals[2]);
        F_primitive r = transcript.challenge_f();

        rs->emplace_back(r);
        verified &= (low_degree_evals[0] + low_degree_evals[1]) == claimed_v;
        claimed_v = degree_2_eval(low_degree_evals, r);

        if (i_var == nb_vars - 1)
        {
            rs = &rwx;
        }

        if (i_var == nb_vars + lg_world_size - 1)
        {
            vx_claim = proof.get_next_and_step();
            transcript.append_f(vx_claim);
            claimed_v -= vx_claim * eval_sparse_circuit_connect_poly(poly.add, rz1, rz2, rw1, rw2, alpha, beta, {rx}, rwx, rwy);
            rs = &ry;
        }

        if (i_var == nb_vars * 2 + lg_world_size - 1)
        {
            rs = &rwy;
        }
    }
    

    F vy_claim = proof.get_next_and_step();
    transcript.append_f(vy_claim);

    claimed_v -= vx_claim * vy_claim * eval_sparse_circuit_connect_poly(poly.mul, rz1, rz2, rw1, rw2, alpha, beta, {rx, ry}, rwx, rwy);
    verified &= claimed_v == F::zero();

    rz1 = rx;
    rz2 = ry;
    rw1 = rwx;
    rw2 = rwy;
    claimed_v1 = vx_claim;
    claimed_v2 = vy_claim;
    
    return verified;
}


}