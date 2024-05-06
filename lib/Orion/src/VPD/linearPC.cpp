#include "infrastructure/merkle_tree.h"
#include "linear_code/linear_code_encode.h"
#include "linear_gkr/verifier.h"
#include "linear_gkr/prover.h"
#include <math.h>
#include <map>
#include "infrastructure/merkle_tree.h"
#include <algorithm>
#include <chrono>
#include "VPD/linearPC.h"
//commit
prime_field::field_element **encoded_codeword;
prime_field::field_element **coef;
long long *codeword_size;
__hhash_digest *mt;
zk_prover p;
zk_verifier v;
__hhash_digest* commit(const prime_field::field_element *src, long long N)
{
    __hhash_digest *stash = new __hhash_digest[N / column_size * 2];
    codeword_size = new long long[column_size];
    assert(N % column_size == 0);
    encoded_codeword = new prime_field::field_element*[column_size];
    coef = new prime_field::field_element*[column_size];
    for(int i = 0; i < column_size; ++i)
    {
        encoded_codeword[i] = new prime_field::field_element[N / column_size * 2];
        coef[i] = new prime_field::field_element[N / column_size];
        memcpy(coef[i], &src[i * N / column_size], sizeof(prime_field::field_element) * N / column_size);
        memset(encoded_codeword[i], 0, sizeof(prime_field::field_element) * N / column_size * 2);
        codeword_size[i] = encode(&src[i * N / column_size], encoded_codeword[i], N / column_size);
    }

    for(int i = 0; i < N / column_size * 2; ++i)
    {
        memset(&stash[i], 0, sizeof(__hhash_digest));
        for(int j = 0; j < column_size / 2; ++j)
        {
            stash[i] = merkle_tree::hash_double_field_element_merkle_damgard(encoded_codeword[2 * j][i], encoded_codeword[2 * j + 1][i], stash[i]);
        }
    }

    merkle_tree::merkle_tree_prover::create_tree(stash, N / column_size * 2, mt, sizeof(__hhash_digest), true);
    return mt;
}

std::map<long long, long long> gates_count;

std::pair<long long, long long> prepare_enc_count(long long input_size, long long output_size_so_far, int depth)
{
    if(input_size <= distance_threshold)
    {
        return std::make_pair(depth, output_size_so_far);
    }
    //output
    gates_count[depth + 1] = output_size_so_far + input_size + C[depth].R;
    auto output_depth_output_size = prepare_enc_count(C[depth].R, output_size_so_far + input_size, depth + 1);
    gates_count[output_depth_output_size.first + 1] = output_depth_output_size.second + D[depth].R;
    return std::make_pair(output_depth_output_size.first + 1, gates_count[output_depth_output_size.first + 1]);
}

int smallest_pow2_larger_or_equal_to(int x)
{
    for(int i = 0; i < 32; ++i)
    {
        if((1 << i) >= x)
            return (1 << i);
    }
    assert(false);
}

void prepare_gates_count(long long *query, long long N, int query_count)
{
    long long query_ptr = 0;
    //input layer
    gates_count[0] = N;

    //expander part
    gates_count[1] = N + C[0].R;
    std::pair<long long, long long> output_depth_output_size = prepare_enc_count(C[0].R, N, 1);

    gates_count[output_depth_output_size.first + 1] = N + output_depth_output_size.second + D[0].R;
    gates_count[output_depth_output_size.first + 2] = query_count;
}

