#include "linear_gkr/prover.h"
#include <iostream>
#include <utility>
#include <vector>
#include "infrastructure/my_hhash.h"
#include "infrastructure/constants.h"
#include "infrastructure/RS_polynomial.h"
#include "infrastructure/utility.h"
#include <queue>
#include <cstring>

prime_field::field_element from_string(const char* str)
{
	prime_field::field_element ret = prime_field::field_element(0);
	int len = strlen(str);
	for(int i = 0; i < len; ++i)
	{
		int digit = str[i] - '0';
		ret = ret * prime_field::field_element(10) + prime_field::field_element(digit);
	}
	return ret;
}
prime_field::field_element inv_2;
void zk_prover::get_circuit(layered_circuit &from_verifier)
{
	C = &from_verifier;
	inv_2 = prime_field::inv(prime_field::field_element(2));
}

prime_field::field_element zk_prover::V_res(const prime_field::field_element* one_minus_r_0, const prime_field::field_element* r_0, const prime_field::field_element* output_raw, int r_0_size, int output_size)
{
	std::chrono::high_resolution_clock::time_point t0 = std::chrono::high_resolution_clock::now();
	prime_field::field_element *output;
	output = new prime_field::field_element[output_size];
	for(int i = 0; i < output_size; ++i)
		output[i] = output_raw[i];
	for(int i = 0; i < r_0_size; ++i)
	{
		for(int j = 0; j < (output_size >> 1); ++j)
		{
			output[j] = (output[j << 1] * one_minus_r_0[i] + output[j << 1 | 1] * r_0[i]);
		}
		output_size >>= 1;
	}
	std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

	std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t0);
	total_time += time_span.count();
	prime_field::field_element res = output[0];
	delete[] output;
	return res;
}

prime_field::field_element* zk_prover::evaluate()
{
	std::chrono::high_resolution_clock::time_point t0 = std::chrono::high_resolution_clock::now();
	//circuit_value[0] = new prime_field::field_element[(1 << C -> circuit[0].bit_length)];
	for(int i = 0; i < (1 << C -> circuit[0].bit_length); ++i)
	{
		int g, u, ty;
		g = i;
		u = C -> circuit[0].gates[g].u;
		ty = C -> circuit[0].gates[g].ty;
		assert(ty == 3 || ty == 2);
	}
	assert(C -> total_depth < 1000000);
	for(int i = 1; i < C -> total_depth; ++i)
	{
		circuit_value[i] = new prime_field::field_element[(1 << C -> circuit[i].bit_length)];
		for(int j = 0; j < (1 << C -> circuit[i].bit_length); ++j)
		{
			int g, u, v, ty;
			g = j;
			ty = C -> circuit[i].gates[g].ty;
			u = C -> circuit[i].gates[g].u;
			v = C -> circuit[i].gates[g].v;
			if(ty == 0)
			{
				circuit_value[i][g] = circuit_value[i - 1][u] + circuit_value[i - 1][v];
			}
			else if(ty == 1)
			{
				assert(u >= 0 && u < ((1 << C -> circuit[i - 1].bit_length)));
				assert(v >= 0 && v < ((1 << C -> circuit[i - 1].bit_length)));
				circuit_value[i][g] = circuit_value[i - 1][u] * circuit_value[i - 1][v];
			}
			else if(ty == 2)
			{
				circuit_value[i][g] = prime_field::field_element(0);
			}
			else if(ty == 3)
			{
				circuit_value[i][g] = prime_field::field_element(u);
			}
			else if(ty == 4)
			{
				circuit_value[i][g] = circuit_value[i - 1][u];
			}
			else if(ty == 5)
			{
				circuit_value[i][g] = prime_field::field_element(0);
				for(int k = u; k < v; ++k)
					circuit_value[i][g] = circuit_value[i][g] + circuit_value[i - 1][k];
			}
			else if(ty == 6)
			{
				circuit_value[i][g] = prime_field::field_element(1) - circuit_value[i - 1][u];
			}
			else if(ty == 7)
			{
				circuit_value[i][g] = circuit_value[i - 1][u] - circuit_value[i - 1][v];
			}
			else if(ty == 8)
			{
				auto &x = circuit_value[i - 1][u], &y = circuit_value[i - 1][v];
				circuit_value[i][g] = x + y - prime_field::field_element(2) * x * y;
			}
			else if(ty == 9)
			{
				assert(u >= 0 && u < ((1 << C -> circuit[i - 1].bit_length)));
				assert(v >= 0 && v < ((1 << C -> circuit[i - 1].bit_length)));
				auto &x = circuit_value[i - 1][u], &y = circuit_value[i - 1][v];
				circuit_value[i][g] = y - x * y;
			}
			else if(ty == 10)
			{
				circuit_value[i][g] = circuit_value[i - 1][u];
			}
			else if(ty == 12)
			{
				circuit_value[i][g] = prime_field::field_element(0);
				assert(v - u + 1 <= 60);
				for(int k = u; k <= v; ++k)
				{
					circuit_value[i][g] = circuit_value[i][g] + circuit_value[i - 1][k] * prime_field::field_element(1ULL << (k - u));
				}
			}
			else if(ty == 13)
			{
				assert(u == v);
				assert(u >= 0 && u < ((1 << C -> circuit[i - 1].bit_length)));
				circuit_value[i][g] = circuit_value[i - 1][u] * (prime_field::field_element(1) - circuit_value[i - 1][v]);
			}
			else if(ty == 14)
			{
				circuit_value[i][g] = prime_field::field_element(0);
				for(int k = 0; k < C -> circuit[i].gates[g].parameter_length; ++k)
				{
					prime_field::field_element weight = C -> circuit[i].gates[g].weight[k];
					long long idx = C -> circuit[i].gates[g].src[k];
					circuit_value[i][g] = circuit_value[i][g] + circuit_value[i - 1][idx] * weight;
				}
			}
			else
			{
				assert(false);
			}
		}
	}

	std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

	std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t0);
	std::cerr << "total evaluation time: " << time_span.count() << " seconds." << std::endl;
	return circuit_value[C -> total_depth - 1];
}

