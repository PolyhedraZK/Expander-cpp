/*
 * The MIT License (MIT)
 * Copyright (c) 2018 Armando Faz
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef SHA_NI_SHA_H
#define SHA_NI_SHA_H

#ifdef __cplusplus
extern "C"{
#endif

#ifndef ALIGN_BYTES
#define ALIGN_BYTES 32
#endif
#ifdef __INTEL_COMPILER
#define ALIGN __declspec(align(ALIGN_BYTES))
#else
#define ALIGN __attribute__ ((aligned (ALIGN_BYTES)))
#endif

#include <stdint.h>
#include <immintrin.h>

#define SHA256_HASH_SIZE 32 /*! <- SHA2-256 produces a 32-byte output. */

#define SHA_CORE_DEF(CORE)\
unsigned char * sha256##_##CORE(const unsigned char *message, long unsigned int message_length,unsigned char *digest);

#define SHA_X2_CORE_DEF(CORE_X2)\
void sha256_x2##_##CORE_X2(\
unsigned char *message[2],\
long unsigned int message_length,\
unsigned char *digest[2]);

#define SHA_X4_CORE_DEF(CORE_X4)\
void sha256_x4##_##CORE_X4(\
unsigned char *message[4],\
long unsigned int message_length,\
unsigned char *digest[4]);

#define SHA_X8_CORE_DEF(CORE_X8)\
void sha256_x8##_##CORE_X8(\
unsigned char *message[8],\
long unsigned int message_length,\
unsigned char *digest[8]);

/* Single-message implementation */
SHA_CORE_DEF(update_shani)

/* Pipelined implementations */
SHA_X2_CORE_DEF(update_shani_2x)
SHA_X4_CORE_DEF(update_shani_4x)
SHA_X8_CORE_DEF(update_shani_8x)

extern const ALIGN uint32_t CONST_K[64];

#define dec_sha256_vec_256b(NUM)  \
void sha256_vec_ ## NUM ## 256b ( \
  uint8_t *message[NUM],          \
  uint8_t *digest[NUM])

#define dec_sha256_Nw(NUM)        \
void sha256_ ## NUM ## w(         \
    uint8_t *message[NUM],        \
    unsigned int message_length,  \
    uint8_t *digest[NUM])

/* For arbitrary-large input messages */
dec_sha256_vec_256b(4);
dec_sha256_vec_256b(8);
/* For 256-bit input messages */
dec_sha256_Nw(4);
dec_sha256_Nw(8);
dec_sha256_Nw(16);

#ifdef __cplusplus
}
#endif

#endif //SHA_NI_SHA_H
