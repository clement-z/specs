#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <systemc.h>
#include <utility>
#include <vector>

#include "optical_signal.h"

// B-015, B-188: the pending entries of an output port, with the lookup of the entry pending at a
// (time, wavelength) that a write merges into.
//
// Order: earlier time first, and for equal times the smaller wavelength first (the convention of
// the PROBE latch). Equal times are common with WDM (the channels share the delays). Two entries
// never share (time, wavelength) (the writers merge), so the order is total, and a merge (a new
// value for a pending entry) does not change its key: the order of emission is fixed by the keys
// alone. (Phase N, decision 2026-10-24; before: the larger |E| first, which a merge changed
// without repairing the heap of that time, B-198.)
//
// Storage (phase N, step 2: a per-time-step queue instead of a heap): the entries in that order
// in a ring buffer, so the entries of one time step are one run (a bucket) in wavelength order,
// the next entry is the first one and a pop is O(1) (the heap's was O(log pending), and pending
// grows with the channel count). A write is appended when it is later than the last entry: a
// device writes at now + its delay, and the entries of a time step usually arrive in wavelength
// order, as the ports upstream emit them in that order. Else (a merge, another wavelength order,
// mixed delays) a binary search finds the entry or the place, and an insertion shifts the entries
// on the shorter side (within the last bucket: at most its size). Same order as the heap of step 1,
// so the same emissions, bit for bit
class OpticalPendingQueue {
public:
    typedef std::pair<sc_time, OpticalSignal> entry_type;

    OpticalPendingQueue() {}
    // (top() and find() point into the ring)
    OpticalPendingQueue(const OpticalPendingQueue &) = delete;
    OpticalPendingQueue &operator=(const OpticalPendingQueue &) = delete;

    bool empty() const
    {
        return m_size == 0;
    }

    size_t size() const
    {
        return m_size;
    }

    // The next entry (earliest time; equal times: smallest wavelength)
    const entry_type &top() const
    {
        return m_ring[m_head];
    }

    // A value written for time t: merged into the entry pending at (t, its wavelength), if any
    // (returns true), else a new entry (false)
    bool write(const sc_time &t, const OpticalSignal &value)
    {
        const uint32_t wavelength_id = value.m_wavelength_id;
        if (wavelength_id >= m_known_wavelengths) [[unlikely]]
            addWavelengths(wavelength_id);
        if (m_size != 0)
        {
            entry_type &last = at(m_size - 1);
            if (t == last.first)
            {
                if (last.second.m_wavelength_id == wavelength_id)
                {
                    last.second = value;
                    return true;
                }
                if (m_wavelengths_m[wavelength_id] < m_wavelengths_m[last.second.m_wavelength_id])
                    return writeBefore(t, value);
            }
            else if (t < last.first)
                return writeBefore(t, value);
        }
        if (m_size == capacity()) [[unlikely]]
            grow();
        at(m_size) = entry_type(t, value);
        ++m_size;
        return false;
    }

    // The entry pending at time t for that wavelength, nullptr if none. Its value can be replaced
    // (same wavelength, so the same key and place). Valid until the next write/push/pop
    entry_type *find(const sc_time &t, uint32_t wavelength_id)
    {
        if (wavelength_id >= m_known_wavelengths)
            addWavelengths(wavelength_id);
        const size_t i = lowerBound(t, m_wavelengths_m[wavelength_id]);
        if (i < m_size && at(i).first == t && at(i).second.m_wavelength_id == wavelength_id)
            return &at(i);
        return nullptr;
    }

    // A new entry (none pending at its time and wavelength: see find())
    void push(const entry_type &e)
    {
        const bool merged = write(e.first, e.second);
        (void)merged;
    }

    void pop()
    {
        m_head = (m_head + 1) & m_mask;
        --m_size;
    }

    void clear()
    {
        m_head = 0;
        m_size = 0;
    }

private:
    size_t capacity() const
    {
        return m_ring.size();
    }

    // The entry of rank i (0: the next one)
    entry_type &at(size_t i)
    {
        return m_ring[(m_head + i) & m_mask];
    }

    // The rank of the first entry not before (t, wavelength_m)
    size_t lowerBound(const sc_time &t, double wavelength_m)
    {
        size_t lo = 0;
        size_t n = m_size;
        while (n > 0)
        {
            const size_t half = n / 2;
            const entry_type &e = at(lo + half);
            if (e.first < t || (e.first == t && m_wavelengths_m[e.second.m_wavelength_id] < wavelength_m))
            {
                lo += half + 1;
                n -= half + 1;
            }
            else
                n = half;
        }
        return lo;
    }

    // A write not after the last entry (out of line: write() appends most values)
    [[gnu::noinline]] bool writeBefore(const sc_time &t, const OpticalSignal &value)
    {
        const size_t i = lowerBound(t, m_wavelengths_m[value.m_wavelength_id]);
        if (i < m_size && at(i).first == t && at(i).second.m_wavelength_id == value.m_wavelength_id)
        {
            at(i).second = value;
            return true;
        }
        if (m_size == capacity())
            grow();
        if (i >= m_size / 2)
        {
            // shift the entries from rank i on one place back
            for (size_t j = m_size; j > i; --j)
                at(j) = at(j - 1);
        }
        else
        {
            // shift the entries before rank i one place forward (the head moves back)
            m_head = (m_head - 1) & m_mask;
            for (size_t j = 0; j < i; ++j)
                at(j) = at(j + 1);
        }
        at(i) = entry_type(t, value);
        ++m_size;
        return false;
    }

    // Twice the capacity (at least 8), the entries in order from slot 0
    [[gnu::noinline]] void grow()
    {
        std::vector<entry_type> ring(std::max<size_t>(8, 2 * capacity()));
        for (size_t i = 0; i < m_size; ++i)
            ring[i] = at(i);
        m_ring.swap(ring);
        m_head = 0;
        m_mask = m_ring.size() - 1;
    }

    // (out of line: once per wavelength). The wavelengths of the registry never change (ids are
    // only added), so the table is kept across clear(). An id not registered yet (the default id
    // 0 of a signal without a wavelength, before any is registered: a device memory read before
    // the first source wrote, in the legacy testbenches) sorts after the registered ones and is
    // looked up again at its next write. Its key can change then; that only matters if entries of
    // another id share its time, which cannot happen while it is the only id (empty registry)
    [[gnu::noinline]] void addWavelengths(uint32_t wavelength_id)
    {
        if (wavelength_id >= m_wavelengths_m.size())
            m_wavelengths_m.resize(wavelength_id + 1, std::numeric_limits<double>::infinity());
        const uint32_t known = std::min<uint32_t>(OpticalSignal::registeredWavelengths(), m_wavelengths_m.size());
        for (uint32_t id = m_known_wavelengths; id < known; ++id)
            m_wavelengths_m[id] = OpticalSignal::getWavelength(id);
        m_known_wavelengths = std::max(m_known_wavelengths, known);
    }

    // The entries, from rank 0 at m_head; the capacity is a power of 2 (or 0), m_mask = capacity - 1
    std::vector<entry_type> m_ring;
    size_t m_head = 0;
    size_t m_size = 0;
    size_t m_mask = 0;
    // The wavelength of each id seen (the key of the order); those of ids below
    // m_known_wavelengths are the registry's
    std::vector<double> m_wavelengths_m;
    uint32_t m_known_wavelengths = 0;
};
