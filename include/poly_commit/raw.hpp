#pragma once

// Raw POLYNOMIAL COMMITMENT
// This file contains the raw polynomial commitment scheme.
// The scheme will transmit all the coefficients of the polynomial.
// With tar.gz compression

// This will be used if the polynomial is small enough and ZK is not needed
#include "poly.hpp"

void compress(const std::vector<char>& input, std::vector<char>& output)
{
    output = input;
}

template<typename F>
char* commit(gkr::MultiLinearPoly<F> poly)
{
    // Serialize the polynomial
    std::vector<char> serialized;
    for (F eval : poly.evals)
    {
        std::vector<char> eval_bytes = eval.to_bytes();
        serialized.insert(serialized.end(), eval_bytes.begin(), eval_bytes.end());
    }

    // Compress the serialized polynomial
    std::vector<char> compressed;
    compress(serialized, compressed);

    // Return the compressed polynomial
    return compressed.data();
}

template<typename F>
char* open(const gkr::MultiLinearPoly<F>& poly, const F& x) // no need to open, because the verifier got all coefficients
{
    return NULL;
}

