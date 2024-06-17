#pragma once

#include <memory>
#include "poly_commit/poly.hpp"
#include "utils/myutil.hpp"
#include "sumcheck_common.hpp"
#include "scratch_pad.hpp"
#include "field/M31.hpp"
#include <cstring>
#ifdef __ARM_NEON
#include <arm_neon.h>
#endif
#include <chrono>
#include <map>
namespace gkr
{

class Timing
{
    public:
    std::map<string, chrono::high_resolution_clock::time_point> timings;
    bool print;
    Timing()
    {
        print = true;
    }
    void set_print(bool p)
    {
        print = p;
    }
    void add_timing(string name)
    {
        if (!print)
            return;
        timings[name] = chrono::high_resolution_clock::now();
    }
    void report_timing(string name)
    {
        if (!print)
            return;
        auto end = chrono::high_resolution_clock::now();
        auto start = timings[name];
        auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
        cout << name << " took " << duration.count() << " microseconds" << endl;
    }
};

template<typename F, typename F_primitive>
class SumcheckMultiLinearHelper
{
public:
    uint32 nb_vars;
    uint32 sumcheck_var_idx;
    F *bookkeeping;
    const F *initial_v;

    std::vector<F_primitive> rx;

    void prepare(uint32 nb_vars_, F* evals, const F *initial_v_)
    {
        nb_vars = nb_vars_;
        sumcheck_var_idx = 0;
        bookkeeping = evals;
        initial_v = initial_v_;
        rx.clear();
    }

    std::vector<F> poly_eval_at(uint32 var_idx)
    {
        F p0 = F::zero();
        F p1 = F::zero();
        
        const F *src_v = (var_idx == 0 ? initial_v : bookkeeping);
        uint32 cur_eval_size = 1 << (nb_vars - var_idx - 1);
        for (uint32 i = 0; i < cur_eval_size; i++)
        {
            p0 += src_v[i * 2];
            p1 += src_v[i * 2 + 1];
        }

        return {p0, p1};
    }

    void receive_challenge(uint32 var_idx, const F_primitive & r)
    {
        const F *src_v = (var_idx == 0 ? initial_v : bookkeeping);
        uint32 cur_eval_size = 1 << (nb_vars - var_idx - 1);
        for (uint32 i = 0; i < cur_eval_size; i++)
        {        
            bookkeeping[i] = src_v[2 * i] + (src_v[2 * i + 1] - src_v[2 * i]) * r;
        }
        rx.emplace_back(r);
    }

};

template<typename F, typename F_primitive>
class SumcheckMultiLinearProdHelper
{
public:

    uint32 nb_vars;
    uint32 sumcheck_var_idx;
    uint32 cur_eval_size;
    F* bookkeeping_f;
    F* bookkeeping_hg;
    const F* initial_v;
    void prepare(uint32 nb_vars_, F* p1_evals, F* p2_evals, const F* v)
    {
        nb_vars = nb_vars_;
        sumcheck_var_idx = 0;
        cur_eval_size = 1 << nb_vars;
        bookkeeping_f = p1_evals;
        bookkeeping_hg = p2_evals;
        initial_v = v;
    }

    std::vector<F> poly_eval_at(uint32 var_idx, uint32 degree, bool *gate_exists=nullptr)
    {
        F p0 = F::zero();
        F p1 = F::zero();
        F p2 = F::zero();
        auto src_v = (var_idx == 0 ? initial_v : bookkeeping_f);
        int evalSize = 1 << (nb_vars - var_idx - 1);

        for (int i = 0; i < evalSize; i++)
        {
            if (gate_exists!= nullptr && !gate_exists[i * 2] && !gate_exists[i * 2 + 1])
            {
                continue;
            }
            F f_v_0 = src_v[i * 2];
            F f_v_1 = src_v[i * 2 + 1];
            F hg_v_0 = bookkeeping_hg[i * 2];
            F hg_v_1 = bookkeeping_hg[i * 2 + 1];

            p0 += f_v_0 * hg_v_0;
            p1 += f_v_1 * hg_v_1;
            p2 += (f_v_0 + f_v_1) * (hg_v_0 + hg_v_1);
        }
        p2 = p1 * F(6) + p0 * F(3) - p2 * F(2);
        return {p0, p1, p2};
    }

