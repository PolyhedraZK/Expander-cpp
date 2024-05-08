#pragma once
#include <cstddef>
#include <cassert>
#include <vector>
#include <iostream>
#include <bit>
#include <unordered_map>

using namespace std;

const uint64_t MAGIC_NUM = 3626604230490605891; // b'CIRCUIT2'

typedef uint64_t SegmentId;

struct Allocation
{
    size_t i_offset;
    size_t o_offset;
};

template <typename F>
struct GateMul
{
    size_t in0;
    size_t in1;
    size_t out;
    F coef;
};

template <typename F>
struct GateAdd
{
    size_t in0;
    size_t out;
    F coef;
};

template <typename F>
struct GateConst
{
    size_t out;
    F coef;
};

template <typename F>
class CircuitRaw;

template <typename F>
class Segment
{
public:
    static F current_rand_sentinel;

    size_t i_len;
    size_t o_len;
    vector<pair<SegmentId, vector<Allocation>>> child_segs;
    vector<GateMul<F>> gate_muls;
    vector<GateAdd<F>> gate_adds;
    vector<GateConst<F>> gate_consts;
    int bit_width(unsigned long long x) 
    {
        int cnt = 0;
        // return log2(x) + 1
        while(x)
        {
            x >>= 1;
            cnt++;
        }
        return cnt;
    }
    const size_t i_len_log2() const
    {
        return bit_width(i_len) - 1;
    }
    const size_t o_len_log2() const
    {
        return bit_width(o_len) - 1;
    }
    const bool contain_gates() const
    {
        return gate_muls.size() > 0 || gate_adds.size() > 0 || gate_consts.size() > 0;
    }
    typedef unordered_map<SegmentId, vector<Allocation>> Leaves;
    Leaves const scan_leaf_segments(const CircuitRaw<F> &circuit, SegmentId cur) const
    {
        // all segments in the map can be seen as without children
        auto ret = unordered_map<SegmentId, vector<Allocation>>();
        if (contain_gates())
        {
            ret[cur] = {Allocation{0, 0}};
        }
        for (auto &child : child_segs)
        {
            const SegmentId &child_sid = child.first;
            const vector<Allocation> &child_allocs = child.second;
            auto leaves = circuit.segments[child_sid].scan_leaf_segments(circuit, child_sid);
            for (auto &leaf : leaves)
            {
                const SegmentId &leaf_sid = leaf.first;
                const vector<Allocation> &leaf_allocs = leaf.second;
                for (auto &leaf_alloc : leaf_allocs)
                {
                    if (!ret.count(leaf_sid))
                    {
                        ret[leaf_sid] = vector<Allocation>();
                    }
                    for (auto &child_alloc : child_allocs)
                    {
                        ret[leaf_sid].push_back(Allocation{
                            child_alloc.i_offset + leaf_alloc.i_offset,
                            child_alloc.o_offset + leaf_alloc.o_offset});
                    }
                }
            }
        }
        return ret;
    }
    void debug_print_leaves(const CircuitRaw<F> &circuit, SegmentId cur) const
    {
        auto leaves = scan_leaf_segments(circuit, cur);
        cout << "DEBUG PRINT LEAVES: Seg-" << cur << "(2^" << i_len_log2()
             << "-->2^" << o_len_log2() << ") with #leaves: " << leaves.size() << endl;
        for (auto &leaf : leaves)
        {
            cout << "  - Leaf Seg-" << leaf.first << "(2^" << circuit.segments[leaf.first].i_len_log2()
                 << "-->2^" << circuit.segments[leaf.first].o_len_log2() << ")"
                 << " with #allocations: " << leaf.second.size() << endl;
            int cnt = 0;
            for (auto &alloc : leaf.second)
            {
                cout << "        ("
                     << alloc.i_offset << "~" << alloc.i_offset + circuit.segments[leaf.first].i_len
                     << ", "
                     << alloc.o_offset << "~" << alloc.o_offset + circuit.segments[leaf.first].o_len
                     << ") " << endl;
                if (++cnt > 3)
                {
                    cout << "        ..." << endl;
                    break;
                }
            }
        }
    }
    vector<GateMul<F>> leaf_gate_muls(const CircuitRaw<F> &circuit, const Leaves &leaves) const
    {
        vector<GateMul<F>> ret;
        for (auto &leaf : leaves)
        {
            const SegmentId &leaf_seg_id = leaf.first;
            auto &leaf_seg = circuit.segments[leaf_seg_id];
            const vector<Allocation> &leaf_allocs = leaf.second;
            for (auto &alloc : leaf_allocs)
            {
                for (auto &gate_mul : leaf_seg.gate_muls)
                {
                    ret.push_back(GateMul<F>{
                        gate_mul.in0 + alloc.i_offset,
                        gate_mul.in1 + alloc.i_offset,
                        gate_mul.out + alloc.o_offset,
                        gate_mul.coef});
                }
            }
        }
        return ret;
    }
    vector<GateAdd<F>> leaf_gate_adds(const CircuitRaw<F> &circuit, const Leaves &leaves) const
    {
        vector<GateAdd<F>> ret;
        for (auto &leaf : leaves)
        {
            const SegmentId &leaf_seg_id = leaf.first;
            auto &leaf_seg = circuit.segments[leaf_seg_id];
            const vector<Allocation> &leaf_allocs = leaf.second;
            for (auto &alloc : leaf_allocs)
            {
                for (auto &gate_add : leaf_seg.gate_adds)
                {
                    ret.push_back(GateAdd<F>{
                        gate_add.in0 + alloc.i_offset,
                        gate_add.out + alloc.o_offset,
                        gate_add.coef});
                }
            }
        }
        return ret;
    }
    vector<GateConst<F>> leaf_gate_consts(const CircuitRaw<F> &circuit, const Leaves &leaves) const
    {
        vector<GateConst<F>> ret;
        for (auto &leaf : leaves)
        {
            const SegmentId &leaf_seg_id = leaf.first;
            auto &leaf_seg = circuit.segments[leaf_seg_id];
            const vector<Allocation> &leaf_allocs = leaf.second;
            for (auto &alloc : leaf_allocs)
            {
                for (auto &gate_const : leaf_seg.gate_consts)
                {
                    ret.push_back(GateConst<F>{
                        gate_const.out + alloc.o_offset,
                        gate_const.coef});
                }
            }
        }
        return ret;
    }

