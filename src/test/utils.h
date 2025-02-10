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
EXPECT_PRED_FORMAT2(angleIsEqual, x, y);

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

static_assert(false, "Work in progress: implement angleIsEqual (it should validate angle modulo 2pi");

template<typename T1, typename T2>
bool angleIsEqual(T1 v1, T2 v2)
{
    return std::arg(v1 - v2) < std::numeric_limits<T1>::epsilon();
}

template<typename FloatT>
requires is_floating_point_v<FloatT>
bool angleIsEqual(std::complex<FloatT> v1, std::complex<FloatT> v2)
{
    return std::abs(v1 - v2) < std::numeric_limits<FloatT>::epsilon();
}

template<typename FloatT, typename T>
requires is_floating_point_v<FloatT> && is_arithmetic_v<T>
bool angleIsEqual(std::complex<FloatT> v, T angle)
{
    return std::fmod(std::arg(v) - angle, 2*std::numbers::pi) < std::numeric_limits<FloatT>::epsilon();
}
template<typename FloatT, typename T>
requires is_floating_point_v<FloatT> && is_arithmetic_v<T>
bool angleIsEqual(T angle, std::complex<FloatT> v)
{
    return angleIsEqual(v, angle);
}