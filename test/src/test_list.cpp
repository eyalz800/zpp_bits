#include "test.h"

#include <list>

namespace test_list
{

TEST(list, integer)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(std::list{1,2,3,4}).or_throw();

    EXPECT_EQ(encode_hex(data),
              "04000000"
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    std::list<int> v;
    in(v).or_throw();

    EXPECT_EQ(v, (std::list{1,2,3,4}));
}

TEST(list, const_integer)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    const std::list<int> o{1,2,3,4};
    out(o).or_throw();

    EXPECT_EQ(encode_hex(data),
              "04000000"
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    std::list<int> v;
    in(v).or_throw();

    EXPECT_EQ(v, (std::list{1,2,3,4}));
}

TEST(list, string)
{
    using namespace std::string_literals;
    auto [data, in, out] = zpp::bits::data_in_out();
    out(std::list{"1"s,"2"s,"3"s,"4"s}).or_throw();

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

    std::list<std::string> v;
    in(v).or_throw();

    EXPECT_EQ(v, (std::list{"1"s,"2"s,"3"s,"4"s}));
}

TEST(list, const_string)
{
    using namespace std::string_literals;
    auto [data, in, out] = zpp::bits::data_in_out();
    const std::list<std::string> o{"1"s, "2"s, "3"s, "4"s};
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

    std::list<std::string> v;
    in(v).or_throw();

    EXPECT_EQ(v, (std::list{"1"s,"2"s,"3"s,"4"s}));
}

TEST(list, sized_1b_integer)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(zpp::bits::sized<std::uint8_t>(std::list{1,2,3,4})).or_throw();

    EXPECT_EQ(encode_hex(data),
              "04"
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    std::list<int> v;
    in(zpp::bits::sized<std::uint8_t>(v)).or_throw();

    EXPECT_EQ(v, (std::list{1,2,3,4}));
}

TEST(list, unsized_integer)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(zpp::bits::unsized(std::list{1,2,3,4})).or_throw();

    EXPECT_EQ(encode_hex(data),
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    std::list<int> v(4);
    in(zpp::bits::unsized(v)).or_throw();

    EXPECT_EQ(v, (std::list{1,2,3,4}));
}

TEST(list, sized_t_1b_integer)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(zpp::bits::sized_t<std::list<int>, std::uint8_t>{1,2,3,4}).or_throw();

    EXPECT_EQ(encode_hex(data),
              "04"
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    zpp::bits::sized_t<std::list<int>, std::uint8_t> v;
    in(v).or_throw();

    EXPECT_EQ(v, (std::list{1,2,3,4}));
}

TEST(list, unsized_t_integer)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(zpp::bits::unsized_t<std::list<int>>{1,2,3,4}).or_throw();

    EXPECT_EQ(encode_hex(data),
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    zpp::bits::unsized_t<std::list<int>> v(4);
    in(v).or_throw();

    EXPECT_EQ(v, (std::list{1,2,3,4}));
}

} // namespace test_list
