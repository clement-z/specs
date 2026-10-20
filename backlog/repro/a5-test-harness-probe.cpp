#include <gtest/gtest.h>
#include <systemc.h>
#include "specs.h"
#include "test/utils.h"

// 1. ASSERT_* failure in the child: returns from TestBody before the footer's exit()
TEST(Probe, AssertEscapes)
{
    SPECS_TEST_HEADER()
    {
        std::cout << "child " << getpid() << " parent " << getppid() << " in AssertEscapes before ASSERT" << std::endl;
        ASSERT_TRUE(false) << "deliberate";
        std::cout << "not reached" << std::endl;
    }
    SPECS_TEST_FOOTER();
}

// 2. an ordinary passing test that runs after the escaping one
TEST(Probe, ZAfter)
{
    SPECS_TEST_HEADER()
    {
        std::cout << "child " << getpid() << " parent " << getppid() << " runs ZAfter" << std::endl;
        EXPECT_TRUE(true);
    }
    SPECS_TEST_FOOTER();
}

// 3. >64 KiB of child stdout: parent waitpid()s before draining the pipe
TEST(Probe, BigOutput)
{
    SPECS_TEST_HEADER()
    {
        for (int i = 0; i < 4000; ++i)
            std::cout << "0123456789012345678901234567890123456789012345678901234567890123456789" << std::endl;
    }
    SPECS_TEST_FOOTER();
}

// 4. non-std::exception thrown in the child
TEST(Probe, ThrowInt)
{
    SPECS_TEST_HEADER()
    {
        throw 42;
    }
    SPECS_TEST_FOOTER();
}
