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
#include "flo-shani.h"

#define LOAD(X)       _mm_load_si128((__m128i const *)X)
#define LOAD_U(X)     _mm_loadu_si128((__m128i const *)X)
#define STORE(X, Y)   _mm_store_si128((__m128i *)X,Y)
#define ALIGNR(X, Y)  _mm_alignr_epi8(X,Y,4)
#define ADD(X, Y)     _mm_add_epi32(X,Y)
//#define HIGH(X)     _mm_srli_si128(X,8)
#define HIGH(X)       _mm_shuffle_epi32(X, 0x0E)
#define SHA(X, Y, Z)  _mm_sha256rnds2_epu32(X,Y,Z)
#define MSG1(X, Y)    _mm_sha256msg1_epu32(X,Y)
#define MSG2(X, Y)    _mm_sha256msg2_epu32(X,Y)
#define CVLO(X, Y, Z) _mm_shuffle_epi32(_mm_unpacklo_epi64(X,Y),Z)
#define CVHI(X, Y, Z) _mm_shuffle_epi32(_mm_unpackhi_epi64(X,Y),Z)
#define L2B(X)        _mm_shuffle_epi8(X,MASK)

const ALIGN uint32_t CONST_K[64] = {
    0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5,
    0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
    0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3,
    0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
    0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC,
    0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
    0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7,
    0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
    0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13,
    0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
    0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3,
    0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
    0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5,
    0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
    0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208,
    0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2
};

static void update_shani(uint32_t *state, const uint8_t *msg, uint32_t num_blocks) {
  const __m128i MASK = _mm_set_epi32(0x0c0d0e0f, 0x08090a0b, 0x04050607, 0x00010203);
  int i, j;
  __m128i A0, C0, ABEF, CDGH, X0, Y0, Ki, W0[4];

  X0 = LOAD(state + 0);
  Y0 = LOAD(state + 1);

  A0 = CVLO(X0, Y0, 0x1B);
  C0 = CVHI(X0, Y0, 0x1B);

  while (num_blocks > 0) {
    ABEF = A0;
    CDGH = C0;
    for (i = 0; i < 4; i++) {
      Ki = LOAD(CONST_K + i);
      W0[i] = L2B(LOAD_U(msg + i));
      X0 = ADD(W0[i], Ki);
      Y0 = HIGH(X0);
      C0 = SHA(C0, A0, X0);
      A0 = SHA(A0, C0, Y0);
    }
    for (j = 1; j < 4; j++) {
      for (i = 0; i < 4; i++) {
        Ki = LOAD(CONST_K + 4 * j + i);
        X0 = MSG1(W0[i], W0[(i + 1) % 4]);
        Y0 = ALIGNR(W0[(i + 3) % 4], W0[(i + 2) % 4]);
        X0 = ADD(X0, Y0);
        W0[i] = MSG2(X0, W0[(i + 3) % 4]);
        X0 = ADD(W0[i], Ki);
        Y0 = HIGH(X0);
        C0 = SHA(C0, A0, X0);
        A0 = SHA(A0, C0, Y0);
      }
    }
    A0 = ADD(A0, ABEF);
    C0 = ADD(C0, CDGH);
    msg += 64;
    num_blocks--;
  }

  X0 = CVHI(A0, C0, 0xB1);
  Y0 = CVLO(A0, C0, 0xB1);

  STORE(state + 0, X0);
  STORE(state + 1, Y0);
}

static void update_shani_2x(
    uint32_t *state0,  const uint8_t *msg0,
    uint32_t *state1,  const uint8_t *msg1,
    uint32_t num_blocks) {
  const __m128i MASK = _mm_set_epi32(0x0c0d0e0f, 0x08090a0b, 0x04050607, 0x00010203);
  int i, j;
  __m128i Ki;
  __m128i A0, C0, ABEF0, CDGH0, X0, Y0, W0[4];
  __m128i A1, C1, ABEF1, CDGH1, X1, Y1, W1[4];

  X0 = LOAD(state0 + 0);
  X1 = LOAD(state1 + 0);
  Y0 = LOAD(state0 + 1);
  Y1 = LOAD(state1 + 1);
  A0 = CVLO(X0, Y0, 0x1B);
  A1 = CVLO(X1, Y1, 0x1B);
  C0 = CVHI(X0, Y0, 0x1B);
  C1 = CVHI(X1, Y1, 0x1B);

  while (num_blocks > 0) {
    ABEF0 = A0;
    ABEF1 = A1;
    CDGH0 = C0;
    CDGH1 = C1;

    for (i = 0; i < 4; i++) {
      Ki = LOAD(CONST_K + i);
      W0[i] = L2B(LOAD_U(msg0 + i));
      W1[i] = L2B(LOAD_U(msg1 + i));
      X0 = ADD(W0[i], Ki);
      X1 = ADD(W1[i], Ki);
      Y0 = HIGH(X0);
      Y1 = HIGH(X1);
      C0 = SHA(C0, A0, X0);
      C1 = SHA(C1, A1, X1);
      A0 = SHA(A0, C0, Y0);
      A1 = SHA(A1, C1, Y1);
    }
    for (j = 1; j < 4; j++) {
      for (i = 0; i < 4; i++) {
        Ki = LOAD(CONST_K + 4 * j + i);
        X0 = MSG1(W0[i], W0[(i+1)%4]);
        X1 = MSG1(W1[i], W1[(i+1)%4]);
        Y0 = ALIGNR(W0[(i+3)%4], W0[(i+2)%4]);
        Y1 = ALIGNR(W1[(i+3)%4], W1[(i+2)%4]);
        X0 = ADD(X0, Y0);
        X1 = ADD(X1, Y1);
        W0[i] = MSG2(X0, W0[(i+3)%4]);
        W1[i] = MSG2(X1, W1[(i+3)%4]);
        X0 = ADD(W0[i], Ki);
        X1 = ADD(W1[i], Ki);
        Y0 = HIGH(X0);
        Y1 = HIGH(X1);
        C0 = SHA(C0, A0, X0);
        C1 = SHA(C1, A1, X1);
        A0 = SHA(A0, C0, Y0);
        A1 = SHA(A1, C1, Y1);
      }
    }

    A0 = ADD(A0, ABEF0);
    A1 = ADD(A1, ABEF1);
    C0 = ADD(C0, CDGH0);
    C1 = ADD(C1, CDGH1);
    msg0 += 64;
    msg1 += 64;
    num_blocks--;
  }

  X0 = CVHI(A0, C0, 0xB1);
  X1 = CVHI(A1, C1, 0xB1);
  Y0 = CVLO(A0, C0, 0xB1);
  Y1 = CVLO(A1, C1, 0xB1);

  STORE(state0 + 0, X0);
  STORE(state1 + 0, X1);
  STORE(state0 + 1, Y0);
  STORE(state1 + 1, Y1);
}

