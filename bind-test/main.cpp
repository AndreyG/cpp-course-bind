#include "bind.h"
#include <gtest/gtest.h>

namespace 
{
    int add(int x, int y)
    {
        return x + y;
    }

    TEST(simple, without_placeholders)
    {
        auto binder = bind(add, 2, 3);
        EXPECT_EQ(binder(), 5);
        const auto cbinder = bind(add, 3, 4);
        EXPECT_EQ(cbinder(), 7);
    }
}

int main(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv);
    RUN_ALL_TESTS();
}