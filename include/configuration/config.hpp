#pragma once
#include <cassert>
#include "field/M31.hpp"

enum Polynomial_commitment_type {
    Raw,
    KZG,
    Orion,
    FRI,
};

enum Field_type {
    M31,
    BabyBear,
    BN254,
};

enum FiatShamir_hash_type {
    SHA256,
    Keccak256,
    Poseidon,
    Animoe,
    MIMC7,
};

class Config
{
private:
    int num_repetitions;
    int vectorize_size;

public:
    int field_size;
    int security_bits;
    int grinding_bits;
    int nb_parallel;
    Polynomial_commitment_type PC_type;
    Field_type field_type;
    FiatShamir_hash_type FS_hash;
    void initialize_config()
    {
        security_bits = 100;
        switch (field_type)
        {
        case M31:
            field_size = 31;
            // TODO: set the value of vectorize_size in VectorizedM31
            vectorize_size = nb_parallel / gkr::M31_field::PackedM31::pack_size();
            break;
        case BabyBear:
            field_size = 31;
            break;
        case BN254:
            field_size = 254;
            break;
        default:
            assert(false);
            break;
        }
        
        num_repetitions = (security_bits - grinding_bits) / field_size;
        if ((security_bits - grinding_bits) % field_size != 0)
        {
            num_repetitions++;
        }

        if (PC_type == KZG)
        {
            assert(field_size == BN254);
        }
    }
    Config()
    {
        security_bits = 100;
        grinding_bits = 10;
        nb_parallel = 16;
        PC_type = Raw;
        field_type = M31;
        FS_hash = SHA256;
        initialize_config();
    }
    int get_num_repetitions() const
    {
        return num_repetitions;
    }
};

