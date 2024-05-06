#include "VPD/vpd_prover.h"
#include "infrastructure/RS_polynomial.h"
#include "infrastructure/constants.h"
#include "VPD/fri.h"

__hhash_digest merkle_root;

__hhash_digest vpd_prover_init(prime_field::field_element *l_eval, prime_field::field_element *&l_coef, int log_input_length, int slice_size, int slice_count)
{
    //fft and apply mask
    merkle_root = fri::request_init_commit(log_input_length, 0);
    return merkle_root;
}