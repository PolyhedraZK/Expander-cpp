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
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

/** Random number Generator:
 * Taken from: https://github.com/relic-toolkit/relic/src/rand/relic_rand_call.c
 *
 * @warning Provide a secure random number generator.
 * @param buffer
 * @param num_bytes
 */
void random_bytes(uint8_t *buffer, size_t num_bytes) {
  size_t l;
  int c, fd = open("/dev/urandom", O_RDONLY);

  if (fd == -1) {
    printf("Error opening /dev/urandom\n");
  }

  l = 0;
  do {
    c = read(fd, buffer + l, num_bytes - l);
    l += c;
    if (c == -1) {
      printf("Error reading /dev/urandom\n");
    }
  } while (l < num_bytes);

  close(fd);
}

void print_hex_bytes(uint8_t *A, size_t num_bytes) {
  size_t i;
  printf("0x");
  for (i = 0; i < num_bytes; i++) {
    printf("%02x", A[num_bytes - 1 - i]);
  }
  printf("\n");
}
