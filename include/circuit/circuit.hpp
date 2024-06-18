#pragma once

#include "poly_commit/poly.hpp"
#include "utils/types.hpp"
#include "utils/myutil.hpp"
#include "fiat_shamir/transcript.hpp"

#include "circuit_raw.hpp"
#include <iostream>
#include <fstream>

namespace gkr
{

template<typename F, uint32 nb_input>
class Gate
{
public:
    uint32 o_id;
    uint32 i_ids[nb_input];
    F coef;
    Gate(){}
    Gate(uint32 o_id, uint32 i_ids[nb_input], F coef)
    {
        this->o_id = o_id;
        for (uint32 i = 0; i < nb_input; i++)
        {
            this->i_ids[i] = i_ids[i];
        }
        this->coef = coef;
    }
};

template<typename F, uint32 nb_input>
class SparseCircuitConnection
{
public:
    uint32 nb_output_vars;
    uint32 nb_input_vars;
    std::vector<Gate<F, nb_input>> sparse_evals; 

    static SparseCircuitConnection random(uint32 nb_output_vars, uint32 nb_input_vars, bool enable_rnd_coef=false)
    {
        SparseCircuitConnection poly;
        poly.nb_input_vars = nb_input_vars;
        poly.nb_output_vars = nb_output_vars;
        uint32 output_size = 1 << nb_output_vars;
        uint32 input_size = 1 << nb_input_vars;

        for (uint32 i = 0; i < output_size; i++)
        {
            // to make sure all o_gates are used
            uint32 o_gate = i;
            uint32 i_gates[nb_input];
            uint32 i_gate = i;
            for (uint32 j = 0; j < nb_input; j++)
            {
                i_gates[j] = i_gate % input_size;
                i_gate = i_gate + output_size;
            }

            F coef;
            if (enable_rnd_coef && rand() % 10 == 0)
            {
                // random gate, the value will be filled only after protocol starts
                coef = F::default_rand_sentinel();
            }
            else 
            {
                coef = F::random();
            }

            poly.sparse_evals.emplace_back(
                Gate<F, nb_input> (o_gate, i_gates, coef)
            );
        }
        return poly;
    }
};

template<typename F, typename F_primitive>
class CircuitLayer
{
public: 
    uint32 nb_output_vars;
    uint32 nb_input_vars;
    MultiLinearPoly<F> input_layer_vals;
    MultiLinearPoly<F> output_layer_vals;

    SparseCircuitConnection<F_primitive, 0> cst;
    SparseCircuitConnection<F_primitive, 1> add;
    SparseCircuitConnection<F_primitive, 2> mul;

    static CircuitLayer random(uint32 nb_output_vars, uint32 nb_input_vars, bool enable_rnd_coef=false)
    {
        CircuitLayer poly;
        poly.nb_output_vars = nb_output_vars;
        poly.nb_input_vars = nb_input_vars;
        poly.input_layer_vals = MultiLinearPoly<F>(nb_input_vars);

        // poly.cst = SparseCircuitConnection<F_primitive, 0>::random(nb_output_vars, nb_input_vars, enable_rnd_coef);
        poly.add = SparseCircuitConnection<F_primitive, 1>::random(nb_output_vars, nb_input_vars, enable_rnd_coef);
        poly.mul = SparseCircuitConnection<F_primitive, 2>::random(nb_output_vars, nb_input_vars, enable_rnd_coef); 
        return poly;
    }

    void evaluate(std::vector<F> &output) const
    {
        output.clear();
        output.resize(1 << nb_output_vars, F::zero());
        for (const Gate<F_primitive, 2> &gate: mul.sparse_evals)
        {
            output[gate.o_id] += input_layer_vals.evals[gate.i_ids[0]] * input_layer_vals.evals[gate.i_ids[1]] * gate.coef;
        }

        for (const Gate<F_primitive, 1> &gate: add.sparse_evals)
        {
            output[gate.o_id] += input_layer_vals.evals[gate.i_ids[0]] * gate.coef;
        }

        for (const Gate<F_primitive, 0> &gate: cst.sparse_evals)
        {
            output[gate.o_id] = output[gate.o_id] + gate.coef;
        }
    }

    uint32 nb_mul_gates() const
    {
        return mul.sparse_evals.size();
    }

    uint32 nb_add_gates() const
    {
        return add.sparse_evals.size();
    }

