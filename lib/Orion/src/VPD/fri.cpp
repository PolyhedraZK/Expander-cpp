#include "VPD/fri.h"
#include "linear_gkr/prime_field.h"
#include <chrono>
#include "infrastructure/constants.h"
#include "infrastructure/RS_polynomial.h"
#include <utility>
#include "infrastructure/utility.h"
#include <algorithm>
#include "linear_gkr/prover.h"
#include "poly_commitment/poly_commit.h"

//variables
int fri::log_current_witness_size_per_slice, fri::current_step_no, fri::witness_bit_length_per_slice;
fri::commit_phase_data fri::cpd;
double fri::__fri_timer;

__hhash_digest* fri::witness_merkle[2];
int witness_merkle_size[2];
//prime_field::field_element *witness_rs_codeword[slice_number], *witness_poly_coef[slice_number];
prime_field::field_element *fri::witness_rs_codeword_before_arrange[2][slice_number];
prime_field::field_element *fri::witness_rs_codeword_interleaved[2];

int *fri::witness_rs_mapping[2][slice_number];
prime_field::field_element *fri::L_group;
bool* fri::visited[max_bit_length];
bool* fri::visited_init[2];
bool* fri::visited_witness[2];
__hhash_digest* fri::leaf_hash[2];

prime_field::field_element *fri::r_extended;
//extern int slice_size, slice_count, slice_real_ele_cnt, mask_position_gap;
//extern prime_field::field_element *l_eval, *l_coef, *h_coef, *h_eval_arr;
prime_field::field_element *fri::virtual_oracle_witness;
int *fri::virtual_oracle_witness_mapping;

