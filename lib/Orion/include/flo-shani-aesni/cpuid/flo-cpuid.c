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
#include <stdint.h>
#include <cpuid.h>
#include <openssl/objects.h>

#define my_cpuid(ra, rb, rc, rd)\
  __asm volatile (   \
    "mov %0, %%eax ;"\
    "mov %1, %%ebx ;"\
    "mov %2, %%ecx ;"\
    "mov %3, %%edx ;"\
    "cpuid         ;"\
    "mov %%eax, %0 ;"\
    "mov %%ebx, %1 ;"\
    "mov %%ecx, %2 ;"\
    "mov %%edx, %3 ;"\
  :"+r" (ra), "+r" (rb), "+r" (rc), "+r" (rd)\
  :  \
  : "%eax", "%ebx", "%ecx", "%edx")

#define test_capability(REG, CAP) \
 printf("%-12s: [%s]\n",#CAP,( (REG & CAP) != 0 )?"Yes":"No");

#define supports_capability(REG, CAP)  ( (REG & CAP) != 0 )

void machine_info() {
  unsigned int eax, ebx, ecx, edx;

  printf("=== Environment Information ====\n");
  printf("Program compiled with: %s\n", __VERSION__);

  eax = 1;
  ebx = 0;
  ecx = 0;
  edx = 0;
  my_cpuid(eax, ebx, ecx, edx);

  test_capability(edx, bit_CMOV);
  test_capability(edx, bit_SSE);
  test_capability(edx, bit_SSE2);
  test_capability(ecx, bit_SSE3);
  test_capability(ecx, bit_SSSE3);
  test_capability(ecx, bit_SSE4_1);
  test_capability(ecx, bit_SSE4_2);
  test_capability(ecx, bit_AVX);

  eax = 7;
  ebx = 0;
  ecx = 0;
  edx = 0;
  my_cpuid(eax, ebx, ecx, edx);
  test_capability(ebx, bit_AVX2);
  test_capability(ebx, bit_BMI);
  test_capability(ebx, bit_BMI2);
  test_capability(ebx, bit_ADX);
  test_capability(ebx, bit_SHA);
}

void openssl_version() {
  printf("OpenSSL version: %s\n", OPENSSL_VERSION_TEXT);
}

int hasSHANI() {
  unsigned int eax, ebx, ecx, edx;
  eax = 7;
  ebx = 0;
  ecx = 0;
  edx = 0;
  my_cpuid(eax, ebx, ecx, edx);
  return supports_capability(ebx, bit_SHA);
}

void disableSHANI() {
  if (OPENSSL_VERSION_NUMBER < 0x10002FFF) {
    extern unsigned long *OPENSSL_ia32cap_loc(void);
    uint64_t *c = OPENSSL_ia32cap_loc();
    c[1] &= ~0x20000000;
  }
}
