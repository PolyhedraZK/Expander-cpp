## Overview

This repo implements a gkr-based proving system aiming to prove relations that can be represented by layered arithmetic circuit.

## Structure

The major part of the code lies in the 'include' folder, including the definition of field, circuit, transcript, gkr protocol, and polynomial commitment. Details follow:

- `include/field`. This contains the definition of fields. For now we mainly use _M31_, a prime field with $p=2^{31}-1$ being a mersenne prime. Any field implements the `include/field/basefield.hpp` should work well with the remaining part of the protocol.
- `include/LinearGKR`. This is the major part of the protocol where we implement the sumcheck and the gkr protocol.
- `include/poly_commit`. Here we implements several polynomial commitment scheme including _kzg_, _fri_ and _orion_, these PC schemes are mainly used to commit the input of a layered arithmetic circuit to reduce the proof size and the verifier's computation cost.
- `include/fiat_shamir`. This is where we define the transcript used to perform fiat-shamir transformation in order to achieve non-interactivity.

## The GKR Protocol

The details can be found in the [Libra paper](https://eprint.iacr.org/2019/317.pdf). We give a short description here, presuming the readers are already familiar with the basic GKR protocol.

Given a layered arithmetic circuit $C$ with $d$ layers. The output of each layer can be written in the sumcheck form as follows:

> $$
> \tilde{v}_{i+1}(z) = \sum_x\sum_y \tilde{mul}(z, x ,y)\cdot \tilde{v}_i(x)\tilde{v}_i(y)+
> \tilde{add}(z, x)\tilde{v}_i(x) $$.
> $$

Given $x, y, z$,

- $v(z)$ evaluates into the value of the $z^{th}$ gate.
- $mul(z, x, y) = 1$ if there is a multiplication gate that connects the $z^{th}$ gate at the output layer to the $x^{th}$ and $y^{th}$ gate in the input layer, and it is 0 otherwise.
- $add(z, x) = 1$ if there is a addition gate that connects the $z^{th}$ gate at the output layer to the $x^{th}$ gate in the input layer. Note that an addition gate may have more than one input, and in this case, $add(z, x_1)=1, add(z, x_2)=1...add(z, x_n)=1$ for all $x_1, x_2...x_n$.

`~` refers to the multi-linear extension.

In order for the prover to efficiently compute the sumcheck for this function, we divide it into two phases.

### Phase one

We perform the sumcheck on variable $x$:

> $f_1(x) = \sum_x \tilde{g}(x) \cdot \tilde{v}(x)$.

where $\tilde{g}(x) = \tilde{add}(rz,x) + \sum_y \tilde{mul}(rz, x, y)\tilde{v}_i(y)$. Note that $z$ is a random point that has been chosen by the verifier at the moment, so we name it as $rz$ here.

### Phase two

We perform sumcheck on variable $y$:

> $f_2(y) = \tilde{v}(rx) \cdot \sum_y h(y) $.

where $h(y)=\tilde{mul}(rz, rx, y)\cdot\tilde{v}(y)$. Note that at this point $x$ has been fixed into some random value chosen by the verifier, so we use $rx$ here.

### Combining two claims

In gkr reduction, a random value $\tilde{v}_{i+1}(rz)$ is reduced to two claims $\tilde{v}_i(rx)$ and $\tilde{v}_i(ry)$. To combine the two claims, we simply perform sumcheck on $\alpha \tilde{v}_i(rx) + \beta \tilde{v}_i(ry)$ in the next layer.

## The GKR Protocol in the Codebase

- `include/LinearGKR/gkr.hpp` provides the interface for gkr prover and verifier, and defines the structure of the proof.
- `include/LinearGKR/scratch_pad.hpp` pre-allocates some memory to avoid extensive memory allocation and copy in the executaion of the protocol.
- `include/LinearGKR/sumcheck_helper.hpp` provides the helper function for the prover.
  - Note that $f_1$ and $f_2$ are both the in the form of _production of two multi-linear functions_, so we use a class **SumcheckMultiLinearProdHelper** to facilitate the computation. This class, when taking the evaluation of the two multi-linear functions at **prepare()**, can output the required low-degree polynomials in the process of sumcheck. The interface is named **poly_eval_at()**. Whenever the verifier sends a challenge, it can take it using **receive_challenge()**, and do some computation internally to help produce the next low degree polynomial.
  - **SumcheckGKRHelper** computes the evaluations required by **SumcheckMultiLinearProdHelper**, and instantiates two of the instances.
- `include/LinearGKR/sumcheck_common.hpp` provides a function that will be used by both the prover and the verifier, i.e. evaluates $\tilde{eq}(r, a)$ at some specific random vector $r$ and all the possible $a$ from $0$ to $2^{r.size()} - 1$. The returned value shoud have size $2^{r.size()}$.
- `include/LinearGKR/sumcheck_verifier_utils` contains two functions.

  - **degree_2_eval** evaluates a degree-2 polynomial at some value $x$, the degree-2 polynomial is defined by its evaluation at $0, 1, 2$ as given in **vals**.

  - **eval_sparse_circuit_connect_poly** evaluates $add$ or $mul$ at some specific point $rx$, $ry$ and $rz$. Since we have two claims, we actually compute $\alpha\cdot add(rz_1, rx) + \beta\cdot add(rz_2, rx)$, and it is similar in the case of $mul$.

- `include/LinearGKR/sumcheck.hpp` is where the sumcheck actually take place. It is relatively simple since all the complexity has gone to the utility files.

## Polynomial Commitment

We offer several choices of polynomial commitment to be used to commit the input layer of the circuit including _kzg, fri, Orion_. In general, they should have _setup, commit, open, verify_ interfaces.
