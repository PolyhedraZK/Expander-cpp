#pragma once

#include <vector>
#include <functional>

#include "../utils/types.hpp"
#include "../field/utils.hpp"

namespace gkr {

template<typename F>
class UniPoly
{
public:    
    // coefficients, low degree to high degree
    std::vector<F> coefs;

    UniPoly() = default;
    UniPoly(const std::vector<F>& coefs_)
    {
        coefs = coefs_;
    }

    static UniPoly random(uint32 degree) 
    {
        std::vector<F> coefs;
        for(uint32 i = 0; i < degree; ++i) 
        {
            coefs.emplace_back(F::random());
        }
        return coefs;
    }

    F eval_at(const F& x) const 
    {
        F sum = F::zero();
        for (auto it = coefs.rbegin(); it != coefs.rend(); it++) 
        {
            sum = sum * x + *it;
        }
        return sum;
    }

    // divide the polynomial by (X-x)
    UniPoly quotient(const F& x) const 
    {
        uint32 degree = coefs.size();
        // must be a power of 2
        assert((degree > 0) && ((degree & (degree - 1)) == 0));
        uint32 order = __builtin_ctz(degree); 
        F root = get_primitive_root_of_unity<F>(order);

        // it might be more efficient to use long division? 
        // since the (X - x) has degree one
        
        // evaluate (X - x) at powers of root of unities
        std::vector<F> dividend_evals;
        dividend_evals.emplace_back(F::one());
        for (uint32 i = 1; i < degree; i++)
        {
            dividend_evals.emplace_back(dividend_evals[i - 1] * root);
        }
        for (uint32 i = 0; i < degree; i++)
        {
            dividend_evals[i] = dividend_evals[i] - x;
        }
        
        // evaluate the coefs at root of unities
        std::vector<F> evals = ntt(coefs);

        // divide poly by (X - x)
        std::vector<F> quotient_evals;
        for (uint32 i = 0; i < degree; i++)
        {
            quotient_evals.emplace_back(evals[i] * dividend_evals[i].inv());
        }

        return intt(quotient_evals);
    }

    // return the (max degree + 1) / #coefs 
    uint32 degree() const
    { 
        return coefs.size();
    }
};

template<typename F, typename F_primitive>
F eval_multilinear(const std::vector<F>& evals, const std::vector<F_primitive>& x)
{
    assert((1UL << x.size()) == evals.size());
    std::vector<F> scratch = evals;

    uint32 cur_eval_size = evals.size() >> 1;
    for (const F_primitive& r: x)
    {
        for (uint32 i = 0; i < cur_eval_size; i++)
        {
            scratch[i] = scratch[(i << 1)] + (scratch[(i << 1) + 1] - scratch[(i << 1)]) * r;
        }
        cur_eval_size >>= 1;
    }  
    return scratch[0];
}

template<typename F>
class MultiLinearPoly
{
public:
    uint32 nb_vars;
    std::vector<F> evals;

    static MultiLinearPoly random(uint32 nb_vars)
    {
        MultiLinearPoly poly;
        poly.nb_vars = nb_vars;
        uint32 n_evals = 1 << nb_vars;
        for (uint32 i = 0; i < n_evals; i++)
        {
            poly.evals.emplace_back(F::random());
        }
        return poly;
    }
};

} // namespace LinearGKR


