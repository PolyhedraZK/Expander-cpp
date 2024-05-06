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
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "flo-aesni.h"

enum aes_numRounds {
  AES_128_numRounds = 10,
  AES_192_numRounds = 12,
  AES_256_numRounds = 14
};

#define ADD _mm_add_epi32
#define XOR _mm_xor_si128
#define AESENC _mm_aesenc_si128
#define AESENCLAST _mm_aesenclast_si128
#define AESDEC _mm_aesdec_si128
#define AESDECLAST _mm_aesdeclast_si128

static inline __m128i AES_128_ASSIST(__m128i temp1, __m128i temp2) {
  __m128i temp3;
  temp2 = _mm_shuffle_epi32(temp2, 0xff);
  temp3 = _mm_slli_si128(temp1, 0x4);
  temp1 = XOR(temp1, temp3);
  temp3 = _mm_slli_si128(temp3, 0x4);
  temp1 = XOR(temp1, temp3);
  temp3 = _mm_slli_si128(temp3, 0x4);
  temp1 = XOR(temp1, temp3);
  temp1 = XOR(temp1, temp2);
  return temp1;
}

static void AES_128_Key_Expansion(const unsigned char* userkey,
                                  unsigned char* key) {
  __m128i temp1, temp2;
  __m128i* Key_Schedule = (__m128i*)key;
  temp1 = _mm_loadu_si128((__m128i*)userkey);
  Key_Schedule[0] = temp1;
  temp2 = _mm_aeskeygenassist_si128(temp1, 0x1);
  temp1 = AES_128_ASSIST(temp1, temp2);
  Key_Schedule[1] = temp1;
  temp2 = _mm_aeskeygenassist_si128(temp1, 0x2);
  temp1 = AES_128_ASSIST(temp1, temp2);
  Key_Schedule[2] = temp1;
  temp2 = _mm_aeskeygenassist_si128(temp1, 0x4);
  temp1 = AES_128_ASSIST(temp1, temp2);
  Key_Schedule[3] = temp1;
  temp2 = _mm_aeskeygenassist_si128(temp1, 0x8);
  temp1 = AES_128_ASSIST(temp1, temp2);
  Key_Schedule[4] = temp1;
  temp2 = _mm_aeskeygenassist_si128(temp1, 0x10);
  temp1 = AES_128_ASSIST(temp1, temp2);
  Key_Schedule[5] = temp1;
  temp2 = _mm_aeskeygenassist_si128(temp1, 0x20);
  temp1 = AES_128_ASSIST(temp1, temp2);
  Key_Schedule[6] = temp1;
  temp2 = _mm_aeskeygenassist_si128(temp1, 0x40);
  temp1 = AES_128_ASSIST(temp1, temp2);
  Key_Schedule[7] = temp1;
  temp2 = _mm_aeskeygenassist_si128(temp1, 0x80);
  temp1 = AES_128_ASSIST(temp1, temp2);
  Key_Schedule[8] = temp1;
  temp2 = _mm_aeskeygenassist_si128(temp1, 0x1b);
  temp1 = AES_128_ASSIST(temp1, temp2);
  Key_Schedule[9] = temp1;
  temp2 = _mm_aeskeygenassist_si128(temp1, 0x36);
  temp1 = AES_128_ASSIST(temp1, temp2);
  Key_Schedule[10] = temp1;
}

static inline void KEY_192_ASSIST(__m128i* temp1, __m128i* temp2,
                                  __m128i* temp3) {
  __m128i temp4;
  *temp2 = _mm_shuffle_epi32(*temp2, 0x55);
  temp4 = _mm_slli_si128(*temp1, 0x4);
  *temp1 = _mm_xor_si128(*temp1, temp4);
  temp4 = _mm_slli_si128(temp4, 0x4);
  *temp1 = _mm_xor_si128(*temp1, temp4);
  temp4 = _mm_slli_si128(temp4, 0x4);
  *temp1 = _mm_xor_si128(*temp1, temp4);
  *temp1 = _mm_xor_si128(*temp1, *temp2);
  *temp2 = _mm_shuffle_epi32(*temp1, 0xff);
  temp4 = _mm_slli_si128(*temp3, 0x4);
  *temp3 = _mm_xor_si128(*temp3, temp4);
  *temp3 = _mm_xor_si128(*temp3, *temp2);
}

static void AES_192_Key_Expansion(const unsigned char* userkey,
                                  unsigned char* key) {
  __m128i temp1, temp2, temp3;
  __m128i* Key_Schedule = (__m128i*)key;
  temp1 = _mm_loadu_si128((__m128i*)userkey);
  temp3 = _mm_loadu_si128((__m128i*)(userkey + 16));
  Key_Schedule[0] = temp1;
  Key_Schedule[1] = temp3;
  temp2 = _mm_aeskeygenassist_si128(temp3, 0x1);
  KEY_192_ASSIST(&temp1, &temp2, &temp3);
  Key_Schedule[1] =
      (__m128i)_mm_shuffle_pd((__m128d)Key_Schedule[1], (__m128d)temp1, 0);
  Key_Schedule[2] = (__m128i)_mm_shuffle_pd((__m128d)temp1, (__m128d)temp3, 1);
  temp2 = _mm_aeskeygenassist_si128(temp3, 0x2);
  KEY_192_ASSIST(&temp1, &temp2, &temp3);
  Key_Schedule[3] = temp1;
  Key_Schedule[4] = temp3;
  temp2 = _mm_aeskeygenassist_si128(temp3, 0x4);
  KEY_192_ASSIST(&temp1, &temp2, &temp3);
  Key_Schedule[4] =
      (__m128i)_mm_shuffle_pd((__m128d)Key_Schedule[4], (__m128d)temp1, 0);
  Key_Schedule[5] = (__m128i)_mm_shuffle_pd((__m128d)temp1, (__m128d)temp3, 1);
  temp2 = _mm_aeskeygenassist_si128(temp3, 0x8);
  KEY_192_ASSIST(&temp1, &temp2, &temp3);
  Key_Schedule[6] = temp1;
  Key_Schedule[7] = temp3;
  temp2 = _mm_aeskeygenassist_si128(temp3, 0x10);
  KEY_192_ASSIST(&temp1, &temp2, &temp3);
  Key_Schedule[7] =
      (__m128i)_mm_shuffle_pd((__m128d)Key_Schedule[7], (__m128d)temp1, 0);
  Key_Schedule[8] = (__m128i)_mm_shuffle_pd((__m128d)temp1, (__m128d)temp3, 1);
  temp2 = _mm_aeskeygenassist_si128(temp3, 0x20);
  KEY_192_ASSIST(&temp1, &temp2, &temp3);
  Key_Schedule[9] = temp1;
  Key_Schedule[10] = temp3;
  temp2 = _mm_aeskeygenassist_si128(temp3, 0x40);
  KEY_192_ASSIST(&temp1, &temp2, &temp3);
  Key_Schedule[10] =
      (__m128i)_mm_shuffle_pd((__m128d)Key_Schedule[10], (__m128d)temp1, 0);
  Key_Schedule[11] = (__m128i)_mm_shuffle_pd((__m128d)temp1, (__m128d)temp3, 1);
  temp2 = _mm_aeskeygenassist_si128(temp3, 0x80);
  KEY_192_ASSIST(&temp1, &temp2, &temp3);
  Key_Schedule[12] = temp1;
}