static void update_shani_4x(
    uint32_t *state0,  const uint8_t *msg0,
    uint32_t *state1,  const uint8_t *msg1,
    uint32_t *state2,  const uint8_t *msg2,
    uint32_t *state3,  const uint8_t *msg3,
    uint32_t num_blocks) {
  const __m128i MASK = _mm_set_epi32(0x0c0d0e0f, 0x08090a0b, 0x04050607, 0x00010203);
  uint8_t i, j;
  __m128i Ki;
  __m128i A0, C0, ABEF0, CDGH0, X0, Y0, W0[4];
  __m128i A1, C1, ABEF1, CDGH1, X1, Y1, W1[4];
  __m128i A2, C2, ABEF2, CDGH2, X2, Y2, W2[4];
  __m128i A3, C3, ABEF3, CDGH3, X3, Y3, W3[4];

  X0 = LOAD(state0 + 0);
  Y0 = LOAD(state0 + 1);
  X1 = LOAD(state1 + 0);
  Y1 = LOAD(state1 + 1);
  X2 = LOAD(state2 + 0);
  Y2 = LOAD(state2 + 1);
  X3 = LOAD(state3 + 0);
  Y3 = LOAD(state3 + 1);

  A0 = CVLO(X0, Y0, 0x1B);
  C0 = CVHI(X0, Y0, 0x1B);
  A1 = CVLO(X1, Y1, 0x1B);
  C1 = CVHI(X1, Y1, 0x1B);
  A2 = CVLO(X2, Y2, 0x1B);
  C2 = CVHI(X2, Y2, 0x1B);
  A3 = CVLO(X3, Y3, 0x1B);
  C3 = CVHI(X3, Y3, 0x1B);

  while (num_blocks > 0) {
    ABEF0 = A0;
    CDGH0 = C0;
    ABEF1 = A1;
    CDGH1 = C1;
    ABEF2 = A2;
    CDGH2 = C2;
    ABEF3 = A3;
    CDGH3 = C3;

    for (i = 0; i < 4; i++) {
      Ki = LOAD(CONST_K + i);
      W0[i] = LOAD_U(msg0 + i);
      W1[i] = LOAD_U(msg1 + i);
      W2[i] = LOAD_U(msg2 + i);
      W3[i] = LOAD_U(msg3 + i);
      W0[i] = L2B(W0[i]);
      W1[i] = L2B(W1[i]);
      W2[i] = L2B(W2[i]);
      W3[i] = L2B(W3[i]);
      X0 = ADD(W0[i], Ki);
      X1 = ADD(W1[i], Ki);
      X2 = ADD(W2[i], Ki);
      X3 = ADD(W3[i], Ki);
      Y0 = HIGH(X0);
      Y1 = HIGH(X1);
      Y2 = HIGH(X2);
      Y3 = HIGH(X3);
      C0 = SHA(C0, A0, X0);
      C1 = SHA(C1, A1, X1);
      C2 = SHA(C2, A2, X2);
      C3 = SHA(C3, A3, X3);
      A0 = SHA(A0, C0, Y0);
      A1 = SHA(A1, C1, Y1);
      A2 = SHA(A2, C2, Y2);
      A3 = SHA(A3, C3, Y3);
    }

    for (j = 1; j < 4; j++) {
      for (i = 0; i < 4; i++) {
        Ki = LOAD(CONST_K + 4 * j + i);
        X0 = MSG1(W0[i], W0[(i + 1) % 4]);
        X1 = MSG1(W1[i], W1[(i + 1) % 4]);
        X2 = MSG1(W2[i], W2[(i + 1) % 4]);
        X3 = MSG1(W3[i], W3[(i + 1) % 4]);
        Y0 = ALIGNR(W0[(i + 3) % 4], W0[(i + 2) % 4]);
        Y1 = ALIGNR(W1[(i + 3) % 4], W1[(i + 2) % 4]);
        Y2 = ALIGNR(W2[(i + 3) % 4], W2[(i + 2) % 4]);
        Y3 = ALIGNR(W3[(i + 3) % 4], W3[(i + 2) % 4]);
        X0 = ADD(X0, Y0);
        X1 = ADD(X1, Y1);
        X2 = ADD(X2, Y2);
        X3 = ADD(X3, Y3);
        W0[i] = MSG2(X0, W0[(i + 3) % 4]);
        W1[i] = MSG2(X1, W1[(i + 3) % 4]);
        W2[i] = MSG2(X2, W2[(i + 3) % 4]);
        W3[i] = MSG2(X3, W3[(i + 3) % 4]);
        X0 = ADD(W0[i], Ki);
        X1 = ADD(W1[i], Ki);
        X2 = ADD(W2[i], Ki);
        X3 = ADD(W3[i], Ki);
        C0 = SHA(C0, A0, X0);
        C1 = SHA(C1, A1, X1);
        C2 = SHA(C2, A2, X2);
        C3 = SHA(C3, A3, X3);
        Y0 = HIGH(X0);
        Y1 = HIGH(X1);
        Y2 = HIGH(X2);
        Y3 = HIGH(X3);
        A0 = SHA(A0, C0, Y0);
        A1 = SHA(A1, C1, Y1);
        A2 = SHA(A2, C2, Y2);
        A3 = SHA(A3, C3, Y3);
      }
    }

    A0 = ADD(A0, ABEF0);
    A1 = ADD(A1, ABEF1);
    A2 = ADD(A2, ABEF2);
    A3 = ADD(A3, ABEF3);
    C0 = ADD(C0, CDGH0);
    C1 = ADD(C1, CDGH1);
    C2 = ADD(C2, CDGH2);
    C3 = ADD(C3, CDGH3);

    msg0 += 64;
    msg1 += 64;
    msg2 += 64;
    msg3 += 64;
    num_blocks--;
  }
  X0 = CVHI(A0, C0, 0xB1);
  Y0 = CVLO(A0, C0, 0xB1);
  X1 = CVHI(A1, C1, 0xB1);
  Y1 = CVLO(A1, C1, 0xB1);
  X2 = CVHI(A2, C2, 0xB1);
  Y2 = CVLO(A2, C2, 0xB1);
  X3 = CVHI(A3, C3, 0xB1);
  Y3 = CVLO(A3, C3, 0xB1);

  STORE(state0 + 0, X0);
  STORE(state0 + 1, Y0);
  STORE(state1 + 0, X1);
  STORE(state1 + 1, Y1);
  STORE(state2 + 0, X2);
  STORE(state2 + 1, Y2);
  STORE(state3 + 0, X3);
  STORE(state3 + 1, Y3);
}

