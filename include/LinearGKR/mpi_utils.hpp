#pragma once

#include "fiat_shamir/transcript.hpp"
#include "sumcheck_common.hpp"

namespace gkr
{

template<typename F, typename F_primitive>
void _mpi_rnd_combine(
    const F &local_v,
    F &global_v,
    const std::vector<F_primitive> &rand_coefs
)
{
    int world_rank, world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    //TODO: use customized types/ops and mpi reduce
    std::vector<F> local_values(world_size);
    MPI_Gather(&local_v, sizeof(F), MPI_CHAR, local_values.data(), sizeof(F), MPI_CHAR, 0, MPI_COMM_WORLD);

    if(world_rank == 0)
    {
        assert(rand_coefs.size() == world_size);

        global_v = F::zero();
        for (uint32 i = 0; i < world_size; i++)
        {
            global_v += local_values[i] * rand_coefs[i];
        }
    }
}


template<typename F, typename F_primitive>
void _mpi_rnd_combine(
    const std::vector<F> &local_v,
    std::vector<F> &global_v,
    const F_primitive *comb_coef
)
{
    int world_rank, world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    
    uint32 local_size = local_v.size();
    if (world_rank == 0)
    {
        global_v.resize(local_size * world_size);
    }
    
    MPI_Gather(local_v.data(), sizeof(F) * local_size, MPI_CHAR, 
        global_v.data(), sizeof(F) * local_size, MPI_CHAR, 0, MPI_COMM_WORLD);

    if(world_rank == 0)
    {
        for (uint32 i = 0; i < local_size; i++)
        {
            global_v[i] = global_v[i] * comb_coef[0];
        }
        

        for (uint32 j = 1; j < world_size; j++) 
        {
            for (uint32 i = 0; i < local_size; i++)
            {
                uint32 idx = j * local_size + i;
                global_v[i] += global_v[idx] * comb_coef[j];
            } 
        }
        global_v.resize(local_size);
    }

}



template<typename F>
void _mpi_sum_combine(
    const F &local_v,
    F &global_v
)
{
    int world_rank, world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    
    //TODO: use customized types/ops and mpi reduce
    std::vector<F> local_values(world_size);
    MPI_Gather(&local_v, sizeof(F), MPI_CHAR, local_values.data(), sizeof(F), MPI_CHAR, 0, MPI_COMM_WORLD);

    if(world_rank == 0)
    {
        for (uint32 i = 0; i < world_size; i++)
        {
            global_v += local_values[i];
        }
    }
}


template<typename F>
void _mpi_sum_combine(
    const std::vector<F> &local_v,
    std::vector<F> &global_v
)
{
    int world_rank, world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    
    //TODO: use customized types/ops and mpi reduce
    uint32 local_size = local_v.size();
    if (world_rank == 0)
    {
        global_v.resize(local_size * world_size);
    }

    MPI_Gather(local_v.data(), sizeof(F) * local_size, MPI_CHAR, 
        global_v.data(), sizeof(F) * local_size, MPI_CHAR, 0, MPI_COMM_WORLD);

    if(world_rank == 0)
    {
        for (uint32 j = 1; j < world_size; j++) 
        {
            for (uint32 i = 0; i < local_size; i++)
            {
                uint32 idx = j * local_size + i;
                global_v[i] += global_v[idx];
            } 
        }
        global_v.resize(local_size);
    }

}

} // namespace gkr
