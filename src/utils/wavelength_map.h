#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

// B-188: values by wavelength id, for the per-wavelength memories that the output ports and the
// devices read and write per event. Wavelength ids are small dense integers (the registry of
// OpticalSignal::getIDFromWavelength()), so a vector indexed by the id replaces the std::map these
// were (a tree walk per access, with a cache miss per level, deeper with more channels). It
// behaves as the map did for what they use: operator[] (adds T() for an id not present, also when
// it only reads), try_emplace(id), clear(), size(), and iteration over the present ids in
// increasing order, the order of the map (sums over the wavelengths add in the same order).
// One vector of (id, value): the slot of an absent id holds ABSENT as id (one cache line per access)
template <class T>
class WavelengthMap {
public:
    typedef std::pair<uint32_t, T> value_type;
    static constexpr uint32_t ABSENT = UINT32_MAX;

    template <class Map, class Value>
    class basic_iterator {
    public:
        basic_iterator(Map *map, size_t i)
            : m_map(map)
            , m_i(i)
        {
            skip();
        }

        Value &operator*() const
        {
            return m_map->m_values[m_i];
        }

        Value *operator->() const
        {
            return &m_map->m_values[m_i];
        }

        basic_iterator &operator++()
        {
            ++m_i;
            skip();
            return *this;
        }

        bool operator==(const basic_iterator &rhs) const
        {
            return m_i == rhs.m_i;
        }

        bool operator!=(const basic_iterator &rhs) const
        {
            return m_i != rhs.m_i;
        }

    private:
        void skip()
        {
            while (m_i < m_map->m_values.size() && m_map->m_values[m_i].first == ABSENT)
                ++m_i;
        }

        Map *m_map;
        size_t m_i;
    };

    typedef basic_iterator<WavelengthMap, value_type> iterator;
    typedef basic_iterator<const WavelengthMap, const value_type> const_iterator;

    // The value of that wavelength, T() added if it is not present
    T &operator[](uint32_t id)
    {
        return try_emplace(id).first->second;
    }

    // As std::map::try_emplace(id): the entry of that wavelength, and whether it was added (T())
    std::pair<value_type *, bool> try_emplace(uint32_t id)
    {
        if (id < m_values.size() && m_values[id].first == id) [[likely]]
            return {&m_values[id], false};
        return {&add(id), true};
    }

    size_t size() const
    {
        return m_size;
    }

    bool empty() const
    {
        return m_size == 0;
    }

    void clear()
    {
        m_values.clear();
        m_size = 0;
    }

    iterator begin()
    {
        return iterator(this, 0);
    }

    iterator end()
    {
        return iterator(this, m_values.size());
    }

    const_iterator begin() const
    {
        return const_iterator(this, 0);
    }

    const_iterator end() const
    {
        return const_iterator(this, m_values.size());
    }

private:
    // (out of line: once per wavelength)
    [[gnu::noinline]] value_type &add(uint32_t id)
    {
        if (id >= m_values.size())
            m_values.resize(id + 1, value_type(ABSENT, T()));
        m_values[id] = value_type(id, T());
        ++m_size;
        return m_values[id];
    }

    std::vector<value_type> m_values; // by id
    size_t m_size = 0;
};
