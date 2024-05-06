#pragma once
#ifndef __prime_field
#define __prime_field

//#include <boost/multiprecision/cpp_int.hpp>
//#include <boost/random.hpp>
#include <cassert>
#include <immintrin.h>
#include <vector>
#include <memory>
#include <cstring>

#include "infrastructure/constants.h"
#include "field/mersenne_extension.h"

//using namespace boost::multiprecision;
//using namespace boost::random;

namespace prime_field
{   
    using field_element = gkr::mersen_ext_field::MersenExtField;
    // using field_element_packed = LinearGKR::mersen_ext_field::PackedMersenExtField;
    
    static bool initialized = true;
    static int mod = gkr::mersen_ext_field::mod;
    // static __m256i packed_mod = LinearGKR::mersen_ext_field::packed_mod;
    // static __m256i packed_mod_minus_one = _mm256_set1_epi32(mod - 1);

    inline void init() {}
    // do nothing following the original implementation
    inline void init_random() {}

    inline field_element random() {
        return field_element::random();
    }

    inline field_element fast_pow(const field_element&f, unsigned long long p) {
        return f.exp(p);
    }
    
    inline field_element get_root_of_unity(int log_order) {
        // TODO: fix this
        assert(log_order <= 32);
        field_element r = std::get<0>(gkr::mersen_ext_field::MersenExtField::max_order_primitive_root_of_unity());
        return r.exp(gkr::mersen_ext_field::Scalar(1) << (32 - log_order));
    }

    inline field_element inv(const field_element&f) {
        return f.inv();
    }

    // //extern int512_t mod;
    // extern bool initialized;
    // //extern independent_bits_engine<mt19937, 256, cpp_int> gen;

    // extern const unsigned long long mod;
    // extern __m256i packed_mod, packed_mod_minus_one;

    // void init();
    // void init_random();
    // inline unsigned long long myMod(unsigned long long x)
    // {
    //     return (x >> 61) + (x & mod);
    // }
    // inline unsigned long long mymult(const unsigned long long x, const unsigned long long y)
    // {
    //     //return a value between [0, 2PRIME) = x * y mod PRIME
    //     /*
    //     unsigned long long lo, hi;
    //     lo = _mulx_u64(x, y, &hi);
    //     return ((hi << 3) | (lo >> 61)) + (lo & PRIME);
    //     */
    //     unsigned long long hi;
    //     asm (
    //     "mov %[x_read], %%rdx;\n"
    //     "mulx %[y_read], %%r9, %%r10;"
    //     "shld $0x3, %%r9, %%r10;\n"
    //     "and %[mod_read], %%r9;\n"
    //     "add %%r10, %%r9;\n"
    //     "mov %%r9, %[hi_write]"
    //     : [hi_write]"=r"(hi)
    //     : [x_read]"r"(x), [y_read]"r"(y), [mod_read]"r"(mod)
    //     : "rdx", "r9", "r10"
    //     );
    //     return hi;
    // }
    // inline __m256i packed_mymult(const __m256i x, const __m256i y)
    // {
    //     __m256i ac, ad, bc, bd;
    //     __m256i x_shift, y_shift;
    //     x_shift = _mm256_srli_epi64(x, 32);
    //     y_shift = _mm256_srli_epi64(y, 32);
    //     bd = _mm256_mul_epu32(x, y);
    //     ac = _mm256_mul_epu32(x_shift, y_shift);
    //     ad = _mm256_mul_epu32(x_shift, y);
    //     bc = _mm256_mul_epu32(x, y_shift);

    //     __m256i ad_bc = _mm256_add_epi64(ad, bc);
    //     __m256i bd_srl32 = _mm256_srli_epi64(bd, 32);
    //     __m256i ad_bc_srl32 = _mm256_srli_epi64(_mm256_add_epi64(ad_bc, bd_srl32), 32);
    //     __m256i ad_bc_sll32 = _mm256_slli_epi64(ad_bc, 32);
    //     __m256i hi = _mm256_add_epi64(ac, ad_bc_srl32);

    //     __m256i lo = _mm256_add_epi64(bd, ad_bc_sll32);


