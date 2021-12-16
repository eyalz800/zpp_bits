#include "test.h"

TEST(array, integer)
{
    zpp::try_catch([&]() -> zpp::throwing<void> {
        auto [data, in, out] = zpp::bits::data_in_out();
        const int a1[] = {1,2,3,4};
        co_await out(a1);

        EXPECT_EQ(hexlify(data),
                  "01000000"
                  "02000000"
                  "03000000"
                  "04000000");

        int a2[4]{};
        co_await in(a2);

        for (std::size_t i = 0; i < std::extent_v<decltype(a1)>; ++i) {
            EXPECT_EQ(a1[i], a2[i]);
        }
    }, [&]() {
        FAIL();
    });
}

TEST(array, string)
{
    zpp::try_catch([&]() -> zpp::throwing<void> {
        using namespace std::string_literals;
        auto [data, in, out] = zpp::bits::data_in_out();
        std::string a1[] = {"1"s,"2"s,"3"s,"4"s};
        co_await out(a1);

        EXPECT_EQ(hexlify(data),
                  "01000000"
                  "31"
                  "01000000"
                  "32"
                  "01000000"
                  "33"
                  "01000000"
                  "34");

        std::string a2[4]{};
        co_await in(a2);

        for (std::size_t i = 0; i < std::extent_v<decltype(a1)>; ++i) {
            EXPECT_EQ(a1[i], a2[i]);
        }
    }, [&]() {
        FAIL();
    });
}
