#include "test.h"
#include <unordered_set>

TEST(unordered_set, integer)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(std::unordered_set{1,2,3,4}).or_throw();

    auto count_bytes = encode_hex(data);
    count_bytes.resize(8);
    EXPECT_EQ(count_bytes, "04000000");

    std::unordered_set<int> s;
    in(s).or_throw();

    EXPECT_EQ(s, (std::unordered_set{1,2,3,4}));
}

TEST(unordered_set, const_integer)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    const std::unordered_set<int> o{1,2,3,4};
    out(o).or_throw();

    auto count_bytes = encode_hex(data);
    count_bytes.resize(8);
    EXPECT_EQ(count_bytes, "04000000");

    std::unordered_set<int> s;
    in(s).or_throw();

    EXPECT_EQ(s, (std::unordered_set{1,2,3,4}));
}

TEST(unordered_set, string)
{
    using namespace std::string_literals;
    auto [data, in, out] = zpp::bits::data_in_out();
    out(std::unordered_set{"1"s,"2"s,"3"s,"4"s}).or_throw();

    auto count_bytes = encode_hex(data);
    count_bytes.resize(8);
    EXPECT_EQ(count_bytes, "04000000");

    std::unordered_set<std::string> s;
    in(s).or_throw();

    EXPECT_EQ(s, (std::unordered_set{"1"s,"2"s,"3"s,"4"s}));
}

TEST(unordered_set, const_string)
{
    using namespace std::string_literals;
    auto [data, in, out] = zpp::bits::data_in_out();
    const std::unordered_set<std::string> o{"1"s,"2"s,"3"s,"4"s};
    out(o).or_throw();

    auto count_bytes = encode_hex(data);
    count_bytes.resize(8);
    EXPECT_EQ(count_bytes, "04000000");

    std::unordered_set<std::string> s;
    in(s).or_throw();

    EXPECT_EQ(s, (std::unordered_set{"1"s,"2"s,"3"s,"4"s}));
}

