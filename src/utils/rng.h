#pragma once

#include <cstdint>
#include <random>
#include <string>
#include <vector>

// Reproducible random streams (F-01, B-055).
//
// One master seed, SPECSConfig::seed (`specs --seed N` or `.options seed=N`, default 0), drives every
// random draw of a simulation. Each consumer takes its own stream:
//     spx::Rng rng = spx::rng_for(specsGlobalConfig.seed, name());
// The stream name is the hierarchical name of the device (e.g. "ROOT/pdet1"), not a creation index,
// so adding, removing or reordering other devices never shifts a device's sequence. Take the stream
// once per simulation, after the configuration is complete (CLI and .options are applied after the
// elements are created): start_of_simulation() is the place (see Detector).
//
// The sequences are fully specified by the C++ standard, so they are the same with libstdc++, libc++
// and MSVC: std::mt19937_64 seeded through std::seed_seq (both algorithms are specified), from an
// injective encoding of (master seed, stream name) (seed_words()), so two different names never get
// the same seed sequence. uniform() and normal() are implemented here, because the <random>
// distributions (and std::default_random_engine, std::hash) are implementation-defined. normal()
// uses std::log and std::sqrt: sqrt is correctly rounded everywhere, and the result is the same on
// every platform whose log returns the same value (glibc for both libstdc++ and libc++ on Linux).
//
// Future draws (F-02 `.param gauss()`, Monte-Carlo, F-14 variability; not implemented yet) must take
// their stream from
//     spx::rng_for(specsGlobalConfig.seed, <hierarchical name of the parameter, or another stable stream name>)
// so that one master seed reproduces the whole run. Do not seed them from std::random_device or from
// a creation counter.
namespace spx {

class Rng {
public:
    typedef std::mt19937_64 engine_type;
    typedef engine_type::result_type result_type;

    // The stream rng_for(0, "")
    Rng();
    // The stream rng_for(master_seed, stream_name)
    Rng(uint64_t master_seed, const std::string &stream_name);

    // Restart as the stream rng_for(master_seed, stream_name) (also drops the cached normal value)
    void seed(uint64_t master_seed, const std::string &stream_name);

    // Next raw 64-bit output of the engine
    result_type next();

    // Uniform in [0, 1): the top 53 bits of next() times 2^-53 (one engine output)
    double uniform();

    // Standard normal (mean 0, standard deviation 1), Marsaglia polar method: a pair of values from
    // pairs of uniform() draws; the second value is cached (part of the stream state) and returned by
    // the next call
    double normal();

    // Input of the std::seed_seq for (master_seed, stream_name), 32-bit words: master seed (low, high),
    // name length in bytes (low, high), then the name bytes, 4 per word, little-endian, the last word
    // zero-padded. The fixed-size fields and the length make the encoding injective.
    static std::vector<uint32_t> seed_words(uint64_t master_seed, const std::string &stream_name);

private:
    engine_type m_engine;
    double m_cached_normal = 0;
    bool m_has_cached_normal = false;
};

// Random stream of `stream_name` (a hierarchical instance name) for the master seed `master_seed`
Rng rng_for(uint64_t master_seed, const std::string &stream_name);

}