void zk_prover::get_witness(prime_field::field_element *inputs, int N)
{
	circuit_value[0] = new prime_field::field_element[(1 << C -> circuit[0].bit_length)];
	for(int i = 0; i < N; ++i)
	{
		circuit_value[0][i] = inputs[i];
	}
}


void zk_prover::sumcheck_init(int layer_id, int bit_length_g, int bit_length_u, int bit_length_v, 
	const prime_field::field_element &a, const prime_field::field_element &b, 
	const prime_field::field_element* R_0, const prime_field::field_element* R_1,
	prime_field::field_element* o_r_0, prime_field::field_element *o_r_1)
{
	r_0 = R_0;
	r_1 = R_1;
	alpha = a;
	beta = b;
	sumcheck_layer_id = layer_id;
	length_g = bit_length_g;
	length_u = bit_length_u;
	length_v = bit_length_v;
	one_minus_r_0 = o_r_0;
	one_minus_r_1 = o_r_1;
}

linear_poly *V_mult_add_new, *addV_array_new, *add_mult_sum_new;
bool gate_meet[15];
quadratic_poly *rets_prev, *rets_cur;
void zk_prover::init_array(int max_bit_length)
{
	memset(gate_meet, 0, sizeof(bool) * 15);
	int half_length = (max_bit_length >> 1) + 1;
	beta_g_r0_fhalf = new prime_field::field_element[(1 << half_length)];
	beta_g_r0_shalf = new prime_field::field_element[(1 << half_length)];
	beta_g_r1_fhalf = new prime_field::field_element[(1 << half_length)];
	beta_g_r1_shalf = new prime_field::field_element[(1 << half_length)];
	beta_u_fhalf = new prime_field::field_element[(1 << half_length)];
	beta_u_shalf = new prime_field::field_element[(1 << half_length)];
	add_mult_sum = new linear_poly[1 << max_bit_length];
	V_mult_add = new linear_poly[1 << max_bit_length];
	addV_array = new linear_poly[1 << max_bit_length];
	V_mult_add_new = new linear_poly[1 << max_bit_length];
	addV_array_new = new linear_poly[1 << max_bit_length];
	add_mult_sum_new = new linear_poly[1 << max_bit_length];
	rets_prev  = new quadratic_poly[1 << max_bit_length];
	rets_cur = new quadratic_poly[1 << max_bit_length];
}

void zk_prover::delete_self()
{
	delete[] add_mult_sum;
	delete[] V_mult_add;
	delete[] addV_array;
	delete[] V_mult_add_new;
	delete[] addV_array_new;
	delete[] add_mult_sum_new;

	delete[] beta_g_r0_fhalf;
	delete[] beta_g_r0_shalf;
	delete[] beta_g_r1_fhalf;
	delete[] beta_g_r1_shalf;
	delete[] beta_u_fhalf;
	delete[] beta_u_shalf;

	delete[] rets_prev;
	delete[] rets_cur;
	for(int i = 0; i < C -> total_depth; ++i)
		delete[] circuit_value[i];
}

zk_prover::~zk_prover()
{
}



