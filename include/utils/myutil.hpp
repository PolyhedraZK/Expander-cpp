#pragma once
#include <vector>
#include <string>
#include <chrono>
#include <unordered_map>
#include <iomanip>
#include <iostream>

#include "types.hpp"

using namespace std;

void report_timing(string event_name, bool is_start)
{
    static unordered_map<string, chrono::time_point<chrono::system_clock>> last;
    auto nw = chrono::system_clock::now();
    if (is_start)
    {
        last[event_name] = nw;
    }
    else
    {
        auto duration = chrono::duration_cast<chrono::microseconds>(nw - last.at(event_name));
        cout << "* " << event_name << "\t" << fixed << setprecision(3)
             << double(duration.count()) / 1000 << " ms" << endl;
        last.erase(event_name);
    }
}

namespace gkr 
{

bool is_pow_2(uint32_t n)
{
    return (n & (n - 1)) == 0;
}

uint32_t next_pow_of_2(uint32_t n)
{
    if (n == 0 || is_pow_2(n))
    {
        return n;
    }
    else 
    {
        return ((uint32_t) 1) << (32 - __builtin_clz(n));
    }
}

uint32_t log_2(uint32_t n)
{
    assert(is_pow_2(n));
    return __builtin_ctz(n);
}

}