# Expander

Expander is a proof generation backend for Polyhedra Network. It aims to support fast proof generation.

This is the "core" repo and more on "demo" purpose, we will continue develop on the repo to support more features.

For more technical introduction, visit other markdown files [here](https://github.com/PolyhedraZK/Expander/tree/main/docs).

## Circuit Compiler

Stay tuned, we will open-source our circuit compiler in the incoming month.

## Roadmap

### Fields:
- [x] Mersenne31
- [x] BN254

### Polynomial commitments:
- [x] RAW
- [x] KZG
- [x] FRI
- [x] Orion
- [ ] Bi-Variate KZG

### Hashes
- [x] Keccak256
- [ ] Poseidon
- [ ] SHA256

### Features
- [x] Data parallel circuit
- [ ] Dynamic circuit (what is this? We will let you know later.)
- [ ] Lookup
- [ ] In-circuit random number 

## System requirements
We requires latest CPU to run our experiments, following are currently supported CPU families:
1. ARM CPU with NEON support, example: Apple M series.
2. Intel: Knights Landing, Knights Mill, Skylake-SP/X, Cannon Lake, Cascade Lake, Cooper Lake, Ice Lake, Tiger Lake, Alder Lake, Sapphire Rapids.
3. AMD: Zen 4/5.

Unfortuantely Intel announced that they will not enable AVX-512 on newer CPU, we might remove AVX-512 requirment if AMD also removes AVX512 in future CPU generations.

It's recommended to disable CPU hyper-threading.

We highly recommend you to use Clang as your default compiler to compile the code, as most testing and benchmarks are done on MacOS.

## Environment Setup

```sh
sudo ./scripts/setup.sh
```

## Benchmarks

Build command:

```sh
mkdir bin
cmake .
make keccak_benchmark
```

Run command:
```sh
./bin/keccak_benchmark
```

## FAQ

### Illegal instruction (core dumped)
It indicates your CPU is not supported, please check the supported CPU list above. We might support older CPU if there is a demand.


### Benchmark method
We run the program for 5 minutes and take average of the results.

## How to contribute?

Thank you for your interest in contributing to our project! We seek contributors with a robust background in cryptography and programming, aiming to improve and expand the capabilities of our proof generation system.

### Contribution Guidelines:

#### Pull Requests
We welcome your pull requests (PRs) and ask that you follow these guidelines to facilitate the review process:

- **General Procedure**:
  1. **Fork the repository** and clone it locally.
  2. **Create a branch** for your changes related to a specific issue or improvement.
  3. **Commit your changes**: Use clear and meaningful commit messages.
  4. **Push your changes** to your fork and then **submit a pull request** to the main repository.

- **PR Types and Specific Guidelines**:
  - **[BUG]** for bug fixes:
    - **Title**: Start with `[BUG]` followed by a brief description.
    - **Content**: Explain the issue being fixed, steps to reproduce, and the impact of the bug. Include any relevant error logs or screenshots.
    - **Tests**: Include tests that ensure the bug is fixed and will not recur.
  - **[FEATURE]** for new features:
    - **Title**: Start with `[FEATURE]` followed by a concise feature description.
    - **Content**: Discuss the benefits of the feature, possible use cases, and any changes it introduces to existing functionality.
    - **Documentation**: Update relevant documentation and examples.
    - **Tests**: Add tests that cover the new feature's functionality.
  - **[DOC]** for documentation improvements:
    - **Title**: Start with `[DOC]` and a short description of what is being improved.
    - **Content**: Detail the changes made and why they are necessary, focusing on clarity and accessibility.
  - **[TEST]** for adding or improving tests:
    - **Title**: Begin with `[TEST]` and describe the type of testing enhancement.
    - **Content**: Explain what the tests cover and how they improve the project's reliability.
  - **[PERF]** for performance improvements:
    - **Title**: Use `[PERF]` and a brief note on the enhancement.
    - **Content**: Provide a clear comparison of performance before and after your changes, including benchmarks or profiling data.
    - **Tests/Benchmarks**: Add tests that cover the new feature's functionality, and benchmarks to prove your improvement.

#### Review Process
Each pull request will undergo a review by one or more core contributors. We may ask for changes to better align with the project's goals and standards. Once approved, a maintainer will merge the PR.

We value your contributions greatly and are excited to see what you bring to this project. Let’s build something great together!
