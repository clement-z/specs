//
// Created by czrounba on 2/2/25.
//

#include <gtest/gtest.h>
#include <systemc.h>

#include "devices/waveguide.h"
#include "optical_output_port.h"
#include "specs.h"

#include "test/utils.h"

class WaveguideTest {
public:
    WaveguideTest()
    {
        // Bidirectional variant
        uut.m_attenuation_dB_cm = 3.0103;
        uut.m_neff = 2.01;
        uut.m_ng = uut.m_neff;
        uut.m_length_cm = 1;

        uut.p0_in(a_in);
        uut.p0_out(a_out);
        uut.p1_in(b_in);
        uut.p1_out(b_out);
    }

    double expected_phase_delay(double wavelength) const
    {
        return std::fmod(2.0f * pi * uut.m_neff * uut.m_length_cm * 1e-2 / wavelength, 2 * pi);
    }

    void reset()
    {
        uut.m_p0_out_writer.reset();
        uut.m_p1_out_writer.reset();
        sc_start(1, SC_SEC);
    }

public:
    WaveguideBi uut{"uut"};
    spx::oa_signal_type a_in{"a_in"};
    spx::oa_signal_type b_in{"b_in"};
    spx::oa_signal_type a_out{"a_out"};
    spx::oa_signal_type b_out{"b_out"};
};

TEST(Waveguide, Instantiation)
{
    SPECS_TEST_HEADER()
    {
        specsGlobalConfig.analysis_type = SPECSConfig::OP;
        specsGlobalConfig.trace_all_optical_nets = false;
        specsGlobalConfig.trace_filename = testing::TempDir() + "specs_test_" + std::to_string(getpid());
        specsGlobalConfig.engine_timescale = SPECSConfig::ONE_FS;
        specsGlobalConfig.applyEngineResolution();

        WaveguideTest tb;

        EXPECT_CPLX_EQ(tb.a_in.read().m_field, std::complex(0,0));
        EXPECT_CPLX_EQ(tb.b_in.read().m_field, std::complex(0,0));
        EXPECT_CPLX_EQ(tb.a_out.read().m_field, std::complex(0,0));
        EXPECT_CPLX_EQ(tb.b_out.read().m_field, std::complex(0,0));
    }
    SPECS_TEST_FOOTER();
}

TEST(Waveguide, OperatingPoint)
{
    SPECS_TEST_HEADER()
    {
        specsGlobalConfig.analysis_type = SPECSConfig::OP;
        specsGlobalConfig.trace_all_optical_nets = false;
        specsGlobalConfig.trace_filename = testing::TempDir() + "specs_test_" + std::to_string(getpid());
        specsGlobalConfig.engine_timescale = SPECSConfig::ONE_FS;
        specsGlobalConfig.applyEngineResolution();

        WaveguideTest tb;

        EXPECT_CPLX_EQ(tb.a_out.read().m_field, std::complex(0,0));
        EXPECT_CPLX_EQ(tb.b_out.read().m_field, std::complex(0,0));

        // Prepare simulation
        specsGlobalConfig.prepareSimulation();
        sc_start(SC_ZERO_TIME); // Run with no signals present to get event baseline

        auto wavelengths = {1.3e-6, 1.451e-6, 1.55e-6, 1.551e-6, 1.57e-6, 1.6e-6};
        for (const auto& wl: wavelengths) {
            // Write signals to UUT
            tb.reset();
            sc_start(SC_ZERO_TIME);
            tb.a_in.write(OpticalSignal(OpticalSignal::field_type{1,0}, wl));

            // Run analysis
            specsGlobalConfig.runOPAnalysis();

            EXPECT_ANGLE_EQ(tb.b_out.read(), tb.expected_phase_delay(wl));
        }
    }
    SPECS_TEST_FOOTER();
}
