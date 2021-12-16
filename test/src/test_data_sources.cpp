#include "test.h"

TEST(data_sources, vector_bytes)
{
    std::vector<std::byte> data;
    auto [in, out] = zpp::bits::in_out(data);
    out(std::vector{1,2,3,4}).or_throw();

    EXPECT_EQ(hexlify(data),
              "04000000"
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    std::vector<int> v;
    in(v).or_throw();

    EXPECT_EQ(v, (std::vector{1,2,3,4}));
}