std::pair<long long, long long> generate_enc_circuit(long long input_size, long long output_size_so_far, long long recursion_depth, long long input_depth)
{
    if(input_size <= distance_threshold)
    {
        return std::make_pair(input_depth, output_size_so_far);
    }
    //relay the output
    for(int i = 0; i < output_size_so_far; ++i)
    {
        v.C.circuit[input_depth + 1].gates[i] = gate(gate_types::relay, i, 0);
    }
    v.C.circuit[input_depth + 1].src_expander_C_mempool = new int[cn * C[recursion_depth].L];
    v.C.circuit[input_depth + 1].weight_expander_C_mempool = new prime_field::field_element[cn * C[recursion_depth].L];
    int mempool_ptr = 0;
    for(int i = 0; i < C[recursion_depth].R; ++i)
    {
        int neighbor_size = C[recursion_depth].r_neighbor[i].size();
        v.C.circuit[input_depth + 1].gates[output_size_so_far + i].ty = gate_types::custom_linear_comb;
        v.C.circuit[input_depth + 1].gates[output_size_so_far + i].parameter_length = neighbor_size;
        v.C.circuit[input_depth + 1].gates[output_size_so_far + i].src = &v.C.circuit[input_depth + 1].src_expander_C_mempool[mempool_ptr];
        v.C.circuit[input_depth + 1].gates[output_size_so_far + i].weight = &v.C.circuit[input_depth + 1].weight_expander_C_mempool[mempool_ptr];
        mempool_ptr += C[recursion_depth].r_neighbor[i].size();
        int C_input_offset = output_size_so_far - input_size;
        for(int j = 0; j < neighbor_size; ++j)
        {
            v.C.circuit[input_depth + 1].gates[output_size_so_far + i].src[j] = C[recursion_depth].r_neighbor[i][j] + C_input_offset;
            v.C.circuit[input_depth + 1].gates[output_size_so_far + i].weight[j] = C[recursion_depth].r_weight[i][j];
        }
    }

    auto output_depth_output_size = generate_enc_circuit(C[recursion_depth].R, output_size_so_far + C[recursion_depth].R, recursion_depth + 1, input_depth + 1);

    long long D_input_offset = output_size_so_far;

    long long final_output_depth = output_depth_output_size.first + 1;

    output_size_so_far = output_depth_output_size.second;
    mempool_ptr = 0;

    
    //relay the output
    for(long long i = 0; i < output_size_so_far; ++i)
    {
        v.C.circuit[final_output_depth].gates[i] = gate(gate_types::relay, i, 0);
    }

    v.C.circuit[final_output_depth].src_expander_D_mempool = new int[dn * D[recursion_depth].L];
    v.C.circuit[final_output_depth].weight_expander_D_mempool = new prime_field::field_element[dn * D[recursion_depth].L];

    for(long long i = 0; i < D[recursion_depth].R; ++i)
    {
        long long neighbor_size = D[recursion_depth].r_neighbor[i].size();
        v.C.circuit[final_output_depth].gates[output_size_so_far + i].ty = gate_types::custom_linear_comb;
        v.C.circuit[final_output_depth].gates[output_size_so_far + i].parameter_length = neighbor_size;
        v.C.circuit[final_output_depth].gates[output_size_so_far + i].src = &v.C.circuit[final_output_depth].src_expander_D_mempool[mempool_ptr];
        v.C.circuit[final_output_depth].gates[output_size_so_far + i].weight = &v.C.circuit[final_output_depth].weight_expander_D_mempool[mempool_ptr];
        mempool_ptr += D[recursion_depth].r_neighbor[i].size();
        for(long long j = 0; j < neighbor_size; ++j)
        {
            v.C.circuit[final_output_depth].gates[output_size_so_far + i].src[j] = D[recursion_depth].r_neighbor[i][j] + D_input_offset;
            v.C.circuit[final_output_depth].gates[output_size_so_far + i].weight[j] = D[recursion_depth].r_weight[i][j];
        }
    }
    return std::make_pair(final_output_depth, output_size_so_far + D[recursion_depth].R);
}