static inline void KEY_256_ASSIST_1(__m128i* temp1, __m128i* temp2) {
  __m128i temp4;
  *temp2 = _mm_shuffle_epi32(*temp2, 0xff);
  temp4 = _mm_slli_si128(*temp1, 0x4);
  *temp1 = _mm_xor_si128(*temp1, temp4);
  temp4 = _mm_slli_si128(temp4, 0x4);
  *temp1 = _mm_xor_si128(*temp1, temp4);
  temp4 = _mm_slli_si128(temp4, 0x4);
  *temp1 = _mm_xor_si128(*temp1, temp4);
  *temp1 = _mm_xor_si128(*temp1, *temp2);
}

static inline void KEY_256_ASSIST_2(__m128i* temp1, __m128i* temp3) {
  __m128i temp2, temp4;
  temp4 = _mm_aeskeygenassist_si128(*temp1, 0x0);
  temp2 = _mm_shuffle_epi32(temp4, 0xaa);
  temp4 = _mm_slli_si128(*temp3, 0x4);
  *temp3 = _mm_xor_si128(*temp3, temp4);
  temp4 = _mm_slli_si128(temp4, 0x4);
  *temp3 = _mm_xor_si128(*temp3, temp4);
  temp4 = _mm_slli_si128(temp4, 0x4);
  *temp3 = _mm_xor_si128(*temp3, temp4);
  *temp3 = _mm_xor_si128(*temp3, temp2);
}

static void AES_256_Key_Expansion(const unsigned char* userkey,
                                  unsigned char* key) {
  __m128i temp1, temp2, temp3;
  __m128i* Key_Schedule = (__m128i*)key;
  temp1 = _mm_loadu_si128((__m128i*)userkey);
  temp3 = _mm_loadu_si128((__m128i*)(userkey + 16));
  Key_Schedule[0] = temp1;
  Key_Schedule[1] = temp3;
  temp2 = _mm_aeskeygenassist_si128(temp3, 0x01);
  KEY_256_ASSIST_1(&temp1, &temp2);
  Key_Schedule[2] = temp1;
  KEY_256_ASSIST_2(&temp1, &temp3);
  Key_Schedule[3] = temp3;
  temp2 = _mm_aeskeygenassist_si128(temp3, 0x02);
  KEY_256_ASSIST_1(&temp1, &temp2);
  Key_Schedule[4] = temp1;
  KEY_256_ASSIST_2(&temp1, &temp3);
  Key_Schedule[5] = temp3;
  temp2 = _mm_aeskeygenassist_si128(temp3, 0x04);
  KEY_256_ASSIST_1(&temp1, &temp2);
  Key_Schedule[6] = temp1;
  KEY_256_ASSIST_2(&temp1, &temp3);
  Key_Schedule[7] = temp3;
  temp2 = _mm_aeskeygenassist_si128(temp3, 0x08);
  KEY_256_ASSIST_1(&temp1, &temp2);
  Key_Schedule[8] = temp1;
  KEY_256_ASSIST_2(&temp1, &temp3);
  Key_Schedule[9] = temp3;
  temp2 = _mm_aeskeygenassist_si128(temp3, 0x10);
  KEY_256_ASSIST_1(&temp1, &temp2);
  Key_Schedule[10] = temp1;
  KEY_256_ASSIST_2(&temp1, &temp3);
  Key_Schedule[11] = temp3;
  temp2 = _mm_aeskeygenassist_si128(temp3, 0x20);
  KEY_256_ASSIST_1(&temp1, &temp2);
  Key_Schedule[12] = temp1;
  KEY_256_ASSIST_2(&temp1, &temp3);
  Key_Schedule[13] = temp3;
  temp2 = _mm_aeskeygenassist_si128(temp3, 0x40);
  KEY_256_ASSIST_1(&temp1, &temp2);
  Key_Schedule[14] = temp1;
}

/** AES-128-CTR Pipelined Implementations **/
static ALIGN const uint32_t ONE[4] = {1, 0, 0, 0};
static ALIGN const uint32_t TWO[4] = {2, 0, 0, 0};
static ALIGN const uint32_t FOUR[4] = {4, 0, 0, 0};
static ALIGN const uint32_t EIGHT[4] = {8, 0, 0, 0};
static ALIGN const uint64_t BSWAP64[2] = {0x08090a0b0c0d0e0f,
                                          0x0001020304050607};

#define AES1_CIPHER(BLOCK, KEY)                      \
  (BLOCK) = XOR((BLOCK), ((__m128i*)(KEY))[0]);      \
  for (j = 1; j < number_of_rounds; j++) {           \
    (BLOCK) = AESENC((BLOCK), ((__m128i*)(KEY))[j]); \
  };                                                 \
  (BLOCK) = AESENCLAST((BLOCK), ((__m128i*)(KEY))[j]);

#define AES2_CIPHER(BLOCK0, BLOCK1, KEY)                 \
  (BLOCK0) = XOR((BLOCK0), ((__m128i*)(KEY))[0]);        \
  (BLOCK1) = XOR((BLOCK1), ((__m128i*)(KEY))[0]);        \
  for (j = 1; j < number_of_rounds; j++) {               \
    (BLOCK0) = AESENC((BLOCK0), ((__m128i*)(KEY))[j]);   \
    (BLOCK1) = AESENC((BLOCK1), ((__m128i*)(KEY))[j]);   \
  };                                                     \
  (BLOCK0) = AESENCLAST((BLOCK0), ((__m128i*)(KEY))[j]); \
  (BLOCK1) = AESENCLAST((BLOCK1), ((__m128i*)(KEY))[j]);

#define AES4_CIPHER(BLOCK0, BLOCK1, BLOCK2, BLOCK3, KEY) \
  (BLOCK0) = XOR((BLOCK0), ((__m128i*)(KEY))[0]);        \
  (BLOCK1) = XOR((BLOCK1), ((__m128i*)(KEY))[0]);        \
  (BLOCK2) = XOR((BLOCK2), ((__m128i*)(KEY))[0]);        \
  (BLOCK3) = XOR((BLOCK3), ((__m128i*)(KEY))[0]);        \
  for (j = 1; j < number_of_rounds; j++) {               \
    (BLOCK0) = AESENC((BLOCK0), ((__m128i*)(KEY))[j]);   \
    (BLOCK1) = AESENC((BLOCK1), ((__m128i*)(KEY))[j]);   \
    (BLOCK2) = AESENC((BLOCK2), ((__m128i*)(KEY))[j]);   \
    (BLOCK3) = AESENC((BLOCK3), ((__m128i*)(KEY))[j]);   \
  };                                                     \
  (BLOCK0) = AESENCLAST((BLOCK0), ((__m128i*)(KEY))[j]); \
  (BLOCK1) = AESENCLAST((BLOCK1), ((__m128i*)(KEY))[j]); \
  (BLOCK2) = AESENCLAST((BLOCK2), ((__m128i*)(KEY))[j]); \
  (BLOCK3) = AESENCLAST((BLOCK3), ((__m128i*)(KEY))[j]);

