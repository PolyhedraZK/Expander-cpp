#ifndef GKR_BASEFIELD_H
#define GKR_BASEFIELD_H

#include <vector>
#include "../utils/types.hpp"

namespace gkr
{

    // For the design pattern, see "Curiously recurring template pattern"

    // F: the actual field type
    // Scalar refers to the scalar type used for exponentiation F^Scalar
    // Scalar must be able to hold the size of the field
    template <typename F, typename Scalar>
    class BaseField
    {
    public:
        // addition and multiplication identity
        static F zero();
        static F one();
        inline static F inv_2() { return F::INV_2; }
        static std::tuple<Scalar, uint32> size();
        static F random();

        // intepret bytes as a large number n in little endian, and compute n mod p as a field element
        // WARNING: somewhat inefficient, for now this is only used in mimc initialization
        static F from_bytes_mod_order(const std::vector<uint8> &bytes)
        {
            F v = F::zero();
            F shift = F(256);
            for (auto it = bytes.rbegin(); it != bytes.rend(); it++)
            {
                v = v * shift + F(*it);
            }
            return v;
        }

        F operator+(const F &rhs) const;

        F operator*(const F &rhs) const;

        F operator-() const;

        F operator-(const F &rhs) const;

        bool operator==(const F &rhs) const;
        bool operator!=(const F &rhs) {return !(*this == rhs);}

        void operator+=(const F &rhs) { *static_cast<F *>(this) = *static_cast<F *>(this) + rhs; }
        void operator-=(const F &rhs) { *static_cast<F *>(this) = *static_cast<F *>(this) - rhs; }
        void operator*=(const F &rhs) { *static_cast<F *>(this) = *static_cast<F *>(this) * rhs; }

        // default exp algorithm
        F exp(const Scalar &s) const
        {
            Scalar ss = s;
            F r = F::one();
            F t = *reinterpret_cast<const F *>(this);

            while (ss != 0)
            {
                Scalar b = ss & 1;
                if (b == 1)
                {
                    r *= t;
                }
                t = t * t;
                ss = ss >> 1;
            }
            return r;
        }

        // default inv algorithm
        F inv() const
        {
            Scalar p;
            uint32 n;
            std::tie(p, n) = F::size();

            // x^{p^n - 2}
            Scalar e = p;
            for (uint32 i = 0; i < n - 1; i++)
            {
                e = e * p;
            }
            e = e - 2;
            return exp(e);
        };

        // serialization
        void to_bytes(uint8 *output) const;
        void from_bytes(const uint8 *input);
    };


    // TODO: Make the inheritance hierarchy more reasonable
    template<typename F>
    class FFTFriendlyField
    {
    public:
        // return the primitive_root_of_unity and its order
        static std::tuple<F, uint32> max_order_primitive_root_of_unity();
    };
}



#endif