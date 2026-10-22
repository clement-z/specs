// SHA-256 unit tests (F-01 run manifest): NIST test vectors (FIPS 180-4 examples, "SHA-256" in the
// NIST CSRC example values) and the file helper.
#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <string>

#include "utils/sha256.h"
#include "test/utils.h"

namespace {
const std::string nist_448_bits = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
}

TEST(Sha256, NistVectors)
{
    SPECS_TEST_HEADER()
    {
        EXPECT_EQ(spx::sha256_hex(""), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
        EXPECT_EQ(spx::sha256_hex("abc"), "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
        // 56 bytes: the padding does not fit in the last block
        EXPECT_EQ(spx::sha256_hex(nist_448_bits), "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
        // 10^6 bytes, a multiple of the block size
        EXPECT_EQ(spx::sha256_hex(std::string(1000000, 'a')), "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
    }
    SPECS_TEST_FOOTER();
}

TEST(Sha256, IncrementalUpdatesMatchOneShot)
{
    SPECS_TEST_HEADER()
    {
        for (size_t split = 0; split <= nist_448_bits.size(); ++split)
        {
            spx::Sha256 h;
            h.update(nist_448_bits.substr(0, split));
            h.update(nist_448_bits.substr(split));
            EXPECT_EQ(h.hex_digest(), "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1") << "split at " << split;
        }

        spx::Sha256 h;
        for (int i = 0; i < 1000000; ++i)
            h.update("a", 1);
        EXPECT_EQ(h.hex_digest(), "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");

        // lengths around the block and padding boundaries
        for (size_t n : { 55, 56, 63, 64, 65, 119, 120, 128 })
        {
            const std::string msg(n, 'x');
            spx::Sha256 h2;
            for (char c : msg)
                h2.update(&c, 1);
            EXPECT_EQ(h2.hex_digest(), spx::sha256_hex(msg)) << n << " bytes";
        }
    }
    SPECS_TEST_FOOTER();
}

TEST(Sha256, File)
{
    SPECS_TEST_HEADER()
    {
        const std::string path = testing::TempDir() + "specs_test_" + std::to_string(getpid()) + "_sha256.bin";
        {
            // binary content: the file is hashed byte for byte
            std::ofstream f(path, std::ios::binary);
            f << nist_448_bits << '\0' << "\r\n";
        }
        std::string hex = "unchanged";
        EXPECT_TRUE(spx::sha256_file_hex(path, hex));
        EXPECT_EQ(hex, spx::sha256_hex(nist_448_bits + std::string(1, '\0') + "\r\n"));
        std::remove(path.c_str());

        hex = "unchanged";
        EXPECT_FALSE(spx::sha256_file_hex(path, hex));
        EXPECT_EQ(hex, "unchanged");
        // a directory can be opened but not read
        EXPECT_FALSE(spx::sha256_file_hex(testing::TempDir(), hex));
        EXPECT_EQ(hex, "unchanged");
    }
    SPECS_TEST_FOOTER();
}
