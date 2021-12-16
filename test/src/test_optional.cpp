#include "test.h"

TEST(optional, valid)
{
    zpp::try_catch([&]() -> zpp::throwing<void> {
        auto [data, in, out] = zpp::bits::data_in_out();
        co_await out(std::optional{std::vector{1,2,3,4}});

        EXPECT_EQ(hexlify(data),
                  "01"
                  "04000000"
                  "01000000"
                  "02000000"
                  "03000000"
                  "04000000");

        std::optional<std::vector<int>> o;
        co_await in(o);

        EXPECT_EQ(*o, (std::vector{1,2,3,4}));
    }, [&]() {
        FAIL();
    });
}

TEST(optional, nullopt)
{
    zpp::try_catch([&]() -> zpp::throwing<void> {
        auto [data, in, out] = zpp::bits::data_in_out();
        co_await out(std::optional<std::vector<int>>{});

        EXPECT_EQ(hexlify(data),
                  "00");

        std::optional<std::vector<int>> o = std::vector{1, 2, 3};
        co_await in(o);

        EXPECT_FALSE(o.has_value());
    }, [&]() {
        FAIL();
    });
}

