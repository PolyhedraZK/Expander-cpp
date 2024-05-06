#include <gtest/gtest.h>
#include <fstream>
#include <iostream>

#include "field/M31.hpp"
#include "fiat_shamir/transcript.hpp"
#include "hash/hashes.hpp"

TEST(HASH_TEST, SHA256_REPEAT)
{
    using namespace gkr;
    SHA256Hasher hasher;

    uint32 input_size = 32, output_size = 32;
    const uint8 *input = new uint8[input_size];
    uint8 *output_1 = new uint8[output_size];
    uint8 *output_2 = new uint8[output_size];
    hasher.hash(output_1, input, input_size);
    hasher.hash(output_2, input, input_size);
    for(uint32 i = 0; i < output_size; i++)
    {
        EXPECT_EQ(output_1[i], output_2[i]);
    }
}

TEST(TRANSCRIPT_TEST, TRANSCRIPT_CROSS_PLATFORM_SYNC)
{
    using namespace gkr;
    using F = M31_field::VectorizedM31;
    using F_primitive = M31_field::M31;

#ifdef __ARM_NEON
    const char* output_file_name = "../build/output_arm.txt";
#else
    const char* output_file_name = "../build/output_x86.txt";
#endif

    uint32 test_size = 32;
    std::vector<F_primitive> fs;
    for (uint32 i = 0; i < test_size; i++)
    {
        fs.emplace_back(F_primitive(i));
    }

    std::vector<F> vectorized_F = F::pack_field_elements(fs);
    Transcript<F, F_primitive> transcript;

    transcript.append_f(vectorized_F[0]);
    F_primitive alpha = transcript.challenge_f();
    transcript.append_f(vectorized_F[1]);
    F_primitive beta = transcript.challenge_f();
    F_primitive gamma = transcript.challenge_f();

    std::ofstream output;
    output.open(output_file_name);
    uint8* alpha_bytes = new uint8[4];
    uint8* beta_bytes = new uint8[4];
    uint8* gamma_bytes = new uint8[4];
    
    alpha.to_bytes(alpha_bytes);
    beta.to_bytes(beta_bytes);
    gamma.to_bytes(gamma_bytes);

    for (size_t i = 0; i < 4; i++)
    {
        output << (uint32) alpha_bytes[i] << " " << (uint32) beta_bytes[i] << " " << (uint32) gamma_bytes[i] << "\n";
    }
    output << std::endl;
    output.close();
}