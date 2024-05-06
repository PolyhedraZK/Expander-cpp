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
#include <openssl/sha.h>
#include <flo-shani.h>
#include <flo-random.h>
#include <flo-cpuid.h>
#include "clocks.h"

/**
 * Maximum size of the message to be hashed: 2^MAX_SIZE_BITS.
 * E.g. for hashing messages of 4096 bytes, set MAX_SIZE_BITS to 13.
 */
#define MAX_SIZE_BITS 13

struct seqTimings {
  uint64_t size;
  uint64_t openssl_shani;
  uint64_t openssl_native;
  uint64_t shani;
};

struct parallelTimings {
  uint64_t size;
  uint64_t _1x;
  uint64_t _2x;
  uint64_t _4x;
  uint64_t _8x;
};

void print_tablePipelined(struct parallelTimings *table, int items) {
  int i;
  printf("                Cycles per byte \n");
  printf("╔═════════╦═════════╦═════════╦═════════╦═════════╗\n");
  printf("║  bytes  ║   1x    ║   2x    ║   4x    ║   8x    ║\n");
  printf("╠═════════╩═════════╩═════════╩═════════╩═════════╣\n");
  for (i = 0; i < items; i++) {
    printf("║%9ld║%9.2f║%9.2f║%9.2f║%9.2f║\n",
           table[i].size,
           table[i]._1x / (double) table[i].size / 1.0,
           table[i]._2x / (double) table[i].size / 2.0,
           table[i]._4x / (double) table[i].size / 4.0,
           table[i]._8x / (double) table[i].size / 8.0);
  }
  printf("╚═════════╩═════════╩═════════╩═════════╩═════════╝\n");
  printf("               Speedup  \n");
  printf("╔═════════╦═════════╦═════════╦═════════╦═════════╗\n");
  printf("║  bytes  ║   1x    ║   2x    ║   4x    ║   8x    ║\n");
  printf("╠═════════╩═════════╩═════════╩═════════╩═════════╣\n");
  for (i = 0; i < items; i++) {
    printf("║%9ld║%9.2f║%9.2f║%9.2f║%9.2f║\n",
           table[i].size,
           1.0 * table[i]._1x / (double) table[i]._1x,
           2.0 * table[i]._1x / (double) table[i]._2x,
           4.0 * table[i]._1x / (double) table[i]._4x,
           8.0 * table[i]._1x / (double) table[i]._8x);
  }
  printf("╚═════════╩═════════╩═════════╩═════════╩═════════╝\n");

  printf("                Savings \n");
  printf("╔═════════╦═════════╦═════════╦═════════╦═════════╗\n");
  printf("║  bytes  ║   1x    ║   2x    ║   4x    ║   8x    ║\n");
  printf("╠═════════╩═════════╩═════════╩═════════╩═════════╣\n");
  for (i = 0; i < items; i++) {
    printf("║%9ld║%8.2f%%║%8.2f%%║%8.2f%%║%8.2f%%║\n",
           table[i].size,
           100.0 * (1 - table[i]._1x / ((double) table[i]._1x * 1.0)),
           100.0 * (1 - table[i]._2x / ((double) table[i]._1x * 2.0)),
           100.0 * (1 - table[i]._4x / ((double) table[i]._1x * 4.0)),
           100.0 * (1 - table[i]._8x / ((double) table[i]._1x * 8.0)));
  }
  printf("╚═════════╩═════════╩═════════╩═════════╩═════════╝\n");
}


