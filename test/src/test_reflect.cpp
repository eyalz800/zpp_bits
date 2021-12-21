#include "test.h"

namespace test_reflect
{

struct point
{
    int x;
    int y;
};

#if !ZPP_BITS_AUTODETECT_MEMBERS_MODE
auto serialize(point) -> zpp::bits::members<2>;
#endif

static_assert(zpp::bits::number_of_members<point>() == 2);

TEST(test_reflect, sanity)
{
    constexpr auto sum = zpp::bits::visit_members(
        point{1, 2}, [](auto x, auto y) { return x + y; });

    static_assert(sum == 3);

    constexpr auto generic_sum = zpp::bits::visit_members(
        point{1, 2}, [](auto... members) { return (0 + ... + members); });

    static_assert(generic_sum == 3);

#if !(defined __clang__ && __clang_major__ < 13)
    constexpr auto is_two_integers =
        zpp::bits::visit_members_types<point>([]<typename... Types>() {
            if constexpr (std::same_as<std::tuple<Types...>,
                                       std::tuple<int, int>>) {
                return std::true_type{};
            } else {
                return std::false_type{};
            }
        })();

    static_assert(is_two_integers);
#endif
}

TEST(test_reflect, visit_point)
{
    auto result = zpp::bits::visit_members(point{1337, 1338}, [](auto x, auto y) {
        EXPECT_EQ(x, 1337);
        EXPECT_EQ(y, 1338);
        return 1339;
    });

    EXPECT_EQ(result, 1339);

    result = zpp::bits::visit_members(point{1337, 1338}, [](auto... members){
        return (0 + ... + members);
    });

    EXPECT_EQ(result, (1337 + 1338));
}

#if !(defined __clang__ && __clang_major__ < 13)
TEST(test_reflect, visit_point_types)
{
    auto result = zpp::bits::visit_members_types<point>([]<typename... Types>() {
        EXPECT_EQ(sizeof...(Types), std::size_t{2});
        EXPECT_TRUE((std::same_as<std::tuple<Types...>, std::tuple<int, int>>));
        return std::integral_constant<int, 1337>{};
    })();

    EXPECT_EQ(result, 1337);
}
#endif

} // namespace test_reflect
