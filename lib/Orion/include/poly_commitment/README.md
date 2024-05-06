# Guide to use this vector commitment

Suggested header file:
```c++
#include "poly_commitment/poly_commit.h"
#include "infrastructure/RS_polynomial.h"
#include "infrastructure/utility.h"
#include "infrastructure/constants.h"
#include "linear_gkr/prime_field.h"
#include "infrastructure/my_hhash.h"
```

The vector commitment scheme is divided into two classes in the following:

```c++
class poly_commit_verifier
{
public:
	poly_commit_prover *p;
	bool verify_poly_commitment(prime_field::field_element* all_sum, int log_length, prime_field::field_element *processed_public_array, std::vector<prime_field::field_element> &all_pub_mask, double &v_time, int &proof_size, double &p_time, __hhash_digest merkle_tree_l, __hhash_digest merkle_tree_h);
};
  
class poly_commit_prover
{
public:
  __hhash_digest commit_private_array(prime_field::field_element *private_array, int log_array_length, std::vector<prime_field::field_element> private_mask_array);
  __hhash_digest commit_public_array(std::vector<prime_field::field_element> &all_pub_msk, prime_field::field_element *public_array, int r_0_len, prime_field::field_element target_sum, prime_field::field_element *all_sum);
};
```
To use these two classes, you first need to instantiate them in the following

```c++
  poly_commit_verifier poly_verifier;
  poly_commit_prover poly_prover;
```

and need to assign the pointer of prover to the verifier, so that the verifier is able to interact with prover
```c++
verifier.p = &prover;
```

Suppose the verifier has it's own public array, the prover have the same public array and it's own private array:

```c++
\\verifier side definition
prime_field::field_element *public_array;

\\prover side definition
prime_field::field_element *public_array, *private_array;
```

First step, the prover need to commit to the private array in the following way:

```c++
auto merkle_root_l = poly_prover.commit_private_array(private_array, log_length, all_pri_mask);
```

Here the variable ```all_pri_mask``` is used in our own project, and you can leave it as an empty vector. And we assume the length of the private array is ```1 << log_length```.

The verifier get the variable value ```merkle_root_l```. 

Second step, the prover get a commitment on the public array and the projected inner product value:

```c++
//slice_number is defined in infrastructure/constants.h
all_sum = new prime_field::field_element[slice_number + 1];
inner_product_sum = inner_prod(private_array, public_array);
auto merkle_root_h = poly_prover.commit_public_array(all_pub_mask, public_array, log_length, inner_product_sum, all_sum);
```

Here ```all_pub_mask``` is used in our own project, and you can leave it as an empty vector, ```all_sum``` is useless in the user's side, you just need to initialize it as we suggested.


Now all commitments are done, both verifier and prover shares ```merkle_root_h, merkle_root_l``` and the claimed inner product ```inner_product_sum```.


Third step, to verify the claim, the verifier runs:

```c++
processed_public_array = public_array_prepare_generic(public_array, log_length);
poly_ver.verify_poly_commitment(all_sum, log_length, processed_public_array, all_pub_mask, verification_time, proof_size, prover_time, merkle_root_l, merkle_root_h);
```

Here ```all_sum``` sums to ```inner_product_sum```, so we don't need to pass ```inner_product_sum``` to the verification routine.

It will return a boolean value, indicating the verification result.
