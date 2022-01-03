#include "test.h"

namespace test_optional
{

TEST(optional, valid)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(std::optional{std::vector{1,2,3,4}}).or_throw();

    EXPECT_EQ(encode_hex(data),
              "01"
              "04000000"
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    std::optional<std::vector<int>> o;
    in(o).or_throw();

    EXPECT_EQ(*o, (std::vector{1,2,3,4}));
}

TEST(optional, const_valid)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    const std::optional o{std::vector<int>{1,2,3,4}};
    out(o).or_throw();

    EXPECT_EQ(encode_hex(data),
              "01"
              "04000000"
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    std::optional<std::vector<int>> opt;
    in(opt).or_throw();

    EXPECT_EQ(*opt, (std::vector{1,2,3,4}));
}

TEST(optional, nullopt)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(std::optional<std::vector<int>>{}).or_throw();

    EXPECT_EQ(encode_hex(data),
              "00");

    std::optional<std::vector<int>> o = std::vector{1, 2, 3};
    in(o).or_throw();

    EXPECT_FALSE(o.has_value());
}

} // namespace test_optional