    uint32 nb_cst_gates() const
    {
        return cst.sparse_evals.size();
    }

};

template<typename F, typename F_primitive>
class Circuit
{
public:
    std::vector<F_primitive*> rnd_coefs;
    std::vector<CircuitLayer<F, F_primitive>> layers;

    void _compute_nb_vars()
    {
        for (uint32 i = 0; i < layers.size(); i++)
        {
            CircuitLayer<F, F_primitive> &layer = layers[i];

            uint32 max_o_gate_id = 0, max_i_gate_id = 0;
            for (size_t j = 0; j < layer.mul.sparse_evals.size(); j++)
            {
                const Gate<F_primitive, 2> &gate = layer.mul.sparse_evals[j];
                max_o_gate_id = std::max(max_o_gate_id, gate.o_id);
                max_i_gate_id = std::max({max_i_gate_id, gate.i_ids[0], gate.i_ids[1]});
            }

            for (size_t j = 0; j < layer.add.sparse_evals.size(); j++)
            {
                const Gate<F_primitive, 1> &gate = layer.add.sparse_evals[j];
                max_o_gate_id = std::max(max_o_gate_id, gate.o_id);
                max_i_gate_id = std::max(max_i_gate_id, gate.i_ids[0]);
            }

            layer.nb_input_vars = max_i_gate_id > 0 ? __builtin_ctz(next_pow_of_2(max_i_gate_id)) : 0;
            layer.nb_output_vars = max_o_gate_id > 0 ? __builtin_ctz(next_pow_of_2(max_o_gate_id)) : 0;
            layer.input_layer_vals.nb_vars = layer.nb_input_vars;
        }
    }

    void _extract_rnd_gates()
    {
        rnd_coefs.clear();

        for (auto &layer: layers)
        {
            for (auto &gate: layer.mul.sparse_evals)
            {
                if (!gate.coef.is_valid())
                {
                    rnd_coefs.emplace_back(&gate.coef);
                }
            }

            for (auto &gate: layer.add.sparse_evals)
            {
                if (!gate.coef.is_valid())
                {
                    rnd_coefs.emplace_back(&gate.coef);
                }
            }

            for (auto &gate: layer.cst.sparse_evals)
            {
                if (!gate.coef.is_valid())
                {
                    rnd_coefs.emplace_back(&gate.coef);
                }
            }
        }
    }

    //TODO: Keep an eye on rounding up the number of gates, efficiency & security
    static Circuit load_extracted_gates(const char *filename_mul, const char *filename_add)
    {
        Circuit c;

        // a little inefficient
        std::ifstream tmp_file(filename_mul);
        std::string line;
        uint32 nb_layers = 0;
        while(std::getline(tmp_file, line))
            nb_layers++;
        c.layers.resize(nb_layers);

        FILE* file = fopen(filename_mul, "r");
        size_t num_gates = 0;
        for (uint32 i = 0; i < nb_layers; i++) 
        {
            CircuitLayer<F, F_primitive>& layer = c.layers[nb_layers - 1 - i];
            layer.nb_input_vars = layer.nb_output_vars = 0;
            
            fscanf(file, "%lu", &num_gates);
            SparseCircuitConnection<F_primitive, 2>& mul_connect = layer.mul;

            mul_connect.sparse_evals.resize(num_gates);
            for (size_t i = 0; i < num_gates; i++)
            {
                Gate<F_primitive, 2>& mul_gate = mul_connect.sparse_evals[i];
                size_t o_id, i_id_0, i_id_1;
                unsigned coef;
                fscanf(file, "%lu%lu%lu%u", &i_id_0, &i_id_1, &o_id, &coef);

                mul_gate.o_id = o_id;
                mul_gate.i_ids[0] = i_id_0;
                mul_gate.i_ids[1] = i_id_1;
                mul_gate.coef = F_primitive(coef);
            }
        }
        fclose(file);

        file = fopen(filename_add, "r");
        for (uint32 i = 0; i < nb_layers; i++)
        {
            size_t num_gates = 0;
            fscanf(file, "%lu", &num_gates);
            SparseCircuitConnection<F_primitive, 1>& add_connect = c.layers[nb_layers - 1 - i].add;
            add_connect.sparse_evals.resize(num_gates);

            for (size_t j = 0; j < num_gates; j++)
            {
                Gate<F_primitive, 1>& add_gate = add_connect.sparse_evals[j];
                size_t o_id, i_id;
                unsigned coef;
                fscanf(file, "%lu%lu%u", &i_id, &o_id, &coef);

                add_gate.o_id = o_id;
                add_gate.i_ids[0] = i_id;
                add_gate.coef = F_primitive(coef);
            }
         
        }
        fclose(file);
        
        c._compute_nb_vars();
        return c;
    }

