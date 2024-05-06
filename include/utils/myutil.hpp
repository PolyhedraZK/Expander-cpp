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

bool is_pow_2(uint32 n)
{
    return (n & (n - 1)) == 0;
}

uint32 next_pow_of_2(uint32 n)
{
    if (n == 0 || is_pow_2(n))
    {
        return n;
    }
    else 
    {
        return ((uint32) 1) << (32 - __builtin_clz(n));
    }
}

}