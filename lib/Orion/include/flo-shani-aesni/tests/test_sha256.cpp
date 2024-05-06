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
#include <gtest/gtest.h>
#include <openssl/sha.h>
#include <flo-random.h>
#include <flo-shani.h>

#define TEST_TIMES 1000

#define SHA256_DigestSize 32
typedef uint8_t Digest[SHA256_DigestSize];

static std::ostream &operator<<(std::ostream &os, const Digest &d) {
  int i = 0;
  for (i = 0; i < SHA256_DigestSize; i++) {
    os << std::setbase(16) << std::setfill('0') << std::setw(2)
       << static_cast<int>(d[i]);
  }
  return os << std::endl;
}

TEST(SHA256, ZERO_LENGHT) {
  unsigned long mlen = 2;
  uint8_t digest0[32];
  uint8_t digest1[32];
  uint8_t *message0 = (uint8_t *) _mm_malloc(mlen, ALIGN_BYTES);
  uint8_t *message1 = (uint8_t *) _mm_malloc(mlen, ALIGN_BYTES);

  SHA256(message0, 0, digest0);
  sha256_update_shani(message1, 0, digest1);
  ASSERT_EQ(memcmp(digest0, digest1, SHA256_DigestSize), 0)
                << "want: " << digest0
                << "get:  " << digest1;
  _mm_free(message0);
  _mm_free(message1);
}

TEST(SHA256, ONE_BLOCK) {
  unsigned long mlen = rand() % 56;
  uint8_t digest0[32];
  uint8_t digest1[32];
  uint8_t *message0 = (uint8_t *) _mm_malloc(mlen, ALIGN_BYTES);
  uint8_t *message1 = (uint8_t *) _mm_malloc(mlen, ALIGN_BYTES);
  int k = 0;
  do {
    random_bytes(message0, mlen);
    memcpy(message1, message0, mlen);

    SHA256(message0, mlen, digest0);
    sha256_update_shani(message1, mlen, digest1);
    k++;
    ASSERT_EQ(memcmp(digest0, digest1, SHA256_DigestSize), 0)
                  << "want: " << digest0
                  << "get:  " << digest1;
  } while (k < TEST_TIMES);

  _mm_free(message0);
  _mm_free(message1);
}

TEST(SHA256, TWO_BLOCKS) {
  unsigned long mlen = rand() % 120;
  uint8_t digest0[32];
  uint8_t digest1[32];
  uint8_t *message0 = (uint8_t *) _mm_malloc(mlen, ALIGN_BYTES);
  uint8_t *message1 = (uint8_t *) _mm_malloc(mlen, ALIGN_BYTES);
  int k = 0;
  do {
    random_bytes(message0, mlen);
    memcpy(message1, message0, mlen);

    SHA256(message0, mlen, digest0);
    sha256_update_shani(message1, mlen, digest1);
    k++;
    ASSERT_EQ(memcmp(digest0, digest1, SHA256_DigestSize), 0)
                  << "want: " << digest0
                  << "get:  " << digest1;
  } while (k < TEST_TIMES);

  _mm_free(message0);
  _mm_free(message1);
}

TEST(SHA256, LARGE) {
  int k = 0;
  uint8_t digest0[32];
  uint8_t digest1[32];
  uint8_t *message0 = NULL;
  uint8_t *message1 = NULL;
  unsigned long mlen = 0;

  do {
    mlen = rand() % (1 << 16);
    message0 = (uint8_t *) _mm_malloc(mlen, ALIGN_BYTES);
    message1 = (uint8_t *) _mm_malloc(mlen, ALIGN_BYTES);

    random_bytes(message0, mlen);
    memcpy(message1, message0, mlen);

    SHA256(message0, mlen, digest0);
    sha256_update_shani(message1, mlen, digest1);
    k++;
    ASSERT_EQ(memcmp(digest0, digest1, SHA256_DigestSize), 0)
                  << "want: " << digest0
                  << "get:  " << digest1;
    _mm_free(message0);
    _mm_free(message1);
  } while (k < TEST_TIMES);
}

#define COMPARE_MULTI(FUNC0, FUNC1, MSG_LEN, NUM)  \
do{                                             \
    int i_multi=0;                              \
    unsigned i_count=0;                         \
    uint8_t *message[NUM];                      \
    uint8_t *digest_func0[NUM];                 \
    uint8_t *digest_func1[NUM];                 \
    for(i_multi=0;i_multi<NUM;i_multi++)        \
    {                                           \
        message[i_multi] = (uint8_t*)_mm_malloc(MSG_LEN+1,ALIGN_BYTES);  \
        digest_func0[i_multi] = (uint8_t*)_mm_malloc(32,ALIGN_BYTES);  \
        digest_func1[i_multi] = (uint8_t*)_mm_malloc(32,ALIGN_BYTES);  \
        random_bytes(message[i_multi],MSG_LEN); \
    }                                           \
    for(i_multi=0;i_multi<NUM;i_multi++)        \
    {                                           \
        FUNC0(message[i_multi],MSG_LEN,digest_func0[i_multi]); \
    }                                           \
    FUNC1(message,MSG_LEN,digest_func1);        \
    for(i_multi=0;i_multi<NUM;i_multi++)        \
    {                                           \
        i_count += memcmp(digest_func0[i_multi],digest_func1[i_multi],32) == 0; \
    }                                           \
    for(i_multi=0;i_multi<NUM;i_multi++)        \
    {                                           \
        _mm_free(message[i_multi]);             \
        _mm_free(digest_func0[i_multi]);        \
        _mm_free(digest_func1[i_multi]);        \
    }                                           \
    if(i_count == NUM) {ret++;}                 \
}while(0)

TEST(SHA256, MULTI_2x_PIPE) {
  int k = 0;
  long ret = 0;
  do {
    COMPARE_MULTI(SHA256, sha256_x2_update_shani_2x, k, 2);
    k++;
  } while (k < TEST_TIMES);
  ASSERT_EQ(ret, TEST_TIMES) << "want:\n ";
}

TEST(SHA256, MULTI_4x_PIPE) {
  int k = 0;
  long ret = 0;
  do {
    COMPARE_MULTI(SHA256, sha256_x4_update_shani_4x, k, 4);
    k++;
  } while (k < TEST_TIMES);
  ASSERT_EQ(ret, TEST_TIMES) << "want:\n ";
}

TEST(SHA256, MULTI_8x_PIPE) {
  int k = 0;
  long ret = 0;
  do {
    COMPARE_MULTI(SHA256, sha256_x8_update_shani_8x, k, 8);
    k++;
  } while (k < TEST_TIMES);
  ASSERT_EQ(ret, TEST_TIMES) << "want:\n ";
}

TEST(SHA256, MULTI_4x_VEC) {
  int k = 0;
  long ret = 0;
  do {
    COMPARE_MULTI(SHA256, sha256_4w, k, 4);
    k++;
  } while (k < TEST_TIMES);
  ASSERT_EQ(ret, TEST_TIMES) << "want:\n ";
}

TEST(SHA256, MULTI_8x_VEC) {
  int k = 0;
  long ret = 0;
  do {
    COMPARE_MULTI(SHA256, sha256_8w, k, 8);
    k++;
  } while (k < TEST_TIMES);
  ASSERT_EQ(ret, TEST_TIMES) << "want:\n ";
}
