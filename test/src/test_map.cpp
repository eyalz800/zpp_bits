#include "test.h"
#include <map>

TEST(map, integer)
{
    zpp::try_catch([&]() -> zpp::throwing<void> {
        auto [data, in, out] = zpp::bits::data_in_out();
        co_await out(std::map<int, int>{{1, 1}, {2, 2}, {3, 3}, {4, 4}});

        auto count_bytes = hexlify(data);
        count_bytes.resize(8);
        EXPECT_EQ(count_bytes, "04000000");

        std::map<int, int> s;
        co_await in(s);

        EXPECT_EQ(s, (std::map<int, int>{{1, 1}, {2, 2}, {3, 3}, {4, 4}}));
    }, [&]() {
        FAIL();
    });
}

TEST(map, string)
{
    zpp::try_catch([&]() -> zpp::throwing<void> {
        using namespace std::string_literals;
        auto [data, in, out] = zpp::bits::data_in_out();
        co_await out(std::map<std::string, std::string>{
            {"1"s, "1"s}, {"2"s, "2"s}, {"3"s, "3"s}, {"4"s, "4"s}});

        auto count_bytes = hexlify(data);
        std::cout << count_bytes << '\n';
        count_bytes.resize(8);
        EXPECT_EQ(count_bytes, "04000000");

        std::map<std::string, std::string> s;
        co_await in(s);

        EXPECT_EQ(
            s,
            (std::map<std::string, std::string>{
                {"1"s, "1"s}, {"2"s, "2"s}, {"3"s, "3"s}, {"4"s, "4"s}}));
    }, [&]() {
        FAIL();
    });
}

