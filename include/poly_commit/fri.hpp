#pragma once

#include <cassert>

#include "../utils/myutil.hpp"
#include "../fiat_shamir/transcript.hpp"
#include "merkle.hpp"
#include "poly.hpp"

namespace gkr
{
namespace fri
{

// TODO: 
// 0. Separate Different functions
// 1. Bit reverse
// 2. Mul-Fold: 
//    a. How to determine the security parameters
//    b. Fold multiple times with the same randomness?
// 3. Poly Commit
// 

const uint32 RATE_INV = 4;
const uint32 SECURITY_BITS = 128;

uint32 nb_queries(uint32 rate_inv, uint32 security_bits)
{
    assert(is_pow_2(rate_inv));
    uint32 log_rate_inv = __builtin_ctz(rate_inv);
    return (security_bits + log_rate_inv - 1) / log_rate_inv; // security_bits / log_rate_inv, rounded up
}

const uint32 NB_QUERIES = nb_queries(RATE_INV, SECURITY_BITS);


// Read this for a basic understanding: https://aszepieniec.github.io/stark-anatomy/fri.html
template<typename F>
struct FriLDTProof
{
    std::vector<merkle_tree::Digest> oracle_digests;
    F constant_poly;
    
    // d1: #queries d2: log_degree d3: answer/merkle_proof
    std::vector<std::vector<std::pair<F, F>>> answer_to_queries;
    std::vector<std::vector<std::pair<merkle_tree::Proof, merkle_tree::Proof>>> merkle_proof;
};

using _Tree = merkle_tree::MerkleTree;

template<typename F>
struct Oracles
{
    _Tree merkle_tree;
    std::vector<F> evals;

