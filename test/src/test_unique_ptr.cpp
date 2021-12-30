#include "test.h"

TEST(unique_ptr, regular)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(std::make_unique<std::vector<int>>(std::vector{1, 2, 3, 4}))
        .or_throw();

    EXPECT_EQ(encode_hex(data),
              "04000000"
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    std::unique_ptr<std::vector<int>> p;
    in(p).or_throw();

    EXPECT_EQ(*p, (std::vector{1,2,3,4}));
}
