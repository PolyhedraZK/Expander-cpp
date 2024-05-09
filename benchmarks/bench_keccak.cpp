#include "field/M31.hpp"
#include "LinearGKR/LinearGKR.hpp"
#include <thread>
#include <utility>
#include <unistd.h>
#include <mutex>
#include <string.h>
#include <stdio.h>
#include <sched.h>

using namespace gkr;
using F = gkr::M31_field::VectorizedM31;
using F_primitive = gkr::M31_field::M31;

const bool debug = false;
const bool set_print = debug;

const char* filename_mul = "data/ExtractedCircuitMul.txt"; 
const char* filename_add = "data/ExtractedCircuitAdd.txt"; 
// returns the proving time and number of keccaks proved

int num_thread;
Circuit<F, F_primitive> *circuits;

void load_circuit(int i)
{
    circuits[i] = Circuit<F, F_primitive>::load_extracted_gates(filename_mul, filename_add);
    circuits[i].set_random_boolean_input();
    circuits[i].evaluate();
}

std::pair<float, int> bench_func(int thread_id, Config &local_config)
{
    Prover<F, F_primitive> prover(local_config);
    prover.prepare_mem(circuits[thread_id]); // exclude this because we can reuse the memory for different instances once allocated
    auto t0 = std::chrono::high_resolution_clock::now();
    prover.prove(circuits[thread_id]);
    auto t1 = std::chrono::high_resolution_clock::now();
    float proving_time = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    const int circuit_copy_size = 8;
    return std::make_pair(proving_time, gkr::M31_field::PackedM31::pack_size() * gkr::M31_field::vectorize_size * circuit_copy_size);
}

int main(int argc, char* argv[])
{
    if (!debug)
    {        
        if (argc <= 1)
        {
            num_thread = 4;
            std::cout << "Use ./keccak_benchmark number_of_threads. Default to 4." << std::endl;        
        }
        else 
        {
            num_thread = atoi(argv[1]);
            if (num_thread == 0)
            {
                std::cout << "Argumemt #1 number_of_threads is incorrect." << std::endl;
                return 1;
            }
        }
    }
    else 
    {
        num_thread = 1;
    }
    std::cout << "Benchmarking with " << num_thread << " threads" << std::endl;

    circuits = new Circuit<F, F_primitive>[num_thread];

    Config local_config;
    printf("Default parallel repetition config %d\n", local_config.get_num_repetitions());
    std::vector<std::thread> threads(num_thread);
    int *partial_proofs = new int[num_thread];
    memset(partial_proofs, 0, sizeof(partial_proofs));
    auto start_time = std::chrono::high_resolution_clock::now();
    std::mutex m;
    for(int i = 0; i < num_thread; i++)
    {
        load_circuit(i);
    }
    for (int i = 0; i < num_thread; i++)
    {
        threads[i] = std::thread([&m, &partial_proofs, &local_config, i](){
            while(true) {
                auto [proving_time, num_proofs] = bench_func(i, local_config);
                partial_proofs[i] += num_proofs;
                if (debug) {
                    break;
                }
            }
        });
    }
    printf("Circuit loaded!\n");
    printf("We are now calculating average throughput, please wait for 1 minutes\n");
    while(!debug) {
        std::this_thread::sleep_for(std::chrono::milliseconds(60000)); // 1 minute per sample
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();
        if (duration == 0) {
            continue;
        }
        int total_proofs = 0;
        for(int i = 0; i < num_thread; i++)
            total_proofs += partial_proofs[i];
        auto throughput = total_proofs / duration;
        std::cout << "Throughput: " << throughput << " keccaks/s" << std::endl;
    }
    for (auto& thread : threads) {
        thread.join();
    }

    delete[] circuits;
    delete[] partial_proofs;

    return 0;
}