void zk_prover::sumcheck_phase1_init()
{
	std::chrono::high_resolution_clock::time_point t0 = std::chrono::high_resolution_clock::now();
	//mult init
	total_uv = (1 << C -> circuit[sumcheck_layer_id - 1].bit_length);
	prime_field::field_element zero = prime_field::field_element(0);
	for(int i = 0; i < total_uv; ++i)
	{
		V_mult_add[i] = circuit_value[sumcheck_layer_id - 1][i];

		addV_array[i].a = zero;
		addV_array[i].b = zero;
		add_mult_sum[i].a = zero;
		add_mult_sum[i].b = zero;
	}
	
	beta_g_r0_fhalf[0] = alpha;
	beta_g_r1_fhalf[0] = beta;
	beta_g_r0_shalf[0] = prime_field::field_element(1);
	beta_g_r1_shalf[0] = prime_field::field_element(1);

	int first_half = length_g >> 1, second_half = length_g - first_half;

	for(int i = 0; i < first_half; ++i)
	{
		for(int j = 0; j < (1 << i); ++j)
		{
			beta_g_r0_fhalf[j | (1 << i)] = beta_g_r0_fhalf[j] * r_0[i];
			beta_g_r0_fhalf[j] = beta_g_r0_fhalf[j] * one_minus_r_0[i];
			beta_g_r1_fhalf[j | (1 << i)] = beta_g_r1_fhalf[j] * r_1[i];
			beta_g_r1_fhalf[j] = beta_g_r1_fhalf[j] * one_minus_r_1[i];
		}
	}

	for(int i = 0; i < second_half; ++i)
	{
		for(int j = 0; j < (1 << i); ++j)
		{
			beta_g_r0_shalf[j | (1 << i)] = beta_g_r0_shalf[j] * r_0[i + first_half];
			beta_g_r0_shalf[j] = beta_g_r0_shalf[j] * one_minus_r_0[i + first_half];
			beta_g_r1_shalf[j | (1 << i)] = beta_g_r1_shalf[j] * r_1[i + first_half];
			beta_g_r1_shalf[j] = beta_g_r1_shalf[j] * one_minus_r_1[i + first_half];
		}
	}

	int mask_fhalf = (1 << first_half) - 1;

	prime_field::field_element *intermediates0 = new prime_field::field_element[1 << length_g];
	prime_field::field_element *intermediates1 = new prime_field::field_element[1 << length_g];

	#pragma omp parallel for
	for(int i = 0; i < (1 << length_g); ++i)
	{
		int u, v;
		u = C -> circuit[sumcheck_layer_id].gates[i].u;
		v = C -> circuit[sumcheck_layer_id].gates[i].v;
		switch(C -> circuit[sumcheck_layer_id].gates[i].ty)
		{
			case 0: //add gate
			{
				auto tmp = (beta_g_r0_fhalf[i & mask_fhalf] * beta_g_r0_shalf[i >> first_half] 
						+ beta_g_r1_fhalf[i & mask_fhalf] * beta_g_r1_shalf[i >> first_half]);
				intermediates0[i] = circuit_value[sumcheck_layer_id - 1][v] * tmp;
				intermediates1[i] = tmp;
				break;
			}
			case 2:
			{
				break;
			}
			case 1: //mult gate
			{
				auto tmp = (beta_g_r0_fhalf[i & mask_fhalf] * beta_g_r0_shalf[i >> first_half] 
						+ beta_g_r1_fhalf[i & mask_fhalf] * beta_g_r1_shalf[i >> first_half]);
				intermediates0[i] = circuit_value[sumcheck_layer_id - 1][v] * tmp;
				break;
			}
			case 5: //sum gate
			{
				auto tmp = beta_g_r0_fhalf[i & mask_fhalf] * beta_g_r0_shalf[i >> first_half] 
					+ beta_g_r1_fhalf[i & mask_fhalf] * beta_g_r1_shalf[i >> first_half];
				intermediates1[i] = tmp;
				break;
			}
			case 12: //exp sum gate
			{
				auto tmp = beta_g_r0_fhalf[i & mask_fhalf] * beta_g_r0_shalf[i >> first_half] 
					+ beta_g_r1_fhalf[i & mask_fhalf] * beta_g_r1_shalf[i >> first_half];
				intermediates1[i] = tmp;
				break;
			}
			case 4: //direct relay gate
			{
				auto tmp = (beta_g_r0_fhalf[u & mask_fhalf] * beta_g_r0_shalf[u >> first_half] 
						+ beta_g_r1_fhalf[u & mask_fhalf] * beta_g_r1_shalf[u >> first_half]);
				intermediates1[i] = tmp;
				break;
			}
			case 6: //NOT gate
			{
				auto tmp = (beta_g_r0_fhalf[i & mask_fhalf] * beta_g_r0_shalf[i >> first_half] 
						+ beta_g_r1_fhalf[i & mask_fhalf] * beta_g_r1_shalf[i >> first_half]);
				intermediates1[i] = tmp;
				break;
			}
			case 7: //minus gate
			{
				auto tmp = (beta_g_r0_fhalf[i & mask_fhalf] * beta_g_r0_shalf[i >> first_half] 
						+ beta_g_r1_fhalf[i & mask_fhalf] * beta_g_r1_shalf[i >> first_half]);
				intermediates0[i] = circuit_value[sumcheck_layer_id - 1][v] * tmp;
				intermediates1[i] = tmp;
				break;
			}
			case 8: //XOR gate
			{
				auto tmp = (beta_g_r0_fhalf[i & mask_fhalf] * beta_g_r0_shalf[i >> first_half] 
						+ beta_g_r1_fhalf[i & mask_fhalf] * beta_g_r1_shalf[i >> first_half]);
				auto tmp_V = tmp * circuit_value[sumcheck_layer_id - 1][v];
				auto tmp_2V = tmp_V + tmp_V;
				intermediates0[i] = tmp_V;
				intermediates1[i] = tmp;
				break;
			}
			case 13: //bit-test gate
			{
				auto tmp = (beta_g_r0_fhalf[i & mask_fhalf] * beta_g_r0_shalf[i >> first_half] 
						+ beta_g_r1_fhalf[i & mask_fhalf] * beta_g_r1_shalf[i >> first_half]);
				auto tmp_V = tmp * circuit_value[sumcheck_layer_id - 1][v];
				intermediates0[i] = tmp_V;
				intermediates1[i] = tmp;
				break;
			}
			case 9: //NAAB gate
			{
				auto tmp = (beta_g_r0_fhalf[i & mask_fhalf] * beta_g_r0_shalf[i >> first_half] 
						+ beta_g_r1_fhalf[i & mask_fhalf] * beta_g_r1_shalf[i >> first_half]);
				auto tmpV = tmp * circuit_value[sumcheck_layer_id - 1][v];
				intermediates1[i] = tmpV;
				break;
			}
			case 10: //relay gate
			{
				auto tmp = (beta_g_r0_fhalf[i & mask_fhalf] * beta_g_r0_shalf[i >> first_half] 
						+ beta_g_r1_fhalf[i & mask_fhalf] * beta_g_r1_shalf[i >> first_half]);
				intermediates0[i] = tmp;
				break;
			}
			case 14: //custom comb
			{
				auto tmp = beta_g_r0_fhalf[i & mask_fhalf] * beta_g_r0_shalf[i >> first_half] 
					+ beta_g_r1_fhalf[i & mask_fhalf] * beta_g_r1_shalf[i >> first_half];
				intermediates1[i] = tmp;
				break;
			}
			default:
			{
				printf("Warning Unknown gate %d\n", C -> circuit[sumcheck_layer_id].gates[i].ty);
				break;
			}
		}
	}

	for(int i = 0; i < (1 << length_g); ++i)
	{
		int u, v;
		u = C -> circuit[sumcheck_layer_id].gates[i].u;
		v = C -> circuit[sumcheck_layer_id].gates[i].v;
		switch(C -> circuit[sumcheck_layer_id].gates[i].ty)
		{
			case 0: //add gate
			{
				if(!gate_meet[C -> circuit[sumcheck_layer_id].gates[i].ty])
				{
					//printf("first meet %d gate\n", C -> circuit[sumcheck_layer_id].gates[i].ty);
					gate_meet[C -> circuit[sumcheck_layer_id].gates[i].ty] = true;
				}
				addV_array[u].b = (addV_array[u].b + intermediates0[i]);
				add_mult_sum[u].b = (add_mult_sum[u].b + intermediates1[i]);
				break;
			}
			case 2:
			{
				break;
			}
			case 1: //mult gate
			{
				if(!gate_meet[C -> circuit[sumcheck_layer_id].gates[i].ty])
				{
					//printf("first meet %d gate\n", C -> circuit[sumcheck_layer_id].gates[i].ty);
					gate_meet[C -> circuit[sumcheck_layer_id].gates[i].ty] = true;
				}
				add_mult_sum[u].b = (add_mult_sum[u].b + intermediates0[i]);
				break;
			}
			case 5: //sum gate
			{
				if(!gate_meet[C -> circuit[sumcheck_layer_id].gates[i].ty])
				{
					//printf("first meet %d gate\n", C -> circuit[sumcheck_layer_id].gates[i].ty);
					gate_meet[C -> circuit[sumcheck_layer_id].gates[i].ty] = true;
				}
				for(int j = u; j < v; ++j)
				{
					add_mult_sum[j].b = (add_mult_sum[j].b + intermediates1[i]);
				}
				break;
			}
			case 12: //exp sum gate
			{
				if(!gate_meet[C -> circuit[sumcheck_layer_id].gates[i].ty])
				{
					//printf("first meet %d gate\n", C -> circuit[sumcheck_layer_id].gates[i].ty);
					gate_meet[C -> circuit[sumcheck_layer_id].gates[i].ty] = true;
				}
				auto tmp = intermediates1[i];
				for(int j = u; j <= v; ++j)
				{
					add_mult_sum[j].b = (add_mult_sum[j].b + tmp);
					tmp = tmp + tmp;
				}
				break;
			}
			case 14:
			{
				if(!gate_meet[C -> circuit[sumcheck_layer_id].gates[i].ty])
				{
					//printf("first meet %d gate\n", C -> circuit[sumcheck_layer_id].gates[i].ty);
					gate_meet[C -> circuit[sumcheck_layer_id].gates[i].ty] = true;
				}
				auto tmp = intermediates1[i];
				for(int j = 0; j < C -> circuit[sumcheck_layer_id].gates[i].parameter_length; ++j)
				{
					int src = C -> circuit[sumcheck_layer_id].gates[i].src[j];
					prime_field::field_element weight = C -> circuit[sumcheck_layer_id].gates[i].weight[j];
					add_mult_sum[src].b = add_mult_sum[src].b + weight * tmp;
				}
				break;
			}
			case 4: //direct relay gate
			{
				if(!gate_meet[C -> circuit[sumcheck_layer_id].gates[i].ty])
				{
					//printf("first meet %d gate\n", C -> circuit[sumcheck_layer_id].gates[i].ty);
					gate_meet[C -> circuit[sumcheck_layer_id].gates[i].ty] = true;
				}
				add_mult_sum[u].b = (add_mult_sum[u].b + intermediates1[i]);
				break;
			}
			case 6: //NOT gate
			{
				if(!gate_meet[C -> circuit[sumcheck_layer_id].gates[i].ty])
				{
					//printf("first meet %d gate\n", C -> circuit[sumcheck_layer_id].gates[i].ty);
					gate_meet[C -> circuit[sumcheck_layer_id].gates[i].ty] = true;
				}
				add_mult_sum[u].b = (add_mult_sum[u].b - intermediates1[i]);
				addV_array[u].b = (addV_array[u].b + intermediates1[i]);
				break;
			}
			case 7: //minus gate
			{
				if(!gate_meet[C -> circuit[sumcheck_layer_id].gates[i].ty])
				{
					//printf("first meet %d gate\n", C -> circuit[sumcheck_layer_id].gates[i].ty);
					gate_meet[C -> circuit[sumcheck_layer_id].gates[i].ty] = true;
				}
				addV_array[u].b = (addV_array[u].b - (intermediates0[i]));
				add_mult_sum[u].b = (add_mult_sum[u].b + intermediates1[i]);
				break;
			}
			case 8: //XOR gate
			{
				if(!gate_meet[C -> circuit[sumcheck_layer_id].gates[i].ty])
				{
					//printf("first meet %d gate\n", C -> circuit[sumcheck_layer_id].gates[i].ty);
					gate_meet[C -> circuit[sumcheck_layer_id].gates[i].ty] = true;
				}
				addV_array[u].b = (addV_array[u].b + intermediates0[i]);
				add_mult_sum[u].b = (add_mult_sum[u].b + intermediates1[i] - intermediates0[i] - intermediates0[i]);
				break;
			}
			case 13: //bit-test gate
			{
				if(!gate_meet[C -> circuit[sumcheck_layer_id].gates[i].ty])
				{
					//printf("first meet %d gate\n", C -> circuit[sumcheck_layer_id].gates[i].ty);
					gate_meet[C -> circuit[sumcheck_layer_id].gates[i].ty] = true;
				}
				add_mult_sum[u].b = (add_mult_sum[u].b - intermediates0[i] + intermediates1[i]);
				break;
			}
			case 9: //NAAB gate
			{
				if(!gate_meet[C -> circuit[sumcheck_layer_id].gates[i].ty])
				{
					//printf("first meet %d gate\n", C -> circuit[sumcheck_layer_id].gates[i].ty);
					gate_meet[C -> circuit[sumcheck_layer_id].gates[i].ty] = true;
				}
				addV_array[u].b = (addV_array[u].b + intermediates1[i]);
				add_mult_sum[u].b = (add_mult_sum[u].b - intermediates1[i]);
				break;
			}
			case 10: //relay gate
			{
				if(!gate_meet[C -> circuit[sumcheck_layer_id].gates[i].ty])
				{
					//printf("first meet %d gate\n", C -> circuit[sumcheck_layer_id].gates[i].ty);
					gate_meet[C -> circuit[sumcheck_layer_id].gates[i].ty] = true;
				}
				add_mult_sum[u].b = (add_mult_sum[u].b + intermediates0[i]);
				break;
			}
			default:
			{
				printf("Warning Unknown gate %d\n", C -> circuit[sumcheck_layer_id].gates[i].ty);
				break;
			}
		}
	}

	delete[] intermediates0;
	delete[] intermediates1;
	
	std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t0);
	total_time += time_span.count();
}


