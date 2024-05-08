#pragma once

#include "circuit/circuit.hpp"
#include "field/M31.hpp"
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

class Prover
{
public:
    const Config &config;

public:
    Prover(const Config &config_): config(config_)
    {
        assert(config.field_type == Field_type::M31);
        assert(config.FS_hash == FiatShamir_hash_type::SHA256);
        assert(config.PC_type == Polynomial_commitment_type::Raw);
    }

    template<typename F, typename F_primitive>
    std::tuple<F, Proof<F>> prove(const Circuit<F, F_primitive>& circuit)
    {
        GKRScratchPad<F, F_primitive> scratch_pad;
        scratch_pad.prepare(circuit);

        // pc commit
        RawPC<F, F_primitive> raw_pc;
        RawCommitment<F> commitment = raw_pc.commit(circuit.layers[0].input_layer_vals.evals);
        uint8* buffer = (uint8*) scratch_pad.v_evals;
        commitment.to_bytes(buffer);

        Transcript<F, F_primitive> transcript;
        transcript.append_bytes(buffer, commitment.size());

        //grinding
        grind(transcript, config);

        // gkr
        auto t = gkr_prove<F, F_primitive>(circuit, scratch_pad, transcript);
        
        // open
        F claimed_v = std::get<0>(t);
        std::vector<F_primitive> rz1 = std::get<1>(t);
        std::vector<F_primitive> rz2 = std::get<2>(t);
        
        RawOpening opening1 = raw_pc.open(rz1);
        opening1.to_bytes(buffer);
        transcript.append_bytes(buffer, opening1.size());

        RawOpening opening2 = raw_pc.open(rz2);
        opening2.to_bytes(buffer);
        transcript.append_bytes(buffer, opening2.size());
        
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
    bool verify(const Circuit<F, F_primitive>& circuit, const F& claimed_v, Proof<F>& proof)
    {
        GKRScratchPad<F, F_primitive> scratch_pad;
        scratch_pad.prepare(circuit);

        // get commitment
        uint32 poly_size = circuit.layers[0].input_layer_vals.evals.size();
        RawCommitment<F> commitment;
        commitment.from_bytes(proof.bytes_head(), poly_size);

        Transcript<F, F_primitive> transcript;
        transcript.append_bytes(proof.bytes_head(), commitment.size());
        
        //grinding
        grind(transcript, config);

        proof.step(commitment.size() + 256/8);

        // gkr
        auto t = gkr_verify<F, F_primitive>(circuit, claimed_v, transcript, proof);
        
        // verify pc
        bool verified = std::get<0>(t);
        std::vector<F_primitive> rz1 = std::get<1>(t);
        std::vector<F_primitive> rz2 = std::get<2>(t);
        F claimed_v1 = std::get<3>(t);
        F claimed_v2 = std::get<4>(t);
        
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