__hhash_digest fri::request_init_commit(const int bit_len, const int oracle_indicator)
{
	assert(poly_commit::slice_size * poly_commit::slice_count == (1 << (bit_len + rs_code_rate - log_slice_number)) * (1 << log_slice_number));
	//TO CHECK BUG
	__fri_timer = 0;
	//Take care of mem leak
	witness_merkle[oracle_indicator] = NULL;
	witness_rs_codeword_interleaved[oracle_indicator] = NULL;
	current_step_no = 0;
	assert((1 << log_slice_number) == slice_number);
	log_current_witness_size_per_slice = (bit_len + rs_code_rate - log_slice_number);
	witness_bit_length_per_slice = bit_len - log_slice_number;
	
	auto t0 = std::chrono::high_resolution_clock::now();

	int sliced_input_length_per_block = 1 << witness_bit_length_per_slice;
	assert(witness_bit_length_per_slice >= 0);
	
	auto root_of_unity = prime_field::get_root_of_unity(log_current_witness_size_per_slice);
	if(oracle_indicator == 0)
	{
		L_group = new prime_field::field_element[1 << log_current_witness_size_per_slice];
		L_group[0] = prime_field::field_element(1);
		for(int i = 1; i < (1 << log_current_witness_size_per_slice); ++i)
			L_group[i] = L_group[i - 1] * root_of_unity;
		assert(L_group[(1 << log_current_witness_size_per_slice) - 1] * root_of_unity == prime_field::field_element(1));
	}
	//commit the witness
	//assert(sizeof(prime_field::field_element) == 16);
	witness_rs_codeword_interleaved[oracle_indicator] = new prime_field::field_element[1 << (bit_len + rs_code_rate)];
	
	const int log_leaf_size = log_slice_number + 1;
	for(int i = 0; i < slice_number; ++i)
	{
		//RS Code
		assert(log_current_witness_size_per_slice - rs_code_rate == witness_bit_length_per_slice);
		root_of_unity = prime_field::get_root_of_unity(witness_bit_length_per_slice);
		if(oracle_indicator == 0)
		{
			witness_rs_codeword_before_arrange[0][i] = &poly_commit::l_eval[i * poly_commit::slice_size];
		}
		else
		{
			witness_rs_codeword_before_arrange[1][i] = &poly_commit::h_eval_arr[i * poly_commit::slice_size];
		}
		
		root_of_unity = prime_field::get_root_of_unity(log_current_witness_size_per_slice);
		
		witness_rs_mapping[oracle_indicator][i] = new int[1 << log_current_witness_size_per_slice];
		prime_field::field_element a;
		for(int j = 0; j < (1 << (log_current_witness_size_per_slice - 1)); j++)
		{
			assert(((j) << log_leaf_size | (i << 1) | 1) < (1 << (bit_len + rs_code_rate)));
			assert(((j) << log_leaf_size | (i << 1) | 1) < poly_commit::slice_size * poly_commit::slice_count);
			witness_rs_mapping[oracle_indicator][i][j] = (j) << log_leaf_size | (i << 1) | 0;
			witness_rs_mapping[oracle_indicator][i][j + (1 << log_current_witness_size_per_slice) / 2] = (j) << log_leaf_size | (i << 1) | 0; //0 is correct, we just want the starting address
			
			witness_rs_codeword_interleaved[oracle_indicator][(j) << log_leaf_size | (i << 1) | 0] = witness_rs_codeword_before_arrange[oracle_indicator][i][j];
			witness_rs_codeword_interleaved[oracle_indicator][(j) << log_leaf_size | (i << 1) | 1] = witness_rs_codeword_before_arrange[oracle_indicator][i][j + (1 << log_current_witness_size_per_slice) / 2];
		}
	}
	leaf_hash[oracle_indicator] = new __hhash_digest[1 << (log_current_witness_size_per_slice - 1)];
	for(int i = 0; i < (1 << (log_current_witness_size_per_slice - 1)); ++i)
	{
		__hhash_digest tmp_hash;
		memset(&tmp_hash, 0, sizeof(tmp_hash));
		__hhash_digest data[2];
		for(int j = 0; j < (1 << (log_leaf_size)); j += 2)
		{
			memcpy(data, &witness_rs_codeword_interleaved[oracle_indicator][i << log_leaf_size | j], 2 * sizeof(prime_field::field_element));
			data[1] = tmp_hash;
			my_hhash(data, &tmp_hash);
		}
		leaf_hash[oracle_indicator][i] = tmp_hash;
	}
	merkle_tree::merkle_tree_prover::create_tree(leaf_hash[oracle_indicator], 1 << (log_current_witness_size_per_slice - 1), witness_merkle[oracle_indicator], sizeof(__hhash_digest), true);
	witness_merkle_size[oracle_indicator] = 1 << (log_current_witness_size_per_slice - 1);
	visited_init[oracle_indicator] = new bool[1 << (log_current_witness_size_per_slice)];
	visited_witness[oracle_indicator] = new bool[1 << (bit_len + rs_code_rate)];
	memset(visited_init[oracle_indicator], false, sizeof(bool) * (1 << (log_current_witness_size_per_slice)));
	memset(visited_witness[oracle_indicator], false, sizeof(bool) * (1 << (bit_len + rs_code_rate)));
	auto t1 = std::chrono::high_resolution_clock::now();
	auto time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t0);
	double delta = time_span.count();
	__fri_timer = delta;
	//printf("Init %lf\n", delta);
	return witness_merkle[oracle_indicator][1];
}

