#pragma once

#include <mpi.h>

#include "circuit/circuit.hpp"
#include "configuration/config.hpp"
#include "gkr.hpp"
#include "poly_commit/raw.hpp"

namespace gkr
{

template<typename F, typename F_primitive>
void grind(Transcript<F, F_primitive> &transcript, const Config &local_config)
{
    auto initial_hash = transcript.challenge_fs(256/local_config.field_size);
    uint8 hash_bytes[256 / 8];
    memset(hash_bytes, 0, 256 / 8);
    auto bytes_ptr = 0;
    for(int i = 0; i < (int)initial_hash.size(); i++)
    {
        auto element = initial_hash[i];
        element.to_bytes(&hash_bytes[bytes_ptr]);
        bytes_ptr += ceil(local_config.field_size, 8);
    }

    for(int i = 0; i < (1 << local_config.grinding_bits); i++)
    {
        transcript.hasher.hash(hash_bytes, hash_bytes, 256 / 8);
    }
    transcript.append_bytes(hash_bytes, 256/8);
}

template<typename F, typename F_primitive>
class Prover
{
public:
    int world_rank, world_size;
    const Config &config;
    GKRScratchPad<F, F_primitive> scratch_pad;

public:
    Prover(const Config &config_): config(config_)
    {
        MPI_Comm_size(MPI_COMM_WORLD, &world_size);
        MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
        assert(config.pc_type == PCType::Raw);
    }

    void prepare_mem(const Circuit<F, F_primitive>& circuit)
    {
        scratch_pad.prepare(circuit);
    }

    std::tuple<F, Proof<F>> prove(Circuit<F, F_primitive>& circuit)
    {
        // pc commit        
        RawPC<F, F_primitive> raw_pc;
        RawCommitment<F> commitment = raw_pc.commit(circuit.layers[0].input_layer_vals.evals);
        
        Transcript<F, F_primitive> transcript;
        if (world_rank == 0)
        {
            transcript.append_bytes(reinterpret_cast<uint8*>(commitment.poly_vals.data()), commitment.size());
            // grinding
            grind(transcript, config);
        }
        
        // rnd gate
        circuit.fill_rnd_gate(transcript);
        circuit.evaluate();

        // gkr
        auto t = gkr_prove<F, F_primitive>(circuit, scratch_pad, transcript, config);
        
        // open
        auto claimed_v = std::get<0>(t);
        auto rz1 = std::get<1>(t);
        auto rz2 = std::get<2>(t);
        auto rw1 = std::get<3>(t);
        auto rw2 = std::get<4>(t);
        
        rz1.insert(rz1.end(), rw1.begin(), rw1.end());
        rz2.insert(rz2.end(), rw2.begin(), rw2.end());
        RawOpening opening1 = raw_pc.open(rz1);
        RawOpening opening2 = raw_pc.open(rz2);

        if (world_rank == 0)
        {
            transcript.append_bytes(reinterpret_cast<uint8*>(&opening1), opening1.size());
            transcript.append_bytes(reinterpret_cast<uint8*>(&opening2), opening2.size());
        }
    
        return {claimed_v, transcript.proof};
    }
};

class Verifier
{
public:
    const Config &config;

public:
    Verifier(const Config &config_): config(config_)
    {
        
    }

    template<typename F, typename F_primitive>
    bool verify(Circuit<F, F_primitive>& circuit, const F& claimed_v, Proof<F>& proof)
    {        
        // get commitment
        uint32 poly_size = circuit.layers[0].input_layer_vals.evals.size() * config.mpi_world_size;
        RawCommitment<F> commitment;
        commitment.from_bytes(proof.bytes_head(), poly_size);
        proof.step(commitment.size());

        Transcript<F, F_primitive> transcript;
        transcript.append_bytes(proof.bytes_head(), commitment.size());
        
        //grinding
        grind(transcript, config);
        proof.step(256 / 8);

        // rnd gate
        circuit.fill_rnd_gate(transcript);

        // gkr
        auto t = gkr_verify<F, F_primitive>(circuit, claimed_v, transcript, proof, config);
        
        // verify pc
        bool verified = std::get<0>(t);
        auto rz1 = std::get<1>(t);
        auto rz2 = std::get<2>(t);
        auto rw1 = std::get<3>(t);
        auto rw2 = std::get<4>(t);
        auto claimed_v1 = std::get<5>(t);
        auto claimed_v2 = std::get<6>(t);

        rz1.insert(rz1.end(), rw1.begin(), rw1.end());
        rz2.insert(rz2.end(), rw2.begin(), rw2.end());

        RawOpening opening1;
        opening1.from_bytes(proof.bytes_head(), poly_size);
        proof.step(opening1.size());

        RawOpening opening2;
        opening2.from_bytes(proof.bytes_head(), poly_size);
        proof.step(opening2.size());
        
        RawPC<F, F_primitive> raw_pc;
        verified &= raw_pc.verify(commitment, opening1, rz1, claimed_v1);
        verified &= raw_pc.verify(commitment, opening2, rz2, claimed_v2);
    
        return verified;
    }
};

};
