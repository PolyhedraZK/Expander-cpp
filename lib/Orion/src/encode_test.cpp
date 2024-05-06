#include "linear_code/linear_code_encode.h"
#include <chrono>
#include "infrastructure/RS_polynomial.h"
int main()
{
    prime_field::init();
    int lgN = 20;
    int N = 1 << lgN;
    int lgRate = 5;
    int rs_rate = 1 << lgRate;
    expander_init(N);
    init_scratch_pad(N * rs_rate * 2);
    prime_field::field_element *arr, *dst, *rsdst, *rscoef;
    dst = new prime_field::field_element[2 * N];
    arr = new prime_field::field_element[N];
    rscoef = new prime_field::field_element[N];
    rsdst = new prime_field::field_element[rs_rate * N];
    for(int i = 0; i < N; ++i)
        arr[i] = prime_field::random();
    auto t0 = std::chrono::high_resolution_clock::now();
    int final_size = encode(arr, dst, N);
    auto t1 = std::chrono::high_resolution_clock::now();
    double duration = std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t0).count();

    t0 = std::chrono::high_resolution_clock::now();
    inverse_fast_fourier_transform(arr, N, N, prime_field::get_root_of_unity(lgN), rscoef);
    fast_fourier_transform(rscoef, N, rs_rate * N, prime_field::get_root_of_unity(lgN + lgRate), rsdst);
    t1 = std::chrono::high_resolution_clock::now();
    double rs_duration = std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t0).count();
    printf("Encode time %lf, final size %d\n", duration, final_size);
    printf("RS Encode time %lf\n", rs_duration);
    return 0;
}