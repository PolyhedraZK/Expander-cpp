#ifndef GKR_TRANSCRIPT_H
#define GKR_TRANSCRIPT_H

#include <vector>

#include "hash/hashes.hpp"

namespace gkr
{

template <typename F, typename F_primitive>
class Transcript
{
private:
    uint32 _mem_pool_size()
    {
        return digest_size_in_bytes + mem_pool_size_in_Fs * sizeof(F);
    }

public:
    static const uint32 digest_size_in_bytes = 32;
    static const uint32 mem_pool_size_in_Fs = 10;

public:
    SHA256Hasher hasher;
    uint8 mem[digest_size_in_bytes + mem_pool_size_in_Fs * sizeof(F)];
    uint32 cur_mem_size;

    Transcript()
    {
        hasher = SHA256Hasher();
        memset(mem, 0, digest_size_in_bytes);
        cur_mem_size = digest_size_in_bytes;
    }

    void append_f(const F &f)
    {
        assert(cur_mem_size + sizeof(F) <= _mem_pool_size());
        f.to_bytes(mem + cur_mem_size);
        cur_mem_size += sizeof(F);
    }

    F_primitive challenge_f()
    {
        hasher.hash(mem, mem, cur_mem_size);
        cur_mem_size = digest_size_in_bytes;

        F_primitive f;
        f.from_bytes(mem); 
        return f;
    }

    uint64 challenge_uint64()
    {
        hasher.hash(mem, mem, cur_mem_size);
        cur_mem_size = digest_size_in_bytes;
        return *reinterpret_cast<uint64*>(mem);
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

};

}

#endif