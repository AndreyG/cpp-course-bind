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

        long cf(short delta) const
        {
            return field + delta;
        }
    };

    TEST(member_function, simple)
    {
        X x { 10 };
        auto f1 = bind(&X::f, x, _1);
        EXPECT_EQ(f1(20), 30);
        //std::as_const(f1)(20);
        const auto cf1 = bind(&X::cf, x, _1);
        EXPECT_EQ(cf1(20), 30);
        cf1(20);
        auto f2 = bind(&X::f, &x, _1);
        EXPECT_EQ(f2(20), 30);
        x.field = 15;
        EXPECT_EQ(f1(20), 30);
        EXPECT_EQ(f2(20), 35);
    }

    TEST(member_function, smart_this)
    {
        auto x1 = std::make_unique<X>(X { 10 });
        auto f1 = ::bind(&X::f, std::move(x1), _1);
        EXPECT_EQ(f1(20), 30);
        auto x2 = std::make_shared<X>(X { 10 });
        auto f2 = ::bind(&X::f, x2, _1);
        EXPECT_EQ(f2(20), 30);
        x2->field = 15;
        EXPECT_EQ(f2(20), 35);
    }

    TEST(member_function, reference_wrapper_this)
    {
        X x { 10 };
        auto f = ::bind(&X::f, std::ref(x), _1);
        EXPECT_EQ(f(20), 30);
        x.field = 15;
        EXPECT_EQ(f(20), 35);

        auto cf = ::bind(&X::cf, std::cref(x), _1);
        EXPECT_EQ(cf(20), 35);
        x.field = 5;
        EXPECT_EQ(cf(20), 25);
    }
}

int main(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv);
    RUN_ALL_TESTS();
}