static void update_shani_8x(
    uint32_t *state0, const uint8_t *msg0,
    uint32_t *state1, const uint8_t *msg1,
    uint32_t *state2, const uint8_t *msg2,
    uint32_t *state3, const uint8_t *msg3,
    uint32_t *state4, const uint8_t *msg4,
    uint32_t *state5, const uint8_t *msg5,
    uint32_t *state6, const uint8_t *msg6,
    uint32_t *state7, const uint8_t *msg7,
    uint32_t num_blocks) {
  const __m128i MASK = _mm_set_epi32(0x0c0d0e0f, 0x08090a0b, 0x04050607, 0x00010203);
  int i, j, i1, i2, i3;
  __m128i Ki;
  __m128i A0, C0, ABEF0, CDGH0, X0, Y0, W0[4];
  __m128i A1, C1, ABEF1, CDGH1, X1, Y1, W1[4];
  __m128i A2, C2, ABEF2, CDGH2, X2, Y2, W2[4];
  __m128i A3, C3, ABEF3, CDGH3, X3, Y3, W3[4];
  __m128i A4, C4, ABEF4, CDGH4, X4, Y4, W4[4];
  __m128i A5, C5, ABEF5, CDGH5, X5, Y5, W5[4];
  __m128i A6, C6, ABEF6, CDGH6, X6, Y6, W6[4];
  __m128i A7, C7, ABEF7, CDGH7, X7, Y7, W7[4];

  X0 = LOAD(state0 + 0);    X1 = LOAD(state1 + 0);    X2 = LOAD(state2 + 0);    X3 = LOAD(state3 + 0);    X4 = LOAD(state4 + 0);    X5 = LOAD(state5 + 0);    X6 = LOAD(state6 + 0);    X7 = LOAD(state7 + 0);
  Y0 = LOAD(state0 + 1);    Y1 = LOAD(state1 + 1);    Y2 = LOAD(state2 + 1);    Y3 = LOAD(state3 + 1);    Y4 = LOAD(state4 + 1);    Y5 = LOAD(state5 + 1);    Y6 = LOAD(state6 + 1);    Y7 = LOAD(state7 + 1);
  A0 = CVLO(X0, Y0, 0x1B);  A1 = CVLO(X1, Y1, 0x1B);  A2 = CVLO(X2, Y2, 0x1B);  A3 = CVLO(X3, Y3, 0x1B);  A4 = CVLO(X4, Y4, 0x1B);  A5 = CVLO(X5, Y5, 0x1B);  A6 = CVLO(X6, Y6, 0x1B);  A7 = CVLO(X7, Y7, 0x1B);
  C0 = CVHI(X0, Y0, 0x1B);  C1 = CVHI(X1, Y1, 0x1B);  C2 = CVHI(X2, Y2, 0x1B);  C3 = CVHI(X3, Y3, 0x1B);  C4 = CVHI(X4, Y4, 0x1B);  C5 = CVHI(X5, Y5, 0x1B);  C6 = CVHI(X6, Y6, 0x1B);  C7 = CVHI(X7, Y7, 0x1B);

  while (num_blocks > 0) {
    ABEF0 = A0;  ABEF1 = A1;  ABEF2 = A2;  ABEF3 = A3;  ABEF4 = A4;  ABEF5 = A5;  ABEF6 = A6;  ABEF7 = A7;
    CDGH0 = C0;  CDGH1 = C1;  CDGH2 = C2;  CDGH3 = C3;  CDGH4 = C4;  CDGH5 = C5;  CDGH6 = C6;  CDGH7 = C7;

    for (i = 0; i < 4; i++) {
      Ki = LOAD(CONST_K + i);
      W0[i] = L2B(LOAD_U(msg0+i));  W1[i] = L2B(LOAD_U(msg1+i));  W2[i] = L2B(LOAD_U(msg2+i));  W3[i] = L2B(LOAD_U(msg3+i));  W4[i] = L2B(LOAD_U(msg4+i));  W5[i] = L2B(LOAD_U(msg5+i));  W6[i] = L2B(LOAD_U(msg6+i));  W7[i] = L2B(LOAD_U(msg7+i));
      X0 = ADD(W0[i], Ki);          X1 = ADD(W1[i], Ki);          X2 = ADD(W2[i], Ki);          X3 = ADD(W3[i], Ki);          X4 = ADD(W4[i], Ki);          X5 = ADD(W5[i], Ki);          X6 = ADD(W6[i], Ki);          X7 = ADD(W7[i], Ki);
      Y0 = HIGH(X0);                Y1 = HIGH(X1);                Y2 = HIGH(X2);                Y3 = HIGH(X3);                Y4 = HIGH(X4);                Y5 = HIGH(X5);                Y6 = HIGH(X6);                Y7 = HIGH(X7);
      C0 = SHA(C0, A0, X0);         C1 = SHA(C1, A1, X1);         C2 = SHA(C2, A2, X2);         C3 = SHA(C3, A3, X3);         C4 = SHA(C4, A4, X4);         C5 = SHA(C5, A5, X5);         C6 = SHA(C6, A6, X6);         C7 = SHA(C7, A7, X7);
      A0 = SHA(A0, C0, Y0);         A1 = SHA(A1, C1, Y1);         A2 = SHA(A2, C2, Y2);         A3 = SHA(A3, C3, Y3);         A4 = SHA(A4, C4, Y4);         A5 = SHA(A5, C5, Y5);         A6 = SHA(A6, C6, Y6);         A7 = SHA(A7, C7, Y7);
    }
    for (j = 1; j < 4; j++) {
      for (i = 0, i1 = 1, i2 = 2, i3 = 3; i < 4; i++) {
        Ki = LOAD(CONST_K + 4 * j + i);
        X0 = MSG1(W0[i], W0[i1]);     X1 = MSG1(W1[i], W1[i1]);     X2 = MSG1(W2[i], W2[i1]);     X3 = MSG1(W3[i], W3[i1]);     X4 = MSG1(W4[i], W4[i1]);     X5 = MSG1(W5[i], W5[i1]);     X6 = MSG1(W6[i], W6[i1]);     X7 = MSG1(W7[i], W7[i1]);
        Y0 = ALIGNR(W0[i3], W0[i2]);  Y1 = ALIGNR(W1[i3], W1[i2]);  Y2 = ALIGNR(W2[i3], W2[i2]);  Y3 = ALIGNR(W3[i3], W3[i2]);  Y4 = ALIGNR(W4[i3], W4[i2]);  Y5 = ALIGNR(W5[i3], W5[i2]);  Y6 = ALIGNR(W6[i3], W6[i2]);  Y7 = ALIGNR(W7[i3], W7[i2]);
        X0 = ADD(X0, Y0);             X1 = ADD(X1, Y1);             X2 = ADD(X2, Y2);             X3 = ADD(X3, Y3);             X4 = ADD(X4, Y4);             X5 = ADD(X5, Y5);             X6 = ADD(X6, Y6);             X7 = ADD(X7, Y7);
        W0[i] = MSG2(X0, W0[i3]);     W1[i] = MSG2(X1, W1[i3]);     W2[i] = MSG2(X2, W2[i3]);     W3[i] = MSG2(X3, W3[i3]);     W4[i] = MSG2(X4, W4[i3]);     W5[i] = MSG2(X5, W5[i3]);     W6[i] = MSG2(X6, W6[i3]);     W7[i] = MSG2(X7, W7[i3]);
        X0 = ADD(W0[i], Ki);          X1 = ADD(W1[i], Ki);          X2 = ADD(W2[i], Ki);          X3 = ADD(W3[i], Ki);          X4 = ADD(W4[i], Ki);          X5 = ADD(W5[i], Ki);          X6 = ADD(W6[i], Ki);          X7 = ADD(W7[i], Ki);
        Y0 = HIGH(X0);                Y1 = HIGH(X1);                Y2 = HIGH(X2);                Y3 = HIGH(X3);                Y4 = HIGH(X4);                Y5 = HIGH(X5);                Y6 = HIGH(X6);                Y7 = HIGH(X7);
        C0 = SHA(C0, A0, X0);         C1 = SHA(C1, A1, X1);         C2 = SHA(C2, A2, X2);         C3 = SHA(C3, A3, X3);         C4 = SHA(C4, A4, X4);         C5 = SHA(C5, A5, X5);         C6 = SHA(C6, A6, X6);         C7 = SHA(C7, A7, X7);
        A0 = SHA(A0, C0, Y0);         A1 = SHA(A1, C1, Y1);         A2 = SHA(A2, C2, Y2);         A3 = SHA(A3, C3, Y3);         A4 = SHA(A4, C4, Y4);         A5 = SHA(A5, C5, Y5);         A6 = SHA(A6, C6, Y6);         A7 = SHA(A7, C7, Y7);
        i1 = i2;
        i2 = i3;
        i3 = i;
      }
    }

    A0 = ADD(A0, ABEF0);  A1 = ADD(A1, ABEF1);  A2 = ADD(A2, ABEF2);  A3 = ADD(A3, ABEF3);  A4 = ADD(A4, ABEF4);  A5 = ADD(A5, ABEF5);  A6 = ADD(A6, ABEF6);  A7 = ADD(A7, ABEF7);
    C0 = ADD(C0, CDGH0);  C1 = ADD(C1, CDGH1);  C2 = ADD(C2, CDGH2);  C3 = ADD(C3, CDGH3);  C4 = ADD(C4, CDGH4);  C5 = ADD(C5, CDGH5);  C6 = ADD(C6, CDGH6);  C7 = ADD(C7, CDGH7);
    msg0 += 64;           msg1 += 64;           msg2 += 64;           msg3 += 64;           msg4 += 64;           msg5 += 64;           msg6 += 64;           msg7 += 64;
    num_blocks--;
  }

  X0 = CVHI(A0, C0, 0xB1);  X1 = CVHI(A1, C1, 0xB1);  X2 = CVHI(A2, C2, 0xB1);  X3 = CVHI(A3, C3, 0xB1);  X4 = CVHI(A4, C4, 0xB1);  X5 = CVHI(A5, C5, 0xB1);  X6 = CVHI(A6, C6, 0xB1);  X7 = CVHI(A7, C7, 0xB1);
  Y0 = CVLO(A0, C0, 0xB1);  Y1 = CVLO(A1, C1, 0xB1);  Y2 = CVLO(A2, C2, 0xB1);  Y3 = CVLO(A3, C3, 0xB1);  Y4 = CVLO(A4, C4, 0xB1);  Y5 = CVLO(A5, C5, 0xB1);  Y6 = CVLO(A6, C6, 0xB1);  Y7 = CVLO(A7, C7, 0xB1);

  STORE(state0 + 0, X0);  STORE(state1 + 0, X1);  STORE(state2 + 0, X2);  STORE(state3 + 0, X3);  STORE(state4 + 0, X4);  STORE(state5 + 0, X5);  STORE(state6 + 0, X6);  STORE(state7 + 0, X7);
  STORE(state0 + 1, Y0);  STORE(state1 + 1, Y1);  STORE(state2 + 1, Y2);  STORE(state3 + 1, Y3);  STORE(state4 + 1, Y4);  STORE(state5 + 1, Y5);  STORE(state6 + 1, Y6);  STORE(state7 + 1, Y7);
}

