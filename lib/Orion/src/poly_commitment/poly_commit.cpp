#include "poly_commitment/poly_commit.h"

prime_field::field_element *poly_commit::inner_prod_evals;
prime_field::field_element *poly_commit::l_coef, *poly_commit::l_eval, *poly_commit::q_coef, *poly_commit::q_eval; //l for private input, q for public randomness
prime_field::field_element *poly_commit::lq_eval, *poly_commit::h_coef, *poly_commit::lq_coef, *poly_commit::h_eval;
prime_field::field_element *poly_commit::h_eval_arr;
int poly_commit::l_coef_len, poly_commit::l_eval_len, poly_commit::q_coef_len, poly_commit::q_eval_len;
int poly_commit::slice_size, poly_commit::slice_count, poly_commit::slice_real_ele_cnt;
bool poly_commit::pre_prepare_executed = false;