void generate_circuit(long long* query, long long N, int query_count, prime_field::field_element *input)
{
    std::sort(query, query + query_count);
    prepare_gates_count(query, N, query_count);
    printf("Depth %d\n", gates_count.size());
    assert((1LL << mylog(N)) == N);
    v.C.inputs = new prime_field::field_element[N];
    v.C.total_depth = gates_count.size() + 1;
    v.C.circuit = new layer[v.C.total_depth];
    v.C.circuit[0].bit_length = mylog(N);
    v.C.circuit[0].gates = new gate[1LL << v.C.circuit[0].bit_length];

    for(int i = 0; i < gates_count.size(); ++i)
    {
        v.C.circuit[i + 1].bit_length = mylog(smallest_pow2_larger_or_equal_to(gates_count[i]));
        v.C.circuit[i + 1].gates = new gate[1LL << v.C.circuit[i + 1].bit_length];
    }
    v.C.circuit[gates_count.size() + 1].bit_length = mylog(smallest_pow2_larger_or_equal_to(query_count));
    v.C.circuit[gates_count.size() + 1].gates = new gate[1LL << v.C.circuit[gates_count.size() + 1].bit_length];

    for(long long i = 0; i < N; ++i)
    {
        v.C.inputs[i] = input[i];
        v.C.circuit[0].gates[i] = gate(gate_types::input, 0, 0);
        v.C.circuit[1].gates[i] = gate(gate_types::direct_relay, i, 0);
    }

    for(long long i = 0; i < N; ++i)
    {
        v.C.circuit[2].gates[i] = gate(gate_types::relay, i, 0);
    }

    v.C.circuit[2].src_expander_C_mempool = new int[cn * C[0].L];
    v.C.circuit[2].weight_expander_C_mempool = new prime_field::field_element[cn * C[0].L];
    int C_mempool_ptr = 0, D_mempool_ptr = 0;
    for(long long i = 0; i < C[0].R; ++i)
    {
        v.C.circuit[2].gates[i + N] = gate(gate_types::custom_linear_comb, 0, 0);
        v.C.circuit[2].gates[i + N].parameter_length = C[0].r_neighbor[i].size();
        v.C.circuit[2].gates[i + N].src = &v.C.circuit[2].src_expander_C_mempool[C_mempool_ptr];
        v.C.circuit[2].gates[i + N].weight = &v.C.circuit[2].weight_expander_C_mempool[C_mempool_ptr];
        C_mempool_ptr += C[0].r_neighbor[i].size();
        for(long long j = 0; j < C[0].r_neighbor[i].size(); ++j)
        {
            long long L = C[0].r_neighbor[i][j];
            long long R = i;
            prime_field::field_element weight = C[0].r_weight[R][j];
            v.C.circuit[2].gates[i + N].src[j] = L;
            v.C.circuit[2].gates[i + N].weight[j] = weight;
        }
    }

    auto output_depth_output_size = generate_enc_circuit(C[0].R, N + C[0].R, 1, 2);
    //add final output
    long long final_output_depth = output_depth_output_size.first + 1;
    for(long long i = 0; i < output_depth_output_size.second; ++i)
    {
        v.C.circuit[final_output_depth].gates[i] = gate(gate_types::relay, i, 0);
    }

    long long D_input_offset = N;
    long long output_so_far = output_depth_output_size.second;
    v.C.circuit[final_output_depth].src_expander_D_mempool = new int[dn * D[0].L];
    v.C.circuit[final_output_depth].weight_expander_D_mempool = new prime_field::field_element[dn * D[0].L];

    for(long long i = 0; i < D[0].R; ++i)
    {
        v.C.circuit[final_output_depth].gates[output_so_far + i].ty = gate_types::custom_linear_comb;
        v.C.circuit[final_output_depth].gates[output_so_far + i].parameter_length = D[0].r_neighbor[i].size();
        v.C.circuit[final_output_depth].gates[output_so_far + i].src = &v.C.circuit[final_output_depth].src_expander_D_mempool[D_mempool_ptr];
        v.C.circuit[final_output_depth].gates[output_so_far + i].weight = &v.C.circuit[final_output_depth].weight_expander_D_mempool[D_mempool_ptr];
        D_mempool_ptr += D[0].r_neighbor[i].size();
        for(long long j = 0; j < D[0].r_neighbor[i].size(); ++j)
        {
            v.C.circuit[final_output_depth].gates[output_so_far + i].src[j] = D[0].r_neighbor[i][j] + N;
            v.C.circuit[final_output_depth].gates[output_so_far + i].weight[j] = D[0].r_weight[i][j];
        }
    }

    for(long long i = 0; i < query_count; ++i)
    {
        v.C.circuit[final_output_depth + 1].gates[i] = gate(gate_types::relay, query[i], 0);
    }
    assert(C_mempool_ptr == cn * C[0].L);
}

