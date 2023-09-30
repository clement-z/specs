#pragma once

#include <cmath>
#include <vector>
#include <iostream>

using std::abs;
using std::vector;
using std::cerr;
using std::endl;

template <typename T1, typename T2>
inline bool is_close(const T1& x1, const T2& x2, const double &precision = 1e-12)
{
    return abs(x1-x2) < precision;
}

//double linspace(double minval, double maxval, double npoints);

template <typename T>
vector<T> range(const T &minval, const T &maxval, const T &step)
{
    vector<T> ret;
    if (maxval <= minval || step <= 0)
    {
        cerr << "Invalid range parameters" << endl;
        // because i don't have time to cover all cases
        exit(1);
    }
    int n = floor((maxval - minval) / step);
    ret.reserve(n);
    for (T val = minval; val < maxval; val += step)
        ret.push_back(val);
    return ret;
}