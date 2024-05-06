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
#include <flo-aesni.h>
#include <flo-cpuid.h>
#include <flo-random.h>
#include <stdio.h>
#include "clocks.h"

struct aes_timings {
  uint64_t size;
  uint64_t _1x;
  uint64_t _2x;
  uint64_t _4x;
  uint64_t _6x;
  uint64_t _8x;
};

#define MAX_SIZE_BITS 13
#define AES128_Rounds 10

#define BENCH_SIZE_1W_CBC(FUNC, K)                                             \
  do {                                                                         \
    long BENCH_TIMES = 0, CYCLES = 0;                                          \
    const unsigned long MAX_SIZE = 1 << MAX_SIZE_BITS;                         \
    unsigned long it = 0;                                                      \
    uint8_t *message = (uint8_t *)_mm_malloc(MAX_SIZE + 1, ALIGN_BYTES);       \
    uint8_t *cipher = (uint8_t *)_mm_malloc(MAX_SIZE + 1, ALIGN_BYTES);        \
    uint8_t *key = (uint8_t *)_mm_malloc(AES_128 / 8, ALIGN_BYTES);            \
    uint8_t *iv = (uint8_t *)_mm_malloc(AES_BlockSize / 8, ALIGN_BYTES);       \
    random_bytes(key, AES_128 / 8);                                            \
    random_bytes(iv, AES_BlockSize / 8);                                       \
    uint8_t *round_keys = AES_KeyExpansion(key, AES_128);                      \
    for (it = 0; it < MAX_SIZE_BITS; it++) {                                   \
      int message_size = 1 << it;                                              \
      BENCH_TIMES = 512 - it * 20;                                             \
      CLOCKS(                                                                  \
          FUNC(message, cipher, iv, message_size, round_keys, AES128_Rounds)); \
      table[it].size = message_size;                                           \
      table[it]._##K##x = CYCLES;                                              \
    }                                                                          \
    _mm_free(key);                                                             \
    _mm_free(iv);                                                              \
    _mm_free(round_keys);                                                      \
    _mm_free(message);                                                         \
    _mm_free(cipher);                                                          \
  } while (0)

#define CALL_CIPHER(FUNC, W, TYPE) CALL_CIPHER_##TYPE(FUNC, W)

#define CALL_CIPHER_SEQ(FUNC, W) \
  CLOCKS(FUNC(message, cipher, iv, message_size, round_keys, AES_128));

#define CALL_CIPHER_PIPE(FUNC, W) \
  CLOCKS(FUNC(message, cipher, iv, message_size, round_keys, AES_128, W));

#define BENCH_SIZE_1W_CTR(FUNC, K, TYPE)                                 \
  do {                                                                   \
    long BENCH_TIMES = 0, CYCLES = 0;                                    \
    const unsigned long MAX_SIZE = 1 << MAX_SIZE_BITS;                   \
    unsigned long it = 0;                                                \
    uint8_t *message = (uint8_t *)_mm_malloc(MAX_SIZE + 1, ALIGN_BYTES); \
    uint8_t *cipher = (uint8_t *)_mm_malloc(MAX_SIZE + 1, ALIGN_BYTES);  \
    uint8_t *key = (uint8_t *)_mm_malloc(AES_128 / 8, ALIGN_BYTES);      \
    uint8_t *iv = (uint8_t *)_mm_malloc(AES_BlockSize / 8, ALIGN_BYTES); \
    random_bytes(key, AES_128 / 8);                                      \
    random_bytes(iv, AES_BlockSize / 8);                                 \
    uint8_t *round_keys = AES_KeyExpansion(key, AES_128);                \
    for (it = 0; it < MAX_SIZE_BITS; it++) {                             \
      int message_size = 1 << it;                                        \
      BENCH_TIMES = 512 - it * 20;                                       \
      CALL_CIPHER(FUNC, K, TYPE)                                         \
      table[it].size = message_size;                                     \
      table[it]._##K##x = CYCLES;                                        \
    }                                                                    \
    _mm_free(key);                                                       \
    _mm_free(iv);                                                        \
    _mm_free(round_keys);                                                \
    _mm_free(message);                                                   \
    _mm_free(cipher);                                                    \
  } while (0)

#define BENCH_CBC_MULTI(FUNC, MSG_LEN, N)                                   \
  do {                                                                      \
    int i_multi = 0;                                                        \
    uint8_t *message[N];                                                    \
    uint8_t *cipher[N];                                                     \
    uint8_t *iv[N];                                                         \
    uint8_t *key = (uint8_t *)_mm_malloc(AES_128 / 8, ALIGN_BYTES);         \
    random_bytes(key, AES_128 / 8);                                         \
    uint8_t *round_keys = AES_KeyExpansion(key, AES_128);                   \
    for (i_multi = 0; i_multi < N; i_multi++) {                             \
      message[i_multi] = (uint8_t *)_mm_malloc((MSG_LEN) + 1, ALIGN_BYTES); \
      cipher[i_multi] = (uint8_t *)_mm_malloc((MSG_LEN) + 1, ALIGN_BYTES);  \
      iv[i_multi] = (uint8_t *)_mm_malloc(16, ALIGN_BYTES);                 \
      random_bytes(iv[i_multi], 16);                                        \
      random_bytes(message[i_multi], MSG_LEN);                              \
    }                                                                       \
    CLOCKS(FUNC((const unsigned char **)cipher, message, iv, MSG_LEN,       \
                round_keys, AES128_Rounds));                                \
    table[it].size = message_size;                                          \
    table[it]._##N##x = CYCLES;                                             \
    _mm_free(round_keys);                                                   \
    for (i_multi = 0; i_multi < N; i_multi++) {                             \
      _mm_free(message[i_multi]);                                           \
      _mm_free(cipher[i_multi]);                                            \
      _mm_free(iv[i_multi]);                                                \
    }                                                                       \
  } while (0)

