#include "test.h"
#include <set>

TEST(set, integer)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(std::set{1,2,3,4}).or_throw();

    auto count_bytes = encode_hex(data);
    count_bytes.resize(8);
    EXPECT_EQ(count_bytes, "04000000");

    std::set<int> s;
    in(s).or_throw();

    EXPECT_EQ(s, (std::set{1,2,3,4}));
}

TEST(set, const_integer)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    const std::set<int> o{1,2,3,4};
    out(o).or_throw();

    auto count_bytes = encode_hex(data);
    count_bytes.resize(8);
    EXPECT_EQ(count_bytes, "04000000");

    std::set<int> s;
    in(s).or_throw();

    EXPECT_EQ(s, (std::set{1,2,3,4}));
}

TEST(set, string)
{
    using namespace std::string_literals;
    auto [data, in, out] = zpp::bits::data_in_out();
    out(std::set{"1"s,"2"s,"3"s,"4"s}).or_throw();

    auto count_bytes = encode_hex(data);
    count_bytes.resize(8);
    EXPECT_EQ(count_bytes, "04000000");

    std::set<std::string> s;
    in(s).or_throw();

    EXPECT_EQ(s, (std::set{"1"s,"2"s,"3"s,"4"s}));
}

TEST(set, const_string)
{
    using namespace std::string_literals;
    auto [data, in, out] = zpp::bits::data_in_out();
    const std::set<std::string> o{"1"s,"2"s,"3"s,"4"s};
    out(o).or_throw();

    auto count_bytes = encode_hex(data);
    count_bytes.resize(8);
    EXPECT_EQ(count_bytes, "04000000");

    std::set<std::string> s;
    in(s).or_throw();

    EXPECT_EQ(s, (std::set{"1"s,"2"s,"3"s,"4"s}));
}
