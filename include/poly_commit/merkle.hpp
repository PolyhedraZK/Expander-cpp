#pragma once

#include <cassert>
#include <vector>
#include <openssl/sha.h>

#include "../utils/types.hpp"

namespace gkr
{
namespace merkle_tree
{

static const uint32 DIGEST_BYTE_LENGTH = 32;

struct Digest
{
    uint8 data[DIGEST_BYTE_LENGTH];

    std::vector<uint8> as_bytes() const
    {
        return {data, data + DIGEST_BYTE_LENGTH};
    }
};

typedef std::vector<Digest> Proof;

class MerkleTree
{
public:
    std::vector<std::vector<Digest>> tree_digests;

public:

    static Digest two_to_one_hash(const Digest& d1, const Digest& d2)
    {
        uint8 input[DIGEST_BYTE_LENGTH * 2];
        memcpy(input, d1.data, DIGEST_BYTE_LENGTH);
        memcpy(input + DIGEST_BYTE_LENGTH, d2.data, DIGEST_BYTE_LENGTH);
        
        Digest output;
        SHA256(input, DIGEST_BYTE_LENGTH * 2, output.data);
        
        return output;
    }

    // F needs to implement to_bytes method
    template<typename F>
    static MerkleTree build_tree(const std::vector<F>& input)
    {
        MerkleTree tree;
        std::vector<std::vector<Digest>>& tree_digests = tree.tree_digests;

        uint32 size = input.size();
        assert((size > 0) && ((size & (size - 1)) == 0)); // power of 2
        uint32 log_size = __builtin_ctz(size);

        tree_digests.resize(log_size + 1);
        tree_digests[0].reserve(size);
        uint8 bytes[F::byte_length()];
        for (const F& f: input)
        {
            f.to_bytes(bytes);
            
            for(uint32 i = 0; i < F::byte_length(); i++)
            {
                printf("%02x", bytes[i]);
            }
            printf("\n");

            Digest digest;
            SHA256(bytes, F::byte_length(), digest.data);
            tree_digests[0].emplace_back(digest);
            for(uint32 i = 0; i < DIGEST_BYTE_LENGTH; i++)
            {
                printf("%02x", digest.data[i]);
            }
            printf("\n");
        }
        printf("log_size: %d\n", log_size);
        for (uint32 i = 1; i <= log_size; i++)
        {
            uint32 layer_size = size >> i;
            tree_digests[i].reserve(layer_size);
            for (uint32 j = 0; j < layer_size; j++)
            {
                Digest digest = two_to_one_hash(tree_digests[i - 1][j * 2], tree_digests[i - 1][j * 2 + 1]);
                tree_digests[i].emplace_back(digest);
            }
            
        }
        for(uint32 i = 0; i < tree_digests.size(); i++)
        {
            printf("layer %d\n", i);
            for(uint32 j = 0; j < tree_digests[i].size(); j++)
            {
                for(uint32 k = 0; k < DIGEST_BYTE_LENGTH; k++)
                {
                    printf("%02x", tree_digests[i][j].data[k]);
                }
                printf("\n");
            }
        }
        
        return tree;
    }

    Digest root() const
    {
        return tree_digests.back()[0];
    }

    uint32 depth() const
    {
        return tree_digests.size();
    }

    Proof prove(uint32 idx) const
    {
        assert(idx < tree_digests[0].size());
        Proof proof;
        for (uint32 i = 0; i < depth() - 1; i++)
        {
            proof.emplace_back(tree_digests[i][idx ^ 1]);
            idx >>= 1;
        }
        return proof;
    }

    template<typename F>
    static bool verify(const Digest& root, uint32 idx, const F& leaf, const Proof& proof) 
    {
        uint8 leaf_bytes[F::byte_length()];
        leaf.to_bytes(leaf_bytes);
        for (uint32 i = 0; i < F::byte_length(); i++)
        {
            printf("%02x", leaf_bytes[i]);
        }
        printf("\n");
        Digest hash;
        SHA256(leaf_bytes, F::byte_length(), hash.data);
        printf("leaf hash\n");
        for(uint32 i = 0; i < DIGEST_BYTE_LENGTH; i++)
        {
            printf("%02x", hash.data[i]);
        }
        printf("\n");

        for (uint32 i = 0; i < proof.size(); i++)
        {
            if ((idx & 1) == 0)
            {
                hash = two_to_one_hash(hash, proof[i]);
            }
            else 
            {
                hash = two_to_one_hash(proof[i], hash);
            }
            idx >>= 1;
            printf("hash %d\n", i);
            for(uint32 j = 0; j < DIGEST_BYTE_LENGTH; j++)
            {
                printf("%02x", hash.data[j]);
            }
            printf("\n");
        }
        
        return memcmp(hash.data, root.data, DIGEST_BYTE_LENGTH) == 0;
    }
};

} // namespace merkle_tree
} // namespace LinearGKR
