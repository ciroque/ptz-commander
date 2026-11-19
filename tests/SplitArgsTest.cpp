#include <gtest/gtest.h>

#include "Arguments.h"

TEST(SplitArgs, EmptyAndWhitespaceYieldNoTokens) {
    EXPECT_TRUE(commands::splitArgs("").empty());
    EXPECT_TRUE(commands::splitArgs(" \t  ").empty());
}

TEST(SplitArgs, SplitsOnWhitespaceAndKeepsOrder) {
    const auto tokens = commands::splitArgs("  camera   move\t*  10.5");
    ASSERT_EQ(tokens.size(), 4u);
    EXPECT_EQ(tokens[0], "camera");
    EXPECT_EQ(tokens[1], "move");
    EXPECT_EQ(tokens[2], "*");
    EXPECT_EQ(tokens[3], "10.5");
}