    friend ostream &operator<<(ostream &os, const Segment &s)
    {
        uint64_t i_len = s.i_len;
        os.write((char *)&i_len, sizeof(i_len));
        uint64_t o_len = s.o_len;
        os.write((char *)&o_len, sizeof(o_len));
        uint64_t num_child_segs = s.child_segs.size();
        os.write((char *)&num_child_segs, sizeof(num_child_segs));
        for (size_t i = 0; i < num_child_segs; i++)
        {
            uint64_t seg_id = s.child_segs[i].first;
            os.write((char *)&seg_id, sizeof(seg_id));
            auto allocations = s.child_segs[i].second;
            uint64_t num_allocations = allocations.size();
            os.write((char *)&num_allocations, sizeof(num_allocations));
            for (size_t j = 0; j < num_allocations; j++)
            {
                uint64_t i_offset = allocations[j].i_offset;
                os.write((char *)&i_offset, sizeof(i_offset));
                uint64_t o_offset = allocations[j].o_offset;
                os.write((char *)&o_offset, sizeof(o_offset));
            }
        }

        vector<int> rand_coef_idx;

        uint64_t num_gate_muls = s.gate_muls.size();
        os.write((char *)&num_gate_muls, sizeof(num_gate_muls));
        for (size_t i = 0; i < num_gate_muls; i++)
        {
            uint64_t in0 = s.gate_muls[i].in0;
            os.write((char *)&in0, sizeof(in0));
            uint64_t in1 = s.gate_muls[i].in1;
            os.write((char *)&in1, sizeof(in1));
            uint64_t out = s.gate_muls[i].out;
            os.write((char *)&out, sizeof(out));
            if (s.gate_muls[i].coef == s.current_rand_sentinel) // random coefficient
            {
                rand_coef_idx.push_back(i);
                os << F::zero();
            }
            else
            {
                os << s.gate_muls[i].coef;
            }
        }

        uint64_t num_gate_adds = s.gate_adds.size();
        os.write((char *)&num_gate_adds, sizeof(num_gate_adds));
        for (size_t i = 0; i < num_gate_adds; i++)
        {
            uint64_t in0 = s.gate_adds[i].in0;
            os.write((char *)&in0, sizeof(in0));
            uint64_t out = s.gate_adds[i].out;
            os.write((char *)&out, sizeof(out));
            if (s.gate_adds[i].coef == s.current_rand_sentinel) // random coefficient
            {
                rand_coef_idx.push_back(i + s.gate_muls.size());
                os << F::zero();
            }
            else
            {
                os << s.gate_adds[i].coef;
            }
        }

        uint64_t num_gate_consts = s.gate_consts.size();
        os.write((char *)&num_gate_consts, sizeof(num_gate_consts));
        for (size_t i = 0; i < num_gate_consts; i++)
        {
            uint64_t out = s.gate_consts[i].out;
            os.write((char *)&out, sizeof(out));
            if (s.gate_consts[i].coef == s.current_rand_sentinel) // random coefficient
            {
                rand_coef_idx.push_back(i + s.gate_muls.size() + s.gate_adds.size());
                os << F::zero();
            }
            else
            {
                os << s.gate_consts[i].coef;
            }
        }

        uint64_t num_rand_coef_idx = rand_coef_idx.size();
        os.write((char *)&num_rand_coef_idx, sizeof(num_rand_coef_idx));
        for (size_t i = 0; i < num_rand_coef_idx; i++)
        {
            uint64_t idx = rand_coef_idx[i];
            os.write((char *)&idx, sizeof(idx));
        }
        return os;
    }
    friend istream &operator>>(istream &is, Segment &s)
    {
        is.read((char *)&s.i_len, sizeof(s.i_len));
        is.read((char *)&s.o_len, sizeof(s.o_len));
        uint64_t num_child_segs;
        is.read((char *)&num_child_segs, sizeof(num_child_segs));
        s.child_segs.resize(num_child_segs);
        for (size_t i = 0; i < num_child_segs; i++)
        {
            uint64_t seg_id;
            is.read((char *)&seg_id, sizeof(seg_id));
            s.child_segs[i].first = seg_id;
            uint64_t num_allocations;
            is.read((char *)&num_allocations, sizeof(num_allocations));
            s.child_segs[i].second.resize(num_allocations);
            for (size_t j = 0; j < num_allocations; j++)
            {
                is.read((char *)&s.child_segs[i].second[j].i_offset, sizeof(s.child_segs[i].second[j].i_offset));
                is.read((char *)&s.child_segs[i].second[j].o_offset, sizeof(s.child_segs[i].second[j].o_offset));
            }
        }
        uint64_t num_gate_muls;
        is.read((char *)&num_gate_muls, sizeof(num_gate_muls));
        // assert((num_child_segs == 0) || (num_gate_muls == 0));
        s.gate_muls.resize(num_gate_muls);
        for (size_t i = 0; i < num_gate_muls; i++)
        {
            is.read((char *)&s.gate_muls[i].in0, sizeof(s.gate_muls[i].in0));
            is.read((char *)&s.gate_muls[i].in1, sizeof(s.gate_muls[i].in1));
            is.read((char *)&s.gate_muls[i].out, sizeof(s.gate_muls[i].out));
            is >> s.gate_muls[i].coef;
        }

        uint64_t num_gate_adds;
        is.read((char *)&num_gate_adds, sizeof(num_gate_adds));
        // assert((num_child_segs == 0) || (num_gate_adds == 0));
        s.gate_adds.resize(num_gate_adds);
        for (size_t i = 0; i < num_gate_adds; i++)
        {
            is.read((char *)&s.gate_adds[i].in0, sizeof(s.gate_adds[i].in0));
            is.read((char *)&s.gate_adds[i].out, sizeof(s.gate_adds[i].out));
            is >> s.gate_adds[i].coef;
        }

        uint64_t num_gate_consts;
        is.read((char *)&num_gate_consts, sizeof(num_gate_consts));
        // assert((num_child_segs == 0) || (num_gate_consts == 0));
        s.gate_consts.resize(num_gate_consts);
        for (size_t i = 0; i < num_gate_consts; i++)
        {
            is.read((char *)&s.gate_consts[i].out, sizeof(s.gate_consts[i].out));
            is >> s.gate_consts[i].coef;
        }

        uint64_t num_rand_coef_idx;
        is.read((char *)&num_rand_coef_idx, sizeof(num_rand_coef_idx));
        for (size_t i = 0; i < num_rand_coef_idx; i++)
        {
            uint64_t idx;
            is.read((char *)&idx, sizeof(idx));
            if (idx < s.gate_muls.size())
            {
                s.gate_muls[idx].coef = current_rand_sentinel;
            }
            else if (idx < s.gate_muls.size() + s.gate_adds.size())
            {
                s.gate_adds[idx - s.gate_muls.size()].coef = current_rand_sentinel;
            }
            else if (idx < s.gate_muls.size() + s.gate_adds.size() + s.gate_consts.size())
            {
                s.gate_consts[idx - s.gate_muls.size() - s.gate_adds.size()].coef = current_rand_sentinel;
            }
            else
            {
                cerr << "Invalid random coefficient index" << endl;
                exit(1);
            }
        }
        return is;
    }
    bool operator==(const Segment &s) const
    {
        if (i_len != s.i_len || o_len != s.o_len)
            return false;
        if (child_segs.size() != s.child_segs.size())
            return false;
        for (size_t i = 0; i < child_segs.size(); i++)
        {
            if (child_segs[i].first != s.child_segs[i].first)
                return false;
            if (child_segs[i].second.size() != s.child_segs[i].second.size())
                return false;
            for (size_t j = 0; j < child_segs[i].second.size(); j++)
            {
                if (child_segs[i].second[j].i_offset != s.child_segs[i].second[j].i_offset)
                    return false;
                if (child_segs[i].second[j].o_offset != s.child_segs[i].second[j].o_offset)
                    return false;
            }
        }
        if (gate_muls.size() != s.gate_muls.size())
            return false;
        for (size_t i = 0; i < gate_muls.size(); i++)
        {
            if (gate_muls[i].in0 != s.gate_muls[i].in0)
                return false;
            if (gate_muls[i].in1 != s.gate_muls[i].in1)
                return false;
            if (gate_muls[i].out != s.gate_muls[i].out)
                return false;
            if (gate_muls[i].coef != s.gate_muls[i].coef)
                return false;
        }
        if (gate_adds.size() != s.gate_adds.size())
            return false;
        for (size_t i = 0; i < gate_adds.size(); i++)
        {
            if (gate_adds[i].in0 != s.gate_adds[i].in0)
                return false;
            if (gate_adds[i].out != s.gate_adds[i].out)
                return false;
            if (gate_adds[i].coef != s.gate_adds[i].coef)
                return false;
        }
        if (gate_consts.size() != s.gate_consts.size())
            return false;
        for (size_t i = 0; i < gate_consts.size(); i++)
        {
            if (gate_consts[i].out != s.gate_consts[i].out)
                return false;
            if (gate_consts[i].coef != s.gate_consts[i].coef)
                return false;
        }
        return true;
    }
};