//new zk function
quadratic_poly zk_prover::sumcheck_phase1_update(prime_field::field_element previous_random, int current_bit)
{	
	std::chrono::high_resolution_clock::time_point t0 = std::chrono::high_resolution_clock::now();
	quadratic_poly ret = quadratic_poly(prime_field::field_element(0), prime_field::field_element(0), prime_field::field_element(0));
	
	#pragma omp parallel for
	for(int i = 0; i < (total_uv >> 1); ++i)
	{
		prime_field::field_element zero_value, one_value;
		int g_zero = i << 1, g_one = i << 1 | 1;
		if(current_bit == 0)
		{
			V_mult_add_new[i].b = V_mult_add[g_zero].b;
			V_mult_add_new[i].a = V_mult_add[g_one].b - V_mult_add_new[i].b;

			addV_array_new[i].b = addV_array[g_zero].b;
			addV_array_new[i].a = addV_array[g_one].b - addV_array_new[i].b;

			add_mult_sum_new[i].b = add_mult_sum[g_zero].b;
			add_mult_sum_new[i].a = add_mult_sum[g_one].b - add_mult_sum_new[i].b;

		}
		else
		{
			V_mult_add_new[i].b = (V_mult_add[g_zero].a * previous_random + V_mult_add[g_zero].b);
			V_mult_add_new[i].a = (V_mult_add[g_one].a * previous_random + V_mult_add[g_one].b - V_mult_add_new[i].b);

			addV_array_new[i].b = (addV_array[g_zero].a * previous_random + addV_array[g_zero].b);
			addV_array_new[i].a = (addV_array[g_one].a * previous_random + addV_array[g_one].b - addV_array_new[i].b);

			add_mult_sum_new[i].b = (add_mult_sum[g_zero].a * previous_random + add_mult_sum[g_zero].b);
			add_mult_sum_new[i].a = (add_mult_sum[g_one].a * previous_random + add_mult_sum[g_one].b - add_mult_sum_new[i].b);

		}
	}
	std::swap(V_mult_add, V_mult_add_new);
	std::swap(addV_array, addV_array_new);
	std::swap(add_mult_sum, add_mult_sum_new);
	
	//parallel addition tree

	#pragma omp parallel for
	for(int i = 0; i < (total_uv >> 1); ++i)
	{
		rets_prev[i].a = add_mult_sum[i].a * V_mult_add[i].a;
		rets_prev[i].b = add_mult_sum[i].a * V_mult_add[i].b + add_mult_sum[i].b * V_mult_add[i].a + addV_array[i].a;
		rets_prev[i].c = add_mult_sum[i].b * V_mult_add[i].b + addV_array[i].b;
	}

	const int tot = total_uv >> 1;
	for(int i = 1; (1 << i) <= (total_uv >> 1); ++i)
	{
		#pragma omp parallel for
		for(int j = 0; j < (tot >> i); ++j)
		{
			rets_cur[j] = rets_prev[j * 2] + rets_prev[j * 2 + 1];
		}
		#pragma omp barrier
		std::swap(rets_prev, rets_cur);
	}
	ret = rets_prev[0];

	total_uv >>= 1;

	std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t0);
	total_time += time_span.count();
	return ret;
}



