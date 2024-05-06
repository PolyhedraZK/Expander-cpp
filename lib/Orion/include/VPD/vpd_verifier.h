#ifndef __vpd_verifier
#define __vpd_verifier
#include <vector>
#include "linear_gkr/prime_field.h"
bool vpd_verify(prime_field::field_element all_mask_sum, double &v_time);
#endif