#define SHA_CORE(CORE)\
unsigned char * sha256##_##CORE(const unsigned char *message, long unsigned int message_length,unsigned char *digest)\
{\
    uint32_t i=0;\
    uint32_t num_blocks = message_length/64;\
    uint32_t rem_bytes = message_length%64;\
    ALIGN uint8_t pad[128];\
    ALIGN uint32_t state[8];\
\
    /** Initializing state **/\
    state[0] = 0x6a09e667;\
    state[1] = 0xbb67ae85;\
    state[2] = 0x3c6ef372;\
    state[3] = 0xa54ff53a;\
    state[4] = 0x510e527f;\
    state[5] = 0x9b05688c;\
    state[6] = 0x1f83d9ab;\
    state[7] = 0x5be0cd19;\
    \
    CORE(state,message,num_blocks);\
\
    /** Padding message **/\
    for(i=0;i<rem_bytes;i++)\
    {\
        pad[i] = message[64*num_blocks+i];\
    }\
    pad[rem_bytes] = 0x80;\
    if (rem_bytes < 56)\
    {\
        for (i = rem_bytes + 1; i < 56; i++)\
        {\
            pad[i] = 0x0;\
        }\
        ((uint64_t*)pad)[7] = (uint64_t)__builtin_bswap64(message_length*8);\
        CORE(state,pad,1);\
    }\
    else\
    {\
        for (i = rem_bytes + 1; i < 120; i++)\
        {\
            pad[i] = 0x0;\
        }\
        ((uint64_t*)pad)[15] = (uint64_t)__builtin_bswap64(message_length*8);\
        CORE(state,pad,2);\
    }\
\
    for(i=0;i<SHA256_HASH_SIZE/4;i++)\
    {\
        ((uint32_t*)digest)[i] = (uint32_t)__builtin_bswap32(state[i]);\
    }\
\
    return digest;\
}