//This one take care of memory leak, execute after ldt is no longer used
void fri::delete_self()
{
	if(L_group != NULL)
		delete[] L_group;
	cpd.delete_self();
}
std::pair<std::vector<std::pair<prime_field::field_element, prime_field::field_element> >, std::vector<__hhash_digest> > fri::request_init_value_with_merkle(long long pow_0, long long pow_1, int &new_size, const int oracle_indicator)
{
	//pull leaf
	if(pow_0 > pow_1)
		std::swap(pow_0, pow_1);
	assert(pow_0 +  (1 << log_current_witness_size_per_slice) / 2 == pow_1);
	std::vector<std::pair<prime_field::field_element, prime_field::field_element> > value;
	int log_leaf_size = log_slice_number + 1;
	new_size = 0;
	for(int i = 0; i < slice_number; ++i)
	{
		value.push_back(std::make_pair(witness_rs_codeword_interleaved[oracle_indicator][pow_0 << log_leaf_size | i << 1 | 0], witness_rs_codeword_interleaved[oracle_indicator][pow_0 << log_leaf_size | i << 1 | 1]));
		assert(pow_0 << log_leaf_size | i << 1 | 1 == witness_rs_mapping[oracle_indicator][i][pow_1]);
		if(!visited_witness[oracle_indicator][pow_0 << log_leaf_size | i << 1 | 0])
			visited_witness[oracle_indicator][pow_0 << log_leaf_size | i << 1 | 0] = true, new_size += sizeof(prime_field::field_element);
		if(!visited_witness[oracle_indicator][pow_0 << log_leaf_size | i << 1 | 1])
			visited_witness[oracle_indicator][pow_0 << log_leaf_size | i << 1 | 1] = true, new_size += sizeof(prime_field::field_element);
	}
	/*
	if(oracle_indicator == 0)
		value.push_back(std::make_pair(poly_commit::l_eval[(poly_commit::slice_count) * poly_commit::slice_size + pow_0], poly_commit::l_eval[(poly_commit::slice_count) * poly_commit::slice_size + pow_1]));
	else
		value.push_back(std::make_pair(poly_commit::h_eval_arr[(poly_commit::slice_count) * poly_commit::slice_size + pow_0], poly_commit::h_eval_arr[(poly_commit::slice_count) * poly_commit::slice_size + pow_1]));
	*/
	std::vector<__hhash_digest> com_hhash;
	int depth = log_current_witness_size_per_slice - 1;
	com_hhash.resize(depth + 1);
	int pos = pow_0 + (1 << (log_current_witness_size_per_slice - 1)); //minus 1 since each leaf have 2 values (qual resi)
	__hhash_digest test_hash = witness_merkle[oracle_indicator][pos];
	com_hhash[depth] = witness_merkle[oracle_indicator][pos];
	__hhash_digest data[2];
	for(int i = 0; i < depth; ++i)
	{
		if(!visited_init[oracle_indicator][pos ^ 1])
			new_size += sizeof(__hhash_digest);
		visited_init[oracle_indicator][pos] = true;
		visited_init[oracle_indicator][pos ^ 1] = true;
		if((pos & 1) == 1)
		{
			data[0] = witness_merkle[oracle_indicator][pos ^ 1];
			data[1] = test_hash;
		}
		else
		{
			data[0] = test_hash;
			data[1] = witness_merkle[oracle_indicator][pos ^ 1];
		}
		my_hhash(data, &test_hash);
		
		com_hhash[i] = witness_merkle[oracle_indicator][pos ^ 1];
		pos /= 2;
		assert(equals(test_hash, witness_merkle[oracle_indicator][pos]));
	}
	assert(pos == 1);
	return std::make_pair(value, com_hhash);
}

