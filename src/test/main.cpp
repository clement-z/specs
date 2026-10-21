//
// Created by czrounba on 30.01.2025
//
#include <gtest/gtest.h>

#include "specs.h"
#include "test/utils.h"

int sc_main(int argc, char* argv[]) {

    testing::InitGoogleTest(&argc, argv);
    int status = RUN_ALL_TESTS();

    return status;
}


TEST(A, B) {
    SPECS_TEST_HEADER()
    {
        // Do SPECS stuff independently of other tests
    }
    SPECS_TEST_FOOTER();
}
