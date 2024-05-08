#ifndef GKR_TRANSCRIPT_H
#define GKR_TRANSCRIPT_H

#include <iostream>
#include <fstream>
#include <vector>

#include "hash/hashes.hpp"

namespace gkr
{

template<typename F>
class Proof
{
public:
    uint32 idx, commitment_nb_bytes, opening_nb_bytes;
    std::vector<uint8> bytes;

    Proof()
    {
        idx = 0;
        bytes.resize(0);
    }

    void append_bytes(const uint8* input_bytes, uint32 len)
    {
        bytes.insert(bytes.end(), input_bytes, input_bytes + len);
    }

    inline void reset()
    {
        idx = 0;
    }

    const uint8* bytes_head()
    {
        return bytes.data() + idx;
    }

    inline void step(uint32 nb_bytes)
    {
        idx += nb_bytes;
    }

    inline F get_next_and_step()
    {
        F f;
        f.from_bytes(bytes.data() + idx);
        step(sizeof(F));
        return f;
    }

    void write_and_compress(const char* proof_loc="/tmp/proof.txt", const char* compressed_proof_loc="/tmp/compressed_proof.tar.gz")
    {
        ofstream f(proof_loc);

        uint32 nb_bytes = bytes.size();
        f.write((char*) &nb_bytes, 4);
        f.write((char*) bytes.data(), nb_bytes);
        f.close();

        char command[1000];
        sprintf(command, "tar -czf %s --absolute-names %s", compressed_proof_loc, proof_loc);
        system(command);
    }

    void read_compressed_proof(const char* compressed_proof_loc="/tmp/compressed_proof.tar.gz")
    {
        char command[1000];
        sprintf(command, "tar -xzf %s -C %s", compressed_proof_loc, "/tmp");
        read_proof("/tmp/proof.txt");
    }

    void read_proof(const char* proof_loc="/tmp/proof.txt")
    {
        ifstream f(proof_loc);

        uint32 nb_bytes;
        f.read((char*) &nb_bytes, 4);
        bytes.resize(nb_bytes);
        f.read((char*) bytes.data(), nb_bytes);
        f.close();
    }
};

template <typename F, typename F_primitive>
class Transcript
{
private:
    inline void _hash_to_digest()
    {
        uint32 hash_end_idx = proof.bytes.size();
        if (hash_end_idx - hash_start_idx > 0)
        {
            hasher.hash(digest, proof.bytes.data() + hash_start_idx, hash_end_idx - hash_start_idx);
            hash_start_idx = hash_end_idx;
        }
        else 
        {
            hasher.hash(digest, digest, digest_size);
        }
    }

public:
    const static uint32 digest_size = 32;

public:
    // recording the proof
    Proof<F> proof;

public:
    // produce the randomness
    SHA256Hasher hasher;
    uint32 hash_start_idx;
    uint8 digest[digest_size];

    Transcript()
    {
        proof = Proof<F>();

        hasher = SHA256Hasher();
        hash_start_idx = 0;
        memset(digest, 0, digest_size);
    }

    void append_bytes(const uint8* bytes, uint32 len)
    { 
        proof.append_bytes(bytes, len);
    }

    void append_f(const F &f)
    {
        uint32 cur_size = proof.bytes.size();
        proof.bytes.resize(cur_size + sizeof(F));
        f.to_bytes(proof.bytes.data() + cur_size);
    }

    F_primitive challenge_f()
    {
        _hash_to_digest();

        F_primitive f;
        assert(sizeof(F_primitive) <= digest_size);
        f.from_bytes(digest); 
        return f;
    }

    uint64 challenge_uint64()
    {
        _hash_to_digest();
        return *reinterpret_cast<uint64*>(digest);
    }

    std::vector<F_primitive> challenge_fs(size_t n)
    {
        std::vector<F_primitive> cs;
        for (size_t i = 0; i < n; i++)
        {
            cs.push_back(challenge_f());
        }
        return cs;
    }
    void print_state()
    {
        challenge_f();
        std::cout << "Proof size: " << proof.bytes.size() << std::endl;
        std::cout << "Hash start idx: " << hash_start_idx << std::endl;
        for (int i = 0; i < digest_size; i++)
        {
            std::cout << (int) digest[i] << " ";
        }
        std::cout << std::endl;
    }
};

}

#endif