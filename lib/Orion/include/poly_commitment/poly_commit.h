#pragma once
#ifndef __poly_commit
#define __poly_commit
#include <algorithm>
#include "infrastructure/RS_polynomial.h"
#include "infrastructure/utility.h"
#include <vector>
#include "VPD/fri.h"
#include "VPD/vpd_prover.h"
#include <chrono>

extern prime_field::field_element* twiddle_factor;
extern prime_field::field_element* inv_twiddle_factor;
extern long long twiddle_factor_size;
namespace poly_commit
{
	extern prime_field::field_element *inner_prod_evals;
	extern prime_field::field_element *l_coef, *l_eval, *q_coef, *q_eval; //l for private input, q for public randomness
	extern prime_field::field_element *lq_eval, *h_coef, *lq_coef, *h_eval;
	extern prime_field::field_element *h_eval_arr;
	extern int l_coef_len, l_eval_len, q_coef_len, q_eval_len;
	extern int slice_size, slice_count, slice_real_ele_cnt;
	extern bool pre_prepare_executed;
	
	class ldt_commitment
	{
	public:
		__hhash_digest* commitment_hhash;
		prime_field::field_element *randomness;
		prime_field::field_element *final_rs_code;
		int mx_depth;
		int repeat_no;
	};
	
	class poly_commit_prover
	{
	private:
		
	public:
		double total_time;
		
