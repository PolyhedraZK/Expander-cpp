#pragma once
#include <cassert>
#include "field/M31.hpp"

// polynomial commitment type
enum PCType 
{
    Raw,
    KZG,
    Orion,
    FRI,
};

enum FieldType 
{
    M31,
    BabyBear,
    BN254,
};

// fiat-shamir hash type
// enum FSHashType 
// {
//     SHA256,
//     Keccak256,
//     Poseidon,
//     Animoe,
//     MIMC7,
// };

class Config
{
public:    
    // === Field ===
    FieldType field_type;
    int field_size;
    
    int m31_parallel_factor;
    int m31_vectorize_size;
    // ============

    // === PC === 
    PCType pc_type;
    // ==========

    // === security ===
    int security_bits;
    int grinding_bits;
    // ================

    // === MPI ===
    size_t mpi_world_size;
    // ===========

    Config()
    {
        set_field(M31);
        set_pc(PCType::Raw);
        
        security_bits = 100;
        grinding_bits = 10;
        mpi_world_size = 1;
    }

    void set_field(FieldType field_type_)
    {
        field_type = field_type_;

        switch (field_type)
        {
            case M31:
                field_size = 31;
                m31_parallel_factor = 16;
                m31_vectorize_size =  m31_parallel_factor / gkr::M31_field::PackedM31::pack_size();
                break;
            case BabyBear:
                field_size = 31;
                throw("BabyBear is not supported yet.");
                break;
            case BN254:
                field_size = 254;
                break;
            default:
                throw("Unknown field");
                break;
        }
    }

    void set_pc(PCType pc_type_)
    {
        pc_type = pc_type_;
        switch (pc_type)
        {
        case KZG:
            if (field_type != BN254)
            {
                throw("KZG must be used with BN254 Scalar Field");
            }
            break;
        
        default:
            break;
        }
    }

    void set_mpi_size(size_t n)
    {
        mpi_world_size = n;
    }
};

