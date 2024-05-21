#pragma once

#include <mcl/bn256.hpp>

#include "bigint.hpp"
#include "basefield.hpp"

namespace gkr::bn254fr
{

using namespace mcl::bn;
void init()
{
    initPairing(mcl::BN_SNARK1);
}

const BigInt mod = BigInt("21888242871839275222246405745257275088548364400416034343698204186575808495617");

typedef BigInt Scalar;

// a wrapper around mcl fr
class BN254_Fr: 
    public BaseField<BN254_Fr, Scalar>,
    public FFTFriendlyField<BN254_Fr>
{

public:
    typedef bn254fr::Scalar Scalar;

    static BN254_Fr ZERO;
    static BN254_Fr ONE;
    static BN254_Fr INV_2;

    static BN254_Fr TWO_ADIC_ROOT_OF_UNITY;
    static uint32 TWO_ADICITY;

    Fr mcl_data;

public:
    inline static BN254_Fr zero() { return ZERO; }
    inline static BN254_Fr one() {return ONE; }
    inline static BN254_Fr inv_2() { return INV_2; }
    inline static std::tuple<BN254_Fr, uint32> two_adic_root_of_unity() { return {TWO_ADIC_ROOT_OF_UNITY, TWO_ADICITY}; }

    inline static std::tuple<Scalar, uint32> size()
    {
        return {mod, 1};
    }

    inline static BN254_Fr random()
    {
        BN254_Fr f;
        f.mcl_data.setByCSPRNG();
        return f;
    }


public:
    BN254_Fr()
    {
    }

    BN254_Fr(uint64 x)
    {
        mcl_data = Fr(x);
    }

    inline BN254_Fr operator+(const BN254_Fr &rhs) const
    {
        BN254_Fr f;
        Fr::add(f.mcl_data, mcl_data, rhs.mcl_data);
        return f;
    }

    inline BN254_Fr operator*(const BN254_Fr &rhs) const
    {
        BN254_Fr f;
        Fr::mul(f.mcl_data, mcl_data, rhs.mcl_data);
        return f;    
    }

    inline BN254_Fr operator-() const
    {
        BN254_Fr f;
        Fr::neg(f.mcl_data, mcl_data);
        return f;
    }

    inline BN254_Fr operator-(const BN254_Fr &rhs) const
    {
        BN254_Fr f;
        Fr::sub(f.mcl_data, mcl_data, rhs.mcl_data);
        return f;
    }

    inline bool operator==(const BN254_Fr &rhs) const
    {
        return mcl_data == rhs.mcl_data;
    }

    inline void operator+=(const BN254_Fr &rhs) 
    { 
        Fr::add(mcl_data, mcl_data, rhs.mcl_data);
    }

    inline void operator-=(const BN254_Fr &rhs) 
    {
        Fr::sub(mcl_data, mcl_data, rhs.mcl_data);
    }

    inline void operator*=(const BN254_Fr &rhs) 
    {
        Fr::mul(mcl_data, mcl_data, rhs.mcl_data);
    }

    void to_bytes(uint8 *output) const
    {
        memset(output, 0, 32);
        mcl_data.getLittleEndian(output, 32);
    }

    void from_bytes(const uint8 *input)
    {
        mcl_data.setLittleEndianMod(input, 32);
    }
};

bool initialized = []() {init(); return true; }();
BN254_Fr BN254_Fr::ZERO = BN254_Fr{0};
BN254_Fr BN254_Fr::ONE = BN254_Fr{1};
BN254_Fr BN254_Fr::INV_2 = BN254_Fr{2}.inv();
BN254_Fr BN254_Fr::TWO_ADIC_ROOT_OF_UNITY = [](){
    BN254_Fr x;
    x.mcl_data.setStr("19103219067921713944291392827692070036145651957329286315305642004821462161904", 10);
    assert(x.mcl_data.isValid());
    return x;
}();
uint32 BN254_Fr::TWO_ADICITY = 28;

} // namespace gkr::bn254fr