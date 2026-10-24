#include <gtest/gtest.h>
#include <complex>
#include <map>
#include <random>
#include <vector>

#include "utils/wavelength_map.h"

// B-188: WavelengthMap (a vector by wavelength id) behaves as the std::map<uint32_t, T> it
// replaces in the per-wavelength memories: operator[] adds a default value also when it only
// reads, try_emplace() tells whether it added, iteration visits the present ids in increasing
// order with their values, clear() empties it. Random operations on both, compared after each
TEST(WavelengthMap, BehavesAsStdMap)
{
    typedef std::complex<double> F;
    std::map<uint32_t, F> ref;
    WavelengthMap<F> map;
    std::mt19937 rng(188);
    std::uniform_int_distribution<uint32_t> id_of(0, 40);
    std::uniform_int_distribution<int> action(0, 99);
    std::uniform_real_distribution<double> value(-1, 1);
    for (int step = 0; step < 20000; ++step)
    {
        const uint32_t id = id_of(rng);
        const int a = action(rng);
        if (a < 40)
        {
            const F v(value(rng), value(rng));
            ref[id] = v;
            map[id] = v;
        }
        else if (a < 70)
        {
            // a read through operator[] adds the id (as the devices' memories do)
            ASSERT_EQ(map[id], ref[id]) << "step " << step;
        }
        else if (a < 98)
        {
            const auto r = ref.try_emplace(id);
            const auto m = map.try_emplace(id);
            ASSERT_EQ(m.second, r.second) << "step " << step;
            ASSERT_EQ(m.first->first, id);
            ASSERT_EQ(m.first->second, r.first->second);
            const F v(value(rng), 0);
            r.first->second = v;
            m.first->second = v;
        }
        else
        {
            ref.clear();
            map.clear();
        }
        ASSERT_EQ(map.size(), ref.size());
        ASSERT_EQ(map.empty(), ref.empty());
        std::vector<std::pair<uint32_t, F>> seen;
        for (const auto &e : map)
            seen.emplace_back(e.first, e.second);
        ASSERT_EQ(seen, (std::vector<std::pair<uint32_t, F>>(ref.begin(), ref.end()))) << "step " << step;
        // (const iteration too)
        const WavelengthMap<F> &cmap = map;
        size_t n = 0;
        for (auto it = cmap.begin(); it != cmap.end(); ++it)
            ++n;
        ASSERT_EQ(n, ref.size());
    }
}
