#pragma once

#include <vector>
#include "circuit/circuit.hpp"

namespace gkr
{
    
template<typename F_primitive>
inline F_primitive _eq(const F_primitive& x, const F_primitive& y)
{
    // x * y + (1 - x) * (1 - y)
    return x * y * 2 - x - y + 1;
}

template<typename F_primitive>
inline F_primitive _eq(const F_primitive& x, const F_primitive& y, const F_primitive& z)
{
    return x * y * z + (F_primitive::one() - x) * (F_primitive::one() - y) * (F_primitive::one() - z);
}

template<typename F_primitive>
F_primitive _eq_vec(const std::vector<F_primitive> &x, const std::vector<F_primitive> &y)
{
    assert(x.size() == y.size());
    F_primitive prod = F_primitive::one();
    for (uint32 i = 0; i < x.size(); i++)
    {
        prod *= _eq(x[i], y[i]);
    }
    return prod;
}


template<typename F_primitive>
F_primitive _eq_vec(const std::vector<F_primitive> &x, const std::vector<F_primitive> &y, const std::vector<F_primitive> &z)
{
    assert(x.size() == y.size());
    assert(x.size() == z.size());

    F_primitive prod = F_primitive::one();
    for (uint32 i = 0; i < x.size(); i++)
    {
        prod *= _eq(x[i], y[i], z[i]);
    }
    return prod;
}

template<typename F_primitive>
F_primitive _eq(const std::vector<F_primitive> &rw, uint32 world_rank, uint32 lg_world_size)
{
    assert(rw.size() == lg_world_size);

    F_primitive prod = F_primitive::one();
    for (uint32 i = 0; i < lg_world_size; i++)
    {
        if (((world_rank >> i) & 1) == 0)
        {
            prod *= F_primitive::one() - rw[i];
        }
        else
        {
            prod *= rw[i];
        }
    }
    return prod;
}

template<typename F_primitive>
void _eq_evals_at_primitive(const std::vector<F_primitive>& r, const F_primitive& mul_factor, F_primitive* eq_evals)
{
    eq_evals[0] = mul_factor;
    uint32 nb_cur_evals = 1;
    for (uint32 i = 0; i < r.size(); i++)
    {
        F_primitive eq_z_i_zero = _eq(r[i], F_primitive::zero());
        F_primitive eq_z_i_one = _eq(r[i], F_primitive::one());

        for (uint32 j = 0; j < nb_cur_evals; j++)
        {
            eq_evals[j + nb_cur_evals] = eq_evals[j] * eq_z_i_one;
            eq_evals[j] = eq_evals[j] * eq_z_i_zero;
        }
        nb_cur_evals <<= 1;
    }
}

// compute the multilinear extension eq(a, b) at 
// a = r, b = bit at all bits
// the bits are interpreted as little endian numbers
// The returned value is multiplied by the 'mul_factor' argument
template<typename F_primitive>
void _eq_evals_at(const std::vector<F_primitive>& r, const F_primitive& mul_factor, F_primitive* eq_evals, F_primitive* sqrtN1st, F_primitive* sqrtN2nd)
{
    auto first_half_bits = r.size() / 2;
    auto first_half_mask = (1 << first_half_bits) - 1;
    _eq_evals_at_primitive(std::vector<F_primitive>(r.begin(), r.begin() + first_half_bits), mul_factor, sqrtN1st);
    _eq_evals_at_primitive(std::vector<F_primitive>(r.begin() + first_half_bits, r.end()), F_primitive(1), sqrtN2nd);

    for (uint32 i = 0; i < (uint32)(1 << r.size()); i++)
    {
        uint32 first_half = i & first_half_mask;
        uint32 second_half = i >> first_half_bits;
        eq_evals[i] = sqrtN1st[first_half] * sqrtN2nd[second_half];
    }
}

} // namespace LinearGKR