bool merkle_tree_consistency_check(__hhash_digest *merkle, int merkle_leaf_count)
{
	int current_lvl_node_count = 1;
	int starting_pos = 1;
	while(current_lvl_node_count != merkle_leaf_count)
	{
		for(int i = 0; i < current_lvl_node_count; ++i)
		{
			__hhash_digest cur = merkle[starting_pos + i];
			int lch, rch;
			lch = (starting_pos + i) << 1, rch = (starting_pos + i) << 1 | 1;
			__hhash_digest child_h[2];
			child_h[0] = merkle[lch];
			child_h[1] = merkle[rch];
			__hhash_digest h;
			my_hhash(child_h, &h);
			if(!equals(h, cur))	
				return false;
		}
		starting_pos += current_lvl_node_count;
		current_lvl_node_count *= 2;
	}
	return true;
}
std::pair<std::vector<std::pair<prime_field::field_element, prime_field::field_element> >, std::vector<__hhash_digest> > fri::request_step_commit(int lvl, long long pow, int& new_size)
{
	/*
	if(!merkle_tree_consistency_check(witness_merkle[0], witness_merkle_size[0]))
	{
		__asm__("int3;");
	}
	if(!merkle_tree_consistency_check(witness_merkle[1], witness_merkle_size[1]))
	{
		__asm__("int3;");
	}
	for(int lvll = 0; lvll < 30; ++lvll)
	{
		if(cpd.merkle[lvll] == NULL) continue;
		if(!merkle_tree_consistency_check(cpd.merkle[lvll], cpd.merkle_size[lvll]))
		{
			__asm__("int3;");
		}
	}
	 */
	new_size = 0;
	//quad residue is guaranteed by the construction of cpd.rs_codeword_mapping
	int pow_0;
	std::vector<std::pair<prime_field::field_element, prime_field::field_element> > value_vec;
	bool visited_element = false;
	for(int i = 0; i < slice_number; ++i)
	{
		pow_0 = cpd.rs_codeword_mapping[lvl][pow << log_slice_number | i];
		pow_0 /= 2;
		if(!visited[lvl][pow_0 * 2])
		{
			visited[lvl][pow_0 * 2] = true;
			//new_size += sizeof(prime_field::field_element);
		}
		else
		    visited_element = true;
		value_vec.push_back(std::make_pair(cpd.rs_codeword[lvl][pow_0 * 2], cpd.rs_codeword[lvl][pow_0 * 2 + 1]));
	}
	//this can be compressed into one by random linear combination
	if(!visited_element)
	    new_size += sizeof(prime_field::field_element);
	std::vector<__hhash_digest> com_hhash;
	
	pow_0 = (cpd.rs_codeword_mapping[lvl][pow << log_slice_number] >> (log_slice_number + 1)) + cpd.merkle_size[lvl];
	
	auto val_hhash = cpd.merkle[lvl][pow_0];

	while(pow_0 != 1)
	{
		if(!visited[lvl][pow_0 ^ 1])
		{
			new_size += sizeof(__hhash_digest);
			visited[lvl][pow_0 ^ 1] = true;
			visited[lvl][pow_0] = true;
		}
		com_hhash.push_back(cpd.merkle[lvl][pow_0 ^ 1]);
		pow_0 /= 2;
	}
	com_hhash.push_back(val_hhash);
	return std::make_pair(value_vec, com_hhash);
}

