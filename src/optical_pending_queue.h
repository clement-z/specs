#pragma once

#include <algorithm>
#include <complex>
#include <cstdint>
#include <systemc.h>
#include <utility>
#include <vector>

#include "optical_signal.h"
#include "utils/pqueue.h"

// F-15: compares two squared magnitudes (norms) computed with a few roundings each, when those
// roundings cannot change the result of comparing the magnitudes themselves (as std::abs()
// and a division compute them): +1 if x2 > y2, -1 if x2 < y2, 0 if they are within a relative
// 1e-13 (the roundings are below 1e-14) or either is outside [1e-280, 1e280] (zero, subnormal
// components, overflow, inf, NaN), where the caller decides on the magnitudes instead
inline int compareSquaredMagnitudes(double x2, double y2)
{
    constexpr double lo = 1e-280;
    constexpr double hi = 1e280;
    constexpr double margin = 1e-13;
    if (!(x2 >= lo && x2 <= hi && y2 >= lo && y2 <= hi))
        return 0;
    if (x2 > y2 * (1 + margin))
        return 1;
    if (x2 < y2 * (1 - margin))
        return -1;
    return 0;
}

// B-188: the order of the pending entries of an output port, that of std::greater<pair<sc_time,
// OpticalSignal>> (the comparator of the queue before): earlier time first, and for equal times
// the larger |E| first (OpticalSignal::operator<, which compares modulus(): std::abs()). Equal
// times are common with WDM (the channels share the delays), and the pair comparison computed
// up to four std::abs() there. Decided here on squared norms when compareSquaredMagnitudes() is
// sure, else on the magnitudes as before: the same result for every pair of entries, so the
// same heap and the same order of emission, bit for bit
struct PendingEntryLater {
    bool operator()(const std::pair<sc_time, OpticalSignal> &a, const std::pair<sc_time, OpticalSignal> &b) const
    {
        if (a.first != b.first)
            return a.first > b.first;
        // a after b when |a| < |b|
        const int order = compareSquaredMagnitudes(norm(a.second.m_field), norm(b.second.m_field));
        if (order != 0)
            return order < 0;
        return a.second.modulus() < b.second.modulus();
    }
};

// B-015, B-188: the pending entries of an output port, with the lookup of the entry pending at a
// (time, wavelength) that a write merges into. The heap is the std::priority_queue of the port
// before (with PendingEntryLater): same layout, same order. The lookup was a linear scan of all
// entries for every written value (O(pending), and pending grows with the channel count). Now:
// none when the time is later than the latest pending entry of that wavelength (most writes: a
// device writes at now + its delay), else the scan, from the newest entries (the leaves, where
// the entry just pushed at the latest time is likely to be). At most one entry per (time,
// wavelength), the writers merge, so the scan order does not change the result
class OpticalPendingQueue {
public:
    typedef std::pair<sc_time, OpticalSignal> entry_type;

    bool empty() const
    {
        return m_heap.empty();
    }

    size_t size() const
    {
        return m_heap.size();
    }

    // The next entry (earliest time; equal times: larger |E|)
    const entry_type &top() const
    {
        return m_heap.top();
    }

    // The entry pending at time t for that wavelength, nullptr if none. Its value can be replaced
    // (same wavelength): it keeps its place in the heap, as before. Valid until the next push/pop
    entry_type *find(const sc_time &t, uint32_t wavelength_id)
    {
        if (wavelength_id >= m_by_wavelength.size())
            return nullptr;
        const WavelengthState &w = m_by_wavelength[wavelength_id];
        if (w.pending == 0 || t.value() > w.latest)
            return nullptr;
        for (auto it = m_heap.end(); it != m_heap.begin();)
        {
            --it;
            if (it->first == t && it->second.m_wavelength_id == wavelength_id)
                return &*it;
        }
        return nullptr;
    }

    // A new entry (no merge: see find())
    void push(const entry_type &e)
    {
        const uint32_t wavelength_id = e.second.m_wavelength_id;
        if (wavelength_id >= m_by_wavelength.size()) [[unlikely]]
            addWavelengths(wavelength_id);
        WavelengthState &w = m_by_wavelength[wavelength_id];
        const sc_time::value_type t = e.first.value();
        if (w.pending == 0 || t > w.latest)
            w.latest = t;
        ++w.pending;
        m_heap.push(e);
    }

    void pop()
    {
        --m_by_wavelength[m_heap.top().second.m_wavelength_id].pending;
        m_heap.pop();
    }

    void clear()
    {
        m_heap = heap_type();
        m_by_wavelength.clear();
    }

private:
    typedef PQueue<entry_type, PendingEntryLater> heap_type;

    // The pending entries of one wavelength: how many, and the latest time among them. Entries
    // are taken in time order, so the one at that time stays pending while any is: the latest
    // time is exact, not a bound
    struct WavelengthState {
        uint64_t pending = 0;
        sc_time::value_type latest = 0;
    };

    // (out of line: once per wavelength)
    [[gnu::noinline]] void addWavelengths(uint32_t wavelength_id)
    {
        m_by_wavelength.resize(wavelength_id + 1);
    }

    heap_type m_heap;
    std::vector<WavelengthState> m_by_wavelength; // by wavelength id
};
