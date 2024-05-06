#pragma once

#include "circuit/circuit.hpp"

namespace gkr
{

template<typename F, typename F_primitive>
class GKRScratchPad
{
private:
    void _mem_init(uint32 max_nb_output, uint32 max_nb_input)
    {
        uint32 F_size = sizeof(F);

        // may have some problems since there is no initialization here
        // especially when operator= is called, keep an eye on this
        #ifdef __x86_64__
        #define __allocate(x) reinterpret_cast<F*>(aligned_alloc(32, x * F_size))
        #define __allocate_primitive(x) reinterpret_cast<F_primitive*>(aligned_alloc(32, x * sizeof(F_primitive)))
        #else
        #define __allocate(x) reinterpret_cast<F*>(malloc(x * F_size))
        #define __allocate_primitive(x) reinterpret_cast<F_primitive*>(malloc(x * sizeof(F_primitive)))
        #endif
        v_evals = __allocate(max_nb_input);
        hg_evals = __allocate(max_nb_input);
        eq_evals_at_rx = __allocate_primitive(max_nb_input);
        eq_evals_at_rz1 = __allocate_primitive(max_nb_output);
        eq_evals_at_rz2 = __allocate_primitive(max_nb_output);
        eq_evals_first_half = __allocate_primitive(max_nb_output);
        eq_evals_second_half = __allocate_primitive(max_nb_output);
        gate_exists = (bool*)malloc(max_nb_input * sizeof(bool));
    }

public:
    F *v_evals, *hg_evals;
    F_primitive *eq_evals_at_rx;
    F_primitive *eq_evals_at_rz1, *eq_evals_at_rz2;
    F_primitive *eq_evals_first_half, *eq_evals_second_half;
    bool *gate_exists;

    void prepare(const Circuit<F, F_primitive> &circuit)
    {
        uint32 max_nb_output_vars = 0, max_nb_input_vars = 0;
        for (const CircuitLayer<F, F_primitive> &layer: circuit.layers)
        {
            max_nb_output_vars = std::max(max_nb_output_vars, layer.nb_output_vars);
            max_nb_input_vars = std::max(max_nb_input_vars, layer.nb_input_vars);
        }
        _mem_init(1 << max_nb_output_vars, 1 << max_nb_input_vars);
    }

    ~GKRScratchPad()
    {
        #define __free(x) free(reinterpret_cast<void*>(x))
        __free(v_evals);
        __free(hg_evals);
        __free(eq_evals_at_rx);
        __free(eq_evals_at_rz1);
        __free(eq_evals_at_rz2);
        __free(eq_evals_first_half);
        __free(eq_evals_second_half);
        free(gate_exists);
    }
};

} // namespace LinearGKR