#define AES8_CIPHER(BLOCK0, BLOCK1, BLOCK2, BLOCK3, BLOCK4, BLOCK5, BLOCK6, \
                    BLOCK7, KEY)                                            \
  (BLOCK0) = XOR((BLOCK0), ((__m128i*)(KEY))[0]);                           \
  (BLOCK1) = XOR((BLOCK1), ((__m128i*)(KEY))[0]);                           \
  (BLOCK2) = XOR((BLOCK2), ((__m128i*)(KEY))[0]);                           \
  (BLOCK3) = XOR((BLOCK3), ((__m128i*)(KEY))[0]);                           \
  (BLOCK4) = XOR((BLOCK4), ((__m128i*)(KEY))[0]);                           \
  (BLOCK5) = XOR((BLOCK5), ((__m128i*)(KEY))[0]);                           \
  (BLOCK6) = XOR((BLOCK6), ((__m128i*)(KEY))[0]);                           \
  (BLOCK7) = XOR((BLOCK7), ((__m128i*)(KEY))[0]);                           \
  for (j = 1; j < number_of_rounds; j++) {                                  \
    (BLOCK0) = AESENC((BLOCK0), ((__m128i*)(KEY))[j]);                      \
    (BLOCK1) = AESENC((BLOCK1), ((__m128i*)(KEY))[j]);                      \
    (BLOCK2) = AESENC((BLOCK2), ((__m128i*)(KEY))[j]);                      \
    (BLOCK3) = AESENC((BLOCK3), ((__m128i*)(KEY))[j]);                      \
    (BLOCK4) = AESENC((BLOCK4), ((__m128i*)(KEY))[j]);                      \
    (BLOCK5) = AESENC((BLOCK5), ((__m128i*)(KEY))[j]);                      \
    (BLOCK6) = AESENC((BLOCK6), ((__m128i*)(KEY))[j]);                      \
    (BLOCK7) = AESENC((BLOCK7), ((__m128i*)(KEY))[j]);                      \
  };                                                                        \
  (BLOCK0) = AESENCLAST((BLOCK0), ((__m128i*)(KEY))[j]);                    \
  (BLOCK1) = AESENCLAST((BLOCK1), ((__m128i*)(KEY))[j]);                    \
  (BLOCK2) = AESENCLAST((BLOCK2), ((__m128i*)(KEY))[j]);                    \
  (BLOCK3) = AESENCLAST((BLOCK3), ((__m128i*)(KEY))[j]);                    \
  (BLOCK4) = AESENCLAST((BLOCK4), ((__m128i*)(KEY))[j]);                    \
  (BLOCK5) = AESENCLAST((BLOCK5), ((__m128i*)(KEY))[j]);                    \
  (BLOCK6) = AESENCLAST((BLOCK6), ((__m128i*)(KEY))[j]);                    \
  (BLOCK7) = AESENCLAST((BLOCK7), ((__m128i*)(KEY))[j]);

static void AES_CTR_encrypt_all(const unsigned char* in, unsigned char* out,
                                const unsigned char* ivec, unsigned long length,
                                const unsigned char* key,
                                const unsigned int number_of_rounds) {
  const __m128i one = _mm_load_si128((__m128i*)ONE);
  const __m128i bswap = _mm_load_si128((__m128i*)BSWAP64);
  __m128i counter_be, counter_le;
  unsigned long i = 0, j = 0;
  unsigned long remainder_bytes = length % 16;
  unsigned long num_blocks = length / 16;

  counter_be = _mm_loadu_si128((__m128i*)ivec);
  counter_le = _mm_shuffle_epi8(counter_be, bswap);

  for (i = 0; i < num_blocks; i++) {
    counter_be = _mm_shuffle_epi8(counter_le, bswap);
    counter_le = ADD(counter_le, one);
    AES1_CIPHER(counter_be, key);
    counter_be = XOR(counter_be, _mm_loadu_si128((__m128i*)in + i));
    _mm_storeu_si128((__m128i*)out + i, counter_be);
  }
  if (remainder_bytes > 0) {
    counter_be = _mm_shuffle_epi8(counter_le, bswap);
    AES1_CIPHER(counter_be, key);
    for (j = 0; j < remainder_bytes; j++) {
      out[16 * i + j] = ((uint8_t*)&counter_be)[j] ^ in[16 * i + j];
    }
  }
}

static void AES_CTR_encrypt_pipe2(const unsigned char* in, unsigned char* out,
                                  const unsigned char* ivec,
                                  unsigned long length,
                                  const unsigned char* key,
                                  const unsigned int number_of_rounds) {
  const __m128i one = _mm_load_si128((__m128i*)ONE);
  const __m128i two = _mm_load_si128((__m128i*)TWO);
  const __m128i bswap = _mm_load_si128((__m128i*)BSWAP64);
  __m128i counter_be0, counter_le0;
  __m128i counter_be1, counter_le1;
  unsigned long i = 0, j = 0;
  unsigned long remainder_bytes = length % 16;
  unsigned long num_blocks = length / 16;

  counter_be0 = _mm_loadu_si128((__m128i*)ivec);
  counter_le0 = _mm_shuffle_epi8(counter_be0, bswap);
  counter_le1 = ADD(counter_le0, one);

  for (i = 0; i < num_blocks / 2; i++) {
    counter_be0 = _mm_shuffle_epi8(counter_le0, bswap);
    counter_be1 = _mm_shuffle_epi8(counter_le1, bswap);
    counter_le0 = ADD(counter_le0, two);
    counter_le1 = ADD(counter_le1, two);
    AES2_CIPHER(counter_be0, counter_be1, key);
    counter_be0 = XOR(counter_be0, _mm_loadu_si128((__m128i*)in + 2 * i + 0));
    counter_be1 = XOR(counter_be1, _mm_loadu_si128((__m128i*)in + 2 * i + 1));
    _mm_storeu_si128((__m128i*)out + 2 * i + 0, counter_be0);
    _mm_storeu_si128((__m128i*)out + 2 * i + 1, counter_be1);
  }
  for (i = 2 * i; i < num_blocks; i++) {
    counter_be0 = _mm_shuffle_epi8(counter_le0, bswap);
    counter_le0 = ADD(counter_le0, one);
    AES1_CIPHER(counter_be0, key);
    counter_be0 = XOR(counter_be0, _mm_loadu_si128((__m128i*)in + i));
    _mm_storeu_si128((__m128i*)out + i, counter_be0);
  }
  if (remainder_bytes > 0) {
    counter_be0 = _mm_shuffle_epi8(counter_le0, bswap);
    AES1_CIPHER(counter_be0, key);
    for (j = 0; j < remainder_bytes; j++) {
      out[16 * i + j] = ((uint8_t*)&counter_be0)[j] ^ in[16 * i + j];
    }
  }
}