    void receive_challenge(uint32 var_idx, const F_primitive& r, bool *gate_exists=nullptr)
    {
        auto src_v = (var_idx == 0 ? initial_v : bookkeeping_f);
        assert(var_idx == sumcheck_var_idx && 0 <= var_idx && var_idx < nb_vars);
        for (uint32 i = 0; i < (cur_eval_size >> 1); i++)
        {
            if (gate_exists != nullptr)
            {
                if (!gate_exists[i * 2] && !gate_exists[i * 2 + 1])
                {
                    gate_exists[i] = false;
                    bookkeeping_f[i] = src_v[2 * i] + (src_v[2 * i + 1] - src_v[2 * i]) * r;
                    bookkeeping_hg[i] = 0;
                }
                else
                {
                    gate_exists[i] = true;
                    bookkeeping_f[i] = src_v[2 * i] + (src_v[2 * i + 1] - src_v[2 * i]) * r;
                    bookkeeping_hg[i] = bookkeeping_hg[2 * i] + (bookkeeping_hg[2 * i + 1] - bookkeeping_hg[2 * i]) * r;
                }
            }
            else
            {
                bookkeeping_f[i] = src_v[2 * i] + (src_v[2 * i + 1] - src_v[2 * i]) * r;
                bookkeeping_hg[i] = bookkeeping_hg[2 * i] + (bookkeeping_hg[2 * i + 1] - bookkeeping_hg[2 * i]) * r;
            }
        }

        cur_eval_size >>= 1;
        sumcheck_var_idx++;
    }

};

// The basic version:
//  Phase one:
//  f(rz) = \sum_{x, y} mul(rz, x, y) v(x) v(y) + add(rz, x) v(x)
//  where = \sum_x v(x) (add(rz, x) + \sum_y mul(rz, x, y) v(y))
//        = \sum_x v(x) g(x)
//  Phase two:
//  \sum_y h(y)v(y)
//  where h(y) = v(rx) mul(rz, rx, y) 
//
// In practice, we are proving \alpha f(rz1) + \beta f(rz2) together
// Thus g(x) becomes (\alpha add(rz1, x) + \beta add(rz2, x)) + 
//                     \sum_y (\alpha mul(rz1, x, y) + \beta mul(rz2, x, y)) v(y)
//h(y) becomes v(rx) (\alpha mul(rz1, rx, y) + \beta mul(rz2, rx, y))
template<typename F, typename F_primitive>
class SumcheckGKRHelper
{
public:
    uint32 world_rank, world_size;
    uint32 lg_world_size;

    CircuitLayer<F, F_primitive> const* poly_ptr;
    F_primitive alpha, beta;
    GKRScratchPad<F, F_primitive>* pad_ptr;

    std::vector<F_primitive> rx, ry, rwx, rwy;

    // x_helper: v(x)g(x), y_helper: v(y)h(y)
    // where v is the input layer evaluations
    // g and h are defined at the beginning of this template
    SumcheckMultiLinearProdHelper<F, F_primitive> x_helper, y_helper, wx_helper, wy_helper;

public:
    uint32 nb_input_vars;
    uint32 nb_output_vars;
    
public:

    void _prepare_g_x_vals(
        const std::vector<F_primitive>& rz1,
        const std::vector<F_primitive>& rz2,
        const std::vector<F_primitive>& rw1,
        const std::vector<F_primitive>& rw2,
        const F_primitive& alpha,
        const F_primitive& beta,
        const SparseCircuitConnection<F_primitive, 2>& mul,
        const SparseCircuitConnection<F_primitive, 1>& add, 
        const MultiLinearPoly<F>& vals,
        bool* gate_exists
    )
    {
        F *hg_vals = pad_ptr->hg_evals;
        memset(hg_vals, 0, sizeof(F) * vals.evals.size());
        memset(gate_exists, 0, sizeof(bool) * vals.evals.size());

        F_primitive eq_rw1_at_world_rank = _eq(rw1, world_rank, lg_world_size);
        F_primitive eq_rw2_at_world_rank = _eq(rw2, world_rank, lg_world_size);

        _eq_evals_at(rz1, alpha * eq_rw1_at_world_rank, pad_ptr->eq_evals_at_rz1, pad_ptr -> eq_evals_first_half, pad_ptr -> eq_evals_second_half);
        _eq_evals_at(rz2, beta * eq_rw2_at_world_rank, pad_ptr->eq_evals_at_rz2, pad_ptr -> eq_evals_first_half, pad_ptr -> eq_evals_second_half);
        F_primitive * eq_evals_at_rz1 = pad_ptr->eq_evals_at_rz1;
        F_primitive const* eq_evals_at_rz2 = pad_ptr->eq_evals_at_rz2;
        for (int i = 0; i < (1 << rz1.size()); ++i)
        {
            eq_evals_at_rz1[i] = eq_evals_at_rz1[i] + eq_evals_at_rz2[i];
        }

        auto mul_size = mul.sparse_evals.size();
        const Gate<F_primitive, 2>* mul_ptr = mul.sparse_evals.data();
        const F* vals_eval_ptr = vals.evals.data();
        for(long unsigned int i = 0; i < mul_size; i++)
        {
            // g(x) += eq(rz, z) * v(y) * coef
            const Gate<F_primitive, 2> &gate = mul_ptr[i];
            uint32 x = gate.i_ids[0];
            uint32 y = gate.i_ids[1];
            uint32 z = gate.o_id;

            hg_vals[x] += vals_eval_ptr[y] * (gate.coef * eq_evals_at_rz1[z]);
            gate_exists[x] = true;
        }
        
        auto add_size = add.sparse_evals.size();

        const auto add_ptr = add.sparse_evals.data();
        for(long unsigned int i = 0; i < add_size; i++)
        {
            // g(x) += eq(rz, x) * coef
            const auto &gate = add_ptr[i];
            uint32 x = gate.i_ids[0];
            uint32 z = gate.o_id;
            hg_vals[x] = hg_vals[x] + gate.coef * eq_evals_at_rz1[z];
            gate_exists[x] = true;
        }
    }

