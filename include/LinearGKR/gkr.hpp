#pragma once

#include<vector>
#include "scratch_pad.hpp"
#include "sumcheck.hpp"
#include "circuit/circuit.hpp"
#include <map>
#include "configuration/config.hpp"
#include <chrono>

inline int ceil(int a, int b)
{
    if (a % b == 0)
    {
        return a / b;
    }
    return a / b + 1;
}

namespace gkr
{


template<typename F, typename F_primitive>
std::tuple<std::vector<F>, std::vector<std::vector<F_primitive>>, std::vector<std::vector<F_primitive>>> gkr_prove(
    const Circuit<F, F_primitive> &circuit, 
    GKRScratchPad<F, F_primitive> *scratch_pad,
    Transcript<F, F_primitive> &transcript,
    const Config &config,
    bool set_print = false
)
{
    Timing timer;
    timer.set_print(set_print);
    timer.add_timing("start proof");
    uint32 n_layers = circuit.layers.size();
    
    timer.add_timing("out_layer multi linear evals");
    std::vector<std::vector<F_primitive>> rz1, rz2;
    rz1.resize(config.get_num_repetitions());
    rz2.resize(config.get_num_repetitions());
    for (uint32 i = 0; i < circuit.layers.back().nb_output_vars; i++)
    {
        for(int j = 0; j < config.get_num_repetitions(); j++)
        {
            rz1[j].emplace_back(transcript.challenge_f());
            rz2[j].emplace_back(F_primitive::zero());
        }
    }
    F_primitive alpha = F_primitive::one(), beta = F_primitive::zero();
    std::vector<F> claimed_v;
    for(int j = 0; j < config.get_num_repetitions(); j++)
    {
        claimed_v.emplace_back(eval_multilinear(circuit.layers.back().output_layer_vals.evals, rz1[j]));
    }
    timer.report_timing("out_layer multi linear evals");

    for (int i = n_layers - 1; i >= 0; i--)
    {
        timer.add_timing(string("layer " + to_string(i) + " sumcheck layer input size ") + std::to_string(circuit.layers[i].nb_input_vars) + string(" output size ") + std::to_string(circuit.layers[i].nb_output_vars));
        timer.add_timing("layer " + to_string(i) + " sumcheck layer");
        std::tuple<std::vector<std::vector<F_primitive>>, std::vector<std::vector<F_primitive>>> t
            = sumcheck_prove_gkr_layer<F, F_primitive>(circuit.layers[i], rz1, rz2, alpha, beta, transcript, scratch_pad, timer, config); 
        timer.report_timing("layer " + to_string(i) + " sumcheck layer");
        alpha = transcript.challenge_f();
        beta = transcript.challenge_f();
        rz1 = std::get<0>(t);
        rz2 = std::get<1>(t);
        timer.report_timing(string("layer " + to_string(i) + " sumcheck layer input size ") + std::to_string(circuit.layers[i].nb_input_vars) + string(" output size ") + std::to_string(circuit.layers[i].nb_output_vars));
    }
    timer.report_timing("start proof");
    return {claimed_v, rz1, rz2};
}

template<typename F, typename F_primitive>
std::tuple<bool, std::vector<std::vector<F_primitive>>, std::vector<std::vector<F_primitive>>, std::vector<F>, std::vector<F> > gkr_verify(
    const Circuit<F, F_primitive>& circuit,
    const std::vector<F>& claimed_v,
    Transcript<F, F_primitive>& transcript,
    Proof<F> &proof,
    const Config &config
)
{
    uint32 n_layers = circuit.layers.size();
    
    std::vector<std::vector<F_primitive>> rz1, rz2;
    rz1.resize(config.get_num_repetitions());
    rz2.resize(config.get_num_repetitions());
    for (uint32 i = 0; i < circuit.layers.back().nb_output_vars; i++)
    {
        for(int j = 0; j < config.get_num_repetitions(); j++)
        {
            rz1[j].emplace_back(transcript.challenge_f());
            rz2[j].emplace_back(F_primitive::zero());
        }
    }
    F_primitive alpha = F_primitive::one(), beta = F_primitive::zero();
    auto claimed_v1 = claimed_v;
    std::vector<F> claimed_v2;
    for(int j = 0; j < config.get_num_repetitions(); j++)
    {
        claimed_v2.emplace_back(F::zero());
    }

    bool verified = true;
    for (int i = n_layers - 1; i >= 0; i--)
    {
        std::tuple<bool, std::vector<std::vector<F_primitive>>, std::vector<std::vector<F_primitive>>, std::vector<F>, std::vector<F> > t 
            = sumcheck_verify_gkr_layer(circuit.layers[i], rz1, rz2, claimed_v1, claimed_v2, alpha, beta, proof, transcript, config);
        
        verified &= std::get<0>(t);

        alpha = transcript.challenge_f();
        beta = transcript.challenge_f();
        
        rz1 = std::get<1>(t);
        rz2 = std::get<2>(t);
        claimed_v1 = std::get<3>(t);
        claimed_v2 = std::get<4>(t);
    }
    return {verified, rz1, rz2, claimed_v1, claimed_v2};
}

}