    //     //return ((hi << 3) | (lo >> 61)) + (lo & PRIME);
    //     return _mm256_add_epi64(_mm256_or_si256(_mm256_slli_epi64(hi, 3), _mm256_srli_epi64(lo, 61)), _mm256_and_si256(lo, packed_mod));
    // }

    // inline __m256i packed_myMod(const __m256i x)
    // {
    //     //return (x >> 61) + (x & mod);
    //     __m256i srl64 = _mm256_srli_epi64(x, 61);
    //     __m256i and64 = _mm256_and_si256(x, packed_mod);
    //     return _mm256_add_epi64(srl64, and64);
    // }

    // /*
    // This defines a field
    // */
    // class field_element
    // {
    // private:
    // public:
    //     unsigned long long img, real;
    //     std::unique_ptr<char[]> bit_stream()
    //     {
    //         char* p = new char[sizeof(field_element)];
    //         memcpy(p, this, sizeof(field_element));
    //         return std::unique_ptr<char[]>(p);
    //     }

    //     int size() {return sizeof(field_element);}

    //     inline field_element()
    //     {
    //         real = 0;
    //         img = 0;
    //     }
    //     inline field_element(const unsigned long long x)
    //     {
    //         real = x % mod;
    //         img = 0;
    //     }

    //     inline field_element operator + (const field_element &b) const
    //     {
    //         field_element ret;
    //         ret.img = b.img + img;
    //         ret.real = b.real + real;
    //         if(mod <= ret.img)
    //             ret.img = ret.img - mod;
    //         if(mod <= ret.real)
    //             ret.real = ret.real - mod;
    //         return ret;
    //     }
    //     inline field_element operator * (const field_element &b) const
    //     {
    //         field_element ret;
    //         auto all_prod = mymult(img + real, b.img + b.real); //at most 6 * mod
    //         //unsigned long long ac, bd;
    //         //mymult_2vec(real, b.real, img, b.img, ac, bd);
    //         auto ac = mymult(real, b.real), bd = mymult(img, b.img); //at most 1.x * mod
    //         auto nac = ac;
    //         if(bd >= mod)
    //             bd -= mod;
    //         if(nac >= mod)
    //             nac -= mod;
    //         nac ^= mod; //negate
    //         bd ^= mod; //negate

    //         auto t_img = all_prod + nac + bd; //at most 8 * mod
    //         t_img = myMod(t_img);
    //         if(t_img >= mod)
    //             t_img -= mod;
    //         ret.img = t_img;
    //         auto t_real = ac + bd;

    //         while(t_real >= mod)
    //             t_real -= mod;
    //         ret.real = t_real;
    //         return ret;
    //     }
    //     inline field_element operator - (const field_element &b) const
    //     {
    //         field_element ret;
    //         auto tmp_r = b.real ^ mod; //tmp_r == -b.real is true in this prime field
    //         auto tmp_i = b.img ^ mod; //same as above
    //         ret.real = real + tmp_r;
    //         ret.img = img + tmp_i;
    //         if(ret.real >= mod)
    //             ret.real -= mod;
    //         if(ret.img >= mod)
    //             ret.img -= mod;

    //         return ret;
    //     }
    //     inline field_element operator - () const
    //     {
    //         field_element ret;
    //         ret.real = (mod - real) % mod; // do modular in case real = 0
    //         ret.img = (mod - img) % mod;
    //         return ret;
    //     }

    //     bool operator == (const field_element &b) const;
    //     bool operator != (const field_element &b) const;
    // };

    // class field_element_packed
    // {
    // public:
    //     __m256i img, real;

    //     inline field_element_packed()
    //     {
    //         real = _mm256_set_epi64x(0, 0, 0, 0);
    //         img = _mm256_set_epi64x(0, 0, 0, 0);
    //     }

    //     inline field_element_packed(const field_element &x0, const field_element &x1, const field_element &x2, const field_element &x3)
    //     {
    //         real = _mm256_set_epi64x(x3.real, x2.real, x1.real, x0.real);
    //         img = _mm256_set_epi64x(x3.img, x2.img, x1.img, x0.img);
    //     }

