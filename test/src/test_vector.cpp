#include "test.h"

TEST(vector, integer)
{
    zpp::try_catch([&]() -> zpp::throwing<void> {
        auto [data, in, out] = zpp::bits::data_in_out();
        co_await out(std::vector{1,2,3,4});

        EXPECT_EQ(hexlify(data),
                  "04000000"
                  "01000000"
                  "02000000"
                  "03000000"
                  "04000000");

        std::vector<int> v;
        co_await in(v);

        EXPECT_EQ(v, (std::vector{1,2,3,4}));
    }, [&]() {
        FAIL();
    });
}

TEST(vector, string)
{
    zpp::try_catch([&]() -> zpp::throwing<void> {
        using namespace std::string_literals;
        auto [data, in, out] = zpp::bits::data_in_out();
        co_await out(std::vector{"1"s,"2"s,"3"s,"4"s});

        EXPECT_EQ(hexlify(data),
                  "04000000"
                  "01000000"
                  "31"
                  "01000000"
                  "32"
                  "01000000"
                  "33"
                  "01000000"
                  "34");

        std::vector<std::string> v;
        co_await in(v);

        EXPECT_EQ(v, (std::vector{"1"s,"2"s,"3"s,"4"s}));
    }, [&]() {
        FAIL();
    });
}
