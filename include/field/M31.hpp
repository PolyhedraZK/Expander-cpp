#pragma once

#include <iostream>
#include <stdio.h>
#include <time.h>
#include <tuple>
#include <cassert>
#include <cstring>

#ifdef __ARM_NEON
#include <arm_neon.h>
#else
#include <immintrin.h>
#endif

#include "basefield.hpp"

namespace gkr::M31_field {

const int mod = 2147483647;
typedef int64 Scalar;
#define mod_reduce_int(x) (x = (((x) & mod) + ((x) >> 31)))

/*
P = 2^31 - 1
    # 2^31 - 1 = 2147483647

(x, y)

(x, y) + (a, b) = ((x + a) mod P, (y + b) mod P)
(x, y) * (a, b) = (x + iy) * (a + ib) = ((x * a - y * b) mod P, (x * b + y * a) mod P)

(x mod P) = (x & P) + (x >> 31)
*/
class M31 final : public BaseField<M31, Scalar>,
                                public FFTFriendlyField<M31>
{
public:
    static M31 INV_2;

    uint32 x;

public:
    static M31 zero() { return new_unchecked(0); }
    static M31 one() { return new_unchecked(1); }
    static std::tuple<Scalar, uint32> size() { return {mod, 2}; }
    static M31 random() { return M31{static_cast<uint32>(rand())}; }  // FIXME: random cannot be used in production

    static inline M31 new_unchecked(uint32 x)
    {
        M31 f;
        f.x = x;
        return f;
    }

public:
    M31() { this->x = 0; }
    M31(uint32 v)
    {
        mod_reduce_int(v);
        this->x = v;
    }

    inline M31 operator+(const M31 &rhs) const
    {
        M31 result;
        result.x = (x + rhs.x);
        if (result.x >= mod)
            result.x -= mod;
        return result;
    }

    inline M31 operator*(const M31 &rhs) const
    {
        int64 xx = static_cast<int64>(x) * rhs.x;

        mod_reduce_int(xx);
        if (xx >= mod)
            xx -= mod;

        return new_unchecked(xx);
    }

    inline M31 operator-() const
    {
        uint32 x = (this->x == 0) ? 0 : (mod - this->x);
        return new_unchecked(x);
    }

    inline M31 operator-(const M31 &rhs) const
    {
        return *this + (-rhs);
    }

    bool operator==(const M31 &rhs) const
    {
        return this->x == rhs.x;
    };

    // using default exp implementation in BaseField without any override

    void to_bytes(uint8* output) const
    {
        memcpy(output, this, sizeof(*this));
    };

    static int byte_length()
    {
        return 4;
    }

    void from_bytes(const uint8* input)
    {
        memcpy(this, input, 4);
        mod_reduce_int(x);
        if (x >= mod) x -= mod;
    };

    friend std::ostream &operator<<(std::ostream &os, const M31 &f)
    {
        os.write((char *)&f.x, sizeof(f.x));
        return os;
    }

    friend std::istream &operator>>(std::istream &is, M31 &f)
    {
        // TODO: FIX INCORRECT READING
        // the input file uses 256 bits to represent a field element
        uint32 repeat = 32 / sizeof(f.x);
        for (uint32 i = 0; i < repeat; i++)
        {
            is.read((char *)&f.x, sizeof(f.x));
        }
        f.x = 1;
        return is;
    }

    static M31 default_rand_sentinel()
    {
        // FIXME: is this a reasonable value?
        return new_unchecked(4294967295 - 1);
    }

    inline bool is_valid() const
    {
        return x < ((uint32) mod);
    }
};

#ifdef __ARM_NEON
    typedef uint32x4_t DATA_TYPE;
#else
    typedef __m256i DATA_TYPE;
#endif

class PackedM31 final : public BaseField<PackedM31, Scalar>,
                        public FFTFriendlyField<PackedM31>
{
public:
    static PackedM31 INV_2;

    DATA_TYPE x;

    static PackedM31 zero();
    static PackedM31 one();

    static std::tuple<Scalar, uint32> size()
    {
        Scalar s;
        s = mod;
        return {s, 2};
    }

    static PackedM31 random();
    static PackedM31 random_bool();

    inline static PackedM31 new_unchecked(const DATA_TYPE &x);
    inline static PackedM31 pack(const M31 *fs);
    inline static size_t pack_size();


public:
    PackedM31();
    PackedM31(const PackedM31 &x);

    PackedM31(uint32 xx);
    PackedM31(const DATA_TYPE &xx);

    inline DATA_TYPE reduce_sum(DATA_TYPE x) const;
    inline PackedM31 operator+(const PackedM31 &rhs) const;
    inline PackedM31 operator+(const M31 &rhs_single) const;
    inline PackedM31 operator*(const PackedM31 &rhs) const;
    inline PackedM31 operator*(const M31 &rhs_single) const;
    inline PackedM31 operator-() const;
    inline PackedM31 operator-(const PackedM31 &rhs) const;
    inline void operator+=(const PackedM31 &rhs);
    inline bool operator==(const PackedM31 &rhs) const;

    void to_bytes(uint8 *output) const
    {
        memcpy(output, this, sizeof(*this));
    }
    void from_bytes(const uint8* input);

    M31 sum_packed() const {
        int *x = (int *)&this->x;
        int sum_x = 0;
        for (int i = 0; i < 8; i++)
        {
            sum_x += *x;
            x++;
        }
        return M31::new_unchecked(sum_x);
    }
    
    static PackedM31 pack_full(const M31 &f);
    std::vector<M31> unpack() const;
};

// Input: a vector of field elements, length h
// Output: a vector of packed field elements, length ceil(h / 8)
//
inline std::vector<PackedM31> pack_field_elements(const std::vector<M31> &fs);
inline std::vector<M31> unpack_field_elements(const std::vector<PackedM31> &pfs)
{
    std::vector<M31> fs;
    fs.reserve(pfs.size() * PackedM31::pack_size());
    for (const auto &pf : pfs)
    {
        int *x = (int *)&pf.x;
        for (uint32 i = 0; i < 8; i++)
        {
            fs.emplace_back(M31::new_unchecked(*x));
            x++;
        }
    }
    return fs;
}

#ifdef __ARM_NEON
const int vectorize_size = 2;
#else
const int vectorize_size = 1;
#endif

class VectorizedM31 final : public BaseField<VectorizedM31, Scalar>,
                                        public FFTFriendlyField<VectorizedM31>
{
    public:
    typedef M31 primitive_type;
    PackedM31 elements[vectorize_size];

    static VectorizedM31 INV_2;
    static VectorizedM31 zero()
    {
        VectorizedM31 z;
        for (int i = 0; i < vectorize_size; i++)
        {
            z.elements[i] = PackedM31::zero();
        }
        return z;
    }
 
    static VectorizedM31 one();

    static std::tuple<Scalar, uint32> size()
    {
        auto s = mod;
        return {s, 2};
    }

    static VectorizedM31 random()
    {
        VectorizedM31 r;
        for (int i = 0; i < vectorize_size; i++)
        {
            r.elements[i] = PackedM31::random();
        }
        return r;
    }

    static VectorizedM31 random_bool()
    {
        VectorizedM31 r;
        for (int i = 0; i < vectorize_size; i++)
        {
            r.elements[i] = PackedM31::random_bool();
        }
        return r;
    }

    inline static VectorizedM31 new_unchecked(const DATA_TYPE &x)
    {
        VectorizedM31 r;
        for (int i = 0; i < vectorize_size; i++)
        {
            r.elements[i] = PackedM31::new_unchecked(x);
        }
        return r;
    }
    
    static VectorizedM31 pack_full(const M31 &f);
    static std::vector<VectorizedM31> pack_field_elements(const std::vector<M31> &fs);

public:   

    VectorizedM31() {};

    VectorizedM31(uint32 xx)
    {
        mod_reduce_int(xx);
        for (int i = 0; i < vectorize_size; i++)
        {
            elements[i] = PackedM31(xx);
        }
    }

    VectorizedM31(const VectorizedM31 &x)
    {
        for (int i = 0; i < vectorize_size; i++)
        {
            elements[i] = x.elements[i];
        }
    }

    inline VectorizedM31 operator+(const VectorizedM31 &rhs) const
    {
        VectorizedM31 result;
        for (int i = 0; i < vectorize_size; i++)
        {
            result.elements[i] = elements[i] + rhs.elements[i];
        }
        return result;
    }

    inline VectorizedM31 operator*(const VectorizedM31 &rhs) const
    {
        VectorizedM31 result;
        for (int i = 0; i < vectorize_size; i++)
        {
            result.elements[i] = elements[i] * rhs.elements[i];
        }
        return result;
    }

    inline VectorizedM31 operator*(const M31 &rhs) const
    {
        VectorizedM31 result;
        for (int i = 0; i < vectorize_size; i++)
        {
            result.elements[i] = elements[i] * PackedM31::pack_full(rhs);
        }
        return result;
    }

    inline VectorizedM31 operator*(const int &rhs) const
    {
        VectorizedM31 result;
        for (int i = 0; i < vectorize_size; i++)
        {
            result.elements[i] = elements[i] * PackedM31::pack_full(rhs);
        }
        return result;
    }

    inline VectorizedM31 operator+(const M31 &rhs) const
    {
        VectorizedM31 result;
        for (int i = 0; i < vectorize_size; i++)
        {
            result.elements[i] = elements[i] + PackedM31::pack_full(rhs);
        }
        return result;
    }

    inline VectorizedM31 operator+(const int &rhs) const
    {
        VectorizedM31 result;
        for (int i = 0; i < vectorize_size; i++)
        {
            result.elements[i] = elements[i] + PackedM31::pack_full(rhs);
        }
        return result;
    }

    inline void operator += (const VectorizedM31 &rhs)
    {
        for (int i = 0; i < vectorize_size; i++)
        {
            elements[i] += rhs.elements[i];
        }
    }

    inline VectorizedM31 operator-() const
    {
        VectorizedM31 r;
        for (int i = 0; i < vectorize_size; i++)
        {
            r.elements[i] = -elements[i];
        }
        return r;
    }

    inline VectorizedM31 operator-(const VectorizedM31 &rhs) const
    {
        VectorizedM31 r;
        for (int i = 0; i < vectorize_size; i++)
        {
            r.elements[i] = elements[i] - rhs.elements[i];
        }
        return r;
    }

    inline VectorizedM31 operator-(const M31 &rhs) const
    {
        VectorizedM31 result;
        for (int i = 0; i < vectorize_size; i++)
        {
            result.elements[i] = elements[i] - PackedM31::pack_full(rhs);
        }
        return result;
    }

    bool operator==(const VectorizedM31 &rhs) const
    {
        for (int i = 0; i < vectorize_size; i++)
        {
            if (!(elements[i] == rhs.elements[i]))
            {
                return false;
            }
        }
        return true;
    }

    void to_bytes(uint8 *output) const
    {
        for (int i = 0; i < vectorize_size; i++)
        {
            elements[i].to_bytes(output + i * sizeof(PackedM31));
        }
    }

    void from_bytes(const uint8 *input)
    {
        for (int i = 0; i < vectorize_size; i++)
        {
            elements[i].from_bytes(input + i * sizeof(PackedM31));
        }
    }

    std::vector<M31> unpack() const {
        std::vector<M31> result;
        for (int i = 0; i < vectorize_size; i++)
        {
            std::vector<M31> unpacked = elements[i].unpack();
            result.insert(result.end(), unpacked.begin(), unpacked.end());
        }
        return result;
    }

    static size_t pack_size() {
        return vectorize_size * PackedM31::pack_size();
    }
};

M31 M31::INV_2 = (1 << 30);
PackedM31 PackedM31::INV_2 = PackedM31(1 << 30);


} // namespace gkr::M31_field




#ifdef __ARM_NEON
#include "M31_neon.tcc"
#else
#include "M31_avx.tcc"
#endif
