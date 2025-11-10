//
// Created by czrounba on 2/2/25.
//

#include <gtest/gtest.h>
#include <systemc.h>
#include <sys/wait.h>

#include "devices/waveguide.h"
#include "optical_output_port.h"
#include "specs.h"

#include "test/utils.h"

#if 0

#define SPECS_TEST_HEADER() \
int pipe_stdout[2];\
int pipe_stderr[2];\
pipe(pipe_stdout); \
pipe(pipe_stderr);\
\
std::cout << "Forking..." << std::endl;\
int pid = fork();\
char buf;\
ASSERT_NE(pid, -1);\
if (pid > 0) {\
    int status;\
    ::waitpid(pid, &status, 0);\
    close(pipe_stdout[1]); \
    std::string_view prefix = "[child process] ";\
    std::cout << prefix << std::flush;\
    while (read(pipe_stdout[0], &buf, 1) > 0) \
    {\
        write(STDOUT_FILENO, &buf, 1);\
        if (buf == '\n')\
            std::cout << prefix << std::flush;\
    }\
    std::cout << std::endl;\
\
    close(pipe_stdout[0]); \
    close(pipe_stderr[0]); \
\
    std::cout << "Child process return with status: " << WEXITSTATUS(status) << std::endl;\
    ASSERT_EQ(WIFEXITED(status), 1);\
    EXPECT_EQ(WEXITSTATUS(status), 0);\
    std::cout << "Test complete" << std::endl;\
    return;\
}\
else {\
i_am_parent = false;\
EXPECT_NE(dup2(pipe_stdout[1], STDOUT_FILENO), -1);\
close(pipe_stdout[0]);\
close(pipe_stdout[1]);\
close(pipe_stderr[0]);\
close(pipe_stderr[1]);\
try

#define SPECS_TEST_FOOTER() \
catch (std::exception &e) {\
std::cerr << "Test threw an exception: " << e.what() << std::endl;\
std::cout << std::flush;\
std::cerr << std::flush;\
exit(-1);  \
}\
}\
std::cout << std::flush;\
std::cerr << std::flush;\
exit(testing::Test::HasFailure());


#define SPECS_TEST(test_suite_name, test_name) \
TEST_F(SpecsTest, test_suite_name##_##test_name)\
{ try { TestBody_body(); } catch (std::exception &e) { std::cerr << e.what() << std::endl; exit(1); } exit(0); }\
void GTEST_TEST_CLASS_NAME_(SpecsTest, test_suite_name##_##test_name)::TestBody_body()


#endif

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
        specsGlobalConfig.trace_filename = "/tmp/specs_test2";
        specsGlobalConfig.engine_timescale = SPECSConfig::ONE_FS;
        specsGlobalConfig.applyEngineResolution();

        WaveguideTest tb;

        EXPECT_CPLX_EQ(tb.a_in.read().m_field, std::complex(0,0));
        EXPECT_CPLX_EQ(tb.b_in.read().m_field, std::complex(0,0));
        EXPECT_CPLX_EQ(tb.a_out.read().m_field, std::complex(0,0));
        EXPECT_CPLX_EQ(tb.a_out.read().m_field, std::complex(0,0));
    }
    SPECS_TEST_FOOTER();
}

TEST(Waveguide, OperatingPoint)
{
    SPECS_TEST_HEADER()
    {
        specsGlobalConfig.analysis_type = SPECSConfig::OP;
        specsGlobalConfig.trace_all_optical_nets = false;
        specsGlobalConfig.trace_filename = "/tmp/specs_test2";
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
