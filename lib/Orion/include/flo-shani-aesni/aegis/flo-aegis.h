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
#ifndef AEGIS_H
#define AEGIS_H

#ifdef __cplusplus
extern "C"{
#endif

#include <stdint.h>
#include <immintrin.h>

#define ALIGN_BYTES 32
#ifdef __INTEL_COMPILER
#define ALIGN __declspec(align(ALIGN_BYTES))
#else
#define ALIGN __attribute__ ((aligned (ALIGN_BYTES)))
#endif

int crypto_aead_encrypt_opt(
    unsigned char *c, uint64_t *clen,
    unsigned char *m, uint64_t mlen,
    unsigned char *ad, uint64_t adlen,
    unsigned char *npub,
    unsigned char *k);

int crypto_aead_encrypt(
    unsigned char *c, uint64_t *clen,
    const unsigned char *m, uint64_t mlen,
    const unsigned char *ad, uint64_t adlen,
    const unsigned char *npub,
    const unsigned char *k
);

int crypto_aead_decrypt(
    unsigned char *m, uint64_t *mlen,
    unsigned char *nsec,
    const unsigned char *c, uint64_t clen,
    const unsigned char *ad, uint64_t adlen,
    const unsigned char *npub,
    const unsigned char *k
);

#ifdef __cplusplus
}
#endif

#endif /* AEGIS_H */