void print_tableVectorized(struct parallelTimings *table, int items) {
  int i;
  printf("Experiment: comparing OpenSSL vs SHANI\n");
  printf("            Cycles per byte \n");
  printf("╔═════════╦═════════╦═════════╦═════════╗\n");
  printf("║  bytes  ║   1x    ║   4x    ║   8x    ║\n");
  printf("╠═════════╩═════════╩═════════╩═════════╣\n");
  for (i = 0; i < items; i++) {
    printf("║%9ld║%9.2f║%9.2f║%9.2f║\n",
           table[i].size,
           table[i]._1x / (double) table[i].size / 1.0,
           table[i]._4x / (double) table[i].size / 4.0,
           table[i]._8x / (double) table[i].size / 8.0);
  }
  printf("╚═════════╩═════════╩═════════╩═════════╝\n");
  printf("                 Speedup  \n");
  printf("╔═════════╦═════════╦═════════╦═════════╗\n");
  printf("║  bytes  ║   1x    ║   4x    ║   8x    ║\n");
  printf("╠═════════╩═════════╩═════════╩═════════╣\n");
  for (i = 0; i < items; i++) {
    printf("║%9ld║%9.2f║%9.2f║%9.2f║\n",
           table[i].size,
           1.0 * table[i]._1x / (double) table[i]._1x,
           4.0 * table[i]._1x / (double) table[i]._4x,
           8.0 * table[i]._1x /(double) table[i]._8x);
  }
  printf("╚═════════╩═════════╩═════════╩═════════╝\n");

  printf("                 Savings \n");
  printf("╔═════════╦═════════╦═════════╦═════════╗\n");
  printf("║  bytes  ║   1x    ║   4x    ║   8x    ║\n");
  printf("╠═════════╩═════════╩═════════╩═════════╣\n");
  for (i = 0; i < items; i++) {
    printf("║%9ld║%8.2f%%║%8.2f%%║%8.2f%%║\n",
           table[i].size,
           100.0 * (1 - table[i]._1x / ((double) table[i]._1x * 1.0)),
           100.0 * (1 - table[i]._4x / ((double) table[i]._1x * 4.0)),
           100.0 * (1 - table[i]._8x / ((double) table[i]._1x * 8.0)));
  }
  printf("╚═════════╩═════════╩═════════╩═════════╝\n");
}

#define BENCH_SIZE_1W(FUNC, IMPL)      \
  do {                                 \
    unsigned long it = 0;              \
    long BENCH_TIMES = 0, CYCLES = 0;  \
    for(it=0;it<MAX_SIZE_BITS;it++) {  \
      int message_size = 1<<it;        \
      BENCH_TIMES = 512-it*20;         \
      CLOCKS(FUNC(message,message_size,digest));\
      table[it].size = message_size;   \
      table[it].IMPL = CYCLES;         \
    }                                  \
  }while(0)

#define BENCH_SIZE_NW(FUNC, N)                   \
do{                                              \
    long BENCH_TIMES = 0, CYCLES = 0;            \
    unsigned long it=0;                          \
    unsigned long MAX_SIZE = 1 << MAX_SIZE_BITS; \
    uint8_t *message[N];                         \
    uint8_t *digest[N];                          \
    for(it=0;it<N;it++) {                        \
        message[it] = (uint8_t*)_mm_malloc(MAX_SIZE,ALIGN_BYTES);  \
        digest[it] = (uint8_t*)_mm_malloc(32,ALIGN_BYTES);  \
        random_bytes(message[it],MAX_SIZE);  \
    }                                        \
    for(it=0;it<MAX_SIZE_BITS;it++) {        \
        int message_size = 1<<it;            \
        BENCH_TIMES = 512-it*20;             \
        CLOCKS(FUNC(message,message_size,digest));  \
        table[it].size = message_size;   \
        table[it]._ ## N ## x = CYCLES;  \
    }                                    \
    for(it=0;it<N;it++) {                \
        _mm_free(message[it]);           \
        _mm_free(digest[it]);            \
    }                                    \
}while(0)

void bench_Pipelined() {
  struct parallelTimings table[MAX_SIZE_BITS] = { {0,0,0,0,0} };
  unsigned char digest[32];
  unsigned long MAX_SIZE = 1 << MAX_SIZE_BITS;
  unsigned char *message = (unsigned char *) _mm_malloc(MAX_SIZE, ALIGN_BYTES);

  printf("\nExperiment: pipelined implementations of SHA-256.\n");
  if (hasSHANI()) {
    printf("Running 1x:\n");
    BENCH_SIZE_1W(sha256_update_shani, _1x);
    printf("Running 2x:\n");
    BENCH_SIZE_NW(sha256_x2_update_shani_2x, 2);
    printf("Running 4x:\n");
    BENCH_SIZE_NW(sha256_x4_update_shani_4x, 4);
    printf("Running 8x:\n");
    BENCH_SIZE_NW(sha256_x8_update_shani_8x, 8);
    print_tablePipelined(table, MAX_SIZE_BITS);
  } else {
    printf("Failed! This processor does not supports SHANI set.\n");
  }
}

