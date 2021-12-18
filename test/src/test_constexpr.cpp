#include "test.h"

namespace test_constexpr
{

constexpr auto integers()
{
    std::array<std::byte, 0x1000> data{};
    auto [in, out] = zpp::bits::in_out(data);
    out(1,2,3,4,5).or_throw();

    int _1 = 0;
    int _2 = 0;
    int _3 = 0;
    int _4 = 0;
    int _5 = 0;
    in(_1, _2, _3, _4, _5).or_throw();
    return std::tuple{_1, _2, _3, _4, _5};
}

TEST(test_constexpr, integers)
{
    static_assert(integers() == std::tuple{1,2,3,4,5});
    EXPECT_TRUE((integers() == std::tuple{1,2,3,4,5}));
}

constexpr auto tuple_integers()
{
    std::array<std::byte, 0x1000> data{};
    auto [in, out] = zpp::bits::in_out(data);
    out(std::tuple{1,2,3,4,5}).or_throw();

    std::tuple t{0,0,0,0,0};
    in(t).or_throw();
    return t;
}

TEST(test_constexpr, tuple_integers)
{
    static_assert(tuple_integers() == std::tuple{1,2,3,4,5});
    EXPECT_TRUE((tuple_integers() == std::tuple{1,2,3,4,5}));
}

constexpr auto unsized_string_view()
{
    std::array<std::byte, 0x1000> data{};
    auto [in, out] = zpp::bits::in_out(data);
    out(zpp::bits::unsized(std::string_view{"Hello World"})).or_throw();

    std::array<char, std::string_view{"Hello World"}.size()> storage{};
    in(storage).or_throw();
    return storage;
}

TEST(test_constexpr, unsized_string_view)
{
    static_assert(0 == std::char_traits<char>::compare(
                           unsized_string_view().data(),
                           std::string_view{"Hello World"}.data(),
                           std::string_view{"Hello World"}.size()));

    EXPECT_TRUE((0 == std::char_traits<char>::compare(
                          unsized_string_view().data(),
                          std::string_view{"Hello World"}.data(),
                          std::string_view{"Hello World"}.size())));
}

} // namespace test_constexpr
