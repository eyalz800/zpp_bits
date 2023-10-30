#include "test.h"

namespace test_bitfield
{
struct Flags
{
    unsigned char a : 3;
    bool b : 1;
    unsigned char c : 4;

    bool operator==(const Flags&) const = default;
};
static_assert(sizeof(Flags) == 1);

TEST(test_bitfield, test)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    Flags o{0b111, true, 0b1101};
    out(o).or_throw();
    EXPECT_EQ(out.position(), 1u);

    Flags i;
    in(i).or_throw();

    EXPECT_EQ(in.position(), 1u);
    EXPECT_EQ(o, i);
}

} // namespace test_bitfield
