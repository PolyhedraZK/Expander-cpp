#pragma once

#include<cassert>
#include<vector>
#include<tuple>

#include "../utils/types.hpp"

namespace gkr 
{

    // TODO: write a more efficient version
    template<typename F>
    std::vector<F> ntt(const std::vector<F>& coefs)
    {   
        uint32 size = coefs.size();
        assert(size > 0 && ( (size & (size - 1)) == 0)); // test power of 2
        uint32 order = __builtin_ctz(size); // log_2(len)
        
        F primitive_root_of_unity = get_primitive_root_of_unity<F>(order);
        std::vector<F> evals = _ntt_recurse(primitive_root_of_unity, coefs);

        return evals;
    }

    template<typename F>
    std::vector<F> intt(const std::vector<F>& evals) 
    {
        uint32 size = evals.size();
        assert(size > 0 && ( (size & (size - 1)) == 0));
        uint32 order = __builtin_ctz(size); 
        F primitive_root_of_unity_inv = get_primitive_root_of_unity<F>(order).inv();
        
        std::vector<F> coefs = _ntt_recurse(primitive_root_of_unity_inv, evals);

        F size_inv = F(size).inv();
        for(int i = 0; i < size; ++i) {
            coefs[i] = coefs[i] * size_inv;
        }
        return coefs;
    }
    
    template<typename F>
    F get_primitive_root_of_unity(uint32 order) 
    {
        F max_order_primitive_root_of_unity;
        uint32 max_order;
        std::tie(max_order_primitive_root_of_unity, max_order) = F::max_order_primitive_root_of_unity();
        assert(max_order >= order);
        return max_order_primitive_root_of_unity.exp(1 << (max_order - order));
    }


    template<typename F>
    std::vector<F> ntt_expand(const std::vector<F>& coefs, uint32 expansion_factor)
    {
        std::vector<F> coefs_expanded(coefs.begin(), coefs.end());
        coefs_expanded.resize(coefs.size() * expansion_factor); // assuming the default constructor gives F::zero();
        return ntt(coefs_expanded);
    }

    template<typename F>
    std::vector<F> _ntt_recurse(const F& primitive_root_of_unity, const std::vector<F>& coefs) 
    {   
        uint32 size = coefs.size();
        if (size == 1) 
        {
            return {coefs[0]};
        }
        
        std::vector<F> coefs_odd;
        std::vector<F> coefs_even;
        for (uint32 i = 0; i < size; i++)
        {
            if ((i & 1) == 0)
            {
                coefs_even.emplace_back(coefs[i]);
            }
            else 
            {
                coefs_odd.emplace_back(coefs[i]);
            }
        }
        
        F r_square = primitive_root_of_unity * primitive_root_of_unity;
        std::vector<F> evals_odd = _ntt_recurse(r_square, coefs_odd);
        std::vector<F> evals_even = _ntt_recurse(r_square, coefs_even);

        std::vector<F> evals(size, F::zero());
        F pow = F::one();
        for (uint32 i = 0; i < size >> 1; i++)
        {
            evals[i] = evals_even[i] + pow * evals_odd[i];
            evals[i + (size >> 1)] = evals_even[i] - pow * evals_odd[i];
            pow *= primitive_root_of_unity;
        }
        return evals;
    }

}