#define timer_mark(x) auto x = std::chrono::high_resolution_clock::now()
#define time_diff(start, end) (std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count())

std::pair<prime_field::field_element, bool> tensor_product_protocol_with_field_switch(prime_field::field_element *r0, prime_field::field_element *r1, long long size_r0, long long size_r1, long long N, __hhash_digest *com_mt)
{
    double verification_time = 0;
    assert(size_r0 * size_r1 == N);

    bool *visited_com = new bool[N / column_size * 4];
    bool *visited_combined_com = new bool[N / column_size * 4];

    long long proof_size = 0;

    const int query_count = -128 / (log2(1 - target_distance));
    printf("Query count %d\n", query_count);
    printf("Column size %d\n", column_size);
    printf("Number of merkle pathes %d\n", query_count);
    printf("Number of field elements %d\n", query_count * column_size);

    //prover construct the combined codeword

    prime_field::field_element *combined_codeword = new prime_field::field_element[codeword_size[0]];
    __hhash_digest *combined_codeword_hash = new __hhash_digest[N / column_size * 2];
    __hhash_digest *combined_codeword_mt = new __hhash_digest[N / column_size * 4];
    memset(combined_codeword, 0, sizeof(prime_field::field_element) * codeword_size[0]);
    for(int i = 0; i < column_size; ++i)
    {
        for(int j = 0; j < codeword_size[0]; ++j)
        {
            combined_codeword[j] = combined_codeword[j] + r0[i] * encoded_codeword[i][j];
        }
    }

    prime_field::field_element zero;
    memset(&zero, 0, sizeof(zero));
    for(int i = 0; i < N / column_size * 2; ++i)
    {
        if(i < codeword_size[0])
            combined_codeword_hash[i] = merkle_tree::hash_single_field_element(combined_codeword[i]);
        else
            combined_codeword_hash[i] = merkle_tree::hash_single_field_element(zero);
    }

    //merkle commit to combined_codeword
    merkle_tree::merkle_tree_prover::create_tree(combined_codeword_hash, N / column_size * 2, combined_codeword_mt, sizeof(__hhash_digest), false);
    
    //prover construct the combined original message
    prime_field::field_element *combined_message = new prime_field::field_element[N];
    memset(combined_message, 0, sizeof(prime_field::field_element) * N);
    for(int i = 0; i < column_size; ++i)
    {
        for(int j = 0; j < codeword_size[0]; ++j)
        {
            combined_message[j] = combined_message[j] + r0[i] * coef[i][j];
        }
    }

    //check for encode
    {
        prime_field::field_element *test_codeword = new prime_field::field_element[N / column_size * 2];
        int test_codeword_size = encode(combined_message, test_codeword, N / column_size);
        assert(test_codeword_size == codeword_size[0]);
        for(int i = 0; i < test_codeword_size; ++i)
        {
            assert(test_codeword[i] == combined_codeword[i]);
        }
    }

    //verifier random check columns
    timer_mark(v_t0);
    for(int i = 0; i < query_count; ++i)
    {
        long long q = rand() % codeword_size[0];
        prime_field::field_element sum = prime_field::field_element(0ULL);
        for(int j = 0; j < column_size; ++j)
        {
            sum = sum + r0[j] * encoded_codeword[j][q];
        }
        proof_size += sizeof(prime_field::field_element) * column_size;

        //calc hash
        __hhash_digest column_hash;
        memset(&column_hash, 0, sizeof(__hhash_digest));

        for(int j = 0; j < column_size / 2; ++j)
        {
            column_hash = merkle_tree::hash_double_field_element_merkle_damgard(encoded_codeword[2 * j][q], encoded_codeword[2 * j + 1][q], column_hash);
        }

        assert(merkle_tree::merkle_tree_verifier::verify_claim(com_mt[1], com_mt, column_hash, q, N / column_size * 2, visited_com, proof_size));
        assert(merkle_tree::merkle_tree_verifier::verify_claim(combined_codeword_mt[1], combined_codeword_mt, merkle_tree::hash_single_field_element(combined_codeword[q]), q, N / column_size * 2, visited_combined_com, proof_size));

        assert(sum == combined_codeword[q]);
        //add merkle tree open
    }
    timer_mark(v_t1);
    verification_time += time_diff(v_t0, v_t1);

    //setup code-switching
    prime_field::field_element answer = prime_field::field_element(0ULL);
    for(int i = 0; i < N / column_size; ++i)
    {
        answer = answer + r1[i] * combined_message[i];
    }
    
    // prover commit private input

    // verifier samples query
    long long *q = new long long[query_count];
    for(int i = 0; i < query_count; ++i)
    {
        q[i] = rand() % codeword_size[0];
    }
    // generate circuit

    generate_circuit(q, N / column_size, query_count, combined_message);


    v.get_prover(&p);
    p.get_circuit(v.C);
    int max_bit_length = -1;
    for(int i = 0; i < v.C.total_depth; ++i)
        max_bit_length = max(max_bit_length, v.C.circuit[i].bit_length);
    p.init_array(max_bit_length);
    v.init_array(max_bit_length);
    p.get_witness(combined_message, N / column_size);

    bool result = v.verify("log.txt");
    //bool result = true;
    //p.evaluate();
    timer_mark(sample_t0);
    for(int i = 0; i < query_count; ++i)
    {
        assert(p.circuit_value[p.C -> total_depth - 1][i] == combined_codeword[q[i]]);
    }
    timer_mark(sample_t1);
    verification_time += time_diff(sample_t0, sample_t1);
    verification_time += v.v_time;
    proof_size += query_count * sizeof(prime_field::field_element);
	p.delete_self();
	v.delete_self();

    printf("Proof size for tensor IOP %lld bytes\n", proof_size + v.proof_size);
    printf("Verification time %lf\n", verification_time);

    return std::make_pair(answer, result);
}

