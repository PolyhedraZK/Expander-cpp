#pragma once
#include "poly_commit/poly.hpp"
#include "sumcheck_common.hpp"

namespace gkr
{

// TODO: should probably ask the prover for coefs instead of evals
// Current: given f(0) f(1) f(2) of a degree 2 poly, evaluate it at f(x)
template<typename F, typename F_primitive>
F degree_2_eval(const std::vector<F>& vals, const F_primitive& x)
{
    const F& c0 = vals[0];
    F c2 = F::INV_2 * (vals[2] - vals[1] * 2 + vals[0]);
    F c1 = vals[1] - vals[0] - c2;

    return c0 + (c2 * x + c1) * x;
}

// eval alpha add(rz1, rx) + beta add(rz2, rx)
// or alpha mul(rz1, rx, ry) + beta mul(rz2, rx, ry)
template<typename F, typename F_primitive, uint32 nb_input>
F_primitive eval_sparse_circuit_connect_poly(
    const SparseCircuitConnection<F_primitive, nb_input>& poly,
    const std::vector<F_primitive>& rz1,
    const std::vector<F_primitive>& rz2,
    const F_primitive& alpha,
    const F_primitive& beta,
    const std::vector<std::vector<F_primitive>>& ris)
{
    std::vector<F_primitive> eq_evals_at_rz1(1 << rz1.size());
    std::vector<F_primitive> eq_evals_at_rz2(1 << rz2.size());

    _eq_evals_at_primitive(rz1, alpha, eq_evals_at_rz1.data());
    _eq_evals_at_primitive(rz2, beta, eq_evals_at_rz2.data());
    
    std::vector<std::vector<F_primitive>> eq_evals_at_ris(nb_input);
    for (uint32 i = 0; i < nb_input; i++)
    {
        eq_evals_at_ris[i].resize(1 << (ris[i].size()));
        _eq_evals_at_primitive(ris[i], F_primitive::one(), eq_evals_at_ris[i].data());
    }
    
    F_primitive v = F_primitive::zero();
    for (const Gate<F_primitive, nb_input>& gate: poly.sparse_evals)
    {
        auto prod = (eq_evals_at_rz1[gate.o_id] + eq_evals_at_rz2[gate.o_id]);
        for (uint32 i = 0; i < nb_input; i++)
        {
            prod *= eq_evals_at_ris[i][gate.i_ids[i]];
        }
        v += prod * gate.coef;
    }

    return v;
}


}