		__hhash_digest commit_private_array(prime_field::field_element *private_array, int log_array_length)
		{
			total_time = 0;
			std::chrono::high_resolution_clock::time_point t0 = std::chrono::high_resolution_clock::now();
			pre_prepare_executed = true;
			slice_count = 1 << log_slice_number;
			slice_size = (1 << (log_array_length + rs_code_rate - log_slice_number));
			slice_real_ele_cnt = slice_size >> rs_code_rate;
			//prepare polynomial division/decomposition
			//things are going to be sliced
			//get evaluations
			l_eval_len = slice_count * slice_size;
			l_eval = new prime_field::field_element[l_eval_len];

			prime_field::field_element *tmp;
			tmp = new prime_field::field_element[slice_real_ele_cnt];

			auto fft_t0 = std::chrono::high_resolution_clock::now();

			init_scratch_pad(slice_size * slice_count);
			for(int i = 0; i < slice_count; ++i)
			{
				bool all_zero = true;
				auto zero = prime_field::field_element(0);
				for(int j = 0; j < slice_real_ele_cnt; ++j)
				{
					if(private_array[i * slice_real_ele_cnt + j] == zero)
						continue;
					all_zero = false;
					break;
				}
				if(all_zero)
				{
					for(int j = 0; j < slice_size; ++j)
						l_eval[i * slice_size + j] = zero;
				}
				else
				{
					inverse_fast_fourier_transform(&private_array[i * slice_real_ele_cnt], slice_real_ele_cnt, slice_real_ele_cnt, prime_field::get_root_of_unity(mylog(slice_real_ele_cnt)), tmp);
					fast_fourier_transform(tmp, slice_real_ele_cnt, slice_size, prime_field::get_root_of_unity(mylog(slice_size)), &l_eval[i * slice_size]);
				}
			}

			auto fft_t1 = std::chrono::high_resolution_clock::now();
			printf("FFT Prepare time %lf\n", std::chrono::duration_cast<std::chrono::duration<double>>(fft_t1 - fft_t0).count());
			delete[] tmp;
			
			auto ret = vpd_prover_init(l_eval, l_coef, log_array_length, slice_size, slice_count);
			
			std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t0);
			total_time += time_span.count();
			printf("VPD prepare time %lf\n", time_span.count());
			return ret;
		}
		__hhash_digest commit_public_array(prime_field::field_element *public_array, int r_0_len, prime_field::field_element target_sum, prime_field::field_element *all_sum)
		{
			std::chrono::high_resolution_clock::time_point t0 = std::chrono::high_resolution_clock::now();
			assert(pre_prepare_executed);
			fri::virtual_oracle_witness = new prime_field::field_element[slice_size * slice_count];
			fri::virtual_oracle_witness_mapping = new int[slice_size * slice_count];
			q_eval_len = l_eval_len;
			q_eval = new prime_field::field_element[q_eval_len];
			
			prime_field::field_element *tmp;
			tmp = new prime_field::field_element[slice_size];
			
			double fft_time = 0;
			double re_mapping_time = 0;
			
			auto fft_t0 = std::chrono::high_resolution_clock::now();

			for(int i = 0; i < slice_count; ++i)
			{
				inverse_fast_fourier_transform(&public_array[i * slice_real_ele_cnt], slice_real_ele_cnt, slice_real_ele_cnt, prime_field::get_root_of_unity(mylog(slice_real_ele_cnt)), tmp);
				fast_fourier_transform(tmp, slice_real_ele_cnt, slice_size, prime_field::get_root_of_unity(mylog(slice_size)), &q_eval[i * slice_size]);
			}
			
			auto fft_t1 = std::chrono::high_resolution_clock::now();
			fft_time += std::chrono::duration_cast<std::chrono::duration<double>>(fft_t1 - fft_t0).count();

			prime_field::field_element sum = prime_field::field_element(0);
			assert(slice_count * slice_real_ele_cnt == (1 << r_0_len));


			for(int i = 0; i < slice_count * slice_real_ele_cnt; ++i)
			{
				assert((i << rs_code_rate) < q_eval_len);
				sum = sum + q_eval[i << rs_code_rate] * l_eval[i << rs_code_rate];
			}
			assert(sum == target_sum);
			//do fft for q_eval
			//experiment
			//poly mul first
			lq_eval = new prime_field::field_element[2 * slice_real_ele_cnt];
			h_coef = new prime_field::field_element[slice_real_ele_cnt];
			lq_coef = new prime_field::field_element[2 * slice_real_ele_cnt];
			h_eval = new prime_field::field_element[std::max(slice_size, slice_real_ele_cnt)];
			h_eval_arr = new prime_field::field_element[slice_count * slice_size];
			const int log_leaf_size = log_slice_number + 1;
			for(int i = 0; i < slice_count; ++i)
			{
				
				assert(2 * slice_real_ele_cnt <= slice_size);
				bool all_zero = true;
				auto zero = prime_field::field_element(0);
				#pragma omp parallel for
				for(int j = 0; j < 2 * slice_real_ele_cnt; ++j)
				{
					lq_eval[j] = l_eval[i * slice_size + j * (slice_size / (2 * slice_real_ele_cnt))] * q_eval[i * slice_size + j * (slice_size / (2 * slice_real_ele_cnt))];
					if(lq_eval[j] != zero)
					{
						all_zero = false;
					}
				}
				if(all_zero)
				{
					#pragma omp parallel for
					for(int j = 0; j < 2 * slice_real_ele_cnt; ++j)
						lq_coef[j] = zero;
					#pragma omp parallel for
					for(int j = 0; j < slice_real_ele_cnt; ++j)
						h_coef[j] = zero;
					#pragma omp parallel for
					for(int j = 0; j < slice_size; ++j)
						h_eval[j] = zero;
				}
				else
				{
					fft_t0 = std::chrono::high_resolution_clock::now();
					inverse_fast_fourier_transform(lq_eval, 2 * slice_real_ele_cnt, 2 * slice_real_ele_cnt, prime_field::get_root_of_unity(mylog(2 * slice_real_ele_cnt)), lq_coef);
					#pragma omp parallel for
					for(int j = 0; j < slice_real_ele_cnt; ++j)
						h_coef[j] = lq_coef[j + slice_real_ele_cnt];
					fast_fourier_transform(h_coef, slice_real_ele_cnt, slice_size, prime_field::get_root_of_unity(mylog(slice_size)), h_eval);
					fft_t1 = std::chrono::high_resolution_clock::now();
					fft_time += std::chrono::duration_cast<std::chrono::duration<double>>(fft_t1 - fft_t0).count();
				}

				/*
				auto rou = prime_field::get_root_of_unity(mylog(slice_size));
				auto inv_rou = prime_field::inv(rou);
				auto rou_n = prime_field::fast_pow(rou, slice_real_ele_cnt);
				auto x_n = prime_field::field_element(1);
				auto inv_x = prime_field::field_element(slice_real_ele_cnt); //absorb the constant factor
				*/
				const long long twiddle_gap = twiddle_factor_size / slice_size * slice_real_ele_cnt;
				const long long inv_twiddle_gap = twiddle_factor_size / slice_size;

				auto remap_t0 = std::chrono::high_resolution_clock::now();
				auto const_sum = prime_field::field_element(0) - (lq_coef[0] + h_coef[0]);
				#pragma omp parallel for
				for(int j = 0; j < slice_size; ++j)
				{
				//	assert(l_eval[i * slice_size + j] * q_eval[i * slice_size + j] == g_eval[i][j] + (prime_field::fast_pow(x, slice_real_ele_cnt) - 1) * h_eval[i][j]);
					auto g = l_eval[i * slice_size + j] * q_eval[i * slice_size + j] - (twiddle_factor[twiddle_gap * j % twiddle_factor_size] - 1) * h_eval[j];
					if(j < slice_size / 2)
					{
						assert(((j) << log_leaf_size | (i << 1) | 0) < slice_count * slice_size);
						assert((((j) << log_leaf_size) & (i << 1)) == 0);
						fri::virtual_oracle_witness[(j) << log_leaf_size | (i << 1) | 0] = (g + const_sum) * inv_twiddle_factor[inv_twiddle_gap * j % twiddle_factor_size] * prime_field::field_element(slice_real_ele_cnt);
						fri::virtual_oracle_witness_mapping[j << log_slice_number | i] = j << log_leaf_size | (i << 1) | 0;
					}
					else
					{
						int jj = j - slice_size / 2;
						assert(((jj) << log_leaf_size | (i << 1) | 1) < slice_count * slice_size);
						assert((((jj) << log_leaf_size) & (i << 1)) == 0);
						fri::virtual_oracle_witness[(jj) << log_leaf_size | (i << 1) | 1] = (g + const_sum) * inv_twiddle_factor[inv_twiddle_gap * j % twiddle_factor_size] * prime_field::field_element(slice_real_ele_cnt);
						fri::virtual_oracle_witness_mapping[jj << log_slice_number | i] = jj << log_leaf_size | (i << 1) | 0;
					}
					/*
					assert(x_n == twiddle_factor[twiddle_gap * j % twiddle_factor_size]);
					assert(inv_x == inv_twiddle_factor[inv_twiddle_gap * j % twiddle_factor_size] * prime_field::field_element(slice_real_ele_cnt));
					inv_x = inv_x * inv_rou;
					x_n = x_n * rou_n;
					*/
				}
				auto remap_t1 = std::chrono::high_resolution_clock::now();
				re_mapping_time += std::chrono::duration_cast<std::chrono::duration<double>>(remap_t1 - remap_t0).count();
				assert(i < slice_number + 1);
				all_sum[i] = (lq_coef[0] + h_coef[0]) * prime_field::field_element(slice_real_ele_cnt);
				
				#pragma omp parallel for
				for(int j = 0; j < slice_size; ++j)
					h_eval_arr[i * slice_size + j] = h_eval[j];
			}
			delete[] tmp;
			delete[] lq_eval;
			delete[] h_coef;
			delete[] lq_coef;
			delete[] h_eval;
			std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t0);
			total_time += time_span.count();
			printf("PostGKR FFT time %lf\n", fft_time);
			printf("PostGKR remap time %lf\n", re_mapping_time);
			printf("PostGKR prepare time 0 %lf\n", time_span.count());
			
			t0 = std::chrono::high_resolution_clock::now();
			auto ret = fri::request_init_commit(r_0_len, 1);
			t1 = std::chrono::high_resolution_clock::now();
			time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t0);
			total_time += time_span.count();
			
			printf("PostGKR prepare time 1 %lf\n", time_span.count());
			return ret;
		}
		ldt_commitment commit_phase(int log_length);
	};
	

	class poly_commit_verifier
	{
	public:
		poly_commit_prover *p;
		bool verify_poly_commitment(prime_field::field_element* all_sum, int log_length, prime_field::field_element *public_array, double &v_time, int &proof_size, double &p_time, __hhash_digest merkle_tree_l, __hhash_digest merkle_tree_h);
	};
}
#endif
