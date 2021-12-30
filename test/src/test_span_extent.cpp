#include "test.h"

namespace test_span_extent
{

TEST(span_extent, integer)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    std::vector o{1,2,3,4};
    out(std::span<int, 4>{o}).or_throw();

    EXPECT_EQ(encode_hex(data),
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    std::vector<int> v(4);
    std::span<int, 4> s(v);
    in(s).or_throw();

    EXPECT_EQ(v, (std::vector{1,2,3,4}));
}

TEST(span_extent, const_integer)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    const std::vector<int> v{1,2,3,4};
    const std::span<const int, 4> o{v};
    out(o).or_throw();

    EXPECT_EQ(encode_hex(data),
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    std::vector<int> v1(4);
    in(std::span<int, 4>{v1}).or_throw();

    EXPECT_EQ(v1, (std::vector{1,2,3,4}));
}

TEST(span_extent, string)
{
    using namespace std::string_literals;
    auto [data, in, out] = zpp::bits::data_in_out();
    std::vector o{"1"s,"2"s,"3"s,"4"s};
    out(std::span<std::string, 4>{o}).or_throw();

    EXPECT_EQ(encode_hex(data),
              "01000000"
              "31"
              "01000000"
              "32"
              "01000000"
              "33"
              "01000000"
              "34");

    std::vector<std::string> v(4);
    in(std::span<std::string, 4>(v)).or_throw();

    EXPECT_EQ(v, (std::vector{"1"s,"2"s,"3"s,"4"s}));
}

TEST(span_extent, const_string)
{
    using namespace std::string_literals;
    auto [data, in, out] = zpp::bits::data_in_out();
    const std::vector<std::string> o{"1"s, "2"s, "3"s, "4"s};
    out(std::span<const std::string, 4>{o}).or_throw();

    EXPECT_EQ(encode_hex(data),
              "01000000"
              "31"
              "01000000"
              "32"
              "01000000"
              "33"
              "01000000"
              "34");

    std::vector<std::string> v(4);
    in(std::span<std::string, 4>(v)).or_throw();

    EXPECT_EQ(v, (std::vector{"1"s,"2"s,"3"s,"4"s}));
}

} // namespace test_span_extent
