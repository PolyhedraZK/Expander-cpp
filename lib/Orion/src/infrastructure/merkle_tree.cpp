#include "linear_gkr/prime_field.h"
#include "infrastructure/merkle_tree.h"
int merkle_tree::size_after_padding;

__hhash_digest merkle_tree::hash_single_field_element(prime_field::field_element x)
{
    __hhash_digest data[2], ret;
    memset(data, 0, sizeof(data));
    memcpy(&data[0].h0, &x, sizeof(data[0].h0));
    assert(sizeof(data[0].h0) == sizeof(x));
    my_hhash(data, &ret);
    return ret;
}

__hhash_digest merkle_tree::hash_double_field_element_merkle_damgard(prime_field::field_element x, prime_field::field_element y, __hhash_digest prev_hash)
{
   __hhash_digest data[2], ret;
    data[0] = prev_hash;
    prime_field::field_element element[2];
    element[0] = x;
    element[1] = y;
    memcpy(&data[1], element, 2 * sizeof(prime_field::field_element));
    assert(2 * sizeof(prime_field::field_element) == sizeof(__hhash_digest));
    my_hhash(data, &ret);
    return ret;
}

void merkle_tree::merkle_tree_prover::create_tree(void* src_data, int ele_num, __hhash_digest* &dst, const int element_size = 256 / 8, bool alloc_required = false)
{
    //assert(element_size == sizeof(prime_field::field_element) * 2);
    size_after_padding = 1;
    while(size_after_padding < ele_num)
        size_after_padding *= 2;

    __hhash_digest *dst_ptr;
    if(alloc_required)
    {
        dst_ptr = (__hhash_digest*)malloc(size_after_padding * 2 * element_size);
        if(dst_ptr == NULL)
        {
            printf("Merkle Tree Bad Alloc\n");
            abort();
        }
    }
    else
        dst_ptr = dst;
    //线段树要两倍大小
    dst = dst_ptr;
    memset(dst_ptr, 0, size_after_padding * 2 * element_size);

    int start_ptr = size_after_padding;
    int current_lvl_size = size_after_padding;
    #pragma omp parallel for
    for(int i = current_lvl_size - 1; i >= 0; --i)
    {
        __hhash_digest data[2];
        memset(data, 0, sizeof data);
        if(i < ele_num)
        {
            dst_ptr[i + start_ptr] = ((__hhash_digest*)src_data)[i];
        }
        else
        {
            memset(data, 0, sizeof data);
            my_hhash(data, &dst_ptr[i + start_ptr]);
        }
    }
    current_lvl_size /= 2;
    start_ptr -= current_lvl_size;
    while(current_lvl_size >= 1)
    {
        #pragma omp parallel for
        for(int i = 0; i < current_lvl_size; ++i)
        {
            __hhash_digest data[2];
            data[0] = dst_ptr[start_ptr + current_lvl_size + i * 2];
            data[1] = dst_ptr[start_ptr + current_lvl_size + i * 2 + 1];
            my_hhash(data, &dst_ptr[start_ptr + i]);
        }
        current_lvl_size /= 2;
        start_ptr -= current_lvl_size;
    }
}

bool merkle_tree::merkle_tree_verifier::verify_claim(__hhash_digest root_hhash, const __hhash_digest* tree, __hhash_digest leaf_hash, int pos_element_arr, int N, bool *visited, long long &proof_size)
{
    //check N is power of 2
    assert((N & (-N)) == N);

    int pos_element = pos_element_arr + N;
    __hhash_digest data[2];
    while(pos_element != 1)
    {
        data[pos_element & 1] = leaf_hash;
        data[(pos_element & 1) ^ 1] = tree[pos_element ^ 1];
        if(!visited[pos_element ^ 1])
        {
            visited[pos_element ^ 1] = true;
            proof_size += sizeof(__hhash_digest);
        }
        my_hhash(data, &leaf_hash);
        pos_element /= 2;
    }
    return equals(root_hhash, leaf_hash);
}