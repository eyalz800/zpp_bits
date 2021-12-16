#include "test.h"
#include <set>

TEST(set, integer)
{
    zpp::try_catch([&]() -> zpp::throwing<void> {
        auto [data, in, out] = zpp::bits::data_in_out();
        co_await out(std::set{1,2,3,4});

        auto count_bytes = hexlify(data);
        count_bytes.resize(8);
        EXPECT_EQ(count_bytes, "04000000");

        std::set<int> s;
        co_await in(s);

        EXPECT_EQ(s, (std::set{1,2,3,4}));
    }, [&]() {
        FAIL();
    });
}

TEST(set, string)
{
    zpp::try_catch([&]() -> zpp::throwing<void> {
        using namespace std::string_literals;
        auto [data, in, out] = zpp::bits::data_in_out();
        co_await out(std::set{"1"s,"2"s,"3"s,"4"s});

        auto count_bytes = hexlify(data);
        std::cout << count_bytes << '\n';
        count_bytes.resize(8);
        EXPECT_EQ(count_bytes, "04000000");

        std::set<std::string> s;
        co_await in(s);

        EXPECT_EQ(s, (std::set{"1"s,"2"s,"3"s,"4"s}));
    }, [&]() {
        FAIL();
    });
}
