#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <systemc.h>
#include <utility>
#include <vector>

#include "optical_signal.h"
#include "utils/pqueue.h"

// The order of the pending entries of an output port: earlier time first, and for equal times
// the smaller wavelength first (the convention of the PROBE latch). Equal times are common with
// WDM (the channels share the delays). Two entries never share (time, wavelength) (the writers
// merge), so the order is total, and a merge (a new value for a pending entry) does not change
// its key: the order of emission is fixed by the keys alone, whatever the heap layout or the
// standard library. (Phase N, decision 2026-10-24; before: the larger |E| first, which a merge
// changed without repairing the heap, B-198.) The wavelength of an id is read from a table of the
// queue (wavelengths_m, indexed by the id; the ids of the registry, OpticalSignal)
struct PendingEntryLater {
    const std::vector<double> *wavelengths_m = nullptr;

    bool operator()(const std::pair<sc_time, OpticalSignal> &a, const std::pair<sc_time, OpticalSignal> &b) const
    {
        if (a.first != b.first)
            return a.first > b.first;
        return (*wavelengths_m)[a.second.m_wavelength_id] > (*wavelengths_m)[b.second.m_wavelength_id];
    }
};

// B-015, B-188: the pending entries of an output port, in the order of PendingEntryLater, with the
// lookup of the entry pending at a (time, wavelength) that a write merges into. The lookup was a
// linear scan of all entries for every written value (O(pending), and pending grows with the channel count). Now:
// none when the time is later than the latest pending entry of that wavelength (most writes: a
// device writes at now + its delay), else the scan, from the newest entries (the leaves, where
// the entry just pushed at the latest time is likely to be). At most one entry per (time,
// wavelength), the writers merge, so the scan order does not change the result
class OpticalPendingQueue {
public:
    typedef std::pair<sc_time, OpticalSignal> entry_type;

    OpticalPendingQueue()
        : m_heap(PendingEntryLater{&m_wavelengths_m})
    {
    }
    // (the comparator of the heap points to m_wavelengths_m)
    OpticalPendingQueue(const OpticalPendingQueue &) = delete;
    OpticalPendingQueue &operator=(const OpticalPendingQueue &) = delete;

    bool empty() const
    {
        return m_heap.empty();
    }

    size_t size() const
    {
        return m_heap.size();
    }

    // The next entry (earliest time; equal times: smallest wavelength)
    const entry_type &top() const
    {
        return m_heap.top();
    }

    // The entry pending at time t for that wavelength, nullptr if none. Its value can be replaced
    // (same wavelength, so the same key and place). Valid until the next push/pop
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
        if (wavelength_id >= m_known_wavelengths) [[unlikely]]
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
        m_heap = heap_type(PendingEntryLater{&m_wavelengths_m});
        std::fill(m_by_wavelength.begin(), m_by_wavelength.end(), WavelengthState());
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

    // (out of line: once per wavelength). The wavelengths of the registry never change (ids are
    // only added), so the table is kept across clear(). An id not registered yet (the default id
    // 0 of a signal without a wavelength, before any is registered: a device memory read before
    // the first source wrote, in the legacy testbenches) sorts after the registered ones and is
    // looked up again at its next push. Its key can change then; that only matters if entries of
    // another id share its time, which cannot happen while it is the only id (empty registry)
    [[gnu::noinline]] void addWavelengths(uint32_t wavelength_id)
    {
        if (wavelength_id >= m_by_wavelength.size())
        {
            m_by_wavelength.resize(wavelength_id + 1);
            m_wavelengths_m.resize(wavelength_id + 1, std::numeric_limits<double>::infinity());
        }
        const uint32_t known = std::min<uint32_t>(OpticalSignal::registeredWavelengths(), m_wavelengths_m.size());
        for (uint32_t id = m_known_wavelengths; id < known; ++id)
            m_wavelengths_m[id] = OpticalSignal::getWavelength(id);
        m_known_wavelengths = std::max(m_known_wavelengths, known);
    }

    // m_by_wavelength and m_wavelengths_m have one slot per id seen; the wavelengths of ids below
    // m_known_wavelengths are those of the registry
    std::vector<double> m_wavelengths_m; // by wavelength id (PendingEntryLater)
    uint32_t m_known_wavelengths = 0;
    heap_type m_heap;
    std::vector<WavelengthState> m_by_wavelength; // by wavelength id
};
