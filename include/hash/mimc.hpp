#ifndef GKR_MIMC_H
#define GKR_MIMC_H

#include <vector>
#include <string>
#include <cassert>
#include <openssl/evp.h>

#include "../utils/types.hpp"

namespace gkr
{
    namespace mimc
    {

        const std::string SEED = "seed";
        const uint32 N_ROUNDS = 110;
        const struct
        {
            uint32 rate;
            uint32 capacity;
            uint32 output_len;
            uint8 delimited_suffix;
        } KECCAK_PARAMETERS = {1088, 512, 256, 0x01};

        template <typename F>
        std::vector<F> init_mimc_cts()
        {
            auto sha3_hash = [](uint8 *output, const uint8 *input, uint32 bit_length)
            {
                EVP_MD_CTX *mdctx;
                const EVP_MD *md;
                unsigned int md_len;

                md = EVP_sha3_256();
                mdctx = EVP_MD_CTX_new();
                EVP_DigestInit_ex(mdctx, md, NULL);
                EVP_DigestUpdate(mdctx, input, bit_length / 8);
                EVP_DigestFinal_ex(mdctx, output, &md_len);
                EVP_MD_CTX_free(mdctx);
            };

            std::vector<F> cts;

            int output_bytelen = 32;
            uint8 output[output_bytelen];
            sha3_hash(output, reinterpret_cast<const uint8 *>(SEED.c_str()), SEED.length() * 8);

            for (uint32 i = 0; i < N_ROUNDS; ++i)
            {
                sha3_hash(output, output, output_bytelen * 8); 
                // According to the standard, the next step is to intepret the output as big-endian field elements
                // Our inteface assume little-endian, so do a reverse here
                std::vector output_copy(output, output + output_bytelen);
                std::reverse(output_copy.begin(), output_copy.end());
                cts.push_back(F::from_bytes_mod_order(output_copy));
            }

            return cts;
        }

        template <typename F>
        class MIMC
        {
        public:
            const std::vector<F> constants;

            MIMC() : constants(init_mimc_cts<F>()) {}

            F hash(const std::vector<F> &f_vec) const
            {
                F h = F::zero();
                return h.random();
            }

            // h: current state, v: incoming value
            F _mimc5_hash(const F &h, const F &v) const
            {
                F x = v;

                auto pow5 = [](const F &xx)
                {
                    F xx2 = xx * xx;
                    F xx4 = xx2 * xx2;
                    return xx * xx4;
                };

                for (const F &ct : this->constants)
                {
                    x = pow5(x + h + ct);
                }
                return x + h;
            }
        };

    }
}

#endif