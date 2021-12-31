#include "test.h"

namespace test_span
{

TEST(span, integer)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    std::vector o{1,2,3,4};
    out(std::span{o}).or_throw();

    EXPECT_EQ(encode_hex(data),
              "04000000"
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    std::vector<int> v(4);
    std::span s(v);
    in(s).or_throw();

    EXPECT_EQ(v, (std::vector{1,2,3,4}));
}

TEST(span, const_integer)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    const std::vector<int> v{1,2,3,4};
    const std::span o{v};
    out(o).or_throw();

    EXPECT_EQ(encode_hex(data),
              "04000000"
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    std::vector<int> v1(4);
    in(std::span{v1}).or_throw();

    EXPECT_EQ(v1, (std::vector{1,2,3,4}));
}

TEST(span, string)
{
    using namespace std::string_literals;
    auto [data, in, out] = zpp::bits::data_in_out();
    std::vector o{"1"s,"2"s,"3"s,"4"s};
    out(std::span{o}).or_throw();

    EXPECT_EQ(encode_hex(data),
              "04000000"
              "01000000"
              "31"
              "01000000"
              "32"
              "01000000"
              "33"
              "01000000"
              "34");

    std::vector<std::string> v(4);
    in(std::span(v)).or_throw();

    EXPECT_EQ(v, (std::vector{"1"s,"2"s,"3"s,"4"s}));
}

TEST(span, string_input_out_of_range)
{
    using namespace std::string_literals;
    auto [data, in, out] = zpp::bits::data_in_out();
    std::vector o{"1"s,"2"s,"3"s,"4"s};
    out(std::span{o}).or_throw();

    EXPECT_EQ(encode_hex(data),
              "04000000"
              "01000000"
              "31"
              "01000000"
              "32"
              "01000000"
              "33"
              "01000000"
              "34");

    std::vector<std::string> v;
#ifdef __cpp_exceptions
    EXPECT_THROW(in(std::span(v)).or_throw(), std::system_error);
#else
    EXPECT_EQ(in(std::span(v)), std::errc::value_too_large);
#endif

    v.resize(3);
    in.reset();
#ifdef __cpp_exceptions
    EXPECT_THROW(in(std::span(v)).or_throw(), std::system_error);
#else
    EXPECT_EQ(in(std::span(v)), std::errc::value_too_large);
#endif

    v.resize(4);
    in.reset();
    in(std::span(v)).or_throw();

    EXPECT_EQ(v, (std::vector{"1"s,"2"s,"3"s,"4"s}));
}

TEST(span, const_string)
{
    using namespace std::string_literals;
    auto [data, in, out] = zpp::bits::data_in_out();
    const std::vector<std::string> o{"1"s, "2"s, "3"s, "4"s};
    out(std::span{o}).or_throw();

    EXPECT_EQ(encode_hex(data),
              "04000000"
              "01000000"
              "31"
              "01000000"
              "32"
              "01000000"
              "33"
              "01000000"
              "34");

    std::vector<std::string> v(4);
    in(std::span(v)).or_throw();

    EXPECT_EQ(v, (std::vector{"1"s,"2"s,"3"s,"4"s}));
}

} // namespace test_span
