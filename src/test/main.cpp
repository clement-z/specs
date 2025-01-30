//
// Created by czrounba on 30.01.2025
//
#include <gtest/gtest.h>

#include "specs.h"

int sc_main(int argc, char* argv[]) {

    testing::InitGoogleTest(&argc, argv);
    int status = RUN_ALL_TESTS();

    return status;
}

TEST(A, B) {
    EXPECT_EQ(1, 1);
}
