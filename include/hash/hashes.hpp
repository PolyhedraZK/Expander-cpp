#pragma once

#include <stdio.h>
#include <stdlib.h> 
// #include <openssl/crypto.h>
// #include <openssl/evp.h>
// #include <openssl/sha.h>

#include "btc_sha256/crypto/sha256.h"
#include "utils/types.hpp"

namespace gkr
{

class Hasher
{
public:
    // bytes to bytes
    void hash(uint8 *output, const uint8 *input, uint32 input_len);
};

class SHA256Hasher: public Hasher
{
public:
    CSHA256 btc_sha256_hasher;

public:
    inline void hash(uint8 *output, const uint8 *input, uint32 input_len)
    {
        btc_sha256_hasher.Reset().Write(input, input_len).Finalize(output);
    }

};

// // Openssl 3.x seems to have some thread lock problems
// class SHA256Hasher: public Hasher
// {
// public:

//     const EVP_MD *md;
//     EVP_MD_CTX *ctx;

//     SHA256Hasher(): md(EVP_sha3_256())
//     {
//         ctx = EVP_MD_CTX_new();
//     }

//     inline void hash(uint8 *output, const uint8 *input, uint32 input_len)
//     {
//         EVP_DigestInit_ex(ctx, md, NULL);
//         EVP_DigestUpdate(ctx, input, input_len);
//         EVP_DigestFinal_ex(ctx, output, NULL);
//         // SHA256(input, input_len, output);
//     }
// };


}