    static Circuit from_circuit_raw(const CircuitRaw<F_primitive>& circuit_raw)
    {
        Circuit circuit;
        uint32 n_layers = circuit_raw.layers.size();
        circuit.layers.resize(n_layers);

        for (uint32 i = 0; i < n_layers; i++)
        {
            CircuitLayer<F, F_primitive> &layer = circuit.layers[i];
            
            const auto &layer_raw = circuit_raw.layer_at(i);
            const auto leaves = layer_raw.scan_leaf_segments(circuit_raw, circuit_raw.layers[i]);
            for (const auto &gate_mul_raw : layer_raw.leaf_gate_muls(circuit_raw, leaves))
            {
                Gate<F_primitive, 2> gate_mul;
                gate_mul.o_id = gate_mul_raw.out;
                gate_mul.i_ids[0] = gate_mul_raw.in0;
                gate_mul.i_ids[1] = gate_mul_raw.in1;
                gate_mul.coef = gate_mul_raw.coef;
                layer.mul.sparse_evals.emplace_back(gate_mul);
            }

            for (const auto &gate_add_raw : layer_raw.leaf_gate_adds(circuit_raw, leaves))
            {
                Gate<F_primitive, 1> gate_add;
                gate_add.o_id = gate_add_raw.out;
                gate_add.i_ids[0] = gate_add_raw.in0;
                gate_add.coef = gate_add_raw.coef;
                layer.add.sparse_evals.emplace_back(gate_add);
            }

            for (const auto &gate_cst_raw: layer_raw.leaf_gate_consts(circuit_raw, leaves))
            {
                Gate<F_primitive, 0> gate_cst;
                gate_cst.o_id = gate_cst_raw.out;
                gate_cst.coef = gate_cst_raw.coef;
                layer.cst.sparse_evals.emplace_back(gate_cst);
            }
        }

        circuit._compute_nb_vars();
        circuit._extract_rnd_gates();
        return circuit;
    }

    uint32 nb_mul_gates() const
    {
        uint32 sum = 0;
        for (const CircuitLayer<F, F_primitive>& layer: layers)
        {
            sum += layer.nb_mul_gates();
        }
        return sum;
    }
    
    uint32 nb_add_gates() const
    {
        uint32 sum = 0;
        for (const CircuitLayer<F, F_primitive>& layer: layers)
        {
            sum += layer.nb_add_gates();
        }
        return sum;
    }

    uint32 nb_cst_gates() const
    {
        uint32 sum = 0;
        for (const CircuitLayer<F, F_primitive>& layer: layers)
        {
            sum += layer.nb_cst_gates();
        }
        return sum;
    }

    void evaluate()
    {
        for (uint32 i = 0; i < layers.size() - 1; ++i)
        {
            layers[i].evaluate(layers[i + 1].input_layer_vals.evals);
        }
        layers.back().evaluate(layers.back().output_layer_vals.evals);
    }

    void fill_rnd_gate(Transcript<F, F_primitive> &transcript)
    {
        // uint32 nb_rnd_coefs = rnd_coefs.size();
        // uint32 byte_size = nb_rnd_coefs * F::byte_length();
        // uint8* buffer = (uint8*) malloc(byte_size);
        // uint8* buffer_head = buffer;

        for (F_primitive* &rnd_ptr: rnd_coefs)
        {
            *rnd_ptr = transcript.challenge_f();
        }
        // free(buffer);
    }

    uint32 log_input_size() const
    {
        return layers[0].nb_input_vars;
    }

    void set_random_input()
    {
        std::vector<F> &input_layer_vals = layers[0].input_layer_vals.evals;
        input_layer_vals.clear();
        for (uint32 i = 0; i < (1UL << log_input_size()); i++)
        {
            input_layer_vals.emplace_back(F::random());
        }
    }

    void set_random_boolean_input()
    {
        std::vector<F> &input_layer_vals = layers[0].input_layer_vals.evals;
        input_layer_vals.clear();
        for (uint32 i = 0; i < (1 << log_input_size()); i++)
        {
            input_layer_vals.emplace_back(F::random_bool());
        }
    }
};

}