static void AES_CTR_encrypt_pipe4(const unsigned char* in, unsigned char* out,
                                  const unsigned char* ivec,
                                  unsigned long length,
                                  const unsigned char* key,
                                  const unsigned int number_of_rounds) {
  const __m128i one = _mm_load_si128((__m128i*)ONE);
  const __m128i four = _mm_load_si128((__m128i*)FOUR);
  const __m128i bswap = _mm_load_si128((__m128i*)BSWAP64);
  __m128i counter_be0, counter_le0;
  __m128i counter_be1, counter_le1;
  __m128i counter_be2, counter_le2;
  __m128i counter_be3, counter_le3;
  unsigned long i = 0, j = 0;
  unsigned long remainder_bytes = length % 16;
  unsigned long num_blocks = length / 16;

  counter_be0 = _mm_loadu_si128((__m128i*)ivec);
  counter_le0 = _mm_shuffle_epi8(counter_be0, bswap);
  counter_le1 = ADD(counter_le0, one);
  counter_le2 = ADD(counter_le1, one);
  counter_le3 = ADD(counter_le2, one);

  for (i = 0; i < num_blocks / 4; i++) {
    counter_be0 = _mm_shuffle_epi8(counter_le0, bswap);
    counter_be1 = _mm_shuffle_epi8(counter_le1, bswap);
    counter_be2 = _mm_shuffle_epi8(counter_le2, bswap);
    counter_be3 = _mm_shuffle_epi8(counter_le3, bswap);
    counter_le0 = ADD(counter_le0, four);
    counter_le1 = ADD(counter_le1, four);
    counter_le2 = ADD(counter_le2, four);
    counter_le3 = ADD(counter_le3, four);
    AES4_CIPHER(counter_be0, counter_be1, counter_be2, counter_be3, key);
    counter_be0 = XOR(counter_be0, _mm_loadu_si128((__m128i*)in + 4 * i + 0));
    counter_be1 = XOR(counter_be1, _mm_loadu_si128((__m128i*)in + 4 * i + 1));
    counter_be2 = XOR(counter_be2, _mm_loadu_si128((__m128i*)in + 4 * i + 2));
    counter_be3 = XOR(counter_be3, _mm_loadu_si128((__m128i*)in + 4 * i + 3));
    _mm_storeu_si128((__m128i*)out + 4 * i + 0, counter_be0);
    _mm_storeu_si128((__m128i*)out + 4 * i + 1, counter_be1);
    _mm_storeu_si128((__m128i*)out + 4 * i + 2, counter_be2);
    _mm_storeu_si128((__m128i*)out + 4 * i + 3, counter_be3);
  }
  for (i = 4 * i; i < num_blocks; i++) {
    counter_be0 = _mm_shuffle_epi8(counter_le0, bswap);
    counter_le0 = ADD(counter_le0, one);
    AES1_CIPHER(counter_be0, key);
    counter_be0 = XOR(counter_be0, _mm_loadu_si128((__m128i*)in + i));
    _mm_storeu_si128((__m128i*)out + i, counter_be0);
  }
  if (remainder_bytes > 0) {
    counter_be0 = _mm_shuffle_epi8(counter_le0, bswap);
    AES1_CIPHER(counter_be0, key);
    for (j = 0; j < remainder_bytes; j++) {
      out[16 * i + j] = ((uint8_t*)&counter_be0)[j] ^ in[16 * i + j];
    }
  }
}

static void AES_CTR_encrypt_pipe8(const unsigned char* in, unsigned char* out,
                                  const unsigned char* ivec,
                                  unsigned long length,
                                  const unsigned char* key,
                                  const unsigned int number_of_rounds) {
  const __m128i one = _mm_load_si128((__m128i*)ONE);
  const __m128i eight = _mm_load_si128((__m128i*)EIGHT);
  const __m128i bswap = _mm_load_si128((__m128i*)BSWAP64);
  __m128i counter_be0, counter_le0;
  __m128i counter_be1, counter_le1;
  __m128i counter_be2, counter_le2;
  __m128i counter_be3, counter_le3;
  __m128i counter_be4, counter_le4;
  __m128i counter_be5, counter_le5;
  __m128i counter_be6, counter_le6;
  __m128i counter_be7, counter_le7;
  unsigned long i = 0, j = 0;
  unsigned long remainder_bytes = length % 16;
  unsigned long num_blocks = length / 16;

  counter_be0 = _mm_loadu_si128((__m128i*)ivec);
  counter_le0 = _mm_shuffle_epi8(counter_be0, bswap);
  counter_le1 = ADD(counter_le0, one);
  counter_le2 = ADD(counter_le1, one);
  counter_le3 = ADD(counter_le2, one);
  counter_le4 = ADD(counter_le3, one);
  counter_le5 = ADD(counter_le4, one);
  counter_le6 = ADD(counter_le5, one);
  counter_le7 = ADD(counter_le6, one);

  for (i = 0; i < num_blocks / 8; i++) {
    counter_be0 = _mm_shuffle_epi8(counter_le0, bswap);
    counter_be1 = _mm_shuffle_epi8(counter_le1, bswap);
    counter_be2 = _mm_shuffle_epi8(counter_le2, bswap);
    counter_be3 = _mm_shuffle_epi8(counter_le3, bswap);
    counter_be4 = _mm_shuffle_epi8(counter_le4, bswap);
    counter_be5 = _mm_shuffle_epi8(counter_le5, bswap);
    counter_be6 = _mm_shuffle_epi8(counter_le6, bswap);
    counter_be7 = _mm_shuffle_epi8(counter_le7, bswap);
    counter_le0 = ADD(counter_le0, eight);
    counter_le1 = ADD(counter_le1, eight);
    counter_le2 = ADD(counter_le2, eight);
    counter_le3 = ADD(counter_le3, eight);
    counter_le4 = ADD(counter_le4, eight);
    counter_le5 = ADD(counter_le5, eight);
    counter_le6 = ADD(counter_le6, eight);
    counter_le7 = ADD(counter_le7, eight);
    AES8_CIPHER(counter_be0, counter_be1, counter_be2, counter_be3, counter_be4,
                counter_be5, counter_be6, counter_be7, key);
    counter_be0 = XOR(counter_be0, _mm_loadu_si128((__m128i*)in + 8 * i + 0));
    counter_be1 = XOR(counter_be1, _mm_loadu_si128((__m128i*)in + 8 * i + 1));
    counter_be2 = XOR(counter_be2, _mm_loadu_si128((__m128i*)in + 8 * i + 2));
    counter_be3 = XOR(counter_be3, _mm_loadu_si128((__m128i*)in + 8 * i + 3));
    counter_be4 = XOR(counter_be4, _mm_loadu_si128((__m128i*)in + 8 * i + 4));
    counter_be5 = XOR(counter_be5, _mm_loadu_si128((__m128i*)in + 8 * i + 5));
    counter_be6 = XOR(counter_be6, _mm_loadu_si128((__m128i*)in + 8 * i + 6));
    counter_be7 = XOR(counter_be7, _mm_loadu_si128((__m128i*)in + 8 * i + 7));
    _mm_storeu_si128((__m128i*)out + 8 * i + 0, counter_be0);
    _mm_storeu_si128((__m128i*)out + 8 * i + 1, counter_be1);
    _mm_storeu_si128((__m128i*)out + 8 * i + 2, counter_be2);
    _mm_storeu_si128((__m128i*)out + 8 * i + 3, counter_be3);
    _mm_storeu_si128((__m128i*)out + 8 * i + 4, counter_be4);
    _mm_storeu_si128((__m128i*)out + 8 * i + 5, counter_be5);
    _mm_storeu_si128((__m128i*)out + 8 * i + 6, counter_be6);
    _mm_storeu_si128((__m128i*)out + 8 * i + 7, counter_be7);
  }
  for (i = 8 * i; i < num_blocks; i++) {
    counter_be0 = _mm_shuffle_epi8(counter_le0, bswap);
    counter_le0 = ADD(counter_le0, one);
    AES1_CIPHER(counter_be0, key);
    counter_be0 = XOR(counter_be0, _mm_loadu_si128((__m128i*)in + i));
    _mm_storeu_si128((__m128i*)out + i, counter_be0);
  }
  if (remainder_bytes > 0) {
    counter_be0 = _mm_shuffle_epi8(counter_le0, bswap);
    AES1_CIPHER(counter_be0, key);
    for (j = 0; j < remainder_bytes; j++) {
      out[16 * i + j] = ((uint8_t*)&counter_be0)[j] ^ in[16 * i + j];
    }
  }
}

#undef AES1_CIPHER
#undef AES2_CIPHER
#undef AES4_CIPHER
#undef AES8_CIPHER

/** AES-CTR Interface for all Security Levels */

/**
 *
 * @param key
 * @param id
 * @return
 */
