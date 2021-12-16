#include "test.h"

TEST(variant, int)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(std::variant<int, std::string>(0x1337)).or_throw();

    EXPECT_EQ(hexlify(data),
              "00"
              "37130000");

    std::variant<int, std::string> v;
    in(v).or_throw();

    EXPECT_TRUE(std::holds_alternative<int>(v));
    EXPECT_EQ(std::get<int>(v), 0x1337);
}

TEST(variant, string)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(std::variant<int, std::string>("1234")).or_throw();

    EXPECT_EQ(hexlify(data),
              "01"
              "04000000"
              "31323334");

    std::variant<int, std::string> v;
    in(v).or_throw();

    EXPECT_TRUE(std::holds_alternative<std::string>(v));
    EXPECT_EQ(std::get<std::string>(v), "1234");
}
