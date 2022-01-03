#include "test.h"

namespace test_implicit_optional
{

TEST(implicit_optional, valid)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(zpp::bits::implicit_optional{std::vector{1,2,3,4}}).or_throw();

    EXPECT_EQ(encode_hex(data),
              "01"
              "04000000"
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    zpp::bits::implicit_optional<std::vector<int>> o;
    in(o).or_throw();

    EXPECT_TRUE(o.has_value());
    EXPECT_EQ(o.value(), (std::vector{1,2,3,4}));
}

TEST(implicit_optional, const_valid)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    const zpp::bits::implicit_optional o{std::vector<int>{1,2,3,4}};
    out(o).or_throw();

    EXPECT_EQ(encode_hex(data),
              "01"
              "04000000"
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    zpp::bits::implicit_optional<std::vector<int>> ov;
    in(ov).or_throw();

    EXPECT_TRUE(o.has_value());
    EXPECT_EQ(o.value(), (std::vector{1,2,3,4}));
}

TEST(implicit_optional, nullopt)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(zpp::bits::implicit_optional{std::vector<int>{}}).or_throw();

    EXPECT_EQ(encode_hex(data),
              "00");

    zpp::bits::implicit_optional o = std::vector{1, 2, 3};
    in(o).or_throw();

    EXPECT_TRUE(!o.has_value());
}

TEST(implicit_optional_ref, valid)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(zpp::bits::implicit_optional_ref(std::vector{1,2,3,4})).or_throw();

    EXPECT_EQ(encode_hex(data),
              "01"
              "04000000"
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    std::vector<int> o;
    in(zpp::bits::implicit_optional_ref(o)).or_throw();

    EXPECT_EQ(o, (std::vector{1,2,3,4}));
}

TEST(implicit_optional_ref, const_valid)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    const std::vector<int> o{1,2,3,4};
    out(zpp::bits::implicit_optional_ref(o)).or_throw();

    EXPECT_EQ(encode_hex(data),
              "01"
              "04000000"
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    std::vector<int> ov;
    in(zpp::bits::implicit_optional_ref(ov)).or_throw();

    EXPECT_EQ(o, (std::vector{1,2,3,4}));
}

TEST(implicit_optional_ref, nullopt)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(zpp::bits::implicit_optional_ref(std::vector<int>{})).or_throw();

    EXPECT_EQ(encode_hex(data),
              "00");

    std::vector<int> o = std::vector{1, 2, 3};
    in(zpp::bits::implicit_optional_ref(o)).or_throw();

    EXPECT_TRUE(o.empty());
}

} // namespace test_implicit_optional