template <typename F>
F Segment<F>::current_rand_sentinel = F::default_rand_sentinel();

const bool circuit_extraction = false;
const char* extracted_circuit_file_mul = "ExtractedCircuitMul.txt";
const char* extracted_circuit_file_add = "ExtractedCircuitAdd.txt";
template <typename F>
class CircuitRaw
{
public:
    GateMul<F> **mul_gates;
    GateAdd<F> **add_gates;
    size_t *mul_gates_size;
    size_t *add_gates_size;
    vector<Segment<F>> segments;
    vector<size_t> layers; // layer[i] = j means i'th layer is contructed with segments[j]
    F rand_sentinel = F::default_rand_sentinel();
    void load_extracted_circuit(const char *filename_mul, const char *filename_add)
    {
        if(circuit_extraction)
            return;
        FILE *file = fopen(filename_mul, "r");
        
        int depth = layers.size();
        mul_gates = new GateMul<F>*[depth];
        mul_gates_size = new size_t[depth];
        for (int i = 0; i < depth; i++)
        {
            size_t num_gates;
            fscanf(file, "%lu", &num_gates);
            mul_gates[i] = new GateMul<F>[num_gates];
            mul_gates_size[i] = num_gates;
            for (size_t j = 0; j < num_gates; j++)
            {
                unsigned coef;
                fscanf(file, "%lu%lu%lu%u", &mul_gates[i][j].in0, &mul_gates[i][j].in1, &mul_gates[i][j].out, &coef);
                mul_gates[i][j].coef = F(coef);
            }
        }
        fclose(file);

        file = fopen(filename_add, "r");
        add_gates = new GateAdd<F>*[depth];
        add_gates_size = new size_t[depth];
        for (int i = 0; i < depth; i++)
        {
            size_t num_gates;
            fscanf(file, "%lu", &num_gates);
            add_gates[i] = new GateAdd<F>[num_gates];
            add_gates_size[i] = num_gates;
            for (size_t j = 0; j < num_gates; j++)
            {
                unsigned coef;
                fscanf(file, "%lu%lu%u", &add_gates[i][j].in0, &add_gates[i][j].out, &coef);
                add_gates[i][j].coef = F(coef);
            }
        }
        fclose(file);
    }
    const Segment<F> &layer_at(size_t i) const
    {
        return segments[layers[i]];
    }