uint8_t* AES_KeyExpansion(const unsigned char* key, AES_CIPHER_ID id) {
  unsigned char* key_sched = NULL;
  switch (id) {
    case AES_128:
      key_sched = (uint8_t*)_mm_malloc(
          (AES_BlockSize / 8) * (AES_128_numRounds + 1), ALIGN_BYTES);
      AES_128_Key_Expansion(key, key_sched);
      break;
    case AES_192:
      key_sched = (uint8_t*)_mm_malloc(
          (AES_BlockSize / 8) * (AES_192_numRounds + 1), ALIGN_BYTES);
      AES_192_Key_Expansion(key, key_sched);
      break;
    case AES_256:
      key_sched = (uint8_t*)_mm_malloc(
          (AES_BlockSize / 8) * (AES_256_numRounds + 1), ALIGN_BYTES);
      AES_256_Key_Expansion(key, key_sched);
      break;
  }
  return key_sched;
}

/**
 *
 * @param in
 * @param out
 * @param iv
 * @param message_length
 * @param key_sched
 * @param id
 */
void AES_CTR_Encrypt(const unsigned char* in, unsigned char* out,
                     const unsigned char* iv, unsigned long message_length,
                     const unsigned char* key_sched, AES_CIPHER_ID id) {
  switch (id) {
    case AES_128:
      AES_CTR_encrypt_all(in, out, iv, message_length, key_sched,
                          AES_128_numRounds);
      break;
    case AES_192:
      AES_CTR_encrypt_all(in, out, iv, message_length, key_sched,
                          AES_192_numRounds);
      break;
    case AES_256:
      AES_CTR_encrypt_all(in, out, iv, message_length, key_sched,
                          AES_256_numRounds);
      break;
    default:
      return;
  }
}

/**
 *
 * @param in
 * @param out
 * @param iv
 * @param message_length
 * @param key_sched
 * @param id
 * @param pipeline_overlapped
 */
void AES_CTR_Encrypt_Pipelined(const unsigned char* in, unsigned char* out,
                               const unsigned char* iv,
                               unsigned long message_length,
                               const unsigned char* key_sched, AES_CIPHER_ID id,
                               int pipeline_overlapped) {
  switch (pipeline_overlapped) {
    case 2:
      switch (id) {
        case AES_128:
          AES_CTR_encrypt_pipe2(in, out, iv, message_length, key_sched,
                                AES_128_numRounds);
          break;
        case AES_192:
          AES_CTR_encrypt_pipe2(in, out, iv, message_length, key_sched,
                                AES_192_numRounds);
          break;
        case AES_256:
          AES_CTR_encrypt_pipe2(in, out, iv, message_length, key_sched,
                                AES_256_numRounds);
          break;
        default:
          return;
      }
      break;
    case 4:
      switch (id) {
        case AES_128:
          AES_CTR_encrypt_pipe4(in, out, iv, message_length, key_sched,
                                AES_128_numRounds);
          break;
        case AES_192:
          AES_CTR_encrypt_pipe4(in, out, iv, message_length, key_sched,
                                AES_192_numRounds);
          break;
        case AES_256:
          AES_CTR_encrypt_pipe4(in, out, iv, message_length, key_sched,
                                AES_256_numRounds);
          break;
        default:
          return;
      }
      break;
    case 8:
      switch (id) {
        case AES_128:
          AES_CTR_encrypt_pipe8(in, out, iv, message_length, key_sched,
                                AES_128_numRounds);
          break;
        case AES_192:
          AES_CTR_encrypt_pipe8(in, out, iv, message_length, key_sched,
                                AES_192_numRounds);
          break;
        case AES_256:
          AES_CTR_encrypt_pipe8(in, out, iv, message_length, key_sched,
                                AES_256_numRounds);
          break;
        default:
          return;
      }
      break;
    default:
      return;
  }
}

/** AES-CBC Implementations */

void AES_CBC_encrypt(const unsigned char* in, unsigned char* out,
                     unsigned char ivec[16], unsigned long length,
                     unsigned char* key, const int number_of_rounds) {
  __m128i feedback, data;
  int j;
  unsigned long i;
  if (length % 16)
    length = length / 16 + 1;
  else
    length /= 16;
  feedback = _mm_loadu_si128((__m128i*)ivec);
  for (i = 0; i < length; i++) {
    data = _mm_loadu_si128(&((__m128i*)in)[i]);
    feedback = XOR(data, feedback);
    feedback = XOR(feedback, ((__m128i*)key)[0]);
    for (j = 1; j < number_of_rounds; j++) {
      feedback = AESENC(feedback, ((__m128i*)key)[j]);
    }
    feedback = AESENCLAST(feedback, ((__m128i*)key)[j]);
    _mm_storeu_si128(&((__m128i*)out)[i], feedback);
  }
}

void AES_CBC_decrypt(const unsigned char* in, unsigned char* out,
                     unsigned char ivec[16], unsigned long length,
                     unsigned char* key, const int number_of_rounds) {
  __m128i data, feedback, last_in;
  int j;
  unsigned long i;
  if (length % 16) {
    length = length / 16 + 1;
  } else {
    length /= 16;
  }
  feedback = _mm_loadu_si128((__m128i*)ivec);
  for (i = 0; i < length; i++) {
    last_in = _mm_loadu_si128(&((__m128i*)in)[i]);
    data = XOR(last_in, ((__m128i*)key)[0]);
    for (j = 1; j < number_of_rounds; j++) {
      data = AESDEC(data, ((__m128i*)key)[j]);
    }
    data = AESDECLAST(data, ((__m128i*)key)[j]);
    data = XOR(data, feedback);
    _mm_storeu_si128(&((__m128i*)out)[i], data);
    feedback = last_in;
  }
}

void AES_CBC_decrypt_pipe2(const unsigned char* in, unsigned char* out,
                           unsigned char* ivec, unsigned long length,
                           unsigned char* key_schedule, const unsigned int nr) {
  __m128i data1, data2;
  __m128i feedback1, feedback2, last_in;
  unsigned long j;
  unsigned int i;
  if (length % 16) {
    length = length / 16 + 1;
  } else {
    length /= 16;
  }
  feedback1 = _mm_loadu_si128((__m128i*)ivec);
  for (i = 0; i < length / 2; i++) {
    data1 = _mm_loadu_si128(&((__m128i*)in)[i * 4 + 0]);
    data2 = _mm_loadu_si128(&((__m128i*)in)[i * 4 + 1]);
    feedback2 = data1;
    last_in = data2;
    data1 = XOR(data1, ((__m128i*)key_schedule)[0]);
    data2 = XOR(data2, ((__m128i*)key_schedule)[0]);

    for (j = 1; j < nr; j++) {
      data1 = AESDEC(data1, ((__m128i*)key_schedule)[j]);
      data2 = AESDEC(data2, ((__m128i*)key_schedule)[j]);
    }

    data1 = AESDECLAST(data1, ((__m128i*)key_schedule)[j]);
    data2 = AESDECLAST(data2, ((__m128i*)key_schedule)[j]);

    data1 = XOR(data1, feedback1);
    data2 = XOR(data2, feedback2);

    _mm_storeu_si128(&((__m128i*)out)[i * 4 + 0], data1);
    _mm_storeu_si128(&((__m128i*)out)[i * 4 + 1], data2);
    feedback1 = last_in;
  }
  for (j = i * 2; j < length; j++) {
    data1 = _mm_loadu_si128(&((__m128i*)in)[j]);
    last_in = data1;
    data1 = XOR(data1, ((__m128i*)key_schedule)[0]);
    for (i = 1; i < nr; i++) {
      data1 = AESDEC(data1, ((__m128i*)key_schedule)[i]);
    }
    data1 = AESDECLAST(data1, ((__m128i*)key_schedule)[i]);
    data1 = XOR(data1, feedback1);
    _mm_storeu_si128(&((__m128i*)out)[j], data1);
    feedback1 = last_in;
  }
}

