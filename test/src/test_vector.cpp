#include "test.h"

namespace test_vector
{

TEST(vector, integer)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(std::vector{1,2,3,4}).or_throw();

    EXPECT_EQ(encode_hex(data),
              "04000000"
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    std::vector<int> v;
    in(v).or_throw();

    EXPECT_EQ(v, (std::vector{1,2,3,4}));
}

TEST(vector, const_integer)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    const std::vector<int> o{1,2,3,4};
    out(o).or_throw();

    EXPECT_EQ(encode_hex(data),
              "04000000"
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    std::vector<int> v;
    in(v).or_throw();

    EXPECT_EQ(v, (std::vector{1,2,3,4}));
}

TEST(vector, string)
{
    using namespace std::string_literals;
    auto [data, in, out] = zpp::bits::data_in_out();
    out(std::vector{"1"s,"2"s,"3"s,"4"s}).or_throw();

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
    in(v).or_throw();

    EXPECT_EQ(v, (std::vector{"1"s,"2"s,"3"s,"4"s}));
}

TEST(vector, const_string)
{
    using namespace std::string_literals;
    auto [data, in, out] = zpp::bits::data_in_out();
    const std::vector<std::string> o{"1"s, "2"s, "3"s, "4"s};
    out(o).or_throw();

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
    in(v).or_throw();

    EXPECT_EQ(v, (std::vector{"1"s,"2"s,"3"s,"4"s}));
}

TEST(vector, sized_1b_integer)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(zpp::bits::sized<std::uint8_t>(std::vector{1,2,3,4})).or_throw();

    EXPECT_EQ(encode_hex(data),
              "04"
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    std::vector<int> v;
    in(zpp::bits::sized<std::uint8_t>(v)).or_throw();

    EXPECT_EQ(v, (std::vector{1,2,3,4}));
}

TEST(vector, unsized_integer)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(zpp::bits::unsized(std::vector{1,2,3,4})).or_throw();

    EXPECT_EQ(encode_hex(data),
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    std::vector<int> v(4);
    in(zpp::bits::unsized(v)).or_throw();

    EXPECT_EQ(v, (std::vector{1,2,3,4}));
}

TEST(vector, sized_t_1b_integer)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(zpp::bits::sized_t<std::vector<int>, std::uint8_t>{1,2,3,4}).or_throw();

    EXPECT_EQ(encode_hex(data),
              "04"
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    zpp::bits::sized_t<std::vector<int>, std::uint8_t> v;
    in(v).or_throw();

    EXPECT_EQ(v, (std::vector{1,2,3,4}));
}

TEST(vector, unsized_t_integer)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(zpp::bits::unsized_t<std::vector<int>>{1,2,3,4}).or_throw();

    EXPECT_EQ(encode_hex(data),
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    zpp::bits::unsized_t<std::vector<int>> v(4);
    in(v).or_throw();

    EXPECT_EQ(v, (std::vector{1,2,3,4}));
}

TEST(vector, static_vector)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(zpp::bits::static_vector<int>{1,2,3,4}).or_throw();

    EXPECT_EQ(encode_hex(data),
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    zpp::bits::static_vector<int> v(4);
    in(v).or_throw();

    EXPECT_EQ(v, (std::vector{1,2,3,4}));
}

TEST(vector, vector1b)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(zpp::bits::vector1b<int>{1,2,3,4}).or_throw();

    EXPECT_EQ(encode_hex(data),
              "04"
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    zpp::bits::vector1b<int> v(4);
    in(v).or_throw();

    EXPECT_EQ(v, (std::vector{1,2,3,4}));
}

} // namespace test_vector
