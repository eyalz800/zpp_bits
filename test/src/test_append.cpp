#include "test.h"

namespace test_append
{

TEST(append, integer)
{
    std::vector<std::byte> data = {std::byte{0xff}};
    auto [in, out] = in_out(data, zpp::bits::append{});
    out(std::vector{1,2,3,4}).or_throw();

    EXPECT_EQ(encode_hex(data),
              "ff"
              "04000000"
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    std::vector<int> v;
    in.position() += sizeof(std::byte{});
    in(v).or_throw();

    EXPECT_EQ(v, (std::vector{1,2,3,4}));
}

} // namespace test_append