void AES_CBC_decrypt_pipe4(const unsigned char* in, unsigned char* out,
                           unsigned char* ivec, unsigned long length,
                           unsigned char* key_schedule, const unsigned int nr) {
  __m128i data1, data2, data3, data4;
  __m128i feedback1, feedback2, feedback3, feedback4, last_in;
  unsigned long j;
  unsigned int i;
  if (length % 16) {
    length = length / 16 + 1;
  } else {
    length /= 16;
  }
  feedback1 = _mm_loadu_si128((__m128i*)ivec);
  for (i = 0; i < length / 4; i++) {
    data1 = _mm_loadu_si128(&((__m128i*)in)[i * 4 + 0]);
    data2 = _mm_loadu_si128(&((__m128i*)in)[i * 4 + 1]);
    data3 = _mm_loadu_si128(&((__m128i*)in)[i * 4 + 2]);
    data4 = _mm_loadu_si128(&((__m128i*)in)[i * 4 + 3]);
    feedback2 = data1;
    feedback3 = data2;
    feedback4 = data3;
    last_in = data4;
    data1 = XOR(data1, ((__m128i*)key_schedule)[0]);
    data2 = XOR(data2, ((__m128i*)key_schedule)[0]);
    data3 = XOR(data3, ((__m128i*)key_schedule)[0]);
    data4 = XOR(data4, ((__m128i*)key_schedule)[0]);

    for (j = 1; j < nr; j++) {
      data1 = AESDEC(data1, ((__m128i*)key_schedule)[j]);
      data2 = AESDEC(data2, ((__m128i*)key_schedule)[j]);
      data3 = AESDEC(data3, ((__m128i*)key_schedule)[j]);
      data4 = AESDEC(data4, ((__m128i*)key_schedule)[j]);
    }

    data1 = AESDECLAST(data1, ((__m128i*)key_schedule)[j]);
    data2 = AESDECLAST(data2, ((__m128i*)key_schedule)[j]);
    data3 = AESDECLAST(data3, ((__m128i*)key_schedule)[j]);
    data4 = AESDECLAST(data4, ((__m128i*)key_schedule)[j]);

    data1 = XOR(data1, feedback1);
    data2 = XOR(data2, feedback2);
    data3 = XOR(data3, feedback3);
    data4 = XOR(data4, feedback4);

    _mm_storeu_si128(&((__m128i*)out)[i * 4 + 0], data1);
    _mm_storeu_si128(&((__m128i*)out)[i * 4 + 1], data2);
    _mm_storeu_si128(&((__m128i*)out)[i * 4 + 2], data3);
    _mm_storeu_si128(&((__m128i*)out)[i * 4 + 3], data4);
    feedback1 = last_in;
  }
  for (j = i * 4; j < length; j++) {
    data1 = _mm_loadu_si128(&((__m128i*)in)[j]);
    last_in = data1;
    data1 = XOR(data1, ((__m128i*)key_schedule)[0]);
    for (i = 1; i < nr; i++) {
      data1 = AESDEC(data1, ((__m128i*)key_schedule)[i]);
    }
    data1 = AESDECLAST(data1, ((__m128i*)key_schedule)[i]);
    data1 = XOR(data1, feedback1);
    _mm_storeu_si128(&((__m128i*)out)[j], data1);
    feedback1 = last_in;
  }
}

void AES_CBC_decrypt_pipe8(const unsigned char* in, unsigned char* out,
                           unsigned char ivec[16], unsigned long length,
                           unsigned char* key_schedule, const unsigned int nr) {
  __m128i data1, data2, data3, data4, data5, data6, data7, data8;
  __m128i feedback1, feedback2, feedback3, feedback4, feedback5, feedback6,
      feedback7, feedback8, last_in;
  unsigned long j;
  unsigned int i;
  if (length % 16) {
    length = length / 16 + 1;
  } else {
    length /= 16;
  }
  feedback1 = _mm_loadu_si128((__m128i*)ivec);
  for (i = 0; i < length / 8; i++) {
    data1 = _mm_loadu_si128(&((__m128i*)in)[i * 4 + 0]);
    data2 = _mm_loadu_si128(&((__m128i*)in)[i * 4 + 1]);
    data3 = _mm_loadu_si128(&((__m128i*)in)[i * 4 + 2]);
    data4 = _mm_loadu_si128(&((__m128i*)in)[i * 4 + 3]);
    data5 = _mm_loadu_si128(&((__m128i*)in)[i * 4 + 4]);
    data6 = _mm_loadu_si128(&((__m128i*)in)[i * 4 + 5]);
    data7 = _mm_loadu_si128(&((__m128i*)in)[i * 4 + 6]);
    data8 = _mm_loadu_si128(&((__m128i*)in)[i * 4 + 7]);
    feedback2 = data1;
    feedback3 = data2;
    feedback4 = data3;
    feedback5 = data4;
    feedback6 = data5;
    feedback7 = data6;
    feedback8 = data7;
    last_in = data8;
    data1 = XOR(data1, ((__m128i*)key_schedule)[0]);
    data2 = XOR(data2, ((__m128i*)key_schedule)[0]);
    data3 = XOR(data3, ((__m128i*)key_schedule)[0]);
    data4 = XOR(data4, ((__m128i*)key_schedule)[0]);
    data5 = XOR(data5, ((__m128i*)key_schedule)[0]);
    data6 = XOR(data6, ((__m128i*)key_schedule)[0]);
    data7 = XOR(data7, ((__m128i*)key_schedule)[0]);
    data8 = XOR(data8, ((__m128i*)key_schedule)[0]);

    for (j = 1; j < nr; j++) {
      data1 = AESDEC(data1, ((__m128i*)key_schedule)[j]);
      data2 = AESDEC(data2, ((__m128i*)key_schedule)[j]);
      data3 = AESDEC(data3, ((__m128i*)key_schedule)[j]);
      data4 = AESDEC(data4, ((__m128i*)key_schedule)[j]);
      data5 = AESDEC(data5, ((__m128i*)key_schedule)[j]);
      data6 = AESDEC(data6, ((__m128i*)key_schedule)[j]);
      data7 = AESDEC(data7, ((__m128i*)key_schedule)[j]);
      data8 = AESDEC(data8, ((__m128i*)key_schedule)[j]);
    }

    data1 = AESDECLAST(data1, ((__m128i*)key_schedule)[j]);
    data2 = AESDECLAST(data2, ((__m128i*)key_schedule)[j]);
    data3 = AESDECLAST(data3, ((__m128i*)key_schedule)[j]);
    data4 = AESDECLAST(data4, ((__m128i*)key_schedule)[j]);
    data5 = AESDECLAST(data5, ((__m128i*)key_schedule)[j]);
    data6 = AESDECLAST(data6, ((__m128i*)key_schedule)[j]);
    data7 = AESDECLAST(data7, ((__m128i*)key_schedule)[j]);
    data8 = AESDECLAST(data8, ((__m128i*)key_schedule)[j]);

    data1 = XOR(data1, feedback1);
    data2 = XOR(data2, feedback2);
    data3 = XOR(data3, feedback3);
    data4 = XOR(data4, feedback4);
    data5 = XOR(data5, feedback5);
    data6 = XOR(data6, feedback6);
    data7 = XOR(data7, feedback7);
    data8 = XOR(data8, feedback8);

    _mm_storeu_si128(&((__m128i*)out)[i * 4 + 0], data1);
    _mm_storeu_si128(&((__m128i*)out)[i * 4 + 1], data2);
    _mm_storeu_si128(&((__m128i*)out)[i * 4 + 2], data3);
    _mm_storeu_si128(&((__m128i*)out)[i * 4 + 3], data4);
    _mm_storeu_si128(&((__m128i*)out)[i * 4 + 4], data5);
    _mm_storeu_si128(&((__m128i*)out)[i * 4 + 5], data6);
    _mm_storeu_si128(&((__m128i*)out)[i * 4 + 6], data7);
    _mm_storeu_si128(&((__m128i*)out)[i * 4 + 7], data8);
    feedback1 = last_in;
  }
  for (j = i * 8; j < length; j++) {
    data1 = _mm_loadu_si128(&((__m128i*)in)[j]);
    last_in = data1;
    data1 = XOR(data1, ((__m128i*)key_schedule)[0]);
    for (i = 1; i < nr; i++) {
      data1 = AESDEC(data1, ((__m128i*)key_schedule)[i]);
    }
    data1 = AESDECLAST(data1, ((__m128i*)key_schedule)[i]);
    data1 = XOR(data1, feedback1);
    _mm_storeu_si128(&((__m128i*)out)[j], data1);
    feedback1 = last_in;
  }
}