    const size_t input_size() const
    {
        return layer_at(0).i_len;
    }

    const size_t output_size() const
    {
        return layer_at(layers.size() - 1).o_len;
    }
    const size_t max_layer() const 
    {
        size_t max_size = 0;
        for (size_t i = 0; i < layers.size(); i++)
        {
            max_size = max(max_size, segments[layers[i]].i_len);
            max_size = max(max_size, segments[layers[i]].o_len);
        }
        return max_size;
    }
    friend ostream &operator<<(ostream &os, const CircuitRaw &c)
    {
        Segment<F>::current_rand_sentinel = c.rand_sentinel;
        os.write((char *)&MAGIC_NUM, sizeof(MAGIC_NUM));
        uint64_t num_segments = c.segments.size();
        os.write((char *)&num_segments, sizeof(num_segments));
        for (size_t i = 0; i < num_segments; i++)
            os << c.segments[i];
        uint64_t num_layers = c.layers.size();
        os.write((char *)&num_layers, sizeof(num_layers));
        for (size_t i = 0; i < num_layers; i++)
        {
            uint64_t layer = c.layers[i];
            os.write((char *)&layer, sizeof(layer));
        }
        os << c.rand_sentinel;
        return os;
    }

    friend istream &operator>>(istream &is, CircuitRaw &c)
    {
        uint64_t magic_num;
        is.read((char *)&magic_num, sizeof(magic_num));
        if (magic_num != MAGIC_NUM)
        {
            cerr << "Invalid magic number, expecting " << MAGIC_NUM << ", got " << magic_num << endl;
            exit(1);
        }
        uint64_t num_segments;
        is.read((char *)&num_segments, sizeof(num_segments));
        c.segments.resize(num_segments);
        for (size_t i = 0; i < num_segments; i++)
            is >> c.segments[i];
        uint64_t num_layers;
        is.read((char *)&num_layers, sizeof(num_layers));
        c.layers.resize(num_layers);
        for (size_t i = 0; i < num_layers; i++)
        {
            uint64_t layer;
            is.read((char *)&layer, sizeof(layer));
            c.layers[i] = layer;
        }
        is >> c.rand_sentinel; // FIXME: this sentinel should be set earlier (consider changing format?)

        // temporary fix: do a replacement pass
        for (size_t i = 0; i < c.segments.size(); i++)
        {
            for (size_t j = 0; j < c.segments[i].gate_muls.size(); j++)
            {
                if (c.segments[i].gate_muls[j].coef == Segment<F>::current_rand_sentinel)
                {
                    c.segments[i].gate_muls[j].coef = c.rand_sentinel;
                }
            }
            for (size_t j = 0; j < c.segments[i].gate_adds.size(); j++)
            {
                if (c.segments[i].gate_adds[j].coef == Segment<F>::current_rand_sentinel)
                {
                    c.segments[i].gate_adds[j].coef = c.rand_sentinel;
                }
            }
            for (size_t j = 0; j < c.segments[i].gate_consts.size(); j++)
            {
                if (c.segments[i].gate_consts[j].coef == Segment<F>::current_rand_sentinel)
                {
                    c.segments[i].gate_consts[j].coef = c.rand_sentinel;
                }
            }
        }
        Segment<F>::current_rand_sentinel = c.rand_sentinel;

        return is;
    }
    bool operator==(const CircuitRaw &c) const
    {
        if (segments.size() != c.segments.size())
            return false;
        for (size_t i = 0; i < segments.size(); i++)
            if (segments[i] != c.segments[i])
                return false;
        if (layers.size() != c.layers.size())
            return false;
        for (size_t i = 0; i < layers.size(); i++)
            if (layers[i] != c.layers[i])
                return false;
        return rand_sentinel == c.rand_sentinel;
    }
};
