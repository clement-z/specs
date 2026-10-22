#include "utils/sha256.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {

// FIPS 180-4, 4.2.2: first 32 bits of the fractional parts of the cube roots of the first 64 primes
constexpr uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};

// FIPS 180-4, 4.1.2
inline uint32_t rotr(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }
inline uint32_t ch(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (~x & z); }
inline uint32_t maj(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (x & z) ^ (y & z); }
inline uint32_t big_sigma0(uint32_t x) { return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22); }
inline uint32_t big_sigma1(uint32_t x) { return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25); }
inline uint32_t small_sigma0(uint32_t x) { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }
inline uint32_t small_sigma1(uint32_t x) { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }

}

namespace spx {

Sha256::Sha256()
    // FIPS 180-4, 5.3.3: initial hash value
    : m_state { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 }
{
}

// FIPS 180-4, 6.2.2: process one 512-bit block
void Sha256::compress(const unsigned char *block)
{
    uint32_t w[64];
    for (int t = 0; t < 16; ++t)
    {
        w[t] = (uint32_t(block[4 * t]) << 24) | (uint32_t(block[4 * t + 1]) << 16)
             | (uint32_t(block[4 * t + 2]) << 8) | uint32_t(block[4 * t + 3]);
    }
    for (int t = 16; t < 64; ++t)
        w[t] = small_sigma1(w[t - 2]) + w[t - 7] + small_sigma0(w[t - 15]) + w[t - 16];

    uint32_t a = m_state[0], b = m_state[1], c = m_state[2], d = m_state[3];
    uint32_t e = m_state[4], f = m_state[5], g = m_state[6], h = m_state[7];
    for (int t = 0; t < 64; ++t)
    {
        uint32_t t1 = h + big_sigma1(e) + ch(e, f, g) + K[t] + w[t];
        uint32_t t2 = big_sigma0(a) + maj(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }
    m_state[0] += a;
    m_state[1] += b;
    m_state[2] += c;
    m_state[3] += d;
    m_state[4] += e;
    m_state[5] += f;
    m_state[6] += g;
    m_state[7] += h;
}

void Sha256::update(const void *data, size_t size)
{
    const unsigned char *bytes = static_cast<const unsigned char *>(data);
    m_total_bytes += size;
    while (size > 0)
    {
        size_t n = std::min(size, sizeof(m_block) - m_block_bytes);
        std::memcpy(m_block + m_block_bytes, bytes, n);
        m_block_bytes += n;
        bytes += n;
        size -= n;
        if (m_block_bytes == sizeof(m_block))
        {
            compress(m_block);
            m_block_bytes = 0;
        }
    }
}

std::string Sha256::hex_digest()
{
    // FIPS 180-4, 5.1.1: a 1 bit, zeros up to 448 bits mod 512, then the message length in bits
    // (64-bit big-endian)
    const uint64_t total_bits = m_total_bytes * 8;
    unsigned char padding[64] = { 0x80 };
    update(padding, m_block_bytes < 56 ? 56 - m_block_bytes : 120 - m_block_bytes);
    unsigned char length[8];
    for (int i = 0; i < 8; ++i)
        length[i] = static_cast<unsigned char>(total_bits >> (56 - 8 * i));
    update(length, sizeof(length));

    const char *digits = "0123456789abcdef";
    std::string hex;
    hex.reserve(64);
    for (uint32_t word : m_state)
    {
        for (int shift = 28; shift >= 0; shift -= 4)
            hex += digits[(word >> shift) & 0xf];
    }
    return hex;
}

std::string sha256_hex(const std::string &bytes)
{
    Sha256 h;
    h.update(bytes);
    return h.hex_digest();
}

bool sha256_file_hex(const std::string &path, std::string &hex)
{
    FILE *f = fopen(path.c_str(), "rb");
    if (!f)
        return false;

    Sha256 h;
    std::vector<char> buf(1 << 16);
    size_t n;
    while ((n = fread(buf.data(), 1, buf.size(), f)) > 0)
        h.update(buf.data(), n);
    // e.g. EISDIR: a directory can be opened but not read
    bool ok = !ferror(f);
    fclose(f);
    if (!ok)
        return false;

    hex = h.hex_digest();
    return true;
}

}