void AES_CBC_encrypt_2w(const unsigned char** in, unsigned char** out,
                        unsigned char** ivec, unsigned long length,
                        const unsigned char* key, const int nr) {
  __m128i feedback0, data0;
  __m128i feedback1, data1;
  int j;
  unsigned long i;
  if (length % 16) {
    length = length / 16 + 1;
  } else {
    length /= 16;
  }
  feedback0 = _mm_loadu_si128((__m128i*)(ivec[0]));
  feedback1 = _mm_loadu_si128((__m128i*)(ivec[1]));
  for (i = 0; i < length; i++) {
    data0 = _mm_loadu_si128(&((__m128i*)(in[0]))[i]);
    data1 = _mm_loadu_si128(&((__m128i*)(in[1]))[i]);
    feedback0 = XOR(data0, feedback0);
    feedback1 = XOR(data1, feedback1);
    feedback0 = XOR(feedback0, ((__m128i*)key)[0]);
    feedback1 = XOR(feedback1, ((__m128i*)key)[0]);
    for (j = 1; j < nr; j++) {
      feedback0 = AESENC(feedback0, ((__m128i*)key)[j]);
      feedback1 = AESENC(feedback1, ((__m128i*)key)[j]);
    }
    feedback0 = AESENCLAST(feedback0, ((__m128i*)key)[j]);
    feedback1 = AESENCLAST(feedback1, ((__m128i*)key)[j]);
    _mm_storeu_si128(&((__m128i*)(out[0]))[i], feedback0);
    _mm_storeu_si128(&((__m128i*)(out[1]))[i], feedback1);
  }
}

void AES_CBC_encrypt_4w(const unsigned char** in, unsigned char** out,
                        unsigned char** ivec, unsigned long length,
                        const unsigned char* key, const int nr) {
  __m128i feedback0, data0;
  __m128i feedback1, data1;
  __m128i feedback2, data2;
  __m128i feedback3, data3;
  int j;
  unsigned long i;
  if (length % 16) {
    length = length / 16 + 1;
  } else {
    length /= 16;
  }
  feedback0 = _mm_loadu_si128((__m128i*)(ivec[0]));
  feedback1 = _mm_loadu_si128((__m128i*)(ivec[1]));
  feedback2 = _mm_loadu_si128((__m128i*)(ivec[2]));
  feedback3 = _mm_loadu_si128((__m128i*)(ivec[3]));
  for (i = 0; i < length; i++) {
    data0 = _mm_loadu_si128(&((__m128i*)(in[0]))[i]);
    data1 = _mm_loadu_si128(&((__m128i*)(in[1]))[i]);
    data2 = _mm_loadu_si128(&((__m128i*)(in[2]))[i]);
    data3 = _mm_loadu_si128(&((__m128i*)(in[3]))[i]);
    feedback0 = XOR(data0, feedback0);
    feedback1 = XOR(data1, feedback1);
    feedback2 = XOR(data2, feedback2);
    feedback3 = XOR(data3, feedback3);
    feedback0 = XOR(feedback0, ((__m128i*)key)[0]);
    feedback1 = XOR(feedback1, ((__m128i*)key)[0]);
    feedback2 = XOR(feedback2, ((__m128i*)key)[0]);
    feedback3 = XOR(feedback3, ((__m128i*)key)[0]);
    for (j = 1; j < nr; j++) {
      feedback0 = AESENC(feedback0, ((__m128i*)key)[j]);
      feedback1 = AESENC(feedback1, ((__m128i*)key)[j]);
      feedback2 = AESENC(feedback2, ((__m128i*)key)[j]);
      feedback3 = AESENC(feedback3, ((__m128i*)key)[j]);
    }
    feedback0 = AESENCLAST(feedback0, ((__m128i*)key)[j]);
    feedback1 = AESENCLAST(feedback1, ((__m128i*)key)[j]);
    feedback2 = AESENCLAST(feedback2, ((__m128i*)key)[j]);
    feedback3 = AESENCLAST(feedback3, ((__m128i*)key)[j]);
    _mm_storeu_si128(&((__m128i*)(out[0]))[i], feedback0);
    _mm_storeu_si128(&((__m128i*)(out[1]))[i], feedback1);
    _mm_storeu_si128(&((__m128i*)(out[2]))[i], feedback2);
    _mm_storeu_si128(&((__m128i*)(out[3]))[i], feedback3);
  }
}

