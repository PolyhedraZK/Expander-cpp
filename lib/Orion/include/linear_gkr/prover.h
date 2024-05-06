#ifndef __zk_prover
#define __zk_prover

#include "linear_gkr/circuit_fast_track.h"
#include "linear_gkr/prime_field.h"
#include "linear_gkr/polynomial.h"
#include <cstring>
#include <utility>
#include <vector>
#include <chrono>
#include "VPD/vpd_prover.h"
#include "infrastructure/my_hhash.h"
#include "VPD/fri.h"
#include "poly_commitment/poly_commit.h"

class zk_prover
{
public:
	poly_commit::poly_commit_prover poly_prover;
	/** @name Basic
	* Basic information and variables about the arithmetic circuit C.*/
	///@{
	prime_field::field_element v_u, v_v; //!< two random gates v_u and v_v queried by V in each layer
	int total_uv;
	layered_circuit *C;
	prime_field::field_element* circuit_value[1000000];

	int sumcheck_layer_id, length_g, length_u, length_v;
	///@}

	/** @name Randomness
	* Some randomness or values during the proof phase. */
	///@{
	prime_field::field_element alpha, beta;
	const prime_field::field_element *r_0, *r_1;
	prime_field::field_element *one_minus_r_0, *one_minus_r_1;

	linear_poly *addV_array;
	linear_poly *V_mult_add;
	prime_field::field_element *beta_g_r0_fhalf, *beta_g_r0_shalf, *beta_g_r1_fhalf, *beta_g_r1_shalf, *beta_u_fhalf, *beta_u_shalf;
	prime_field::field_element *beta_u, *beta_v, *beta_g;
	linear_poly *add_mult_sum;
	///@}


	double total_time;
	/**Initialize some arrays used in the protocol.*/
	void init_array(int);
	/**Read the circuit from the input file.*/
	void get_circuit(layered_circuit &from_verifier);
	/**Evaluate the output of the circuit.*/
	prime_field::field_element* evaluate();
	void get_witness(prime_field::field_element*, int);
	/**Generate random mask polynomials and initialize parameters. */
	void proof_init();
	
	/** @name Group function for interior sumcheck protocol. 
	*Run linear GKR protocol: for each layer of the circuit, our protocol is based on sumcheck protocol. And there are total three phases of the sumcheck:
	*1. sumcheck for gate u
	*2. sumcheck for gate v
	*3. final round for the extra mask bit
 	*/ 
 	///@{
	void sumcheck_init(int layer_id, int bit_length_g, int bit_length_u, int bit_length_v, const prime_field::field_element &,
		const prime_field::field_element &, const prime_field::field_element*, const prime_field::field_element*, prime_field::field_element*, prime_field::field_element*);
	void sumcheck_phase1_init();
	void sumcheck_phase2_init(prime_field::field_element, const prime_field::field_element*, const prime_field::field_element*);
	quadratic_poly sumcheck_phase1_update(prime_field::field_element, int);
	quadratic_poly sumcheck_phase2_update(prime_field::field_element, int);

	///@}
	/**I do not know what it is*/
	prime_field::field_element V_res(const prime_field::field_element*, const prime_field::field_element*, const prime_field::field_element*, int, int);
	std::pair<prime_field::field_element, prime_field::field_element> sumcheck_finalize(prime_field::field_element);
	void delete_self();
	zk_prover()
	{
		memset(circuit_value, 0, sizeof circuit_value);
	}
	~zk_prover(); 
	__hhash_digest prover_vpd_prepare();
	__hhash_digest prover_vpd_prepare_post_gkr(prime_field::field_element *r_0, prime_field::field_element *one_minus_r_0, int r_0_len, prime_field::field_element target_sum, prime_field::field_element *all_sum);
	///@}
};

#endif
