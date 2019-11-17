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

    TEST(simple, reference)
    {
        int i = 2;
        auto binder = bind(add, 2, i);
        EXPECT_EQ(binder(), 4);
        i = 3;
        EXPECT_EQ(binder(), 4);
    }

    int sub(int x, int y)
    {
        return x - y;
    }

    TEST(simple, placeholders)
    {
        auto add_binder = bind(add, _1, 2);
        EXPECT_EQ(add_binder(3), 5);
        auto sub_binder = bind(sub, _2, _1);
        EXPECT_EQ(sub_binder(3, 5), 2);
    }
}

int main(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv);
    RUN_ALL_TESTS();
}