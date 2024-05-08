#include "field/M31.hpp"
#include "LinearGKR/gkr.hpp"
#include <thread>
#include <utility>
#include <unistd.h>
#include <mutex>
#include "configuration/config.hpp"
using namespace gkr;
using F = gkr::M31_field::VectorizedM31;
using F_primitive = gkr::M31_field::M31;

const bool debug = false;
const bool set_print = debug;

const char* filename_mul = "data/ExtractedCircuitMul.txt"; 
const char* filename_add = "data/ExtractedCircuitAdd.txt"; 
// returns the proving time and number of keccaks proved

const int num_thread = debug ? 1 : 16;
Circuit<F, F_primitive> circuits[num_thread];

GKRScratchPad<F, F_primitive> scratch_pad[num_thread][3];

void load_circuit(int i)
{
    circuits[i] = Circuit<F, F_primitive>::load_extracted_gates(filename_mul, filename_add);
    std::cout << "Number Mul Gate: " << circuits[i].nb_mul_gates() << std::endl;
    std::cout << "Number Add Gate: " << circuits[i].nb_add_gates() << std::endl;
    circuits[i].set_random_boolean_input();
    circuits[i].evaluate();
    scratch_pad[i][0].prepare(circuits[i]);
    scratch_pad[i][1].prepare(circuits[i]);
    scratch_pad[i][2].prepare(circuits[i]);
    std::cout << "Circuit Evaluated" << std::endl;
}

std::pair<float, int> bench_func(int thread_id, Config &local_config)
{
    auto t0 = std::chrono::high_resolution_clock::now();
    Transcript<F, F_primitive> transcript;
    auto claimed_value = std::get<0>(gkr_prove<F, F_primitive>(circuits[thread_id], scratch_pad[thread_id], transcript, set_print));
    auto t1 = std::chrono::high_resolution_clock::now();
    float proving_time = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    const int circuit_copy_size = 8;
    return std::make_pair(proving_time, gkr::M31_field::PackedM31::pack_size() * gkr::M31_field::vectorize_size * circuit_copy_size);
}

int main()
{
    Config local_config;
    printf("Default parallel repetition config %d\n", local_config.get_num_repetitions());
    std::vector<std::thread> threads;
    int partial_proofs[num_thread];
    memset(partial_proofs, 0, sizeof(partial_proofs));
    auto start_time = std::chrono::high_resolution_clock::now();
    std::mutex m;
    for(int i = 0; i < num_thread; i++)
    {
        load_circuit(i);
    }
    for (int i = 0; i < num_thread; i++)
    {
        threads.push_back(std::thread([&m, &partial_proofs, &local_config, i](){
            printf("Thread %d started\n", i);
            while(true) {
                auto [proving_time, num_proofs] = bench_func(i, local_config);
                partial_proofs[i] += num_proofs;
                if (debug) {
                    break;
                }
            }
        }));
    }
    while(!debug) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
    return 0;
}
