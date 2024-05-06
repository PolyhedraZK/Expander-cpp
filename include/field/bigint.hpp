#pragma once

#include<iostream>
#include<gmp.h>

#include "../utils/types.hpp"

namespace gkr
{

// a simple wrapper of gmp mpz_t
class BigInt 
{
public:
    mpz_t data;

    BigInt() 
    {
        mpz_init(data);
        mpz_set_ui(data, 0);
    }

    BigInt(uint64 x)
    {
        mpz_init(data);
        mpz_set_ui(data, x);
    }

    explicit BigInt(const char* s, int base=10) 
    {
        mpz_init(data);
        mpz_set_str(data, s, base);
    }

    explicit BigInt(const mpz_t src) 
    {
        mpz_init(data);
        mpz_set(data, src);
    }

    BigInt(const BigInt& other)
    {
        mpz_init(data);
        mpz_set(data, other.data);
    }

    BigInt operator=(const BigInt& other) 
    {
        mpz_set(data, other.data);
        return *this;
    }

    bool operator==(const BigInt& rhs) const
    {
        return (mpz_cmp(data, rhs.data) == 0);
    }

    bool operator<(const BigInt& rhs) const
    {
        return (mpz_cmp(data, rhs.data) < 0);
    }

    bool operator!=(const BigInt& rhs) const 
    {
        return !(*this == rhs);
    }
    
    BigInt operator+(const BigInt& rhs) const 
    {
        BigInt bi;
        mpz_add(bi.data, data, rhs.data);
        return bi;
    }

    BigInt operator-(const BigInt& rhs) const
    {
        BigInt bi;
        mpz_sub(bi.data, data, rhs.data);
        return bi;
    }

    BigInt operator*(const BigInt& rhs) const
    {
        BigInt bi;
        mpz_mul(bi.data, data, rhs.data);
        return bi;
    }

    BigInt operator&(const BigInt& rhs)
    {
        BigInt bi;
        mpz_and(bi.data, data, rhs.data);
        return bi;
    }

    BigInt operator<<(uint32 x) const
    {
        BigInt bi;
        mpz_mul_2exp(bi.data, data, x);
        return bi;
    }

    BigInt operator>>(uint32 x)
    {
        BigInt bi;
        mpz_fdiv_q_2exp(bi.data, data, x);
        return bi;
    }
    
    ~BigInt()
    {
        mpz_clear(data);
    }
};
    
} // namespace LinearGKR
