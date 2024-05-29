#pragma once

#include <cassert>

#include "utils/types.hpp"
#include "field/bn254_fr.hpp"
#include "poly.hpp"

namespace gkr {

// Fr, G1, G2, GT

using namespace mcl::bn; 

// only support the bn254 field for now
using FF = bn254fr::BN254_Fr;

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
    _setup.pows_g1.resize(degree);
    _setup.pows_g1[0] = G1_ONE;

    for (uint32 i = 1; i < degree; i++) {
        G1::mul(_setup.pows_g1[i], _setup.pows_g1[i - 1], secret.mcl_data);
    }
    G2::mul(_setup.g2_tau, G2_ONE, secret.mcl_data);
    return _setup;
}

KZGCommitment commit(const UniPoly<FF>& poly, const KZGSetup& setup) 
{
    G1 sum, tmp;
    sum.clear();
    for (uint32 i = 0; i < poly.coefs.size(); i++) {
        G1::mul(tmp, setup.pows_g1[i], poly.coefs[i].mcl_data);
        G1::add(sum, sum, tmp);
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

    G1 sum, tmp;
    sum.clear();
    for (uint32 i = 0; i < quotient.coefs.size(); i++) {
        G1::mul(tmp, setup.pows_g1[i], quotient.coefs[i].mcl_data);
        G1::add(sum, sum, tmp);
    }
    opening.p = sum;
    return opening;
}

bool verify(const UniPoly<FF>& poly, const FF& x, const KZGSetup& setup, const KZGCommitment& commitment, KZGOpening& opening) 
{
    // G1 g1 = (x.f * opening.p) + (commitment.c - opening.v.f * G1::G1_one);
    G1 g1[2], tmp;
    G2 g2[2];
    
    // g1[0]
    G1::mul(tmp, opening.p, x.mcl_data);
    G1::add(g1[0], tmp, commitment.c);
    G1::mul(tmp, G1_ONE, opening.v.mcl_data);
    G1::sub(g1[0], g1[0], tmp);

    // g1[1]
    G1::neg(g1[1], opening.p);
    
    // g2[0]
    g2[0] = G2_ONE;
    g2[1] = setup.g2_tau;
    
    GT res;
    millerLoopVec(res, g1, g2, 2);
    finalExp(res, res);
    return res == GT_ONE;
}

}