    std::pair<F, merkle_tree::Proof> query(uint32 idx)
    {
        return {evals[idx], merkle_tree.prove(idx)};
    }
};


// assuming evals are the evaluations of a univariate polynomial on the root_of_unity group
template<typename F>
std::vector<F> fold(const std::vector<F>& evals, const F& alpha, const F& root_of_unity)
{
    uint32 folded_size = evals.size() >> 1; // 2-fold
    std::vector<F> evals_folded;
    evals_folded.reserve(folded_size); 

    F inv_2 = F(2).inv(); // TODO: precompute this in all fields
    F root_of_unity_inverse = root_of_unity.inv();// w^{-1}
    F alpha_w_pow_minus_i = alpha; // alpha * w^{-i}
    for (uint32 i = 0; i < folded_size; i++)
    {
        F v1 = evals[i];
        F v2 = evals[i + folded_size];
        evals_folded.emplace_back(
            inv_2 * ((v1 + v2) + alpha_w_pow_minus_i * (v1 - v2))
        );

        alpha_w_pow_minus_i *= root_of_unity_inverse;
    }

    return evals_folded;
}

template<typename F>
void commit_phase(
    const UniPoly<F>& poly, 
    Transcript<F>& transcript, 
    FriLDTProof<F>& proof,
    std::vector<Oracles<F>>& oracles)
{

    uint32 log_degree = __builtin_ctz(poly.degree());
    uint32 log_inv_rate = __builtin_ctz(RATE_INV);
    uint32 log_evaluation_domain_size = log_degree + log_inv_rate;
    std::vector<F> evals = ntt_expand(poly.coefs, RATE_INV);
    F root = get_primitive_root_of_unity<F>(log_evaluation_domain_size);

    //TODO: try avoiding extensive memory copies
    oracles.reserve(log_degree);
    for (uint32 i = 0; i < log_degree; i++)
    {   
        // prover commit
        _Tree tree = merkle_tree::MerkleTree::build_tree(evals);
        oracles.emplace_back(tree, evals);
        proof.oracle_digests.emplace_back(tree.root());

        // verifier challenge
        transcript.append_f(F::from_bytes(tree.root().as_bytes()));
        F alpha = transcript.challenge_f(); 
        
        // prover folding
        evals = fold(evals, alpha, root);
        root = root * root;
    }

    proof.constant_poly = evals[0];
    transcript.append_f(proof.constant_poly);

#ifndef NDEBUG
    bool is_constant = true;
    for (const F& v: evals)
    {
        is_constant &= evals[0] == v;
    }
    assert(is_constant);
#endif
}



template<typename F>
void query_phase(
    const UniPoly<F>& poly, 
    Transcript<F>& transcript, 
    FriLDTProof<F>& proof, 
    const std::vector<Oracles<F>>& oracles)
{
    uint32 log_degree = __builtin_ctz(poly.degree());
    uint32 log_inv_rate = __builtin_ctz(RATE_INV);
    uint32 log_evaluation_domain_size = log_degree + log_inv_rate;
    uint64 evaluation_domain_size = 1 << log_evaluation_domain_size;
    
    proof.answer_to_queries.resize(NB_QUERIES);
    proof.merkle_proof.resize(NB_QUERIES);
    for (uint32 i = 0; i < NB_QUERIES; i++)
    {
        std::vector<uint8> bytes = transcript.challenge_f().to_bytes();
        uint64 query_idx = transcript.challenge_uint64();
        
        auto& answers_i = proof.answer_to_queries[i];
        auto& merkle_proofs_i = proof.merkle_proof[i];
        for (uint32 j = 0; j < log_degree; j++)
        {
            // query range is half of current length (the length of the next codeword).
            uint64 query_range = evaluation_domain_size >> (j + 1);
            uint64 actual_idx = query_idx & (query_range - 1);
            
            const F& ans_1 = oracles[j].evals[actual_idx];
            const F& ans_2 = oracles[j].evals[actual_idx + query_range];

            answers_i.emplace_back(ans_1, ans_2);
            //TODO: consider batching the proof from multiple queries
            merkle_proofs_i.emplace_back(
                oracles[j].merkle_tree.prove(actual_idx), 
                oracles[j].merkle_tree.prove(actual_idx + query_range)
            );

            transcript.append_f(ans_1);
            transcript.append_f(ans_2);
        }
    }
    
}

template<typename F>
FriLDTProof<F> fri_ldt_prove(const UniPoly<F>& poly, Transcript<F>& transcript)
{
    assert(is_pow_2(poly.degree()));
    FriLDTProof<F> proof;
    std::vector<Oracles<F>> oracles;
    commit_phase(poly, transcript, proof, oracles);
    query_phase(poly, transcript, proof, oracles);
    return proof;
}

// separate the verification into 'reproduce randomness' 'verify merkle' 'verify combine'
template<typename F>
bool fri_ldt_verify(const FriLDTProof<F>& proof, Transcript<F>& transcript, uint32 degree)
{
    assert(is_pow_2(degree));
    uint32 log_degree = __builtin_ctz(degree);
    uint32 log_inv_rate = __builtin_ctz(RATE_INV);
    uint32 log_evaluation_domain_size = log_degree + log_inv_rate;
    uint64 evaluation_domain_size = 1 << log_evaluation_domain_size;

    // verify commit phase: reproduce the randomness from the transcript
    std::vector<F> alphas;
    for (uint32 i = 0; i < log_degree; i++)
    {
        transcript.append_f(F::from_bytes(proof.oracle_digests[i].as_bytes()));
        alphas.emplace_back(transcript.challenge_f());
    }
    transcript.append_f(proof.constant_poly);

    // verify query phase
    bool verified = true;
    const F root = get_primitive_root_of_unity<F>(log_evaluation_domain_size);
    for (uint32 i = 0; i < NB_QUERIES; i++)
    {
        std::vector<uint8> bytes = transcript.challenge_f().to_bytes();
        uint64 query_idx = transcript.challenge_uint64();
        
        F w = root;
        auto& answers_i = proof.answer_to_queries[i];
        auto& merkle_proofs_i = proof.merkle_proof[i];
        for (uint32 j = 0; j < log_degree; j++)
        {
            // query range is half of current length (the length of the next codeword).
            uint64 query_range = evaluation_domain_size >> (j + 1);
            uint64 actual_idx = query_idx & (query_range - 1);
            
            const F& ans_1 = answers_i[j].first;
            const F& ans_2 = answers_i[j].second;

            verified &= merkle_tree::MerkleTree::verify(proof.oracle_digests[j], 
                actual_idx, ans_1, merkle_proofs_i[j].first);
            verified &= merkle_tree::MerkleTree::verify(proof.oracle_digests[j], 
                actual_idx + query_range, ans_2, merkle_proofs_i[j].second);

            transcript.append_f(ans_1);
            transcript.append_f(ans_2);

            // verify the random linear combination
            F w_pow_idx = w.exp(actual_idx);
            F minus_w_pow_idx = -w_pow_idx;
            F alpha = alphas[j];
            F ans_next;
            if (j == log_degree - 1)
            {
                ans_next = proof.constant_poly;
            }
            else
            {
                if (actual_idx < (query_range >> 1))
                {
                    ans_next = answers_i[j + 1].first;
                }
                else
                {
                    ans_next = answers_i[j + 1].second;
                }
            }

            // same line
            verified &= 
                ((ans_2 - ans_1) * (alpha - minus_w_pow_idx)) == 
                    ((ans_next - ans_2) * (minus_w_pow_idx - w_pow_idx));

            w = w * w;
        }
    }

    return verified;
}

template<typename F>
merkle_tree::Digest commit(const UniPoly<F>& poly)
{

}

template<typename F>
FriLDTProof<F> open(const UniPoly<F>& poly, const F& x)
{

}


} // namespace fri
} // namespace LinearGKR