#define SHA_x2_CORE(CORE_X2)\
void sha256_x2##_##CORE_X2(\
unsigned char *message[2],\
long unsigned int message_length,\
unsigned char *digest[2])\
{\
    uint32_t i=0;\
    uint32_t num_blocks = message_length/64;\
    uint32_t rem_bytes = message_length%64;\
    ALIGN uint8_t pad0[128];\
    ALIGN uint8_t pad1[128];\
    ALIGN uint32_t state0[8];\
    ALIGN uint32_t state1[8];\
\
    /** Initializing state **/\
    state0[0] = 0x6a09e667;  state1[0] = 0x6a09e667;\
    state0[1] = 0xbb67ae85;  state1[1] = 0xbb67ae85;\
    state0[2] = 0x3c6ef372;  state1[2] = 0x3c6ef372;\
    state0[3] = 0xa54ff53a;  state1[3] = 0xa54ff53a;\
    state0[4] = 0x510e527f;  state1[4] = 0x510e527f;\
    state0[5] = 0x9b05688c;  state1[5] = 0x9b05688c;\
    state0[6] = 0x1f83d9ab;  state1[6] = 0x1f83d9ab;\
    state0[7] = 0x5be0cd19;  state1[7] = 0x5be0cd19;\
    \
    CORE_X2(state0,message[0],state1,message[1],num_blocks);\
\
    /** Padding message **/\
    for(i=0;i<rem_bytes;i++)\
    {\
        pad0[i] = message[0][64*num_blocks+i];\
        pad1[i] = message[1][64*num_blocks+i];\
    }\
    pad0[rem_bytes] = 0x80;\
    pad1[rem_bytes] = 0x80;\
    if (rem_bytes < 56)\
    {\
        for (i = rem_bytes + 1; i < 56; i++)\
        {\
            pad0[i] = 0x0;\
            pad1[i] = 0x0;\
        }\
        ((uint64_t*)pad0)[7] = (uint64_t)__builtin_bswap64(message_length*8);\
        ((uint64_t*)pad1)[7] = (uint64_t)__builtin_bswap64(message_length*8);\
        CORE_X2(state0,pad0,state1,pad1,1);\
    }\
    else\
    {\
        for (i = rem_bytes + 1; i < 120; i++)\
        {\
            pad0[i] = 0x0;\
            pad1[i] = 0x0;\
        }\
        ((uint64_t*)pad0)[15] = (uint64_t)__builtin_bswap64(message_length*8);\
        ((uint64_t*)pad1)[15] = (uint64_t)__builtin_bswap64(message_length*8);\
        CORE_X2(state0,pad0,state1,pad1,2);\
    }\
\
    for(i=0;i<SHA256_HASH_SIZE/4;i++)\
    {\
        ((uint32_t*)(digest[0]) )[i] = (uint32_t)__builtin_bswap32(state0[i]);\
        ((uint32_t*)(digest[1]) )[i] = (uint32_t)__builtin_bswap32(state1[i]);\
    }\
\
}