void zk_prover::sumcheck_phase2_init(prime_field::field_element previous_random, const prime_field::field_element* r_u, const prime_field::field_element* one_minus_r_u)
{
	std::chrono::high_resolution_clock::time_point t0 = std::chrono::high_resolution_clock::now();
	
	v_u = V_mult_add[0].eval(previous_random);

	int first_half = length_u >> 1, second_half = length_u - first_half;

	beta_u_fhalf[0] = prime_field::field_element(1);
	beta_u_shalf[0] = prime_field::field_element(1);
	for(int i = 0; i < first_half; ++i)
	{
		for(int j = 0; j < (1 << i); ++j)
		{
			beta_u_fhalf[j | (1 << i)] = beta_u_fhalf[j] * r_u[i];
			beta_u_fhalf[j] = beta_u_fhalf[j] * one_minus_r_u[i];
		}
	}

	for(int i = 0; i < second_half; ++i)
	{
		for(int j = 0; j < (1 << i); ++j)
		{
			beta_u_shalf[j | (1 << i)] = beta_u_shalf[j] * r_u[i + first_half];
			beta_u_shalf[j] = beta_u_shalf[j] * one_minus_r_u[i + first_half];
		}
	}

	int mask_fhalf = (1 << first_half) - 1;
	int first_g_half = (length_g >> 1);
	int mask_g_fhalf = (1 << (length_g >> 1)) - 1;
	
	total_uv = (1 << C -> circuit[sumcheck_layer_id - 1].bit_length);
	int total_g = (1 << C -> circuit[sumcheck_layer_id].bit_length);
	prime_field::field_element zero = prime_field::field_element(0);
	for(int i = 0; i < total_uv; ++i)
	{
		add_mult_sum[i].a = zero;
		add_mult_sum[i].b = zero;
		addV_array[i].a = zero;
		addV_array[i].b = zero;
		V_mult_add[i] = circuit_value[sumcheck_layer_id - 1][i];
	}

	prime_field::field_element *intermediates0 = new prime_field::field_element[total_g];
	prime_field::field_element *intermediates1 = new prime_field::field_element[total_g];
	prime_field::field_element *intermediates2 = new prime_field::field_element[total_g];

	#pragma omp parallel for
	for(int i = 0; i < total_g; ++i)
	{
		int ty = C -> circuit[sumcheck_layer_id].gates[i].ty;
		int u = C -> circuit[sumcheck_layer_id].gates[i].u;
		int v = C -> circuit[sumcheck_layer_id].gates[i].v;
		switch(ty)
		{
			case 1: //mult gate
			{
				auto tmp_u = beta_u_fhalf[u & mask_fhalf] * beta_u_shalf[u >> first_half];
				auto tmp_g = (beta_g_r0_fhalf[i & mask_g_fhalf] * beta_g_r0_shalf[i >> first_g_half] 
								+ beta_g_r1_fhalf[i & mask_g_fhalf] * beta_g_r1_shalf[i >> first_g_half]);
				intermediates0[i] = tmp_g * tmp_u * v_u;
				break;
			}
			case 0: //add gate
			{
				auto tmp_u = beta_u_fhalf[u & mask_fhalf] * beta_u_shalf[u >> first_half];
				auto tmp_g = (beta_g_r0_fhalf[i & mask_g_fhalf] * beta_g_r0_shalf[i >> first_g_half] 
								+ beta_g_r1_fhalf[i & mask_g_fhalf] * beta_g_r1_shalf[i >> first_g_half]);
				auto tmp_g_u = tmp_g * tmp_u;
				intermediates0[i] = tmp_g_u;
				intermediates1[i] = tmp_g_u * v_u;
				break;
			}
			case 2:
			{
				break;
			}
			case 4:
			{
				break;
			}
			case 5: //sum gate
			{
				auto tmp_g = (beta_g_r0_fhalf[i & mask_g_fhalf] * beta_g_r0_shalf[i >> first_g_half] 
								+ beta_g_r1_fhalf[i & mask_g_fhalf] * beta_g_r1_shalf[i >> first_g_half]);
				auto tmp_g_vu = tmp_g * v_u;
				intermediates0[i] = tmp_g_vu;
				break;
			}
			case 12: //exp sum gate
			{
				auto tmp_g = (beta_g_r0_fhalf[i & mask_g_fhalf] * beta_g_r0_shalf[i >> first_g_half] 
								+ beta_g_r1_fhalf[i & mask_g_fhalf] * beta_g_r1_shalf[i >> first_g_half]);
				auto tmp_g_vu = tmp_g * v_u;
				intermediates0[i] = tmp_g_vu;
				break;
			}
			case 14: //custom comb gate
			{
				auto tmp_g = (beta_g_r0_fhalf[i & mask_g_fhalf] * beta_g_r0_shalf[i >> first_g_half] 
								+ beta_g_r1_fhalf[i & mask_g_fhalf] * beta_g_r1_shalf[i >> first_g_half]);
				auto tmp_g_vu = tmp_g * v_u;
				intermediates0[i] = tmp_g_vu;
				break;
			}

			case 6: //not gate
			{
				auto tmp_u = beta_u_fhalf[u & mask_fhalf] * beta_u_shalf[u >> first_half];
				auto tmp_g = (beta_g_r0_fhalf[i & mask_g_fhalf] * beta_g_r0_shalf[i >> first_g_half] 
								+ beta_g_r1_fhalf[i & mask_g_fhalf] * beta_g_r1_shalf[i >> first_g_half]);
				auto tmp_g_u = tmp_g * tmp_u;
				intermediates0[i] = tmp_g_u - tmp_g_u * v_u;
				break;
			}
			case 7: //minus gate
			{
				auto tmp_u = beta_u_fhalf[u & mask_fhalf] * beta_u_shalf[u >> first_half];
				auto tmp_g = (beta_g_r0_fhalf[i & mask_g_fhalf] * beta_g_r0_shalf[i >> first_g_half] 
								+ beta_g_r1_fhalf[i & mask_g_fhalf] * beta_g_r1_shalf[i >> first_g_half]);
				auto tmp = tmp_g * tmp_u;
				intermediates0[i] = tmp;
				intermediates1[i] = tmp * v_u;
				break;
			}
			case 8: //xor gate
			{
				auto tmp_u = beta_u_fhalf[u & mask_fhalf] * beta_u_shalf[u >> first_half];
				auto tmp_g = (beta_g_r0_fhalf[i & mask_g_fhalf] * beta_g_r0_shalf[i >> first_g_half] 
								+ beta_g_r1_fhalf[i & mask_g_fhalf] * beta_g_r1_shalf[i >> first_g_half]);
				auto tmp = tmp_g * tmp_u;
				auto tmp_v_u = tmp * v_u;
				intermediates0[i] = tmp - tmp_v_u - tmp_v_u;
				intermediates1[i] = tmp_v_u;
				break;
			}
			case 13: //bit-test gate
			{
				auto tmp_u = beta_u_fhalf[u & mask_fhalf] * beta_u_shalf[u >> first_half];
				auto tmp_g = (beta_g_r0_fhalf[i & mask_g_fhalf] * beta_g_r0_shalf[i >> first_g_half] 
								+ beta_g_r1_fhalf[i & mask_g_fhalf] * beta_g_r1_shalf[i >> first_g_half]);
				auto tmp = tmp_g * tmp_u;
				auto tmp_v_u = tmp * v_u;
				intermediates0[i] = tmp_v_u;
				break;
			}
			case 9: //NAAB gate
			{
				auto tmp_u = beta_u_fhalf[u & mask_fhalf] * beta_u_shalf[u >> first_half];
				auto tmp_g = (beta_g_r0_fhalf[i & mask_g_fhalf] * beta_g_r0_shalf[i >> first_g_half] 
								+ beta_g_r1_fhalf[i & mask_g_fhalf] * beta_g_r1_shalf[i >> first_g_half]);
				auto tmp = tmp_g * tmp_u;
				intermediates0[i] = tmp - v_u * tmp;
				break;
			}
			case 10: //relay gate
			{
				auto tmp_u = beta_u_fhalf[u & mask_fhalf] * beta_u_shalf[u >> first_half];
				auto tmp_g = (beta_g_r0_fhalf[i & mask_g_fhalf] * beta_g_r0_shalf[i >> first_g_half] 
								+ beta_g_r1_fhalf[i & mask_g_fhalf] * beta_g_r1_shalf[i >> first_g_half]);
				auto tmp = tmp_g * tmp_u;
				intermediates0[i] = tmp * v_u;
				break;
			}
			default:
			{
				printf("Warning Unknown gate %d\n", ty);
				break;
			}
		}
	}
	
	
	for(int i = 0; i < total_g; ++i)
	{
		int ty = C -> circuit[sumcheck_layer_id].gates[i].ty;
		int u = C -> circuit[sumcheck_layer_id].gates[i].u;
		int v = C -> circuit[sumcheck_layer_id].gates[i].v;
		switch(ty)
		{
			case 1: //mult gate
			{
				add_mult_sum[v].b = add_mult_sum[v].b + intermediates0[i];
				break;
			}
			case 0: //add gate
			{
				add_mult_sum[v].b = (add_mult_sum[v].b + intermediates0[i]);
				addV_array[v].b = (intermediates1[i] + addV_array[v].b);
				break;
			}
			case 2:
			{
				break;
			}
			case 4:
			{
				break;
			}
			case 5: //sum gate
			{
				for(int j = u; j < v; ++j)
				{
					auto tmp_u = beta_u_fhalf[j & mask_fhalf] * beta_u_shalf[j >> first_half];
					addV_array[0].b = (addV_array[0].b + intermediates0[i] * tmp_u);
				}
				break;
			}
			case 12: //exp sum gate
			{
				auto tmp_g_vu = intermediates0[i];
				
				for(int j = u; j <= v; ++j)
				{
					auto tmp_u = beta_u_fhalf[j & mask_fhalf] * beta_u_shalf[j >> first_half];
					addV_array[0].b = (addV_array[0].b + tmp_g_vu * tmp_u);
					tmp_g_vu = tmp_g_vu + tmp_g_vu;
				}
				break;
			}
			case 14: //custom comb gate
			{
				auto tmp_g_vu = intermediates0[i];
				
				for(int j = 0; j < C -> circuit[sumcheck_layer_id].gates[i].parameter_length; ++j)
				{
					long long src = C -> circuit[sumcheck_layer_id].gates[i].src[j];
					auto tmp_u = beta_u_fhalf[src & mask_fhalf] * beta_u_shalf[src >> first_half];
					prime_field::field_element weight = C -> circuit[sumcheck_layer_id].gates[i].weight[j];
					addV_array[0].b = (addV_array[0].b + tmp_g_vu * tmp_u * weight);
				}
				break;
			}
			case 6: //not gate
			{
				addV_array[v].b = (addV_array[v].b + intermediates0[i]);
				break;
			}
			case 7: //minus gate
			{
				add_mult_sum[v].b = (add_mult_sum[v].b - intermediates0[i]);
				addV_array[v].b = (intermediates1[i] + addV_array[v].b);
				break;
			}
			case 8: //xor gate
			{
				add_mult_sum[v].b = (add_mult_sum[v].b + intermediates0[i]);
				addV_array[v].b = (addV_array[v].b + intermediates1[i]);
				break;
			}
			case 13: //bit-test gate
			{
				add_mult_sum[v].b = (add_mult_sum[v].b - intermediates0[i]);
				addV_array[v].b = (addV_array[v].b + intermediates0[i]);
				break;
			}
			case 9: //NAAB gate
			{
				add_mult_sum[v].b = (add_mult_sum[v].b + intermediates0[i]);
				break;
			}
			case 10: //relay gate
			{
				addV_array[v].b = (addV_array[v].b + intermediates0[i]);
				break;
			}
			default:
			{
				printf("Warning Unknown gate %d\n", ty);
				break;
			}
		}
	}

	delete[] intermediates0;
	delete[] intermediates1;
	delete[] intermediates2;

	std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t0);
	total_time += time_span.count();
}


