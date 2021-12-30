#include "test.h"
#include <map>

TEST(map, integer)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(std::map<int, int>{{1, 1}, {2, 2}, {3, 3}, {4, 4}}).or_throw();

    auto count_bytes = encode_hex(data);
    count_bytes.resize(8);
    EXPECT_EQ(count_bytes, "04000000");

    std::map<int, int> s;
    in(s).or_throw();

    EXPECT_EQ(s, (std::map<int, int>{{1, 1}, {2, 2}, {3, 3}, {4, 4}}));
}

TEST(map, const_integer)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    const std::map<const int, const int> m{{1, 1}, {2, 2}, {3, 3}, {4, 4}};
    out(m).or_throw();

    auto count_bytes = encode_hex(data);
    count_bytes.resize(8);
    EXPECT_EQ(count_bytes, "04000000");

    std::map<int, int> s;
    in(s).or_throw();

    EXPECT_EQ(s, (std::map<int, int>{{1, 1}, {2, 2}, {3, 3}, {4, 4}}));
}

TEST(map, string)
{
    using namespace std::string_literals;
    auto [data, in, out] = zpp::bits::data_in_out();
    out(std::map<std::string, std::string>{
            {"1"s, "1"s}, {"2"s, "2"s}, {"3"s, "3"s}, {"4"s, "4"s}})
        .or_throw();

    auto count_bytes = encode_hex(data);
    count_bytes.resize(8);
    EXPECT_EQ(count_bytes, "04000000");

    std::map<std::string, std::string> s;
    in(s).or_throw();

    EXPECT_EQ(
        s,
        (std::map<std::string, std::string>{
            {"1"s, "1"s}, {"2"s, "2"s}, {"3"s, "3"s}, {"4"s, "4"s}}));
}

