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

// Number of points of an inclusive sweep (.dc start stop step): floor((stop - start)/step) + 1,
// the slack absorbs the rounding of the ratio (29.999999999999996 counts as 30 intervals).
// NaN or infinite parameters are invalid too.
template <typename T>
size_t sweep_count(const T &start, const T &stop, const T &step)
{
    const double n_intervals = floor((stop - start) / step + 1e-9);
    if (!(stop > start) || !(step > 0) || !std::isfinite(n_intervals))
    {
        cerr << "Invalid range parameters" << endl;
        exit(1);
    }
    return static_cast<size_t>(n_intervals) + 1;
}

// Values of an inclusive sweep: sweep_count() values accumulated as in range() (v0 = start,
// v(i+1) = v(i) + step), so the stop value is included when (stop - start)/step is an integer
template <typename T>
vector<T> sweep_values(const T &start, const T &stop, const T &step)
{
    const size_t n = sweep_count(start, stop, step);
    vector<T> ret;
    ret.reserve(n);
    T val = start;
    for (size_t i = 0; i < n; ++i)
    {
        ret.push_back(val);
        val += step;
    }
    return ret;
}