//new zk function

quadratic_poly zk_prover::sumcheck_phase2_update(prime_field::field_element previous_random, int current_bit)
{
	std::chrono::high_resolution_clock::time_point t0 = std::chrono::high_resolution_clock::now();
	quadratic_poly ret = quadratic_poly(prime_field::field_element(0), prime_field::field_element(0), prime_field::field_element(0));
	#pragma omp parallel for
	for(int i = 0; i < (total_uv >> 1); ++i)
	{
		int g_zero = i << 1, g_one = i << 1 | 1;
		if(current_bit == 0)
		{
			V_mult_add_new[i].b = V_mult_add[g_zero].b;
			V_mult_add_new[i].a = V_mult_add[g_one].b - V_mult_add_new[i].b;

			addV_array_new[i].b = addV_array[g_zero].b;
			addV_array_new[i].a = addV_array[g_one].b - addV_array_new[i].b;

			add_mult_sum_new[i].b = add_mult_sum[g_zero].b;
			add_mult_sum_new[i].a = add_mult_sum[g_one].b - add_mult_sum_new[i].b;
		}
		else
		{
			
			V_mult_add_new[i].b = (V_mult_add[g_zero].a * previous_random + V_mult_add[g_zero].b);
			V_mult_add_new[i].a = (V_mult_add[g_one].a * previous_random + V_mult_add[g_one].b - V_mult_add_new[i].b);

			addV_array_new[i].b = (addV_array[g_zero].a * previous_random + addV_array[g_zero].b);
			addV_array_new[i].a = (addV_array[g_one].a * previous_random + addV_array[g_one].b - addV_array_new[i].b);

			add_mult_sum_new[i].b = (add_mult_sum[g_zero].a * previous_random + add_mult_sum[g_zero].b);
			add_mult_sum_new[i].a = (add_mult_sum[g_one].a * previous_random + add_mult_sum[g_one].b - add_mult_sum_new[i].b);
		}

		ret.a = (ret.a + add_mult_sum[i].a * V_mult_add[i].a);
		ret.b = (ret.b + add_mult_sum[i].a * V_mult_add[i].b + add_mult_sum[i].b * V_mult_add[i].a + addV_array[i].a);
		ret.c = (ret.c + add_mult_sum[i].b * V_mult_add[i].b + addV_array[i].b);
	}

	std::swap(V_mult_add, V_mult_add_new);
	std::swap(addV_array, addV_array_new);
	std::swap(add_mult_sum, add_mult_sum_new);
	
	//parallel addition tree

	#pragma omp parallel for
	for(int i = 0; i < (total_uv >> 1); ++i)
	{
		rets_prev[i].a = add_mult_sum[i].a * V_mult_add[i].a;
		rets_prev[i].b = add_mult_sum[i].a * V_mult_add[i].b + add_mult_sum[i].b * V_mult_add[i].a + addV_array[i].a;
		rets_prev[i].c = add_mult_sum[i].b * V_mult_add[i].b + addV_array[i].b;
	}

	const int tot = total_uv >> 1;
	for(int i = 1; (1 << i) <= (total_uv >> 1); ++i)
	{
		#pragma omp parallel for
		for(int j = 0; j < (tot >> i); ++j)
		{
			rets_cur[j] = rets_prev[j * 2] + rets_prev[j * 2 + 1];
		}
		#pragma omp barrier
		std::swap(rets_prev, rets_cur);
	}
	ret = rets_prev[0];

	total_uv >>= 1;
	std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

	std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t0);
	total_time += time_span.count();
	return ret;
}


std::pair<prime_field::field_element, prime_field::field_element> zk_prover::sumcheck_finalize(prime_field::field_element previous_random)
{
	v_v = V_mult_add[0].eval(previous_random);
	return std::make_pair(v_u, v_v);
}
void zk_prover::proof_init()
{
	//todo
}