void AES_CBC_encrypt_6w(const unsigned char** in, unsigned char** out,
                        unsigned char** ivec, unsigned long length,
                        const unsigned char* key, const int nr) {
  __m128i feedback0, data0;
  __m128i feedback1, data1;
  __m128i feedback2, data2;
  __m128i feedback3, data3;
  __m128i feedback4, data4;
  __m128i feedback5, data5;
  int j;
  unsigned long i;
  if (length % 16) {
    length = length / 16 + 1;
  } else {
    length /= 16;
  }
  feedback0 = _mm_loadu_si128((__m128i*)(ivec[0]));
  feedback1 = _mm_loadu_si128((__m128i*)(ivec[1]));
  feedback2 = _mm_loadu_si128((__m128i*)(ivec[2]));
  feedback3 = _mm_loadu_si128((__m128i*)(ivec[3]));
  feedback4 = _mm_loadu_si128((__m128i*)(ivec[4]));
  feedback5 = _mm_loadu_si128((__m128i*)(ivec[5]));
  for (i = 0; i < length; i++) {
    data0 = _mm_loadu_si128(&((__m128i*)(in[0]))[i]);
    data1 = _mm_loadu_si128(&((__m128i*)(in[1]))[i]);
    data2 = _mm_loadu_si128(&((__m128i*)(in[2]))[i]);
    data3 = _mm_loadu_si128(&((__m128i*)(in[3]))[i]);
    data4 = _mm_loadu_si128(&((__m128i*)(in[4]))[i]);
    data5 = _mm_loadu_si128(&((__m128i*)(in[5]))[i]);
    feedback0 = XOR(data0, feedback0);
    feedback1 = XOR(data1, feedback1);
    feedback2 = XOR(data2, feedback2);
    feedback3 = XOR(data3, feedback3);
    feedback4 = XOR(data4, feedback4);
    feedback5 = XOR(data5, feedback5);
    feedback0 = XOR(feedback0, ((__m128i*)key)[0]);
    feedback1 = XOR(feedback1, ((__m128i*)key)[0]);
    feedback2 = XOR(feedback2, ((__m128i*)key)[0]);
    feedback3 = XOR(feedback3, ((__m128i*)key)[0]);
    feedback4 = XOR(feedback4, ((__m128i*)key)[0]);
    feedback5 = XOR(feedback5, ((__m128i*)key)[0]);
    for (j = 1; j < nr; j++) {
      feedback0 = AESENC(feedback0, ((__m128i*)key)[j]);
      feedback1 = AESENC(feedback1, ((__m128i*)key)[j]);
      feedback2 = AESENC(feedback2, ((__m128i*)key)[j]);
      feedback3 = AESENC(feedback3, ((__m128i*)key)[j]);
      feedback4 = AESENC(feedback4, ((__m128i*)key)[j]);
      feedback5 = AESENC(feedback5, ((__m128i*)key)[j]);
    }
    feedback0 = AESENCLAST(feedback0, ((__m128i*)key)[j]);
    feedback1 = AESENCLAST(feedback1, ((__m128i*)key)[j]);
    feedback2 = AESENCLAST(feedback2, ((__m128i*)key)[j]);
    feedback3 = AESENCLAST(feedback3, ((__m128i*)key)[j]);
    feedback4 = AESENCLAST(feedback4, ((__m128i*)key)[j]);
    feedback5 = AESENCLAST(feedback5, ((__m128i*)key)[j]);
    _mm_storeu_si128(&((__m128i*)(out[0]))[i], feedback0);
    _mm_storeu_si128(&((__m128i*)(out[1]))[i], feedback1);
    _mm_storeu_si128(&((__m128i*)(out[2]))[i], feedback2);
    _mm_storeu_si128(&((__m128i*)(out[3]))[i], feedback3);
    _mm_storeu_si128(&((__m128i*)(out[4]))[i], feedback4);
    _mm_storeu_si128(&((__m128i*)(out[5]))[i], feedback5);
  }
}

void AES_CBC_encrypt_8w(const unsigned char** in, unsigned char** out,
                        unsigned char** ivec, unsigned long length,
                        const unsigned char* key, const int nr) {
  __m128i feedback0, data0;
  __m128i feedback1, data1;
  __m128i feedback2, data2;
  __m128i feedback3, data3;
  __m128i feedback4, data4;
  __m128i feedback5, data5;
  __m128i feedback6, data6;
  __m128i feedback7, data7;
  int j;
  unsigned long i;
  if (length % 16) {
    length = length / 16 + 1;
  } else {
    length /= 16;
  }
  feedback0 = _mm_loadu_si128((__m128i*)(ivec[0]));
  feedback1 = _mm_loadu_si128((__m128i*)(ivec[1]));
  feedback2 = _mm_loadu_si128((__m128i*)(ivec[2]));
  feedback3 = _mm_loadu_si128((__m128i*)(ivec[3]));
  feedback4 = _mm_loadu_si128((__m128i*)(ivec[4]));
  feedback5 = _mm_loadu_si128((__m128i*)(ivec[5]));
  feedback6 = _mm_loadu_si128((__m128i*)(ivec[6]));
  feedback7 = _mm_loadu_si128((__m128i*)(ivec[7]));
  for (i = 0; i < length; i++) {
    data0 = _mm_loadu_si128(&((__m128i*)(in[0]))[i]);
    data1 = _mm_loadu_si128(&((__m128i*)(in[1]))[i]);
    data2 = _mm_loadu_si128(&((__m128i*)(in[2]))[i]);
    data3 = _mm_loadu_si128(&((__m128i*)(in[3]))[i]);
    data4 = _mm_loadu_si128(&((__m128i*)(in[4]))[i]);
    data5 = _mm_loadu_si128(&((__m128i*)(in[5]))[i]);
    data6 = _mm_loadu_si128(&((__m128i*)(in[6]))[i]);
    data7 = _mm_loadu_si128(&((__m128i*)(in[7]))[i]);
    feedback0 = XOR(data0, feedback0);
    feedback1 = XOR(data1, feedback1);
    feedback2 = XOR(data2, feedback2);
    feedback3 = XOR(data3, feedback3);
    feedback4 = XOR(data4, feedback4);
    feedback5 = XOR(data5, feedback5);
    feedback6 = XOR(data6, feedback6);
    feedback7 = XOR(data7, feedback7);
    feedback0 = XOR(feedback0, ((__m128i*)key)[0]);
    feedback1 = XOR(feedback1, ((__m128i*)key)[0]);
    feedback2 = XOR(feedback2, ((__m128i*)key)[0]);
    feedback3 = XOR(feedback3, ((__m128i*)key)[0]);
    feedback4 = XOR(feedback4, ((__m128i*)key)[0]);
    feedback5 = XOR(feedback5, ((__m128i*)key)[0]);
    feedback6 = XOR(feedback6, ((__m128i*)key)[0]);
    feedback7 = XOR(feedback7, ((__m128i*)key)[0]);
    for (j = 1; j < nr; j++) {
      feedback0 = AESENC(feedback0, ((__m128i*)key)[j]);
      feedback1 = AESENC(feedback1, ((__m128i*)key)[j]);
      feedback2 = AESENC(feedback2, ((__m128i*)key)[j]);
      feedback3 = AESENC(feedback3, ((__m128i*)key)[j]);
      feedback4 = AESENC(feedback4, ((__m128i*)key)[j]);
      feedback5 = AESENC(feedback5, ((__m128i*)key)[j]);
      feedback6 = AESENC(feedback6, ((__m128i*)key)[j]);
      feedback7 = AESENC(feedback7, ((__m128i*)key)[j]);
    }
    feedback0 = AESENCLAST(feedback0, ((__m128i*)key)[j]);
    feedback1 = AESENCLAST(feedback1, ((__m128i*)key)[j]);
    feedback2 = AESENCLAST(feedback2, ((__m128i*)key)[j]);
    feedback3 = AESENCLAST(feedback3, ((__m128i*)key)[j]);
    feedback4 = AESENCLAST(feedback4, ((__m128i*)key)[j]);
    feedback5 = AESENCLAST(feedback5, ((__m128i*)key)[j]);
    feedback6 = AESENCLAST(feedback6, ((__m128i*)key)[j]);
    feedback7 = AESENCLAST(feedback7, ((__m128i*)key)[j]);
    _mm_storeu_si128(&((__m128i*)(out[0]))[i], feedback0);
    _mm_storeu_si128(&((__m128i*)(out[1]))[i], feedback1);
    _mm_storeu_si128(&((__m128i*)(out[2]))[i], feedback2);
    _mm_storeu_si128(&((__m128i*)(out[3]))[i], feedback3);
    _mm_storeu_si128(&((__m128i*)(out[4]))[i], feedback4);
    _mm_storeu_si128(&((__m128i*)(out[5]))[i], feedback5);
    _mm_storeu_si128(&((__m128i*)(out[6]))[i], feedback6);
    _mm_storeu_si128(&((__m128i*)(out[7]))[i], feedback7);
  }
}
