#include "test.h"
#if __cpp_lib_expected >= 202202L
#include <expected>

namespace test_expected
{

TEST(expected, valid_value)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(std::expected<std::vector<int>, std::string>{
            std::vector{1, 2, 3, 4}})
        .or_throw();

    EXPECT_EQ(encode_hex(data),
              "01"
              "04000000"
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    std::expected<std::vector<int>, std::string> e =
        std::unexpected(std::string("error"));
    in(e).or_throw();

    EXPECT_TRUE(e.has_value());
    EXPECT_EQ(*e, (std::vector{1, 2, 3, 4}));
}

TEST(expected, const_valid_value)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    const std::expected<std::vector<int>, std::string> e{
        std::vector<int>{1, 2, 3, 4}};
    out(e).or_throw();

    EXPECT_EQ(encode_hex(data),
              "01"
              "04000000"
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    std::expected<std::vector<int>, std::string> exp =
        std::unexpected(std::string("error"));
    in(exp).or_throw();

    EXPECT_TRUE(exp.has_value());
    EXPECT_EQ(*exp, (std::vector{1, 2, 3, 4}));
}

TEST(expected, error)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(std::expected<std::vector<int>, std::string>{
            std::unexpected("test error")})
        .or_throw();

    EXPECT_EQ(encode_hex(data),
              "00"
              "0a000000"
              "74657374206572726f72");

    std::expected<std::vector<int>, std::string> e = std::vector{1, 2, 3};
    in(e).or_throw();

    EXPECT_FALSE(e.has_value());
    EXPECT_EQ(e.error(), "test error");
}

TEST(expected, int_value)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(std::expected<int, int>{42}).or_throw();

    EXPECT_EQ(encode_hex(data),
              "01"
              "2a000000");

    std::expected<int, int> e = std::unexpected(0);
    in(e).or_throw();

    EXPECT_TRUE(e.has_value());
    EXPECT_EQ(*e, 42);
}

TEST(expected, int_error)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(std::expected<int, int>{std::unexpected(99)}).or_throw();

    EXPECT_EQ(encode_hex(data),
              "00"
              "63000000");

    std::expected<int, int> e = 42;
    in(e).or_throw();

    EXPECT_FALSE(e.has_value());
    EXPECT_EQ(e.error(), 99);
}

TEST(expected, void_value)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(std::expected<void, std::string>{}).or_throw();

    EXPECT_EQ(encode_hex(data), "01");

    std::expected<void, std::string> e = std::unexpected("error");
    in(e).or_throw();

    EXPECT_TRUE(e.has_value());
}

TEST(expected, void_error)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(std::expected<void, std::string>{std::unexpected("void error")})
        .or_throw();

    EXPECT_EQ(encode_hex(data),
              "00"
              "0a000000"
              "766f6964206572726f72");

    std::expected<void, std::string> e{};
    in(e).or_throw();

    EXPECT_FALSE(e.has_value());
    EXPECT_EQ(e.error(), "void error");
}

TEST(expected, private_default_constructible_value)
{
    struct int_string : std::string
    {
        int_string(int i) : std::string(std::to_string(i))
        {
        }

    private:
        friend zpp::bits::access;
        int_string() = default;
    };
    using expected_type = std::expected<int_string, std::string>;

    auto [data, in, out] = zpp::bits::data_in_out();
    out(expected_type(1234)).or_throw();

    EXPECT_EQ(encode_hex(data),
              "01"
              "04000000"
              "31323334");

    expected_type v = std::unexpected("error");
    in(v).or_throw();

    EXPECT_TRUE(v.has_value());
    EXPECT_EQ(v.value(), "1234");
}

TEST(expected, private_default_constructible_error)
{
    struct int_string : std::string
    {
        int_string(int i) : std::string(std::to_string(i))
        {
        }

    private:
        friend zpp::bits::access;
        int_string() = default;
    };

    using expected_type = std::expected<std::string, int_string>;

    auto [data, in, out] = zpp::bits::data_in_out();
    out(expected_type(std::unexpected(int_string(1234)))).or_throw();

    EXPECT_EQ(encode_hex(data),
              "00"
              "04000000"
              "31323334");

    expected_type v;
    in(v).or_throw();

    EXPECT_FALSE(v.has_value());
    EXPECT_EQ(v.error(), "1234");
}

} // namespace test_expected
#endif // __cpp_lib_expected >= 202202L
