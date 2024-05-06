#ifndef __merkle_tree
#define __merkle_tree
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <immintrin.h>
#include <wmmintrin.h>
#include "my_hhash.h"
#include "linear_gkr/prime_field.h"

namespace merkle_tree
{
extern int size_after_padding;

__hhash_digest hash_single_field_element(prime_field::field_element x);

__hhash_digest hash_double_field_element_merkle_damgard(prime_field::field_element x, prime_field::field_element y, __hhash_digest prev_hash);

namespace merkle_tree_prover
{
    //Merkle tree functions used by the prover
    //void create_tree(void* data_source_array, int lengh, void* &target_tree_array, const int single_element_size = 256/8)
    void create_tree(void* data, int ele_num, __hhash_digest* &dst, const int element_size, bool alloc_required);
}

namespace merkle_tree_verifier
{
    //Merkle tree functions used by the verifier
    bool verify_claim(__hhash_digest root_hhash, const __hhash_digest* tree, __hhash_digest hhash_element, int pos_element, int N, bool *visited, long long &proof_size);
}
}

#endif