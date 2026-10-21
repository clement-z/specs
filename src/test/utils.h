#pragma once

#include <gtest/gtest.h>
#include <string>
#include <iostream>
#include <complex>
#include <sys/wait.h>
#include <unistd.h>

#include "optical_output_port.h"
#include "specs.h"

#define EXPECT_CPLX_EQ(a,b) \
EXPECT_FLOAT_EQ(a.real(), b.real()); \
EXPECT_FLOAT_EQ(a.imag(), b.imag());

#define ASSERT_CPLX_EQ(a,b) \
ASSERT_FLOAT_EQ(a.real(), b.real()); \
ASSERT_FLOAT_EQ(a.imag(), b.imag());

#define EXPECT_ANGLE_EQ(x, y) \
EXPECT_PRED_FORMAT2(::testing::CmpHelperAngleEQ, x, y);

#define ASSERT_ANGLE_EQ(field, phi) \
ASSERT_FLOAT_EQ(field.phase(), std::arg(std::polar<double>(1.0f, phi)));

#define SPECS_TEST_HEADER() \
    int pipe_stdout[2];\
    int pipe_stderr[2];\
    pipe(pipe_stdout); \
    pipe(pipe_stderr);\
    \
    /*std::cout << "Forking..." << std::endl; */\
    int pid = fork();\
    char buf;\
    ASSERT_NE(pid, -1);\
    if (pid > 0) {\
        int status;\
        ::waitpid(pid, &status, 0);\
        close(pipe_stdout[1]); \
        std::string_view prefix = "> ";\
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
        /* std::cout << "Child process return with status: " << WEXITSTATUS(status) << std::endl; */\
    int test_exit_status = WEXITSTATUS(status);\
        if(WIFEXITED(status) != 1 || test_exit_status != 0)\
            FAIL() << "Test case exited abnormally";\
        if(WEXITSTATUS(status) != 0)\
            FAIL() << "Test case exited with non-zero status: " << WEXITSTATUS(status);\
        return;\
    } else {\
        EXPECT_NE(dup2(pipe_stdout[1], STDOUT_FILENO), -1);\
        close(pipe_stdout[0]);\
        close(pipe_stdout[1]);\
        close(pipe_stderr[0]);\
        close(pipe_stderr[1]);\
        try

#define SPECS_TEST_FOOTER() \
        catch (std::exception &e) {\
            ADD_FAILURE() << "Exception caught while running test: " << e.what() << std::endl; \
        }\
    }\
    std::cout << std::flush;\
    exit(testing::Test::HasFailure());


using std::numbers::pi;

namespace testing {
template<typename FloatT>
requires is_floating_point_v<FloatT>
AssertionResult CmpHelperAngleEQ(const char* lhs_expression,
                                 const char* rhs_expression,
                                 std::complex<FloatT> lhs_value,
                                 std::complex<FloatT> rhs_value)
{
    constexpr FloatT kEpsilon = static_cast<FloatT>(1e-8);  // Tolerance for angle comparison

    // Check for zero magnitude to avoid undefined angle
    if (std::abs(lhs_value) < kEpsilon) {
        return AssertionFailure()
               << "Left-hand side value in " << lhs_expression
               << " is near zero and has an undefined angle.";
    }
    if (std::abs(rhs_value) < kEpsilon) {
        return AssertionFailure()
               << "Right-hand side value in " << rhs_expression
               << " is near zero and has an undefined angle.";
    }

    FloatT lhs_angle = std::arg(lhs_value);
    FloatT rhs_angle = std::arg(rhs_value);

    // Normalize difference between -π and π
    FloatT delta = std::fmod(lhs_angle - rhs_angle + static_cast<FloatT>(M_PI),
                             static_cast<FloatT>(2.0 * M_PI));
    if (delta < 0)
        delta += static_cast<FloatT>(2.0 * M_PI);
    delta -= static_cast<FloatT>(M_PI);

    if (std::abs(delta) <= kEpsilon) {
        return AssertionSuccess();
    } else {
        return AssertionFailure()
               << "The angles of the complex numbers differ more than epsilon (" << kEpsilon << ").\n"
               << "  " << lhs_expression << " = " << lhs_value << ", angle = " << lhs_angle << "\n"
               << "  " << rhs_expression << " = " << rhs_value << ", angle = " << rhs_angle << "\n"
               << "  Difference in angle = " << delta;
    }
}

template<typename FloatT, typename T>
requires std::is_floating_point_v<FloatT> && std::is_arithmetic_v<T>
AssertionResult CmpHelperAngleEQ(const char* lhs_expression,
                                 const char* rhs_expression,
                                 std::complex<FloatT> lhs_value,
                                 T rhs_angle)
{
    auto rhs_value = std::polar<FloatT>(1.0, rhs_angle); // Unit magnitude
    return CmpHelperAngleEQ(lhs_expression, rhs_expression, lhs_value, rhs_value);
}

template<typename FloatT, typename T>
requires std::is_floating_point_v<FloatT> && std::is_arithmetic_v<T>
AssertionResult CmpHelperAngleEQ(const char* lhs_expression,
                                 const char* rhs_expression,
                                 T lhs_angle,
                                 std::complex<FloatT> rhs_value)
{
    auto lhs_value = std::polar<FloatT>(1.0, lhs_angle); // Unit magnitude
    return CmpHelperAngleEQ(lhs_expression, rhs_expression, lhs_value, rhs_value);
}

template<typename T>
requires std::is_arithmetic_v<T>
AssertionResult CmpHelperAngleEQ(const char* lhs_expression,
                                 const char* rhs_expression,
                                 T lhs_angle,
                                 OpticalSignal rhs_value)
{
    using FloatT = OpticalSignal::field_type::value_type;
    auto lhs_value = std::polar<FloatT>(1.0, lhs_angle); // Unit magnitude
    return CmpHelperAngleEQ(lhs_expression, rhs_expression, lhs_value, rhs_value.m_field);
}

template<typename T>
requires std::is_arithmetic_v<T>
AssertionResult CmpHelperAngleEQ(const char* lhs_expression,
                                 const char* rhs_expression,
                                 OpticalSignal lhs_value,
                                 T rhs_angle)
{
    using FloatT = OpticalSignal::field_type::value_type;
    auto rhs_value = std::polar<FloatT>(1.0, rhs_angle); // Unit magnitude
    return CmpHelperAngleEQ(lhs_expression, rhs_expression, lhs_value.m_field, rhs_value);
}

inline
AssertionResult CmpHelperAngleEQ(const char* lhs_expression,
                                 const char* rhs_expression,
                                 OpticalSignal lhs_value,
                                 OpticalSignal rhs_value)
{
    return CmpHelperAngleEQ(lhs_expression, rhs_expression, lhs_value.m_field, rhs_value.m_field);
}

template<typename TL, typename TR>
requires std::is_arithmetic_v<TL> || std::is_arithmetic_v<TR>
AssertionResult CmpHelperAngleEQ(const char* lhs_expression,
                                          const char* rhs_expression,
                                          TL lhs_angle,
                                          TR rhs_angle)
{
    auto lhs_value = std::polar(1.0, static_cast<double>(lhs_angle)); // Unit magnitude
    auto rhs_value = std::polar(1.0, static_cast<double>(rhs_angle)); // Unit magnitude
    return CmpHelperAngleEQ(lhs_expression, rhs_expression, lhs_value, rhs_value);
}

}