
#include <iostream>
#include <gtest/gtest.h>
#include <libff/algebra/curves/bn128/bn128_fields.hpp>

#include "field/bn254_fr.hpp"
#include "poly_commit/poly.hpp"
#include "poly_commit/kzg.hpp"
#include "poly_commit/bi-kzg/bi-kzg.hpp"


int main()
{
    using namespace gkr;
    using F = bn254fr::BN254_Fr;

    bn254fr::init();

    uint32 deg = 1024;
    BivariatePoly<F> poly{deg, deg};
    poly.random_init();
    F x = F::random(), y = F::random();

    BiKZG bi_kzg;
    
    auto t0 = std::chrono::high_resolution_clock::now();
    BiKZGSetup setup;
    bi_kzg.setup_gen(setup, deg, deg, F::random(), F::random());
    auto t1 = std::chrono::high_resolution_clock::now();
    float running_time = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    std::cout << "Running time for setup is: " << running_time << " ms" << std::endl;
    

    t0 = std::chrono::high_resolution_clock::now();
    BiKZGCommitment commitment;
    bi_kzg.commit(commitment, poly, setup);
    t1 = std::chrono::high_resolution_clock::now();
    running_time = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    std::cout << "Running time for commit is: " << running_time << " ms" << std::endl;

    t0 = std::chrono::high_resolution_clock::now();
    BiKZGOpening opening;
    bi_kzg.open(opening, poly, x, y, setup);
    std::cout << opening.v.mcl_data << std::endl;
    t1 = std::chrono::high_resolution_clock::now();
    running_time = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    std::cout << "Running time for open is: " << running_time << " ms" << std::endl;

    t0 = std::chrono::high_resolution_clock::now();
    bool verified = bi_kzg.verify(setup, commitment, x, y, opening);    
    t1 = std::chrono::high_resolution_clock::now();
    running_time = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    std::cout << "Running time for verify is: " << running_time << " ms" << std::endl;


}