    void _prepare_h_y_vals(
        const std::vector<F_primitive>& rx,
        const F& v_rx,
        const SparseCircuitConnection<F_primitive, 2>& mul,
        bool *gate_exists
    )
    {
        F *hg_vals = pad_ptr->hg_evals;
        memset(hg_vals, 0, sizeof(F) * (1 << rx.size()));
        memset(gate_exists, 0, sizeof(bool) * (1 << rx.size()));
        
        F_primitive eq_rwx_at_world_rank = _eq(rwx, world_rank, lg_world_size);
        F_primitive const* eq_evals_at_rz1 = pad_ptr->eq_evals_at_rz1; // already computed in g_x preparation
        _eq_evals_at(rx, eq_rwx_at_world_rank, pad_ptr->eq_evals_at_rx, pad_ptr -> eq_evals_first_half, pad_ptr -> eq_evals_second_half);
        F_primitive const* eq_evals_at_rx = pad_ptr->eq_evals_at_rx;
        for(const Gate<F_primitive, 2>& gate: mul.sparse_evals)
        {
            // g(y) += eq(rz, z) * eq(rx, x) * v(y) * coef
            uint32 x = gate.i_ids[0];
            uint32 y = gate.i_ids[1];
            uint32 z = gate.o_id;

            hg_vals[y] += v_rx * (eq_evals_at_rz1[z] * eq_evals_at_rx[x] * gate.coef);
            gate_exists[y] = true;
        }
    }

    void _prepare_wx()
    {
        F vx = pad_ptr->v_evals[0];
        F gx = pad_ptr->hg_evals[0];

        MPI_Gather(&vx, sizeof(F), MPI_CHAR, pad_ptr->v_evals, sizeof(F), MPI_CHAR, 0, MPI_COMM_WORLD);
        MPI_Gather(&gx, sizeof(F), MPI_CHAR, pad_ptr->hg_evals, sizeof(F), MPI_CHAR, 0, MPI_COMM_WORLD);

        if (world_rank == 0)
        {
            wx_helper.prepare(lg_world_size, pad_ptr->v_evals, pad_ptr->hg_evals, pad_ptr->v_evals);
        }
    }

    void _prepare_phase_two()
    {
        F vx = vx_claim();
        MPI_Bcast(&vx, sizeof(F), MPI_CHAR, 0, MPI_COMM_WORLD);
        _prepare_h_y_vals(rx, vx, poly_ptr->mul, pad_ptr->gate_exists);
        y_helper.prepare(nb_input_vars, pad_ptr->v_evals, pad_ptr->hg_evals, poly_ptr->input_layer_vals.evals.data());
    }

    void _root_prepare_phase_two_coefs()
    {
        if (world_rank != 0) 
        {
            return; 
        }

        _eq_evals_at_primitive(rwx, F_primitive::one(), pad_ptr->eq_evals_at_rw);
    }

    void _prepare_wy()
    {
        F vy = pad_ptr->v_evals[0];
        F hy = pad_ptr->hg_evals[0];

        MPI_Gather(&vy, sizeof(F), MPI_CHAR, pad_ptr->v_evals, sizeof(F), MPI_CHAR, 0, MPI_COMM_WORLD);
        MPI_Gather(&hy, sizeof(F), MPI_CHAR, pad_ptr->hg_evals, sizeof(F), MPI_CHAR, 0, MPI_COMM_WORLD);

        if (world_rank == 0)
        {
            for (uint32 i = 0; i < world_size; i++)
            {
                pad_ptr->v_evals[i] = pad_ptr->v_evals[i] * pad_ptr->eq_evals_at_rw[i];
            }
            
            wy_helper.prepare(lg_world_size, pad_ptr->v_evals, pad_ptr->hg_evals, pad_ptr->v_evals);
        }
    }

public:

    void prepare(const CircuitLayer<F, F_primitive> &poly, 
        const std::vector<F_primitive> &rz1, 
        const std::vector<F_primitive> &rz2,
        const std::vector<F_primitive> &rw1,
        const std::vector<F_primitive> &rw2,
        const F_primitive& alpha_,
        const F_primitive& beta_,
        GKRScratchPad<F, F_primitive>& scratch_pad)
    {
        MPI_Comm_size(MPI_COMM_WORLD, reinterpret_cast<int*>(&world_size));
        MPI_Comm_rank(MPI_COMM_WORLD, reinterpret_cast<int*>(&world_rank));
        lg_world_size = log_2(world_size);

        nb_input_vars = poly.nb_input_vars;
        nb_output_vars = poly.nb_output_vars;
        alpha = alpha_;
        beta = beta_;
        poly_ptr = &poly;
        pad_ptr = &scratch_pad;

        // phase one
        _prepare_g_x_vals(rz1, rz2, rw1, rw2, alpha, beta, poly.mul, poly.add, poly.input_layer_vals, pad_ptr->gate_exists);
        x_helper.prepare(nb_input_vars, pad_ptr->v_evals, pad_ptr->hg_evals, poly.input_layer_vals.evals.data());
    }

    std::vector<F> poly_evals_at(uint32 var_idx, uint32 degree)
    {
        assert(degree == 2);

        if (var_idx < nb_input_vars)
        {
            std::vector<F> poly_evals = x_helper.poly_eval_at(var_idx, degree, pad_ptr->gate_exists);
            _mpi_sum_combine(poly_evals, poly_evals);
            return poly_evals;
        }
        else if (nb_input_vars <= var_idx && var_idx < nb_input_vars + lg_world_size)
        {
            if (var_idx == nb_input_vars)
            {
                _prepare_wx();
            }
            
            if (world_rank == 0)
            {
                return wx_helper.poly_eval_at(var_idx - nb_input_vars, degree);
            }
        }
        else if (nb_input_vars + lg_world_size <= var_idx && var_idx < 2 * nb_input_vars + lg_world_size)
        {
            if (var_idx == nb_input_vars + lg_world_size)
            {
                _prepare_phase_two();
            }

            std::vector<F> poly_evals = 
                x_helper.poly_eval_at(var_idx - nb_input_vars - lg_world_size, degree, pad_ptr->gate_exists);
            _mpi_sum_combine(poly_evals, poly_evals);

            return poly_evals;
        }
        else if (2 * nb_input_vars + lg_world_size <= var_idx && var_idx < 2 * (nb_input_vars + lg_world_size))
        {
            if (var_idx == 2 * nb_input_vars + lg_world_size)
            {
                _prepare_wy();
            }

            if (world_rank == 0)
            {
                return wy_helper.poly_eval_at(var_idx - 2 * nb_input_vars - lg_world_size, degree);
            }
        }
        else 
        {
            throw "incorrect var idx";
        }

        return std::vector<F>();
    }

    void receive_challenge(uint32 var_idx, const F_primitive& r)
    {
        if (var_idx < nb_input_vars)
        {
            x_helper.receive_challenge(var_idx, r, pad_ptr->gate_exists);
            rx.emplace_back(r);
        }
        else if (nb_input_vars <= var_idx && var_idx < nb_input_vars + lg_world_size)
        {
            if (world_rank == 0)
            {
                wx_helper.receive_challenge(var_idx - nb_input_vars, r);
                rwx.emplace_back(r);
            }

            if (var_idx == nb_input_vars + lg_world_size - 1)
            {
                rwx.resize(lg_world_size);
                MPI_Bcast(rwx.data(), sizeof(F_primitive) * lg_world_size, MPI_CHAR, 0, MPI_COMM_WORLD);
            }
        }
        else if (nb_input_vars + lg_world_size <= var_idx && var_idx < 2 * nb_input_vars + lg_world_size)
        {
            y_helper.receive_challenge(var_idx - nb_input_vars - lg_world_size, r, pad_ptr->gate_exists);
            ry.emplace_back(r);
        }
        else if (2 * nb_input_vars + lg_world_size <= var_idx && var_idx < 2 * (nb_input_vars + lg_world_size))
        {
            if (world_rank == 0)
            {
                wy_helper.receive_challenge(var_idx - 2 * nb_input_vars - lg_world_size, r);
                rwy.emplace_back(r);
            }
            
            if (var_idx == 2 * (nb_input_vars + lg_world_size) - 1)
            {
                rwy.resize(lg_world_size);
                MPI_Bcast(rwy.data(), sizeof(F_primitive) * lg_world_size, MPI_CHAR, 0, MPI_COMM_WORLD);
            }
        }
        else
        {
            throw "incorrect var idx";
        }
    }

    F vx_claim()
    {
        return pad_ptr->v_evals[0];
    }

    F vy_claim()
    {
        return pad_ptr->v_evals[0];
    }

    F reduced_claim()
    {
        return pad_ptr->v_evals[0] * pad_ptr->hg_evals[0];
    }
};



} // namespace LinearGKR