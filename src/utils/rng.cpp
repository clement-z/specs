#include "utils/rng.h"

#include <cmath>

namespace spx {

Rng::Rng()
{
    seed(0, "");
}

Rng::Rng(uint64_t master_seed, const std::string &stream_name)
{
    seed(master_seed, stream_name);
}

void Rng::seed(uint64_t master_seed, const std::string &stream_name)
{
    const std::vector<uint32_t> words = seed_words(master_seed, stream_name);
    std::seed_seq seq(words.begin(), words.end());
    m_engine.seed(seq);
    m_cached_normal = 0;
    m_has_cached_normal = false;
}

Rng::result_type Rng::next()
{
    return m_engine();
}

double Rng::uniform()
{
    // 53 random bits: every value k * 2^-53, k = 0 .. 2^53 - 1, exactly
    return static_cast<double>(next() >> 11) * 0x1.0p-53;
}

double Rng::normal()
{
    if (m_has_cached_normal)
    {
        m_has_cached_normal = false;
        return m_cached_normal;
    }

    // Marsaglia polar method: (u, v) uniform in the unit disc (rejection), then
    // u * f and v * f, f = sqrt(-2 ln(s) / s), are two independent standard normal values
    double u, v, s;
    do
    {
        u = 2.0 * uniform() - 1.0; // exact
        v = 2.0 * uniform() - 1.0;
        // Explicit fma: with -ffp-contract=fast (gnu++20) the compiler may fuse u * u + v * v
        // or not depending on the target (-march), which would change the last bit of s
        s = std::fma(u, u, v * v);
    } while (s >= 1.0 || s == 0.0);

    const double f = std::sqrt(-2.0 * std::log(s) / s);
    m_cached_normal = v * f;
    m_has_cached_normal = true;
    return u * f;
}

std::vector<uint32_t> Rng::seed_words(uint64_t master_seed, const std::string &stream_name)
{
    const uint64_t length = stream_name.size();
    std::vector<uint32_t> words;
    words.reserve(4 + (stream_name.size() + 3) / 4);
    words.push_back(static_cast<uint32_t>(master_seed));
    words.push_back(static_cast<uint32_t>(master_seed >> 32));
    words.push_back(static_cast<uint32_t>(length));
    words.push_back(static_cast<uint32_t>(length >> 32));
    for (size_t i = 0; i < stream_name.size(); ++i)
    {
        if (i % 4 == 0)
            words.push_back(0);
        // unsigned char: the same bytes whether char is signed or not
        const uint32_t byte = static_cast<unsigned char>(stream_name[i]);
        words.back() |= byte << (8 * (i % 4));
    }
    return words;
}

Rng rng_for(uint64_t master_seed, const std::string &stream_name)
{
    return Rng(master_seed, stream_name);
}

}
