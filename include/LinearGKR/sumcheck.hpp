#pragma once

#include<cassert>

#include "utils/types.hpp"
#include "fiat_shamir/transcript.hpp"
#include "utils/myutil.hpp"
#include "poly_commit/poly.hpp"
#include "sumcheck_helper.hpp"
#include "sumcheck_verifier_utils.hpp"
#include <map>
namespace gkr
{

template<typename F>
struct SumcheckProof
{
    std::vector<std::vector<F>> low_degree_poly_evals;
};


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
    // TODO: CHECK FINAL EVALUATION

    return verified;
}

template<typename F, typename F_primitive>
std::tuple<std::vector<std::vector<F_primitive>>, std::vector<std::vector<F_primitive>>> sumcheck_prove_gkr_layer(
    const CircuitLayer<F, F_primitive>& poly,
    const std::vector<std::vector<F_primitive>>& rz1,
    const std::vector<std::vector<F_primitive>>& rz2,
    const F_primitive& alpha,
    const F_primitive& beta,
    Transcript<F, F_primitive>& transcript,
    GKRScratchPad<F, F_primitive> *scratch_pad,
    Timing &timer
)
{
    SumcheckGKRHelper<F, F_primitive> helper[3];
    timer.add_timing("    prepare time");
    for(int i = 0; i < 3; i++)
    {
        helper[i].prepare(poly, rz1[i], rz2[i], alpha, beta, scratch_pad[i], timer);
    }
    timer.report_timing("    prepare time");
    for (uint32 i_var = 0; i_var < (2 * poly.nb_input_vars); i_var++)
    {
        for (int j = 0; j < 3; j++)
        {
            timer.add_timing("    eval poly " + std::to_string(i_var) + " time");
            std::vector<F> evals = helper[j].poly_evals_at(i_var, 2, timer);
            timer.report_timing("    eval poly " + std::to_string(i_var) + " time");
            timer.add_timing("    append evals " + std::to_string(i_var) + " time");
            transcript.append_f(evals[0]);
            transcript.append_f(evals[1]);
            transcript.append_f(evals[2]);
            auto r = transcript.challenge_f();
            timer.report_timing("    append evals " + std::to_string(i_var) + " time");

            timer.add_timing("    receive challenge " + std::to_string(i_var) + " time");
            helper[j].receive_challenge(i_var, r);
            timer.report_timing("    receive challenge " + std::to_string(i_var) + " time");

            if (i_var == poly.nb_input_vars - 1)
            {
                timer.add_timing("    vx_claim time");
                transcript.append_f(helper[j].vx_claim());
                timer.report_timing("    vx_claim time");
            }
        }
    }
    for(int j = 0; j < 3; j++)
    {
        timer.add_timing("  vy_claim time");
        transcript.append_f(helper[j].vy_claim());
        timer.report_timing("  vy_claim time");
    }
    std::vector<std::vector<F_primitive>> rz1s, rz2s;
    for(int j = 0; j < 3; j++)
    {
        rz1s.emplace_back(helper[j].rx);
        rz2s.emplace_back(helper[j].ry);
    }
    return {rz1s, rz2s};
}

template<typename F, typename F_primitive>
std::tuple<bool, std::vector<F_primitive>, std::vector<F_primitive>, F, F> sumcheck_verify_gkr_layer(
    const CircuitLayer<F, F_primitive>& poly,
    const std::vector<F_primitive>& rz1,
    const std::vector<F_primitive>& rz2,
    const F& claimed_v1,
    const F& claimed_v2,
    const F_primitive& alpha,
    const F_primitive& beta,
    Proof<F>& proof,
    Transcript<F, F_primitive>& transcript
)
{
    uint32 nb_vars = poly.nb_input_vars;
    F sum = claimed_v1 * alpha + claimed_v2 * beta;
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
            sum -= vx_claim * eval_sparse_circuit_connect_poly<F, F_primitive, 1>(poly.add, rz1, rz2, alpha, beta, {rx});
            transcript.append_f(vx_claim);
            rs = &ry;
        }
    }
    const F vy_claim = proof.get_next_and_step();
    verified &= sum == vx_claim * vy_claim * eval_sparse_circuit_connect_poly<F, F_primitive, 2>(poly.mul, rz1, rz2, alpha, beta, {rx, ry});
    transcript.append_f(vy_claim);

    return {verified, rx, ry, vx_claim, vy_claim};
}


}