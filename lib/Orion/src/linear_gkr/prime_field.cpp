// #include "linear_gkr/prime_field.h"
// #include <cmath>
// #include <climits>
// #include <cstdio>
// #include <ctime>
// #include <chrono>
// #include <immintrin.h>

// namespace prime_field
// {
//     const unsigned long long mod = 2305843009213693951LL;
//     __m256i packed_mod, packed_mod_minus_one;
//     bool initialized = false;

// #define MASK 4294967295 //2^32-1
// #define PRIME 2305843009213693951 //2^61-1


//     void init()
//     {
//         packed_mod = _mm256_set_epi64x(mod, mod, mod, mod);
//         packed_mod_minus_one = _mm256_set_epi64x(mod - 1, mod - 1, mod - 1, mod - 1);
//         initialized = true;
//     }
//     void init_random()
//     {
//     }
//     field_element random_real_only()
//     {
//         field_element ret;
//         ret.real = (unsigned long long)rand() % mod;
//         ret.img = 0;
//         return ret;
//     }
//     field_element random()
//     {
//         field_element ret;
//         ret.real = (unsigned long long)rand() % mod;
//         ret.img = (unsigned long long)rand() % mod;
//         return ret;
//     }
//     bool field_element::operator != (const field_element &b) const
//     {
//         return real != b.real || img != b.img;
//     }
//     bool field_element::operator == (const field_element &b) const
//     {
//         return !(*this != b);
//     }
//     __mmask8 field_element_packed::operator == (const field_element_packed &b) const
//     {
//         __m256i res_real = real ^ b.real;
//         __m256i res_img = img ^ b.img;
//         return _mm256_testz_si256(res_real, res_real) && _mm256_testz_si256(res_img, res_img);
//     }
//     __mmask8 field_element_packed::operator != (const field_element_packed &b) const
//     {
//         return !(*this == b);
//     }
//     field_element get_root_of_unity(int log_order)
//     {
//         field_element rou;
//         //general root of unity, have order 2^61
//         rou.img = 1033321771269002680L;
//         rou.real = 2147483648L;

//         assert(log_order <= 61);

//         for(int i = 0; i < __max_order - log_order; ++i)
//             rou = rou * rou;

//         return rou;
//     }
//     field_element fast_pow(field_element x, __uint128_t p)
//     {
//         field_element ret = field_element(1), tmp = x;
//         while(p)
//         {
//             if(p & 1)
//             {
//                 ret = ret * tmp;
//             }
//             tmp = tmp * tmp;
//             p >>= 1;
//         }
//         return ret;
//     }
//     field_element inv(field_element x)
//     {
//         __uint128_t p = 2305843009213693951LL;
//         return fast_pow(x, p*p - 2);
//     }
//     double self_speed_test_mult(int repeat)
//     {
//         prime_field::field_element a, b;
//         a = random();
//         b = random();
//         prime_field::field_element c;
//         auto t0 = std::chrono::high_resolution_clock::now();
//         for(int i = 0; i < repeat; ++i)
//         {
//             c = a * b;
//             b = c * a;
//             a = c * b;
//         }
//         printf("%llu %llu\n", c.img, c.real);
//         auto t1 = std::chrono::high_resolution_clock::now();
//         return std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t0).count();
//     }
//     double self_speed_test_add(int repeat)
//     {
//         prime_field::field_element a, b;
//         a = random();
//         b = random();
//         prime_field::field_element c;
//         auto t0 = std::chrono::high_resolution_clock::now();
//         for(int i = 0; i < repeat; ++i)
//         {
//             c = a + b;
//             b = c + a;
//             a = c + b;
//         }
//         printf("%llu %llu\n", c.img, c.real);
//         auto t1 = std::chrono::high_resolution_clock::now();
//         return std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t0).count();
//     }
// }