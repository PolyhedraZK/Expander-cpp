#ifdef __x86_64
#include <cstring>

namespace gkr::M31_field
{

const __m256i packed_mod = _mm256_set1_epi32(mod);
const __m256i packed_0 = _mm256_set1_epi32(0);
const __m256i packed_mod_epi64 = _mm256_set1_epi64x(mod);
const __m256i packed_mod_square = _mm256_set1_epi64x(mod * static_cast<int64>(mod));

#define mod_reduce(x) (x) = _mm256_add_epi64(_mm256_and_si256((x), packed_mod_epi64), _mm256_srli_epi64((x), 31));
#define mod_reduce_epi32(x) (x) = _mm256_add_epi32(_mm256_and_si256((x), packed_mod), _mm256_srli_epi32((x), 31));

inline void print_m256i(__m256i x)
{
    int *c_x = (int *)&x;
    for (int i = 0; i < 8; i++)
    {
        printf("%d ", c_x[i]);
    }
    printf("\n");
}

inline void print_m256(__m256 x)
{
    int *c_x = (int *)&x;
    for (int i = 0; i < 8; i++)
    {
        printf("%d ", c_x[i]);
    }
    printf("\n");
}

PackedM31 PackedM31::zero()
{
    return PackedM31();
}

PackedM31 PackedM31::one()
{
    return new_unchecked(_mm256_set1_epi32(1));
}

PackedM31 PackedM31::random()
{
    __m256i x = _mm256_setr_epi32(rand(), rand(), rand(), rand(),
                                    rand(), rand(), rand(), rand());
                                    
    mod_reduce_epi32(x);
    mod_reduce_epi32(x);
    return new_unchecked(x);
}

PackedM31 PackedM31::random_bool()
{
    __m256i x = _mm256_setr_epi32(rand() % 2, rand() % 2, rand() % 2, rand() % 2, rand() % 2, rand() % 2, rand() % 2, rand() % 2);
    return new_unchecked(x);
}

void PackedM31::from_bytes(const uint8 *input)
{
    memcpy(this, input, sizeof(*this));
    mod_reduce_epi32(x);
    mod_reduce_epi32(x);
}

PackedM31 PackedM31::new_unchecked(const __m256i &x)
{
    PackedM31 r;
    r.x = x;
    return r;
}

inline PackedM31::PackedM31()
{
    x = _mm256_set1_epi32(0);
}

inline PackedM31::PackedM31(uint32 xx)
{
    mod_reduce_int(xx);
    x = _mm256_set1_epi32(xx);
}

inline PackedM31::PackedM31(const PackedM31 &xx)
{
    memcpy(this, &xx, sizeof(*this));
}

size_t PackedM31::pack_size()
{
    return 8;
}

inline PackedM31 PackedM31::operator+(const PackedM31 &rhs) const
{
    PackedM31 result;
    result.x = _mm256_add_epi32(x, rhs.x);
    auto subx = _mm256_sub_epi32(result.x, packed_mod);
    result.x = _mm256_min_epu32(result.x, subx);
    return result;
}

inline PackedM31 PackedM31::operator*(const PackedM31 &rhs) const
{
    __m256i x_shifted = _mm256_srli_epi64(x, 32);                // latency 1, CPI 1
    __m256i rhs_x_shifted = _mm256_srli_epi64(rhs.x, 32);        // latency 1, CPI 1
    __m256i xa_even = _mm256_mul_epi32(x, rhs.x);                // latency 5, CPI 0.5
    __m256i xa_odd = _mm256_mul_epi32(x_shifted, rhs_x_shifted); // latency 5, CPI 0.5

    mod_reduce(xa_even); // output will be in [0, 2^32)
    mod_reduce(xa_odd);

    PackedM31 result;
    result.x = xa_even | _mm256_slli_epi64(xa_odd, 32); // latency 1, CPI 1
    mod_reduce_epi32(result.x);
    return result;
}

/*
 * let k = a * b mod 2^31
     r = a * b >> 31

    then a * b = k + r * (p + 1) = k + r * p + r === k + r (mod p)

    assuming mod_reduce(a * b) = k + r == p

    then a * b mod p = 0 and a * b \neq 0

    then we have a, b \neq 0

    since a, b \neq 0, we have gcd(a, p) = 1, gcd(b, p) = 1

    then a * b will not have a factor p

    then a * b mod p \neq 0, contradiction
 */

inline PackedM31 PackedM31::operator*(const M31 &_rhs) const
{
    const __m256i rhs_x = _mm256_set1_epi32(_rhs.x);
    const __m256i x_shifted = _mm256_srli_epi64(x, 32);                // latency 1, CPI 0.5
    const __m256i rhs_x_shifted = _mm256_srli_epi64(rhs_x, 32);        // latency 1, CPI 0.5
    __m256i xa_even = _mm256_mul_epi32(x, rhs_x);                // latency 5, CPI 0.5
    __m256i xa_odd = _mm256_mul_epi32(x_shifted, rhs_x_shifted); // latency 5, CPI 0.5

    mod_reduce(xa_even); // output will be in [0, 2^32)
    mod_reduce(xa_odd);

    PackedM31 result;
    result.x = xa_even | _mm256_slli_epi64(xa_odd, 32); // latency 1, CPI 1
    mod_reduce_epi32(result.x);
    return result;
}

inline PackedM31 PackedM31::operator-() const
{
    PackedM31 r;
    auto subx = _mm256_sub_epi32(packed_mod, x);
    auto zero_cmp = _mm256_cmpeq_epi32(x, packed_0);
    r.x = _mm256_andnot_si256(zero_cmp, subx);
    return r;
}

inline PackedM31 PackedM31::operator-(const PackedM31 &rhs) const
{
    PackedM31 result;
    result.x = _mm256_sub_epi32(x, rhs.x);
    auto subx = _mm256_add_epi32(result.x, packed_mod);
    result.x = _mm256_min_epu32(result.x, subx);
    return result;
}

inline void PackedM31::operator+=(const PackedM31 &rhs)
{
    x = _mm256_add_epi32(x, rhs.x);
    auto subx = _mm256_sub_epi32(x, packed_mod);
    x = _mm256_min_epu32(x, subx);
}

bool PackedM31::operator==(const PackedM31 &rhs) const
{
    __m256i pcmp = _mm256_cmpeq_epi32(x, rhs.x);
    unsigned bitmask = _mm256_movemask_epi8(pcmp);
    return (bitmask == 0xffffffffU);
}

PackedM31 PackedM31::pack_full(const M31 &f)
{
    return PackedM31::new_unchecked(_mm256_set1_epi32(f.x));
}

std::vector<M31> PackedM31::unpack() const
{
    std::vector<M31> result;
    result.push_back(M31::new_unchecked(_mm256_extract_epi32(x, 0)));
    result.push_back(M31::new_unchecked(_mm256_extract_epi32(x, 1)));
    result.push_back(M31::new_unchecked(_mm256_extract_epi32(x, 2)));
    result.push_back(M31::new_unchecked(_mm256_extract_epi32(x, 3)));
    result.push_back(M31::new_unchecked(_mm256_extract_epi32(x, 4)));
    result.push_back(M31::new_unchecked(_mm256_extract_epi32(x, 5)));
    result.push_back(M31::new_unchecked(_mm256_extract_epi32(x, 6)));
    result.push_back(M31::new_unchecked(_mm256_extract_epi32(x, 7)));
    return result;
}

inline std::vector<PackedM31> pack_field_elements(const std::vector<M31> &fs)
{
    uint32 n_seg = (fs.size() + 7) / 8; // ceiling
    std::vector<PackedM31> packed_field_elements(n_seg);

    for (uint32 i = 0; i < n_seg - 1; i++)
    {
        uint32 base = i * 8;
        packed_field_elements[i].x = _mm256_setr_epi32(
            fs[base + 0].x,
            fs[base + 1].x,
            fs[base + 2].x,
            fs[base + 3].x,
            fs[base + 4].x,
            fs[base + 5].x,
            fs[base + 6].x,
            fs[base + 7].x);
    }

    // set the remaining value
    uint32 base = (n_seg - 1) * 8;
    std::vector<int> x(8);
    auto x_it = x.begin();
    for (uint32 i = base; i < fs.size(); i++)
    {
        *x_it++ = fs[i].x;
    }
    packed_field_elements[n_seg - 1].x = _mm256_setr_epi32(x[0], x[1], x[2], x[3], x[4], x[5], x[6], x[7]);

    return packed_field_elements;
}

VectorizedM31 VectorizedM31::one()
{
    return new_unchecked(_mm256_set1_epi32(1));
}

std::vector<VectorizedM31> VectorizedM31::pack_field_elements(const std::vector<M31> &fs)
{
{
        uint32 n_seg = (fs.size() + pack_size() - 1) / pack_size(); // ceiling
        std::vector<VectorizedM31> packed_field_elements(n_seg);

        for (uint32 i = 0; i < n_seg - 1; i++)
        {
            for (uint32 j = 0; j < vectorize_size; j++)
            {
                uint32 base = i * pack_size() + j * PackedM31::pack_size();
                //packed_field_elements[i].elements[j].x = vld1q_u32((uint32_t *)&fs[base]);
                packed_field_elements[i].elements[j].x = _mm256_setr_epi32(
                    fs[base + 0].x,
                    fs[base + 1].x,
                    fs[base + 2].x,
                    fs[base + 3].x,
                    fs[base + 4].x,
                    fs[base + 5].x,
                    fs[base + 6].x,
                    fs[base + 7].x);
            }
        }

        // set the remaining value
        uint32 base = (n_seg - 1) * pack_size();
        std::vector<int> x(pack_size());
        auto x_it = x.begin();
        for (uint32 i = base; i < fs.size(); i++)
        {
            *x_it++ = fs[i].x;
        }
        for (uint32 j = 0; j < vectorize_size; j++)
        {
            auto offset = j * PackedM31::pack_size();
            //packed_field_elements[n_seg - 1].elements[j].x = vld1q_u32((uint32_t *)&x[offset]);
            packed_field_elements[n_seg - 1].elements[j].x = _mm256_setr_epi32(x[0 + offset], x[1 + offset], x[2 + offset], x[3 + offset], x[4 + offset], x[5 + offset], x[6 + offset], x[7 + offset]);
        }

        return packed_field_elements;
    }
}

VectorizedM31 VectorizedM31::INV_2 = VectorizedM31::new_unchecked(_mm256_set1_epi32(1 << 30));

}
#endif

