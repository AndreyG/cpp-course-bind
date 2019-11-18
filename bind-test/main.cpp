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

    struct Mapper
    {
        std::string&       operator()(int i, int j)       { return mapping_[{i, j}]; }
        std::string const& operator()(int i, int j) const { return mapping_.at({ i, j }); }

    private:
        std::map<std::pair<int, int>, std::string> mapping_;
    };

    TEST(mapper, simple)
    {
        auto binder = bind(Mapper(), 10, _1);
        binder(20) = "aba";
        EXPECT_EQ(std::as_const(binder)(20), "aba");
    }

    TEST(mapper, ref)
    {
        auto mapper = Mapper();
        auto binder = bind(mapper, 10, _1);
        binder(20) = "aba";
        ASSERT_THROW(std::as_const(mapper)(10, 20), std::out_of_range);
    }

    TEST(mapper, reference_wrapper)
    {
        auto mapper = Mapper();
        auto binder = ::bind(std::ref(mapper), 10, _1);
        binder(20) = "aba";
        EXPECT_EQ(std::as_const(mapper)(10, 20), "aba");
    }

    struct X
    {
        int field;

        long f(short delta)
        {
            return field + delta;
        }
    };

    TEST(member_function, simple)
    {
        X x { 10 };
        auto f = bind(&X::f, x, _1);
        EXPECT_EQ(f(20), 30);
    }
}

int main(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv);
    RUN_ALL_TESTS();
}