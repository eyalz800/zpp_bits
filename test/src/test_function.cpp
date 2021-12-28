#include "test.h"

namespace test_function
{
using namespace std::literals;

int test_function_normal(std::string s, int i)
{
    EXPECT_EQ(s, "hello"s);
    EXPECT_EQ(i, 1337);
    return 1338;
}

void test_function_void(std::string s, int i)
{
    EXPECT_EQ(s, "hello"s);
    EXPECT_EQ(i, 1337);
}

int test_function_no_parameters()
{
    return 1338;
}

void test_function_void_no_parameters()
{
}

std::unique_ptr<int> test_function_return_move(std::string s, int i)
{
    EXPECT_EQ(s, "hello"s);
    EXPECT_EQ(i, 1337);
    return std::make_unique<int>(1338);
}

int test_function_move_only(std::unique_ptr<int> p)
{
    EXPECT_EQ(*p, 1337);
    return 1338;
}

struct a
{
    int test_member_function(std::string s, int i)
    {
        test_function_normal(std::move(s), i);
        return 1339;
    }

    void test_member_function_void(std::string s, int i)
    {
        return test_function_void(std::move(s), i);
    }

    int test_member_move_only(std::unique_ptr<int> p)
    {
        test_function_move_only(std::move(p));
        return 1339;
    }
};

TEST(test_function, normal_function)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out("hello"s, 1337).or_throw();

    EXPECT_EQ((zpp::bits::apply(test_function_normal, in).or_throw()),
              1338);
}

TEST(test_function, normal_function_ret)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out("hello"s, 1337).or_throw();

    if (auto result = zpp::bits::apply(test_function_normal, in);
        failure(result)) {
        FAIL();
    } else {
        EXPECT_EQ(result.value(), 1338);
    }
}

TEST(test_function, function_return_move)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out("hello"s, 1337).or_throw();

    EXPECT_EQ(
        *(zpp::bits::apply(test_function_return_move, in).or_throw()),
        1338);
}

TEST(test_function, function_void)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out("hello"s, 1337).or_throw();

    zpp::bits::apply(test_function_return_move, in).or_throw();

    out("hello"s, 1337).or_throw();
    int j = 0;
    zpp::bits::apply([&](std::string, int){ j = 1; }, in).or_throw();
    EXPECT_EQ(j, 1);
}

TEST(test_function, function_no_parameters)
{
    auto [data, in] = zpp::bits::data_in();
    EXPECT_EQ(zpp::bits::apply(test_function_no_parameters, in), 1338);
}

TEST(test_function, function_void_no_parameters)
{
    auto [data, in] = zpp::bits::data_in();
    zpp::bits::apply(test_function_void_no_parameters, in);

    int i = 0;
    zpp::bits::apply([&] { i = 1; }, in);

    EXPECT_EQ(i, 1);
}

TEST(test_function, member_function)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out("hello"s, 1337).or_throw();

    a a1;
    EXPECT_EQ(
        (zpp::bits::apply(a1, &a::test_member_function, in).or_throw()),
        1339);
}

TEST(test_function, move_only)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(1337).or_throw();

    EXPECT_EQ(
        (zpp::bits::apply(test_function_move_only, in).or_throw()),
        1338);
}

TEST(test_function, member_move_only)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(1337).or_throw();

    a a1;
    EXPECT_EQ(
        (zpp::bits::apply(a1, &a::test_member_move_only, in).or_throw()),
        1339);
}

TEST(test_function, member_function_void)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out("hello"s, 1337).or_throw();

    a a1;
    zpp::bits::apply(a1, &a::test_member_function_void, in).or_throw();
}

TEST(test_function, lambda)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out("hello"s, 1337).or_throw();

    EXPECT_EQ((zpp::bits::apply(
                   [](std::string s, int i) {
                       EXPECT_EQ(s, "hello"s);
                       EXPECT_EQ(i, 1337);
                       return 1338;
                   },
                   in)
                   .or_throw()),
              1338);
}

TEST(test_function, lambda_with_capture)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out("hello"s, 1337).or_throw();

    int n = 1;
    EXPECT_EQ((zpp::bits::apply(
                   [&](std::string s, int i) {
                       EXPECT_EQ(s, "hello"s);
                       EXPECT_EQ(i, (1336 + n));
                       return 1338;
                   },
                   in)
                   .or_throw()),
              1338);
}

#if __has_include("zpp_throwing.h")
TEST(test_function, throwing_normal_function)
{
    zpp::try_catch([]() -> zpp::throwing<void> {
        auto [data, in, out] = zpp::bits::data_in_out();
        co_await out("hello"s, 1337);

        EXPECT_EQ(
            (co_await zpp::bits::apply(test_function_normal, in)),
            1338);
    },
    []() {
        FAIL();
    });
}

TEST(test_function, throwing_function_return_move)
{
    zpp::try_catch([]() -> zpp::throwing<void> {
        auto [data, in, out] = zpp::bits::data_in_out();
        co_await out("hello"s, 1337);

        EXPECT_EQ(
            *(co_await zpp::bits::apply(test_function_return_move, in)),
            1338);
    }, []() {
        FAIL();
    });
}
#endif

} // namespace test_function
