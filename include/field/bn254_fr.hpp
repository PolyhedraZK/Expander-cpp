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

namespace gkr
{

using namespace mcl::bn;

const G1 G1_ZERO = [](){
    G1 x;
    x.clear();
    return x;
}();

const G1 G1_ONE = [](){
    G1 x; 
    x.set(1, 2); 
    assert(x.isValid());
    return x;
}();

const G2 G2_ZERO = [](){
    G2 x;
    x.clear();
    return x;
}();

const G2 G2_ONE = [](){
    G2 x; 
    Fp2 xx, yy;
    xx.a.setStr("10857046999023057135944570762232829481370756359578518086990519993285655852781", 10);    
    xx.b.setStr("11559732032986387107991004021392285783925812861821192530917403151452391805634", 10);
    yy.a.setStr("8495653923123431417604973247489272438418190587263600148770280649306958101930", 10);
    yy.b.setStr("4082367875863433681332203403145435568316851327593401208105741076214120093531", 10);
    x.set(xx, yy);
    assert(x.isValid());
    return x;
}();

const GT GT_ONE  = [](){GT x; x.setOne(); return x;}();

}