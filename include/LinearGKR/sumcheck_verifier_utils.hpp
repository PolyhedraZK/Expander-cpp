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
template<typename F_primitive, uint32 nb_input>
F_primitive eval_sparse_circuit_connect_poly(
    const SparseCircuitConnection<F_primitive, nb_input>& poly,
    const std::vector<F_primitive>& rz1,
    const std::vector<F_primitive>& rz2,
    const std::vector<F_primitive>& rw1,
    const std::vector<F_primitive>& rw2,
    const F_primitive& alpha,
    const F_primitive& beta,
    const std::vector<std::vector<F_primitive>>& ris,
    const std::vector<F_primitive>& rwx,
    const std::vector<F_primitive>& rwy)
{
    std::vector<F_primitive> eq_evals_at_rz1(1 << rz1.size());
    std::vector<F_primitive> eq_evals_at_rz2(1 << rz2.size());

    F_primitive coef_1 = F_primitive::one();
    F_primitive coef_2 = F_primitive::one();

    if (nb_input == 0)
    {
        std::vector<F_primitive> eq_evals_at_rw(1 << rw1.size());
        _eq_evals_at_primitive(rw1, F_primitive::one(), eq_evals_at_rw.data());
        coef_1 = std::accumulate(eq_evals_at_rw.begin(), eq_evals_at_rw.end(), F_primitive::zero());
   
        _eq_evals_at_primitive(rw2, F_primitive::one(), eq_evals_at_rw.data());
        coef_2 = std::accumulate(eq_evals_at_rw.begin(), eq_evals_at_rw.end(), F_primitive::zero());
    }
    else if (nb_input == 1)
    {
        coef_1 = _eq_vec(rw1, rwx);
        coef_2 = _eq_vec(rw2, rwx);
    }
    else if (nb_input == 2)
    {
        coef_1 = _eq_vec(rw1, rwx, rwy);
        coef_2 = _eq_vec(rw2, rwx, rwy);
    }

    _eq_evals_at_primitive(rz1, alpha * coef_1, eq_evals_at_rz1.data());
    _eq_evals_at_primitive(rz2, beta * coef_2, eq_evals_at_rz2.data());
    
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