#include "test.h"

namespace test_implicit_optional
{

TEST(optional_ptr, ptr_valid)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(zpp::bits::optional_ptr{std::make_unique<int>(0x1337)}).or_throw();

    EXPECT_EQ(encode_hex(data),
              "01"
              "37130000");

    zpp::bits::optional_ptr<int> o;
    in(o).or_throw();

    EXPECT_TRUE(o != nullptr);
    EXPECT_EQ(*o, 0x1337);
}

TEST(optional_ptr, ptr_invalid)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(zpp::bits::optional_ptr<int>{}).or_throw();

    EXPECT_EQ(encode_hex(data),
              "00");

    zpp::bits::optional_ptr<int> o = std::make_unique<int>(1337);
    in(o).or_throw();

    EXPECT_TRUE(o == nullptr);
}

} // namespace test_implicit_optional