#define SHA_x4_CORE(CORE_X4)\
void sha256_x4##_##CORE_X4(\
unsigned char *message[4],\
long unsigned int message_length,\
unsigned char *digest[4])\
{\
    uint32_t i=0;\
    uint32_t num_blocks = message_length/64;\
    uint32_t rem_bytes = message_length%64;\
    ALIGN uint8_t pad0[128];\
    ALIGN uint8_t pad1[128];\
    ALIGN uint8_t pad2[128];\
    ALIGN uint8_t pad3[128];\
    ALIGN uint32_t state0[8];\
    ALIGN uint32_t state1[8];\
    ALIGN uint32_t state2[8];\
    ALIGN uint32_t state3[8];\
\
    /** Initializing state **/\
    state0[0] = 0x6a09e667;  state1[0] = 0x6a09e667;   state2[0] = 0x6a09e667;  state3[0] = 0x6a09e667;\
    state0[1] = 0xbb67ae85;  state1[1] = 0xbb67ae85;   state2[1] = 0xbb67ae85;  state3[1] = 0xbb67ae85;\
    state0[2] = 0x3c6ef372;  state1[2] = 0x3c6ef372;   state2[2] = 0x3c6ef372;  state3[2] = 0x3c6ef372;\
    state0[3] = 0xa54ff53a;  state1[3] = 0xa54ff53a;   state2[3] = 0xa54ff53a;  state3[3] = 0xa54ff53a;\
    state0[4] = 0x510e527f;  state1[4] = 0x510e527f;   state2[4] = 0x510e527f;  state3[4] = 0x510e527f;\
    state0[5] = 0x9b05688c;  state1[5] = 0x9b05688c;   state2[5] = 0x9b05688c;  state3[5] = 0x9b05688c;\
    state0[6] = 0x1f83d9ab;  state1[6] = 0x1f83d9ab;   state2[6] = 0x1f83d9ab;  state3[6] = 0x1f83d9ab;\
    state0[7] = 0x5be0cd19;  state1[7] = 0x5be0cd19;   state2[7] = 0x5be0cd19;  state3[7] = 0x5be0cd19;\
    \
    CORE_X4(state0,message[0],state1,message[1],state2,message[2],state3,message[3],num_blocks);\
\
    /** Padding message **/\
    for(i=0;i<rem_bytes;i++)\
    {\
        pad0[i] = message[0][64*num_blocks+i];\
        pad1[i] = message[1][64*num_blocks+i];\
        pad2[i] = message[2][64*num_blocks+i];\
        pad3[i] = message[3][64*num_blocks+i];\
    }\
    pad0[rem_bytes] = 0x80;\
    pad1[rem_bytes] = 0x80;\
    pad2[rem_bytes] = 0x80;\
    pad3[rem_bytes] = 0x80;\
    if (rem_bytes < 56)\
    {\
        for (i = rem_bytes + 1; i < 56; i++)\
        {\
            pad0[i] = 0x0;\
            pad1[i] = 0x0;\
            pad2[i] = 0x0;\
            pad3[i] = 0x0;\
        }\
        ((uint64_t*)pad0)[7] = (uint64_t)__builtin_bswap64(message_length*8);\
        ((uint64_t*)pad1)[7] = (uint64_t)__builtin_bswap64(message_length*8);\
        ((uint64_t*)pad2)[7] = (uint64_t)__builtin_bswap64(message_length*8);\
        ((uint64_t*)pad3)[7] = (uint64_t)__builtin_bswap64(message_length*8);\
        CORE_X4(state0,pad0,state1,pad1,state2,pad2,state3,pad3,1);\
    }\
    else\
    {\
        for (i = rem_bytes + 1; i < 120; i++)\
        {\
            pad0[i] = 0x0;\
            pad1[i] = 0x0;\
            pad2[i] = 0x0;\
            pad3[i] = 0x0;\
        }\
        ((uint64_t*)pad0)[15] = (uint64_t)__builtin_bswap64(message_length*8);\
        ((uint64_t*)pad1)[15] = (uint64_t)__builtin_bswap64(message_length*8);\
        ((uint64_t*)pad2)[15] = (uint64_t)__builtin_bswap64(message_length*8);\
        ((uint64_t*)pad3)[15] = (uint64_t)__builtin_bswap64(message_length*8);\
        CORE_X4(state0,pad0,state1,pad1,state2,pad2,state3,pad3,2);\
    }\
\
    for(i=0;i<SHA256_HASH_SIZE/4;i++)\
    {\
        ((uint32_t*)(digest[0]))[i] = (uint32_t)__builtin_bswap32(state0[i]);\
        ((uint32_t*)(digest[1]))[i] = (uint32_t)__builtin_bswap32(state1[i]);\
        ((uint32_t*)(digest[2]))[i] = (uint32_t)__builtin_bswap32(state2[i]);\
        ((uint32_t*)(digest[3]))[i] = (uint32_t)__builtin_bswap32(state3[i]);\
    }\
\
}

