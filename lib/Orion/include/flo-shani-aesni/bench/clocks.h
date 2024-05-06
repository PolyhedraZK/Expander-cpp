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
#ifndef CLOCKS_H
#define CLOCKS_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __INTEL_COMPILER
#define BARRIER __memory_barrier()
#else
#define BARRIER __asm__ __volatile__("" ::: "memory")
#endif

#define CLOCKS_RANDOM(RANDOM, FUNCTION)    \
  do {                                            \
    uint64_t t_start, t_end;                      \
    int64_t i_bench, j_bench;                     \
    unsigned cycles_high0, cycles_low0;           \
    unsigned cycles_high1, cycles_low1;           \
    __asm__ __volatile__(                         \
        "mfence \n\t"                             \
        "RDTSC \n\t"                              \
        "mov %%edx, %0\n\t"                       \
        "mov %%eax, %1\n\t"                       \
        : "=r"(cycles_high0), "=r"(cycles_low0)   \
        ::"%rax", "%rbx", "%rcx","%rdx");         \
    BARRIER;                                      \
    i_bench = BENCH_TIMES;                        \
    do {                                          \
      j_bench = BENCH_TIMES;                      \
      RANDOM;                                     \
      do {                                        \
        FUNCTION;                                 \
        j_bench--;                                \
      } while (j_bench != 0);                     \
      i_bench--;                                  \
    } while (i_bench != 0);                       \
    BARRIER;                                      \
    __asm__ __volatile__(                         \
        "mfence \n\t"                             \
        "RDTSCP \n\t"                             \
        "mov %%edx, %0\n\t"                       \
        "mov %%eax, %1\n\t"                       \
        : "=r"(cycles_high1), "=r"(cycles_low1)   \
        ::"%rax", "%rbx", "%rcx", "%rdx");        \
    t_start = (((uint64_t)cycles_high0) << 32) | cycles_low0; \
    t_end   = (((uint64_t)cycles_high1) << 32) | cycles_low1; \
    CYCLES = (t_end - t_start) / (BENCH_TIMES * BENCH_TIMES); \
  } while (0)

#define CLOCKS(FUNCTION) CLOCKS_RANDOM(while (0), FUNCTION)

#endif /* CLOCKS_H */
