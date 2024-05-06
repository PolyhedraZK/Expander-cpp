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
#include <stdio.h>
#include <flo-aegis.h>
#include <flo-random.h>
#include <flo-cpuid.h>
#include "clocks.h"

struct aegis_timings {
  uint64_t size;
  uint64_t _ref;
  uint64_t _opt;
};

#define MAX_SIZE_BITS 13
#define TAG_SIZE_BYTES 16

#define BENCH_SIZE_1W(FUNC, IMPL)                   \
do{                                                 \
  long BENCH_TIMES = 0, CYCLES = 0;                 \
    const unsigned long MAX_SIZE=1<<MAX_SIZE_BITS;  \
    unsigned long it=0;                             \
    int adlen = 16;                                 \
    unsigned char * message = (unsigned char*)_mm_malloc(MAX_SIZE,ALIGN_BYTES); \
    uint8_t* c0 = (uint8_t*)_mm_malloc(MAX_SIZE+TAG_SIZE_BYTES,ALIGN_BYTES); \
    uint8_t* ad = (uint8_t*)_mm_malloc(adlen,ALIGN_BYTES); \
    ALIGN uint8_t npub[16];                         \
    ALIGN uint8_t k[16];                            \
    uint64_t clen;                        \
    random_bytes(message, MAX_SIZE);                \
    random_bytes(ad, adlen);                        \
    random_bytes(npub, 16);                         \
    random_bytes(k, 16);                            \
    for(it=0;it<MAX_SIZE_BITS;it++) {               \
      int message_size = 1<<it;                     \
      BENCH_TIMES = 512-it*20;                      \
      CLOCKS(FUNC(c0, &clen, message, message_size, ad, \
                  adlen, npub, k));                 \
      table[it].size = message_size;                \
      table[it]._ ## IMPL = CYCLES;                 \
    }                                               \
    _mm_free(message);                              \
    _mm_free(c0);                                   \
    _mm_free(ad);                                   \
}while(0)

void print_timings(struct aegis_timings *table, int items) {
  int i;
  printf("            Cycles per byte \n");
  printf("╔═════════╦═════════╦═════════╦═════════╦═════════╗\n");
  printf("║  bytes  ║Reference║Optimized║ Speedup ║ Savings ║\n");
  printf("╠═════════╩═════════╩═════════╩═════════╩═════════╣\n");
  for (i = 0; i < items; i++) {
    printf("║%9ld║%9.2f║%9.2f║%9.2f║%8.2f%%║\n",
           table[i].size,
           table[i]._ref / (double) table[i].size,
           table[i]._opt / (double) table[i].size,
           table[i]._ref / (double) table[i]._opt,
           100.0 * (1 - table[i]._opt / (double) table[i]._ref));
  }
  printf("╚═════════╩═════════╩═════════╩═════════╩═════════╝\n");
}

void bench_aegis() {
  struct aegis_timings table[MAX_SIZE_BITS] = {{0, 0, 0}};
  printf("Running: Reference Implementation \n");
  BENCH_SIZE_1W(crypto_aead_encrypt, ref);
  printf("Running: Optimized Implementation \n");
  BENCH_SIZE_1W(crypto_aead_encrypt_opt, opt);
  print_timings(table, MAX_SIZE_BITS);
}

int main() {
  machine_info();
  openssl_version();
  printf("=== Start Benchmarking ===\n");
  bench_aegis();
  printf("=== End Benchmarking ===\n");
  return 0;
}
