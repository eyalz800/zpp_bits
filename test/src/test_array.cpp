#include "test.h"

TEST(array, integer)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    int a1[] = {1,2,3,4};
    out(a1).or_throw();

    EXPECT_EQ(encode_hex(data),
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    int a2[4]{};
    in(a2).or_throw();

    for (std::size_t i = 0; i < std::extent_v<decltype(a1)>; ++i) {
        EXPECT_EQ(a1[i], a2[i]);
    }
}

TEST(array, const_integer)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    const int a1[] = {1,2,3,4};
    out(a1).or_throw();

    EXPECT_EQ(encode_hex(data),
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    int a2[4]{};
    in(a2).or_throw();

    for (std::size_t i = 0; i < std::extent_v<decltype(a1)>; ++i) {
        EXPECT_EQ(a1[i], a2[i]);
    }
}

TEST(array, string)
{
    using namespace std::string_literals;
    auto [data, in, out] = zpp::bits::data_in_out();
    std::string a1[] = {"1"s,"2"s,"3"s,"4"s};
    out(a1).or_throw();

    EXPECT_EQ(encode_hex(data),
              "01000000"
              "31"
              "01000000"
              "32"
              "01000000"
              "33"
              "01000000"
              "34");

    std::string a2[4]{};
    in(a2).or_throw();

    for (std::size_t i = 0; i < std::extent_v<decltype(a1)>; ++i) {
        EXPECT_EQ(a1[i], a2[i]);
    }
}

TEST(array, const_string)
{
    using namespace std::string_literals;
    auto [data, in, out] = zpp::bits::data_in_out();
    const std::string a1[] = {"1"s,"2"s,"3"s,"4"s};
    out(a1).or_throw();

    EXPECT_EQ(encode_hex(data),
              "01000000"
              "31"
              "01000000"
              "32"
              "01000000"
              "33"
              "01000000"
              "34");

    std::string a2[4]{};
    in(a2).or_throw();

    for (std::size_t i = 0; i < std::extent_v<decltype(a1)>; ++i) {
        EXPECT_EQ(a1[i], a2[i]);
    }
}
