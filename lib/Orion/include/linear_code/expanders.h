#pragma once

#include "parameter.h"
#include <vector>
#include <linear_gkr/prime_field.h>
class graph
{
public:
    int degree;
    std::vector<std::vector<long long>> neighbor;
    std::vector<std::vector<long long>> r_neighbor;
    std::vector<std::vector<prime_field::field_element>> weight;
    std::vector<std::vector<prime_field::field_element>> r_weight;
    long long L, R;
};

extern graph C[100], D[100];

inline graph generate_random_expander(long long L, long long R, long long d)
{
    graph ret;
    ret.degree = d;
    ret.neighbor.resize(L);
    ret.weight.resize(L);

    ret.r_neighbor.resize(R);
    ret.r_weight.resize(R);
    for(long long i = 0; i < L; ++i)
    {
        ret.neighbor[i].resize(d);
        ret.weight[i].resize(d);
        for(long long j = 0; j < d; ++j)
        {
            long long target = rand() % R;
            prime_field::field_element weight = prime_field::random();
            ret.neighbor[i][j] = target;
            ret.r_neighbor[target].push_back(i);
            ret.r_weight[target].push_back(weight);
            ret.weight[i][j] = weight;
        }
    }
    ret.L = L;
    ret.R = R;
    return ret;
}

inline long long expander_init(long long n, int dep = 0)
{
    //random graph
    if(n <= distance_threshold)
    {
        return n;
    }
    else
    {
        C[dep] = generate_random_expander(n, (long long)(alpha * n), cn);
        long long L = expander_init((long long)(alpha * n), dep + 1);
        D[dep] = generate_random_expander(L, (long long)(n * (r - 1) - L), dn);
        return n + L + (long long)(n * (r - 1) - L);
    }
}