# Example

To use the gkr lib, we need to first get the circuit from the compiler. In this example, it is `circuit8.txt` in the data folder, which contains 8 copies of KeccakHash. 

We first read in the circuit:

```c++
#include "field/M31.hpp"
#include "LinearGKR/gkr.hpp"
#include <iostream>

using namespace gkr;
using F = gkr::M31_field::VectorizedM31;
using F_primitive = gkr::M31_field::M31;

const char* filename = "../data/circuit8.txt";
CircuitRaw<F_primitive> circuit_raw;
ifstream fs(filename, ios::binary);
fs >> circuit_raw;
auto circuit = Circuit<F, F_primitive>::from_circuit_raw(circuit_raw);
```

Here, `CircuitRaw` is a circuit format exactly the same as the compiler output, while `Circuit` is more suitable for the gkr protocol. `VectorizedM31` can be seen as a vector of `M31`, which helps improves the efficiency of the prover.

After reading the circuit, 
```c++
circuit.set_random_input();
circuit.evaluate();
```
We can set some random input, and evaluate the circuit before proving.

The next step is the proving and verification
```c++
GKRScratchPad<F, F_primitive> scratch_pad{};
scratch_pad.prepare(circuit);

GKRProof<F> proof;
F claimed_value;
std::tie(proof, claimed_value) = gkr_prove<F, F_primitive>(circuit, scratch_pad);
bool verified = gkr_verify<F, F_primitive>(circuit, claimed_value, proof);    
```

the value `verified` should equal to `true` in this case.