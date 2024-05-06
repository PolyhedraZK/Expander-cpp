#pragma once

#include <cassert>
#include <libff/algebra/curves/bn128/bn128_pairing.hpp>

#include "../utils/types.hpp"
#include "field/bn254_fr.hpp"
#include "poly.hpp"

namespace gkr {
namespace kzg {

// only support the bn254 field for now
using F = libff::bn128_Fr;
using FF = bn254_fr::BN254_Fr;
using G1 = libff::bn128_G1;
using G2 = libff::bn128_G2;
using GT = libff::bn128_GT;


struct KZGSetup 
{
    std::vector<G1> pows_g1;
    G2 g2_tau;
};

struct KZGCommitment 
{
    G1 c;
};

struct KZGOpening 
{
    G1 p;
    FF v;
};

KZGSetup setup(const FF& secret, uint32 degree) 
{
    KZGSetup _setup;
    _setup.pows_g1.emplace_back(G1::G1_one);

    for (uint32 i = 1; i < degree; i++) {
        _setup.pows_g1.emplace_back(secret.f * _setup.pows_g1[i - 1]);
    }
    _setup.g2_tau = secret.f * G2::G2_one;
    return _setup;
}

KZGCommitment commit(const UniPoly<FF>& poly, const KZGSetup& setup) 
{
    G1 sum = G1::G1_zero;
    for (uint32 i = 0; i < poly.coefs.size(); i++) {
        sum = sum + poly.coefs[i].f * setup.pows_g1[i];
    }
    KZGCommitment commitment;
    commitment.c = sum;
    return commitment;
}

KZGOpening open(const UniPoly<FF>& poly, const FF& x, const KZGSetup& setup) 
{
    KZGOpening opening;
    opening.v = poly.eval_at(x);

    UniPoly<FF> dividend = poly;
    dividend.coefs[0] = dividend.coefs[0] - opening.v;
    UniPoly<FF> quotient = dividend.quotient(x);    
    assert(quotient.coefs[quotient.coefs.size() - 1] == FF::zero());

    // FF xx = FF::random();
    // FF yy = dividend.eval_at(xx);
    // FF yyy = quotient.eval_at(xx) * (xx - x);
    // assert(yy == yyy);

    G1 sum = G1::G1_zero;
    for (uint32 i = 0; i < quotient.coefs.size(); i++) {
        sum = sum + quotient.coefs[i].f * setup.pows_g1[i];
    }
    opening.p = sum;
    return opening;
}

GT pairing(const G1& g1, const G2& g2) 
{
    return libff::bn128_final_exponentiation(
    libff::bn128_ate_miller_loop(
        libff::bn128_ate_precompute_G1(g1),
        libff::bn128_ate_precompute_G2(g2)
    ));
}

GT double_pairing(const G1& g1, const G2& g2, const G1& gg1, const G2& gg2)
{
    return libff::bn128_final_exponentiation(
        libff::bn128_double_ate_miller_loop(
            libff::bn128_ate_precompute_G1(g1),
            libff::bn128_ate_precompute_G2(g2),
            libff::bn128_ate_precompute_G1(gg1),
            libff::bn128_ate_precompute_G2(gg2)
        )
    );
} 

bool verify(const UniPoly<FF>& poly, const FF& x, const KZGSetup& setup, const KZGCommitment& commitment, KZGOpening& opening) 
{
    G1 g1 = (x.f * opening.p) + (commitment.c - opening.v.f * G1::G1_one);
    G2 g2 = G2::G2_one;
    G1 gg1 = -opening.p;
    G2 gg2 = setup.g2_tau;
    return double_pairing(g1, g2, gg1, gg2) == GT::GT_one;
}

}
}

