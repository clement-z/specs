#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

// SHA-256 (FIPS 180-4), portable, without external dependency. Used to hash the netlist files of
// the run manifest (F-01, src/run_manifest.h).
namespace spx {

class Sha256 {
public:
    Sha256();

    void update(const void *data, size_t size);
    void update(const std::string &bytes) { update(bytes.data(), bytes.size()); }

    // Finish the hash and return it as 64 lowercase hex digits. Call once, after the last update().
    std::string hex_digest();

private:
    void compress(const unsigned char *block);

    uint32_t m_state[8];
    unsigned char m_block[64];
    size_t m_block_bytes = 0;    // bytes waiting in m_block
    uint64_t m_total_bytes = 0;  // bytes hashed so far
};

// SHA-256 of bytes, as 64 lowercase hex digits
std::string sha256_hex(const std::string &bytes);

// SHA-256 of the content of the file at path, as 64 lowercase hex digits. Returns false (hex is
// left unchanged) if the file cannot be opened or read.
bool sha256_file_hex(const std::string &path, std::string &hex);

}
