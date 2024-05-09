#ifdef __ARM_NEON
#include <arm_neon.h>
#include <iostream>

#include <stdio.h>
#include <time.h>
#include <tuple>
#include <cassert>
#include "basefield.hpp"
namespace gkr::M31_field {

const uint32x4_t packed_mod = vdupq_n_u32(mod);
const uint32x4_t packed_0 = vdupq_n_u32(0);

PackedM31 PackedM31::zero()
{
    return PackedM31();
}

PackedM31 PackedM31::one()
{
    PackedM31 r;
    r.x = vdupq_n_u32(1);
    return r;
}

PackedM31 PackedM31::random()
{
    uint32x4_t x;
    uint32_t rand_vector[4];
    for (int i = 0; i < 4; i++)
    {
        rand_vector[i] = rand();
    }
    x = vld1q_u32(rand_vector);
    return new_unchecked(x);
}

PackedM31 PackedM31::random_bool()
{
    uint32x4_t x;
    uint32_t rand_vector[4];
    for (int i = 0; i < 4; i++)
    {
        rand_vector[i] = rand() % 2;
    }
    x = vld1q_u32(rand_vector);
    return new_unchecked(x);
}

void PackedM31::from_bytes(const uint8 *input)
{
    memcpy(this, input, sizeof(*this));
    // FIXME: use a better way to reduce
    x = reduce_sum(x);
    x = reduce_sum(x);
}

PackedM31 PackedM31::new_unchecked(const uint32x4_t &x)
{
    PackedM31 r;
    r.x = x;
    return r;
}

PackedM31::PackedM31()
{
    x = vdupq_n_u32(0);
}

PackedM31::PackedM31(const PackedM31 &x)
{
    this->x = x.x;
}

PackedM31::PackedM31(uint32 xx)
{
    mod_reduce_int(xx);
    x = vdupq_n_u32(xx);
}

PackedM31::PackedM31(const uint32x4_t &xx)
{
    x = xx;
}

size_t PackedM31::pack_size() 
{
    return 4;
}

uint32x4_t PackedM31::reduce_sum(uint32x4_t x) const
{
    auto u = vsubq_u32(x, packed_mod);
    return vminq_u32(x, u);
} 

PackedM31 PackedM31::operator+(const PackedM31 &rhs) const
{
    PackedM31 result;
    result.x = vaddq_u32(x, rhs.x);
    result.x = reduce_sum(result.x);
    return result;
}

PackedM31 PackedM31::operator+(const M31 &rhs_single) const
{
    PackedM31 result;
    auto rhs = PackedM31::pack_full(rhs_single);
    result.x = vaddq_u32(x, rhs.x);
    result.x = reduce_sum(result.x);
    return result;
}

PackedM31 PackedM31::operator*(const PackedM31 &rhs) const
{
    auto prod_hi = vqdmulhq_s32(x, rhs.x);
    auto prod_lo = vmulq_u32(x, rhs.x);
    auto t = vmlsq_u32(prod_lo, prod_hi, packed_mod);
    PackedM31 result;
    result.x = reduce_sum(t);
    return result;
}

PackedM31 PackedM31::operator-() const
{
    PackedM31 r;
    r.x = vsubq_u32(packed_mod, x);
    return r;
}

void PackedM31::operator+=(const PackedM31 &rhs)
{
    x = vaddq_u32(x, rhs.x);
    x = reduce_sum(x);
}

PackedM31 PackedM31::pack(const M31 *fs)
{
    return PackedM31::new_unchecked(vld1q_u32((uint32_t *)fs));
}

PackedM31 PackedM31::operator*(const M31 &rhs_single) const 
{
    auto rhs = PackedM31::pack_full(rhs_single);
    return *this * rhs;
}

PackedM31 PackedM31::operator-(const PackedM31 &rhs) const
{
    PackedM31 r;
    auto diff = vsubq_s32(x, rhs.x);
    auto u = vaddq_u32(diff, packed_mod);
    r.x = vminq_u32(diff, u);
    return r;
}

bool PackedM31::operator==(const PackedM31 &rhs) const
{
    auto x0 = x;
    auto x1 = rhs.x;
    auto eq = vceqq_u32(x0, x1);
    return vgetq_lane_u32(eq, 0) && vgetq_lane_u32(eq, 1) && vgetq_lane_u32(eq, 2) && vgetq_lane_u32(eq, 3);
}

PackedM31 PackedM31::pack_full(const M31 &f)
{
    return PackedM31::new_unchecked(vdupq_n_u32(f.x));
}

std::vector<M31> PackedM31::unpack() const
{
    std::vector<M31> result;

    result.push_back(M31::new_unchecked(vgetq_lane_u32(x, 0)));
    result.push_back(M31::new_unchecked(vgetq_lane_u32(x, 1)));
    result.push_back(M31::new_unchecked(vgetq_lane_u32(x, 2)));
    result.push_back(M31::new_unchecked(vgetq_lane_u32(x, 3)));
    return result;
}

std::vector<PackedM31> pack_field_elements(const std::vector<M31> &fs)
{
    uint32 n_seg = (fs.size() + PackedM31::pack_size() - 1) / PackedM31::pack_size(); // ceiling
    std::vector<PackedM31> packed_field_elements(n_seg);

    for (uint32 i = 0; i < n_seg - 1; i++)
    {
        uint32 base = i * PackedM31::pack_size();
        packed_field_elements[i].x = vld1q_u32((uint32_t *)&fs[base]);
    }
    // set the remaining value
    uint32 base = (n_seg - 1) * PackedM31::pack_size();
    std::vector<int> x(PackedM31::pack_size());
    auto x_it = x.begin();
    for (uint32 i = base; i < fs.size(); i++)
    {
        *x_it++ = fs[i].x;
    }
    packed_field_elements[n_seg - 1].x = vld1q_u32((uint32_t *)&x[0]);

    return packed_field_elements;
}


VectorizedM31 VectorizedM31::one()
{
    return new_unchecked(vdupq_n_u32(1));
}

VectorizedM31 VectorizedM31::pack_full(const M31 &f)
{
    return VectorizedM31::new_unchecked(vdupq_n_u32(f.x));
}

std::vector<VectorizedM31> VectorizedM31::pack_field_elements(const std::vector<M31> &fs)
{
    uint32 n_seg = (fs.size() + pack_size() - 1) / pack_size(); // ceiling
    std::vector<VectorizedM31> packed_field_elements(n_seg);

    for (uint32 i = 0; i < n_seg - 1; i++)
    {
        for (uint32 j = 0; j < vectorize_size; j++)
        {
            uint32 base = i * pack_size() + j * PackedM31::pack_size();
            packed_field_elements[i].elements[j].x = vld1q_u32((uint32_t *)&fs[base]);
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
        packed_field_elements[n_seg - 1].elements[j].x = vld1q_u32((uint32_t *)&x[offset]);
    }

    return packed_field_elements;
}

VectorizedM31 VectorizedM31::INV_2 = VectorizedM31::new_unchecked(vdupq_n_u32(1073741824));
}
#endif