__hhash_digest fri::commit_phase_step(prime_field::field_element r)
{
	int nxt_witness_size = (1 << log_current_witness_size_per_slice) / 2;
	if(cpd.rs_codeword[current_step_no] == NULL)
		cpd.rs_codeword[current_step_no] = new prime_field::field_element[nxt_witness_size * poly_commit::slice_count];
	prime_field::field_element *previous_witness;
	int *previous_witness_mapping;
	if(current_step_no == 0)
	{
		//virtual oracle
		previous_witness = virtual_oracle_witness;
		previous_witness_mapping = virtual_oracle_witness_mapping;
	}
	else
	{
		previous_witness = cpd.rs_codeword[current_step_no - 1];
		previous_witness_mapping = cpd.rs_codeword_mapping[current_step_no - 1];
	}
	
	auto inv_2 = prime_field::inv(prime_field::field_element(2));


	auto t0 = std::chrono::high_resolution_clock::now();
	int log_leaf_size = log_slice_number + 1;
	#pragma omp parallel for
	for(int i = 0; i < nxt_witness_size; ++i)
	{
		int qual_res_0 = i;
		int qual_res_1 = ((1 << (log_current_witness_size_per_slice - 1)) + i) / 2;
		int pos;
		if(qual_res_0 > qual_res_1)
			pos = qual_res_1;
		else
			pos = qual_res_0;
		
		auto inv_mu = L_group[((1 << log_current_witness_size_per_slice) - i) & ((1 << log_current_witness_size_per_slice) - 1)]; 
		#pragma omp parallel for
		for(int j = 0; j < slice_number; ++j)
		{
			int real_pos = previous_witness_mapping[(pos) << log_slice_number | j];
			//BUG BUG BUG, to check
			assert((i << log_slice_number | j) < nxt_witness_size * poly_commit::slice_count);
			cpd.rs_codeword[current_step_no][i << log_slice_number | j] = inv_2 * ((previous_witness[real_pos] + previous_witness[real_pos | 1]) 
																	+ inv_mu * r  * (previous_witness[real_pos] - previous_witness[real_pos | 1]));
		}
	}
	for(int i = 0; i < nxt_witness_size; ++i)
	{
		L_group[i] = L_group[i * 2];
	}

	auto tmp = new prime_field::field_element[nxt_witness_size * poly_commit::slice_count];
	cpd.rs_codeword_mapping[current_step_no] = new int[nxt_witness_size * poly_commit::slice_count];
	#pragma omp parallel for
	for(int i = 0; i < nxt_witness_size / 2; ++i)
	{
		#pragma omp parallel for
		for(int j = 0; j < slice_number; ++j)
		{
			int a = i << log_slice_number | j, b = (i + nxt_witness_size / 2) << log_slice_number | j;
			int c = (i) << log_leaf_size | (j << 1) | 0, d = (i) << log_leaf_size | (j << 1) | 1;
			cpd.rs_codeword_mapping[current_step_no][a] = (i) << log_leaf_size | (j << 1) | 0;
			cpd.rs_codeword_mapping[current_step_no][b] = (i) << log_leaf_size | (j << 1) | 0;
			tmp[c] = cpd.rs_codeword[current_step_no][i << log_slice_number | j];
			tmp[d] = cpd.rs_codeword[current_step_no][(i + nxt_witness_size / 2) << log_slice_number | j];
			assert(a >= 0 && a < nxt_witness_size * slice_number);
			assert(b >= 0 && b < nxt_witness_size * slice_number);
			assert(c >= 0 && c < nxt_witness_size * slice_number);
			assert(d >= 0 && d < nxt_witness_size * slice_number);
		}
	}
	delete[] cpd.rs_codeword[current_step_no];
	cpd.rs_codeword[current_step_no] = tmp;

	visited[current_step_no] = new bool[nxt_witness_size * 4 * poly_commit::slice_count];
	memset(visited[current_step_no], false, sizeof(bool) * nxt_witness_size * 4 * poly_commit::slice_count);
	__hhash_digest htmp, *hash_val;
	hash_val = new __hhash_digest[nxt_witness_size / 2];
	memset(&htmp, 0, sizeof(__hhash_digest));
	for(int i = 0; i < nxt_witness_size / 2; ++i)
	{
		__hhash_digest data[2];
		prime_field::field_element data_ele[2];
		memset(data, 0, 2 * sizeof(__hhash_digest));
		memset(&htmp, 0, sizeof(__hhash_digest));
		for(int j = 0; j < (1 << log_slice_number); ++j)
		{
			int c = (i) << log_leaf_size | (j << 1) | 0, d = (i) << log_leaf_size | (j << 1) | 1;
			data_ele[0] = cpd.rs_codeword[current_step_no][c];
			data_ele[1] = cpd.rs_codeword[current_step_no][d];
			memcpy(&data[0], data_ele, sizeof(__hhash_digest));
			data[1] = htmp;
			my_hhash(data, &htmp);
		}
		hash_val[i] = htmp;
	}
	merkle_tree::merkle_tree_prover::create_tree(hash_val, nxt_witness_size / 2, cpd.merkle[current_step_no], sizeof(__hhash_digest), cpd.merkle[current_step_no] == NULL);
	
	auto t1 = std::chrono::high_resolution_clock::now();
	auto time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t0);
	__fri_timer += time_span.count();

	cpd.merkle_size[current_step_no] = nxt_witness_size / 2;
	log_current_witness_size_per_slice--;
	return cpd.merkle[current_step_no++][1];
}

prime_field::field_element* fri::commit_phase_final()
{
	if(current_step_no == 0)
		assert(false);
	else
		return cpd.rs_codeword[current_step_no - 1];
}
