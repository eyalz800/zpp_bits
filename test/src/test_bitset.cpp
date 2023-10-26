#include "test.h"
#include <bitset>

namespace test_bitset
{

TEST(test_bitset, sanity)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    constexpr std::bitset<8> o(0x7f);
    out(o).or_throw();
    EXPECT_EQ(out.position(), 1u);

    std::bitset<8> i;
    in(i).or_throw();

    EXPECT_EQ(in.position(), 1u);
    EXPECT_EQ(o, i);
}

} // namespace test_bitset
