#pragma once

#include "mpi.h"
#include "poly.hpp"

namespace gkr
{

template<typename F>
class RawCommitment
{
public:
    std::vector<F> poly_vals;

    uint32 size() const
    {
        return sizeof(F) * poly_vals.size();
    }

    void to_bytes(uint8* output) const
    {
        uint32 idx = 0;
        for (const F &x: poly_vals)
        {
            x.to_bytes(output + idx * sizeof(F));
            idx++;
        }
    }

    void from_bytes(const uint8* input, uint32 poly_size)
    {
        uint32 idx = 0;
        poly_vals.resize(poly_size);
        for (uint32 i = 0; i < poly_size; i++)
        {
            poly_vals[i].from_bytes(input + idx);
            idx += sizeof(F);
        }
    }
};

class RawOpening
{
public:
    uint32 size() const
    {
        return 0;
    }

    void to_bytes(uint8* output) const
    {
        return;
    }

    void from_bytes(const uint8* input, uint32 poly_size)
    {
        return; 
    }
};


template<typename F, typename F_primitive>
class RawPC
{
public:
    std::vector<F> poly_vals;

    RawCommitment<F> commit(const std::vector<F> &poly_vals_)
    {
        int world_rank, world_size;
        MPI_Comm_size(MPI_COMM_WORLD, &world_size);
        MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
        
        poly_vals = poly_vals_;

        RawCommitment<F> c;
        std::vector<F> &poly_vals_global = c.poly_vals;
        if (world_rank == 0)
        {
            poly_vals_global.resize(poly_vals_.size() * world_size);
        }

        MPI_Gather(poly_vals.data(), sizeof(F) * poly_vals.size(), MPI_CHAR, 
            poly_vals_global.data(), sizeof(F) * poly_vals.size(), MPI_CHAR, 0, MPI_COMM_WORLD);

        return c;
    }
    RawOpening open(const std::vector<F_primitive> &x) {return RawOpening();};
    bool verify(const RawCommitment<F> &commitment, const RawOpening &opening, const std::vector<F_primitive> &x, const F &y)
    {
        return eval_multilinear(commitment.poly_vals, x) == y;
    };
};

}