#define SHA_x8_CORE(CORE_X8)\
void sha256_x8##_##CORE_X8(\
unsigned char *message[8],\
long unsigned int message_length,\
unsigned char *digest[8])\
{\
    uint32_t i=0;\
    uint32_t num_blocks = message_length/64;\
    uint32_t rem_bytes = message_length%64;\
    ALIGN uint8_t pad0[128];\
    ALIGN uint8_t pad1[128];\
    ALIGN uint8_t pad2[128];\
    ALIGN uint8_t pad3[128];\
    ALIGN uint8_t pad4[128];\
    ALIGN uint8_t pad5[128];\
    ALIGN uint8_t pad6[128];\
    ALIGN uint8_t pad7[128];\
    ALIGN uint32_t state0[8];\
    ALIGN uint32_t state1[8];\
    ALIGN uint32_t state2[8];\
    ALIGN uint32_t state3[8];\
    ALIGN uint32_t state4[8];\
    ALIGN uint32_t state5[8];\
    ALIGN uint32_t state6[8];\
    ALIGN uint32_t state7[8];\
\
    /** Initializing state **/\
    state0[0] = 0x6a09e667;  state1[0] = 0x6a09e667;   state2[0] = 0x6a09e667;  state3[0] = 0x6a09e667;\
    state0[1] = 0xbb67ae85;  state1[1] = 0xbb67ae85;   state2[1] = 0xbb67ae85;  state3[1] = 0xbb67ae85;\
    state0[2] = 0x3c6ef372;  state1[2] = 0x3c6ef372;   state2[2] = 0x3c6ef372;  state3[2] = 0x3c6ef372;\
    state0[3] = 0xa54ff53a;  state1[3] = 0xa54ff53a;   state2[3] = 0xa54ff53a;  state3[3] = 0xa54ff53a;\
    state0[4] = 0x510e527f;  state1[4] = 0x510e527f;   state2[4] = 0x510e527f;  state3[4] = 0x510e527f;\
    state0[5] = 0x9b05688c;  state1[5] = 0x9b05688c;   state2[5] = 0x9b05688c;  state3[5] = 0x9b05688c;\
    state0[6] = 0x1f83d9ab;  state1[6] = 0x1f83d9ab;   state2[6] = 0x1f83d9ab;  state3[6] = 0x1f83d9ab;\
    state0[7] = 0x5be0cd19;  state1[7] = 0x5be0cd19;   state2[7] = 0x5be0cd19;  state3[7] = 0x5be0cd19;\
    state4[0] = 0x6a09e667;  state5[0] = 0x6a09e667;   state6[0] = 0x6a09e667;  state7[0] = 0x6a09e667;\
    state4[1] = 0xbb67ae85;  state5[1] = 0xbb67ae85;   state6[1] = 0xbb67ae85;  state7[1] = 0xbb67ae85;\
    state4[2] = 0x3c6ef372;  state5[2] = 0x3c6ef372;   state6[2] = 0x3c6ef372;  state7[2] = 0x3c6ef372;\
    state4[3] = 0xa54ff53a;  state5[3] = 0xa54ff53a;   state6[3] = 0xa54ff53a;  state7[3] = 0xa54ff53a;\
    state4[4] = 0x510e527f;  state5[4] = 0x510e527f;   state6[4] = 0x510e527f;  state7[4] = 0x510e527f;\
    state4[5] = 0x9b05688c;  state5[5] = 0x9b05688c;   state6[5] = 0x9b05688c;  state7[5] = 0x9b05688c;\
    state4[6] = 0x1f83d9ab;  state5[6] = 0x1f83d9ab;   state6[6] = 0x1f83d9ab;  state7[6] = 0x1f83d9ab;\
    state4[7] = 0x5be0cd19;  state5[7] = 0x5be0cd19;   state6[7] = 0x5be0cd19;  state7[7] = 0x5be0cd19;\
    \
    CORE_X8(state0,message[0],state1,message[1],state2,message[2],state3,message[3],state4,message[4],state5,message[5],state6,message[6],state7,message[7],num_blocks);\
\
    /** Padding message **/\
    for(i=0;i<rem_bytes;i++)\
    {\
        pad0[i] = message[0][64*num_blocks+i];\
        pad1[i] = message[1][64*num_blocks+i];\
        pad2[i] = message[2][64*num_blocks+i];\
        pad3[i] = message[3][64*num_blocks+i];\
        pad4[i] = message[4][64*num_blocks+i];\
        pad5[i] = message[5][64*num_blocks+i];\
        pad6[i] = message[6][64*num_blocks+i];\
        pad7[i] = message[7][64*num_blocks+i];\
    }\
    pad0[rem_bytes] = 0x80;\
    pad1[rem_bytes] = 0x80;\
    pad2[rem_bytes] = 0x80;\
    pad3[rem_bytes] = 0x80;\
    pad4[rem_bytes] = 0x80;\
    pad5[rem_bytes] = 0x80;\
    pad6[rem_bytes] = 0x80;\
    pad7[rem_bytes] = 0x80;\
    if (rem_bytes < 56)\
    {\
        for (i = rem_bytes + 1; i < 56; i++)\
        {\
            pad0[i] = 0x0;\
            pad1[i] = 0x0;\
            pad2[i] = 0x0;\
            pad3[i] = 0x0;\
            pad4[i] = 0x0;\
            pad5[i] = 0x0;\
            pad6[i] = 0x0;\
            pad7[i] = 0x0;\
        }\
        ((uint64_t*)pad0)[7] = (uint64_t)__builtin_bswap64(message_length*8);\
        ((uint64_t*)pad1)[7] = (uint64_t)__builtin_bswap64(message_length*8);\
        ((uint64_t*)pad2)[7] = (uint64_t)__builtin_bswap64(message_length*8);\
        ((uint64_t*)pad3)[7] = (uint64_t)__builtin_bswap64(message_length*8);\
        ((uint64_t*)pad4)[7] = (uint64_t)__builtin_bswap64(message_length*8);\
        ((uint64_t*)pad5)[7] = (uint64_t)__builtin_bswap64(message_length*8);\
        ((uint64_t*)pad6)[7] = (uint64_t)__builtin_bswap64(message_length*8);\
        ((uint64_t*)pad7)[7] = (uint64_t)__builtin_bswap64(message_length*8);\
        CORE_X8(state0,pad0,state1,pad1,state2,pad2,state3,pad3,state4,pad4,state5,pad5,state6,pad6,state7,pad7,1);\
    }\
    else\
    {\
        for (i = rem_bytes + 1; i < 120; i++)\
        {\
            pad0[i] = 0x0;\
            pad1[i] = 0x0;\
            pad2[i] = 0x0;\
            pad3[i] = 0x0;\
            pad4[i] = 0x0;\
            pad5[i] = 0x0;\
            pad6[i] = 0x0;\
            pad7[i] = 0x0;\
        }\
        ((uint64_t*)pad0)[15] = (uint64_t)__builtin_bswap64(message_length*8);\
        ((uint64_t*)pad1)[15] = (uint64_t)__builtin_bswap64(message_length*8);\
        ((uint64_t*)pad2)[15] = (uint64_t)__builtin_bswap64(message_length*8);\
        ((uint64_t*)pad3)[15] = (uint64_t)__builtin_bswap64(message_length*8);\
        ((uint64_t*)pad4)[15] = (uint64_t)__builtin_bswap64(message_length*8);\
        ((uint64_t*)pad5)[15] = (uint64_t)__builtin_bswap64(message_length*8);\
        ((uint64_t*)pad6)[15] = (uint64_t)__builtin_bswap64(message_length*8);\
        ((uint64_t*)pad7)[15] = (uint64_t)__builtin_bswap64(message_length*8);\
        CORE_X8(state0,pad0,state1,pad1,state2,pad2,state3,pad3,state4,pad4,state5,pad5,state6,pad6,state7,pad7,2);\
    }\
\
    for(i=0;i<SHA256_HASH_SIZE/4;i++)\
    {\
        ((uint32_t*)(digest[0]))[i] = (uint32_t)__builtin_bswap32(state0[i]);\
        ((uint32_t*)(digest[1]))[i] = (uint32_t)__builtin_bswap32(state1[i]);\
        ((uint32_t*)(digest[2]))[i] = (uint32_t)__builtin_bswap32(state2[i]);\
        ((uint32_t*)(digest[3]))[i] = (uint32_t)__builtin_bswap32(state3[i]);\
        ((uint32_t*)(digest[4]))[i] = (uint32_t)__builtin_bswap32(state4[i]);\
        ((uint32_t*)(digest[5]))[i] = (uint32_t)__builtin_bswap32(state5[i]);\
        ((uint32_t*)(digest[6]))[i] = (uint32_t)__builtin_bswap32(state6[i]);\
        ((uint32_t*)(digest[7]))[i] = (uint32_t)__builtin_bswap32(state7[i]);\
    }\
\
}

SHA_CORE(update_shani)
SHA_x2_CORE(update_shani_2x)
SHA_x4_CORE(update_shani_4x)
SHA_x8_CORE(update_shani_8x)