std::pair<prime_field::field_element, bool> tensor_product_protocol(prime_field::field_element *r0, prime_field::field_element *r1, long long size_r0, long long size_r1, long long N, __hhash_digest *com_mt)
{
    double verification_time = 0;
    
    assert(size_r0 * size_r1 == N);

    bool *visited_com = new bool[N / column_size * 4];
    bool *visited_combined_com = new bool[N / column_size * 4];

    long long proof_size = 0;

    const int query_count = -128 / (log2(1 - target_distance / 2));
    printf("Query count %d\n", query_count);
    printf("Column size %d\n", column_size);
    printf("Number of merkle pathes %d\n", query_count);
    printf("Number of field elements %d\n", query_count * column_size);

    //prover construct the randomly combined original message
    prime_field::field_element *randomness = new prime_field::field_element[column_size];
    for (int i = 0; i < column_size; i++)
    {
        // in practice, we should get this from transcript
        randomness[i] = prime_field::random();
    }
    prime_field::field_element *random_combined_message = new prime_field::field_element[N];
    memset(random_combined_message, 0, sizeof(prime_field::field_element) * N);
    for(int i = 0; i < column_size; ++i)
    {
        for(int j = 0; j < codeword_size[0]; ++j)
        {
            random_combined_message[j] = random_combined_message[j] + randomness[i] * coef[i][j];
        }
    }

    //prover construct the combined original message
    prime_field::field_element *combined_message = new prime_field::field_element[N];
    memset(combined_message, 0, sizeof(prime_field::field_element) * N);
    for(int i = 0; i < column_size; ++i)
    {
        for(int j = 0; j < codeword_size[0]; ++j)
        {
            combined_message[j] = combined_message[j] + r0[i] * coef[i][j];
        }
    }

    timer_mark(v_t0);

    // prover sends the random_combined and combined messages
    proof_size += 2 * sizeof(prime_field::field_element) * N / column_size;

    //verifier encodes by itself
    prime_field::field_element *random_combined_codeword = new prime_field::field_element[N / column_size * 2];
    prime_field::field_element *combined_codeword = new prime_field::field_element[N / column_size * 2];
    int random_combined_codeword_size = encode(random_combined_message, random_combined_codeword, N / column_size);
    int combined_codeword_size = encode(combined_message, combined_codeword, N / column_size);
    assert(random_combined_codeword_size == codeword_size[0]);
    assert(combined_codeword_size == codeword_size[0]);

    //verifier random check columns
    for(int i = 0; i < query_count; ++i)
    {
        long long q = rand() % codeword_size[0];

        prime_field::field_element sum = prime_field::field_element::zero();
        for(int j = 0; j < column_size; ++j)
        {
            sum = sum + randomness[j] * encoded_codeword[j][q];
        }
        assert(sum == random_combined_codeword[q]);

        sum = prime_field::field_element::zero();
        for(int j = 0; j < column_size; ++j)
        {
            sum = sum + r0[j] * encoded_codeword[j][q];
        }
        assert(sum == combined_codeword[q]);

        proof_size += sizeof(prime_field::field_element) * column_size;

        //calc hash
        __hhash_digest column_hash;
        memset(&column_hash, 0, sizeof(__hhash_digest));

        for(int j = 0; j < column_size / 2; ++j)
        {
            column_hash = merkle_tree::hash_double_field_element_merkle_damgard(encoded_codeword[2 * j][q], encoded_codeword[2 * j + 1][q], column_hash);
        }

        assert(merkle_tree::merkle_tree_verifier::verify_claim(com_mt[1], com_mt, column_hash, q, N / column_size * 2, visited_com, proof_size));
        //add merkle tree open
    }
    timer_mark(v_t1);
    verification_time += time_diff(v_t0, v_t1);

    prime_field::field_element answer = prime_field::field_element::zero();
    for(int i = 0; i < N / column_size; ++i)
    {
        answer = answer + r1[i] * combined_message[i];
    }

    printf("Proof size: %lld bytes\n", proof_size);
    printf("Verification time %lf\n", verification_time);

    // assertions guarantee the correctness, simply return true here.
    return std::make_pair(answer, true);
}