    //     inline field_element_packed operator + (const field_element_packed &b) const
    //     {
    //         field_element_packed ret;
    //         ret.img = b.img + img;
    //         ret.real = b.real + real;
    //         __m256i msk0, msk1;
    //         msk0 = _mm256_cmpgt_epi64(ret.img, packed_mod_minus_one);
    //         msk1 = _mm256_cmpgt_epi64(ret.real, packed_mod_minus_one);
    //         ret.img = ret.img - _mm256_and_si256(msk0, packed_mod);
    //         ret.real = ret.real - _mm256_and_si256(msk1,packed_mod);
    //         return ret;
    //     }
    //     inline field_element_packed operator * (const field_element_packed &b) const
    //     {
    //         field_element_packed ret;
    //         __m256i all_prod = packed_mymult(img + real, b.img + b.real); //at most 6 * mod
    //         __m256i ac = packed_mymult(real, b.real), bd = packed_mymult(img, b.img); //at most 1.x * mod
    //         __m256i nac = ac;
    //         __m256i msk;
    //         msk = _mm256_cmpgt_epi64(bd, packed_mod_minus_one);
    //         bd = _mm256_sub_epi64(bd, _mm256_and_si256(packed_mod, msk));

    //         msk = _mm256_cmpgt_epi64(nac, packed_mod_minus_one);
    //         nac = _mm256_sub_epi64(nac, _mm256_and_si256(packed_mod, msk));

    //         nac = _mm256_xor_si256(nac, packed_mod);
    //         bd = _mm256_xor_si256(bd, packed_mod);

    //         __m256i t_img = _mm256_add_epi64(_mm256_add_epi64(all_prod, nac), bd);
    //         t_img = packed_myMod(t_img);

    //         msk = _mm256_cmpgt_epi64(t_img, packed_mod_minus_one);
    //         t_img = _mm256_sub_epi64(t_img, _mm256_and_si256(packed_mod, msk));

    //         ret.img = t_img;
    //         __m256i t_real = _mm256_add_epi64(ac, bd);
    //         while(1)
    //         {
    //             msk = _mm256_cmpgt_epi64(t_real, packed_mod_minus_one);
    //             int res = _mm256_testz_si256(msk, msk);
    //             if(res)
    //                 break;
    //             t_real = _mm256_sub_epi64(t_real, _mm256_and_si256(packed_mod, msk));
    //         }

    //         ret.real = t_real;
    //         return ret;
    //     }
    //     inline field_element_packed operator - (const field_element_packed &b) const
    //     {
    //         field_element_packed ret;
    //         __m256i tmp_r = b.real ^ packed_mod; //tmp_r == -b.real is true in this prime field
    //         __m256i tmp_i = b.img ^ packed_mod; //same as above
    //         ret.real = real + tmp_r;
    //         ret.img = img + tmp_i;
    //         __m256i msk0, msk1;
    //         msk0 = _mm256_cmpgt_epi64(ret.real, packed_mod_minus_one);
    //         msk1 = _mm256_cmpgt_epi64(ret.img, packed_mod_minus_one);

    //         ret.real = ret.real - _mm256_and_si256(msk0, packed_mod);
    //         ret.img = ret.img - _mm256_and_si256(msk1, packed_mod);

    //         return ret;
    //     }
    //     __mmask8 operator == (const field_element_packed &b) const;
    //     __mmask8 operator != (const field_element_packed &b) const;
    //     inline void get_field_element(field_element *dst) const
    //     {
    //         static unsigned long long real_arr[packed_size], img_arr[packed_size];
    //         _mm256_store_si256((__m256i*)real_arr, real);
    //         _mm256_store_si256((__m256i*)img_arr, img);
    //         for(int i = 0; i < 4; ++i)
    //         {
    //             dst[i].real = real_arr[i];
    //             dst[i].img = img_arr[i];
    //         }
    //     }
    // };


    // const int __max_order = 62;
    // field_element get_root_of_unity(int order); //return a root of unity with order 2^[order]
    // field_element random_real_only();
    // field_element random();
    // field_element fast_pow(field_element x, __uint128_t p);
    // field_element inv(field_element x);
    // double self_speed_test_mult(int repeat);
    // double self_speed_test_add(int repeat);
}
#endif