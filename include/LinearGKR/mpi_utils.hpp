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

} // namespace gkr