#define BENCH_SIZE_NW(FUNC, N)                \
  do {                                        \
    long BENCH_TIMES = 0, CYCLES = 0;         \
    unsigned long it = 0;                     \
    for (it = 0; it < MAX_SIZE_BITS; it++) {  \
      int message_size = 1 << it;             \
      BENCH_TIMES = 512 - it * 20;            \
      BENCH_CBC_MULTI(FUNC, message_size, N); \
    }                                         \
  } while (0)

void print_multiple_message(struct aes_timings *table, int items) {
  int i;
  printf("            Cycles per byte \n");
  printf("╔═════════╦═════════╦═════════╦═════════╦═════════╦═════════╗\n");
  printf("║  bytes  ║   1x    ║   2x    ║   4x    ║   6x    ║   8x    ║\n");
  printf("╠═════════╩═════════╩═════════╩═════════╩═════════╩═════════╣\n");
  for (i = 0; i < items; i++) {
    printf("║%9ld║%9.2f║%9.2f║%9.2f║%9.2f║%9.2f║\n", table[i].size,
           table[i]._1x / (double)table[i].size / 1.0,
           table[i]._2x / (double)table[i].size / 2.0,
           table[i]._4x / (double)table[i].size / 4.0,
           table[i]._6x / (double)table[i].size / 6.0,
           table[i]._8x / (double)table[i].size / 8.0);
  }
  printf("╚═════════╩═════════╩═════════╩═════════╩═════════╩═════════╝\n");
}

void print_pipelined(struct aes_timings *table, int items) {
  int i;
  printf("            Cycles per byte \n");
  printf("╔═════════╦═════════╦═════════╦═════════╦═════════╗\n");
  printf("║  bytes  ║   1x    ║   2x    ║   4x    ║   8x    ║\n");
  printf("╠═════════╩═════════╩═════════╩═════════╩═════════╣\n");
  for (i = 0; i < items; i++) {
    printf("║%9ld║%9.2f║%9.2f║%9.2f║%9.2f║\n", table[i].size,
           table[i]._1x / (double)table[i].size,
           table[i]._2x / (double)table[i].size,
           table[i]._4x / (double)table[i].size,
           table[i]._8x / (double)table[i].size);
  }
  printf("╚═════════╩═════════╩═════════╩═════════╩═════════╝\n");
}

void bench_multiple_message_CBCenc() {
  struct aes_timings table[MAX_SIZE_BITS] = {{0, 0, 0, 0, 0, 0}};

  printf("Multiple-message CBC-Encryption:\n");
  printf("Running: CBC Enc \n");
  BENCH_SIZE_1W_CBC(AES_CBC_encrypt, 1);
  printf("Running: CBC Enc 2w\n");
  BENCH_SIZE_NW(AES_CBC_encrypt_2w, 2);
  printf("Running: CBC Enc 4w\n");
  BENCH_SIZE_NW(AES_CBC_encrypt_4w, 4);
  printf("Running: CBC Enc 6w\n");
  BENCH_SIZE_NW(AES_CBC_encrypt_6w, 6);
  printf("Running: CBC Enc 8w\n");
  BENCH_SIZE_NW(AES_CBC_encrypt_8w, 8);
  print_multiple_message(table, MAX_SIZE_BITS);
}

void bench_pipeline_CBC_dec() {
  struct aes_timings table[MAX_SIZE_BITS] = {{0, 0, 0, 0, 0, 0}};

  printf("Pipelined CBC Decryption:\n");
  printf("Running: CBC Dec \n");
  BENCH_SIZE_1W_CBC(AES_CBC_decrypt, 1);
  printf("Running: CBC Dec Pipe2\n");
  BENCH_SIZE_1W_CBC(AES_CBC_decrypt_pipe2, 2);
  printf("Running: CBC Dec Pipe4\n");
  BENCH_SIZE_1W_CBC(AES_CBC_decrypt_pipe4, 4);
  printf("Running: CBC Dec Pipe8\n");
  BENCH_SIZE_1W_CBC(AES_CBC_decrypt_pipe8, 8);
  print_pipelined(table, MAX_SIZE_BITS);
}

void bench_pipeline_CTR_enc() {
  struct aes_timings table[MAX_SIZE_BITS] = {{0, 0, 0, 0, 0, 0}};
  printf("Pipelined CTR Encryption:\n");
  printf("Running: CTR Enc \n");
  BENCH_SIZE_1W_CTR(AES_CTR_Encrypt, 1, SEQ);
  printf("Running: CTR Enc Pipe2 \n");
  BENCH_SIZE_1W_CTR(AES_CTR_Encrypt_Pipelined, 2, PIPE);
  printf("Running: CTR Enc Pipe4 \n");
  BENCH_SIZE_1W_CTR(AES_CTR_Encrypt_Pipelined, 4, PIPE);
  printf("Running: CTR Enc Pipe8 \n");
  BENCH_SIZE_1W_CTR(AES_CTR_Encrypt_Pipelined, 8, PIPE);
  print_pipelined(table, MAX_SIZE_BITS);
}
void bench_pipeline() {
  bench_pipeline_CBC_dec();
  bench_pipeline_CTR_enc();
}

int main() {
  machine_info();
  openssl_version();
  printf("== Start of Benchmark ===\n");
  bench_multiple_message_CBCenc();
  bench_pipeline();
  printf("== End of Benchmark =====\n");
  return 0;
}
