// F-05: table export of the probed signals (TraceTable, specs --format csv|npz): the CSV and .npz
// layout, one row per (time step, probe, wavelength), and the probes writing to a table without a VCD.
// The CSV and .npz of whole netlists are checked by the golden cases regress/f05-format-*.
#include <gtest/gtest.h>
#include <systemc.h>

#include <complex>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "devices/probe.h"
#include "optical_signal.h"
#include "specs.h"
#include "test/utils.h"
#include "trace_table.h"

namespace {

std::string tempName(const std::string &ext)
{
    return testing::TempDir() + "specs_test_table_" + std::to_string(getpid()) + ext;
}

std::string readFile(const std::string &filename)
{
    std::ifstream f(filename, std::ios::binary);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

uint32_t u32(const std::string &b, size_t i)
{
    return uint32_t((unsigned char)b[i]) | uint32_t((unsigned char)b[i + 1]) << 8
           | uint32_t((unsigned char)b[i + 2]) << 16 | uint32_t((unsigned char)b[i + 3]) << 24;
}

uint16_t u16(const std::string &b, size_t i)
{
    return uint16_t((unsigned char)b[i]) | uint16_t((unsigned char)b[i + 1]) << 8;
}

OpticalSignal sig(double re, double im, double wavelength)
{
    return OpticalSignal(OpticalSignal::field_type(re, im), wavelength);
}

}

TEST(TraceTable, Crc32KnownAnswer)
{
    EXPECT_EQ(TraceTable::crc32("123456789", 9), 0xCBF43926u);
    EXPECT_EQ(TraceTable::crc32("", 0), 0u);
    // continued over two parts
    EXPECT_EQ(TraceTable::crc32("6789", 4, TraceTable::crc32("12345", 5)), 0xCBF43926u);
}

TEST(TraceTable, NpyHeaderIsAlignedTo64Bytes)
{
    for (size_t n : { size_t(0), size_t(3), size_t(123456789) })
    {
        const std::string h = TraceTable::npy_header("<f8", n);
        EXPECT_EQ(h.size() % 64, 0u);
        EXPECT_EQ(h.substr(0, 8), std::string("\x93NUMPY\x01\x00", 8));
        EXPECT_EQ(u16(h, 8), h.size() - 10);
        EXPECT_EQ(h.back(), '\n');
        EXPECT_NE(h.find("{'descr': '<f8', 'fortran_order': False, 'shape': (" + std::to_string(n) + ",), }"),
                  std::string::npos);
    }
}

TEST(TraceTable, CsvOneRowPerTimeStepProbeAndWavelength)
{
    SPECS_TEST_HEADER()
    {
        const std::string csv = tempName(".csv");
        {
            TraceTable table(csv, "");
            const uint32_t a = table.signal_index("PROBE{ROOT/a}");
            const uint32_t b = table.signal_index("odd,\"name\"");
            EXPECT_EQ(table.signal_index("PROBE{ROOT/a}"), a);

            // time step 0: a at 1550 nm twice (the last one wins), a at 1310 nm, b
            table.append(0, a, sig(1, 2, 1.55e-6));
            table.append(0, a, sig(0.5, -0.25, 1.31e-6));
            table.append(0, b, sig(0, 1, 1.55e-6));
            table.append(0, a, sig(3, 4, 1.55e-6));
            // next time step
            table.append(1e-13, a, sig(0.1, 0, 1.55e-6));
            EXPECT_TRUE(table.close());
            EXPECT_EQ(table.rows(), 4u);
            // dropped after close()
            table.append(2e-13, a, sig(1, 1, 1.55e-6));
            EXPECT_EQ(table.rows(), 4u);
        }
        EXPECT_EQ(readFile(csv),
                  "time_s,signal,wavelength_m,power_W,field_re,field_im\n"
                  "0,PROBE{ROOT/a},1.55e-06,25,3,4\n"
                  "0,PROBE{ROOT/a},1.31e-06,0.3125,0.5,-0.25\n"
                  "0,\"odd,\"\"name\"\"\",1.55e-06,1,0,1\n"
                  "1e-13,PROBE{ROOT/a},1.55e-06,0.010000000000000002,0.1,0\n");
        std::remove(csv.c_str());
    }
    SPECS_TEST_FOOTER();
}

TEST(TraceTable, DiscardDropsTheRowOfTheCurrentTimeStep)
{
    SPECS_TEST_HEADER()
    {
        const std::string csv = tempName(".csv");
        {
            TraceTable table(csv, "");
            const uint32_t a = table.signal_index("probe");
            const uint32_t b = table.signal_index("other");
            const uint32_t wl1560 = sig(0, 0, 1.56e-6).m_wavelength_id;

            // time step 0: a at 1560 nm, then 1550 nm; a PROBE latching 1550 nm drops its 1560 nm row
            // (PD5); b keeps its own 1560 nm row
            table.append(0, a, sig(2, 0, 1.56e-6));
            table.append(0, b, sig(0, 1, 1.56e-6));
            table.append(0, a, sig(1, 0, 1.55e-6));
            table.discard(0, a, wl1560);
            // next time step: discarding a past time step, or a row that does not exist, does nothing
            table.append(1e-13, a, sig(3, 0, 1.56e-6));
            table.discard(0, a, wl1560);
            table.discard(1e-13, b, wl1560);
            EXPECT_TRUE(table.close());
            EXPECT_EQ(table.rows(), 3u);
        }
        EXPECT_EQ(readFile(csv),
                  "time_s,signal,wavelength_m,power_W,field_re,field_im\n"
                  "0,other,1.56e-06,1,0,1\n"
                  "0,probe,1.55e-06,1,1,0\n"
                  "1e-13,probe,1.56e-06,9,3,0\n");
        std::remove(csv.c_str());
    }
    SPECS_TEST_FOOTER();
}

TEST(TraceTable, NpzIsAStoredZipOfNpyArrays)
{
    SPECS_TEST_HEADER()
    {
        const std::string npz = tempName(".npz");
        {
            TraceTable table("", npz);
            const uint32_t a = table.signal_index("A");
            const uint32_t b = table.signal_index("PROBE{ROOT/b}");
            table.append(0, b, sig(1, 2, 1.55e-6));
            table.append(2e-13, a, sig(3, 4, 1.31e-6));
            EXPECT_TRUE(table.close());
        }
        const std::string z = readFile(npz);

        const std::vector<std::string> names = { "time_s.npy", "signal.npy", "wavelength_m.npy", "power_W.npy",
                                                 "field_re.npy", "field_im.npy", "signal_names.npy" };
        std::vector<std::string> data;
        size_t pos = 0;
        for (const auto &name : names)
        {
            ASSERT_EQ(u32(z, pos), 0x04034b50u) << name;
            EXPECT_EQ(u16(z, pos + 8), 0) << "stored";
            EXPECT_EQ(u16(z, pos + 12), 0x21) << "1980-01-01";
            const uint32_t crc = u32(z, pos + 14);
            const uint32_t size = u32(z, pos + 18);
            EXPECT_EQ(u32(z, pos + 22), size);
            const uint16_t name_len = u16(z, pos + 26);
            EXPECT_EQ(z.substr(pos + 30, name_len), name);
            const std::string member = z.substr(pos + 30 + name_len + u16(z, pos + 28), size);
            EXPECT_EQ(TraceTable::crc32(member.data(), member.size()), crc) << name;
            EXPECT_EQ(member.substr(0, 6), "\x93NUMPY");
            data.push_back(member.substr(10 + u16(member, 8)));
            pos += 30 + name_len + size;
        }
        // central directory, then the end of central directory record: 7 entries
        EXPECT_EQ(u32(z, pos), 0x02014b50u);
        const size_t eocd = z.size() - 22;
        ASSERT_EQ(u32(z, eocd), 0x06054b50u);
        EXPECT_EQ(u16(z, eocd + 10), names.size());
        EXPECT_EQ(u32(z, eocd + 16), pos);

        double d[2];
        std::memcpy(d, data[0].data(), sizeof(d));
        EXPECT_EQ(d[0], 0);
        EXPECT_EQ(d[1], 2e-13);
        int32_t s[2];
        std::memcpy(s, data[1].data(), sizeof(s));
        EXPECT_EQ(s[0], 1);
        EXPECT_EQ(s[1], 0);
        std::memcpy(d, data[3].data(), sizeof(d));
        EXPECT_EQ(d[0], 5);
        EXPECT_EQ(d[1], 25);
        // signal_names: <U13, UTF-32, zero padded
        ASSERT_EQ(data[6].size(), 2 * 13 * 4u);
        EXPECT_EQ(data[6][0], 'A');
        EXPECT_EQ(data[6][4], '\0');
        EXPECT_EQ(data[6][13 * 4], 'P');
        std::remove(npz.c_str());
    }
    SPECS_TEST_FOOTER();
}

// The probes write their samples to the table also without a trace file (--format csv): a PROBE every
// sample, an MLPROBE the samples of its listed wavelengths (it used to report every sample as an
// unsupported wavelength when it had no VCD signals)
TEST(TraceTable, ProbesWriteToTheTableWithoutVcd)
{
    SPECS_TEST_HEADER()
    {
        specsGlobalConfig.analysis_type = SPECSConfig::TRAN;
        specsGlobalConfig.engine_timescale = SPECSConfig::ONE_FS;
        specsGlobalConfig.applyEngineResolution();
        const std::string csv = tempName(".csv");
        {
            TraceTable table(csv, "");
            spx::oa_signal_type net{"net"};
            Probe probe("probe");
            probe.p_in(net);
            MLambdaProbe mlprobe("mlprobe", { 1.55e-6 });
            mlprobe.p_in(net);
            probe.setTraceTable(&table);
            mlprobe.setTraceTable(&table);

            sc_start(); // initialise the threads
            net.write(sig(1, 0, 1.55e-6));
            sc_start(1, SC_PS);
            net.write(sig(0, 2, 1.31e-6)); // not listed by the MLPROBE
            sc_start(1, SC_PS);
            EXPECT_TRUE(table.close());
            EXPECT_EQ(table.rows(), 3u);
        }
        EXPECT_EQ(readFile(csv),
                  "time_s,signal,wavelength_m,power_W,field_re,field_im\n"
                  "0,probe,1.55e-06,1,1,0\n"
                  "0,mlprobe,1.55e-06,1,1,0\n"
                  "1e-12,probe,1.31e-06,4,0,2\n");
        std::remove(csv.c_str());
    }
    SPECS_TEST_FOOTER();
}
