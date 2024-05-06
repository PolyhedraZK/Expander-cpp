#include "linear_code/linear_code_encode.h"

prime_field::field_element *scratch[2][100];
bool __encode_initialized = false;