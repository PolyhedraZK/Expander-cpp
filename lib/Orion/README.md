# Orion: Zero Knowledge Proof with Linear Prover Time

Orion is a high-speed zero-knowledge proof system, that achieves $O(N)$ prover time of field operations and hash functions and $O(\log^2 N)$ proof size.

This repo provides a C++ implmenetation of Orion polynomial commitment, that can be coupled with other proof systems like (Virgo/Ligero/Hyrax/Spartan...) to achieve linear time zero-knowledge proofs. 

Note that this library has not received a security review or audit.

# Highlights

### General Polynomial Commitment
We supports both univariant and multivariant polynomial commitment schemes. 
### Succinct proof size
The proof size of our system is $O(\log^2 N)$
### State-of-the-art performance
We offers the fastest prover that can prove $2^{27}$ coefficients within $100$s in single thread mode.
### Expander testing
We offer our expander testing protocol for people to setup their own expander.

# Interface

Our main expander generation is defined in ```linear_code/linear_code_encode.h```

Our main PC interface is defined in ```include/VPD/linearPC.h```

To use our protocol, you need to first generate a expander:

```
int col_size = XXX, row_size = N / col_size;
expander_init(row_size);
```

Then commit your secret coefficient array using:

```
auto h = commit(coefs, N);
```

You can open the polynomial at a given point x using:
```
auto result = open_and_verify(x, N, h);
```

# Examples and tests

## Univariate Polynomial Commitment
### Build
```
cmake .
make linearPC
```

### Code
See ```examples/univariate_PC_test.cpp```
## Multivariant Polynomial Commitment
### Build
```
cmake .
make linearPC_multi
```

### code
See ```examples/multivariate_PC_test.cpp```

# Under which license is the Orion distributed?

Most of the source and header files in the Orion are licensed under Apache License 2.0, see LICENSE file for details. The exceptions are the following in XKCP library:

* [`lib/common/brg_endian.h`](lib/common/brg_endian.h) is copyrighted by Brian Gladman and comes with a BSD 3-clause license;
* [`tests/UnitTests/genKAT.c`](tests/UnitTests/genKAT.c) is based on [SHA-3 contest's code by Larry Bassham, NIST](http://csrc.nist.gov/groups/ST/hash/sha-3/documents/KAT1.zip), which he licensed under a BSD 3-clause license;
* [`tests/UnitTests/timing.h`](tests/UnitTests/timing.h) is adapted from Google Benchmark and is licensed under the Apache License, Version 2.0;
* [`KeccakP-1600-AVX2.s`](lib/low/KeccakP-1600/AVX2/KeccakP-1600-AVX2.s) is licensed under the [CRYPTOGAMS license](http://www.openssl.org/~appro/cryptogams/) (BSD-like);
* [`support/Kernel-PMU/enable_arm_pmu.c`](support/Kernel-PMU/enable_arm_pmu.c) is licensed under the GNU General Public License by Bruno Pairault.

