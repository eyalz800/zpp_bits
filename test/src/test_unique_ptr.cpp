#include "test.h"

TEST(unique_ptr, regular)
{
    zpp::try_catch([&]() -> zpp::throwing<void> {
        auto [data, in, out] = zpp::bits::data_in_out();
        co_await out(
            std::make_unique<std::vector<int>>(std::vector{1, 2, 3, 4}));

        EXPECT_EQ(hexlify(data),
                  "04000000"
                  "01000000"
                  "02000000"
                  "03000000"
                  "04000000");

        std::unique_ptr<std::vector<int>> p;
        co_await in(p);

        EXPECT_EQ(*p, (std::vector{1,2,3,4}));
    }, [&]() {
        FAIL();
    });
}
