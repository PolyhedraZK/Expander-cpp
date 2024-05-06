#ifndef __vpd_prover
#define __vpd_prover
#include <vector>
#include "linear_gkr/prime_field.h"
#include "infrastructure/my_hhash.h"
__hhash_digest vpd_prover_init(prime_field::field_element *l_eval, prime_field::field_element *&l_coef, int log_input_length, int slice_size, int slice_count);
#endif