void print_tableSeq(struct seqTimings *table, int items) {
  int i;
  printf("    Cycles per byte \n");
  printf("╔═════════╦═════════╦═════════╦═════════╦═════════╗\n");
  printf("║  Size   ║ OpenSSL ║ OpenSSL ║This work║ Speedup ║\n");
  printf("║ (bytes) ║  (x64)  ║ (shani) ║ (shani) ║x64/shani║\n");
  printf("╠═════════╩═════════╩═════════╩═════════╩═════════╣\n");
  for (i = 0; i < items; i++) {
    printf("║%9ld║%9.2f║%9.2f║%9.2f║%9.2f║\n", table[i].size,
           table[i].openssl_native / (double) table[i].size,
           table[i].openssl_shani / (double) table[i].size,
           table[i].shani / (double) table[i].size,
           table[i].openssl_native / (double) table[i].shani);
  }
  printf("╚═════════╩═════════╩═════════╩═════════╩═════════╝\n");
}

void print_tableOneSeq(struct seqTimings *table, int items) {
  int i;
  printf("   Cycles per byte \n");
  printf("╔═════════╦═════════╗\n");
  printf("║  Size   ║ OpenSSL ║\n");
  printf("║ (bytes) ║  (x64)  ║\n");
  printf("╠═════════╩═════════╣\n");
  for (i = 0; i < items; i++) {
    printf("║%9ld║%9.2f║\n", table[i].size,
           table[i].openssl_native / (double) table[i].size);
  }
  printf("╚═════════╩═════════╝\n");
}

void bench_OpenSSL_vs_SHANI() {
  struct seqTimings table[MAX_SIZE_BITS] = { {0,0,0,0} };;
  unsigned long MAX_SIZE = 1 << MAX_SIZE_BITS;
  unsigned char *message = (unsigned char *) _mm_malloc(MAX_SIZE, ALIGN_BYTES);
  unsigned char digest[32];

  printf("\nExperiment: comparing OpenSSL vs SHANI\n");
  if (hasSHANI()) {
    printf("Running OpenSSL (shani):\n");
    BENCH_SIZE_1W(SHA256, openssl_shani);

    disableSHANI();
    printf("Running OpenSSL (64-bit):\n");
    BENCH_SIZE_1W(SHA256, openssl_native);

    printf("Running shani:\n");
    BENCH_SIZE_1W(sha256_update_shani, shani);
    print_tableSeq(table, MAX_SIZE_BITS);
  } else {
    printf("Running OpenSSL (64-bit):\n");
    BENCH_SIZE_1W(SHA256, openssl_native);
    printf("Failed! This processor does not supports SHANI set.\n");
    printf("        Showing timings of OpenSSL only.\n");
    print_tableOneSeq(table, MAX_SIZE_BITS);
  }
  _mm_free(message);
}

void bench_Vectorized(){
  struct parallelTimings table[MAX_SIZE_BITS] = { {0,0,0,0,0} };
  unsigned long MAX_SIZE = 1 << MAX_SIZE_BITS;
  unsigned char *message = (unsigned char *) _mm_malloc(MAX_SIZE, ALIGN_BYTES);
  unsigned char digest[32];

  printf("\nExperiment: vectorized implementations of SHA-256.\n");
  disableSHANI();
  printf("Running OpenSSL (64-bit):\n");
  BENCH_SIZE_1W(SHA256, _1x);
  printf("Running 4x:\n");
  BENCH_SIZE_NW(sha256_4w, 4);
  printf("Running 8x:\n");
  BENCH_SIZE_NW(sha256_8w, 8);
  print_tableVectorized(table, MAX_SIZE_BITS);
}

int main(void) {
  machine_info();
  openssl_version();
  printf("== Start of Benchmark ===\n");
  bench_OpenSSL_vs_SHANI();
  bench_Vectorized();
  bench_Pipelined();
  printf("== End of Benchmark =====\n");
  return 0;
}