//open & verify

std::pair<prime_field::field_element, bool> open_and_verify(prime_field::field_element x, long long N, __hhash_digest *com_mt)
{
    assert(N % column_size == 0);
    //tensor product of r0 otimes r1
    prime_field::field_element *r0, *r1;
    r0 = new prime_field::field_element[column_size];
    r1 = new prime_field::field_element[N / column_size];

    prime_field::field_element x_n = prime_field::fast_pow(x, N / column_size);
    r0[0] = prime_field::field_element(1ULL);
    for(long long j = 1; j < column_size; ++j)
    {
        r0[j] = r0[j - 1] * x_n;
    }
    r1[0] = prime_field::field_element(1ULL);
    for(long long j = 1; j < N / column_size; ++j)
    {
        r1[j] = r1[j - 1] * x;
    }

    auto answer = tensor_product_protocol(r0, r1, column_size, N / column_size, N, com_mt);

    delete[] r0;
    delete[] r1;
    return answer;
}

void dfs(prime_field::field_element *dst, prime_field::field_element *r, int size, int depth, prime_field::field_element val)
{
    if(size == 1)
    {
        *dst = val;
    }
    else
    {
        dfs(dst, r, size / 2, depth + 1, val * (prime_field::field_element(1) - r[depth]));
        dfs(dst + (size / 2), r, size / 2, depth + 1, val * r[depth]);
    }
}

std::pair<prime_field::field_element, bool> open_and_verify(prime_field::field_element *r, int size_r, int N, __hhash_digest *com_mt)
{
    assert(N % column_size == 0);
    prime_field::field_element *r0, *r1;

    r0 = new prime_field::field_element[column_size];
    r1 = new prime_field::field_element[N / column_size];
    int log_column_size = 0;
    while(true)
    {
        if((1 << log_column_size) == column_size)
        {
            break;
        }
        log_column_size++;
    }
    dfs(r0, r, column_size, 0, prime_field::field_element(1));
    dfs(r1, r + log_column_size, N / column_size, 0, prime_field::field_element(1));



    auto answer = tensor_product_protocol(r0, r1, column_size, N / column_size, N, com_mt);
    delete[] r0;
    delete[] r1;
    return answer;
}
