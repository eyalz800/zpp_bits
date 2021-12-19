#include "test.h"
#include <compare>

namespace test_byte_serializable
{

struct integers
{
    int x;
    int y;
};

struct explicit_integers
{
    using serialize = zpp::bits::explicit_members<2>;
    int x;
    int y;
};

struct explicit_integers_auto
{
    using serialize = zpp::bits::explicit_members<>;
    int x;
    int y;
};

struct ref
{
    int & x;
    int y;
};

struct pointer
{
    int * x;
    int y;
};

struct array
{
    int x[5];
    int y;
};

struct stdarray
{
    std::array<int, 5> x;
    int y;
};

struct stdarray_array
{
    std::array<int, 5> x[5];
    int y;
};

struct vector
{
    std::vector<char> x;
    int y;
};

static_assert(zpp::bits::concepts::byte_serializable<integers>);
static_assert(!zpp::bits::concepts::byte_serializable<explicit_integers>);
static_assert(
    !zpp::bits::concepts::byte_serializable<explicit_integers_auto>);
static_assert(!zpp::bits::concepts::byte_serializable<ref>);
static_assert(!zpp::bits::concepts::byte_serializable<pointer>);
static_assert(zpp::bits::concepts::byte_serializable<array>);
static_assert(zpp::bits::concepts::byte_serializable<stdarray>);
static_assert(zpp::bits::concepts::byte_serializable<stdarray_array>);
static_assert(!zpp::bits::concepts::byte_serializable<vector>);
static_assert(!zpp::bits::concepts::byte_serializable<std::vector<char>>);
static_assert(!zpp::bits::concepts::byte_serializable<std::string>);
static_assert(!zpp::bits::concepts::byte_serializable<std::string_view>);
static_assert(!zpp::bits::concepts::byte_serializable<std::span<char>>);
static_assert(zpp::bits::concepts::byte_serializable<std::array<char, 2>>);

class inaccessible
{
public:
    inaccessible() = default;
    inaccessible(int x, int y) : x(x), y(y)
    {
    }

    std::strong_ordering operator<=>(const inaccessible & other) const =
        default;

public:
    int x{};
    int y{};
};

TEST(byte_serializable, inaccessible)
{
    static_assert(zpp::bits::concepts::byte_serializable<inaccessible>);

    auto [data, in, out] = zpp::bits::data_in_out();
    out(inaccessible{1337, 1338}).or_throw();

    inaccessible s;
    in(s).or_throw();

    EXPECT_EQ(s, (inaccessible{1337, 1338}));
}

struct requires_padding
{
    std::byte b{};
    std::int32_t i32{};
};

TEST(byte_serializable, requires_padding)
{
    static_assert(
        zpp::bits::concepts::byte_serializable<requires_padding>);
    static_assert(sizeof(requires_padding) > 5);

    auto [data, in, out] = zpp::bits::data_in_out();
    out(requires_padding{std::byte{0x25}, 0x1337}).or_throw();

    EXPECT_EQ(data.size(), sizeof(requires_padding));
    EXPECT_NE(hexlify(data),
              "25"
              "37130000");

    requires_padding s;
    in(s).or_throw();
    EXPECT_EQ(in.position(), sizeof(requires_padding));
    EXPECT_EQ(s.b, std::byte{0x25});
    EXPECT_EQ(s.i32, 0x1337);
}

struct explicit_requires_padding
{
    using serialize = zpp::bits::explicit_members<2>;
    std::byte b{};
    std::int32_t i32{};
};

TEST(byte_serializable, explicit_requires_padding)
{
    static_assert(!zpp::bits::concepts::byte_serializable<
                  explicit_requires_padding>);
    static_assert(sizeof(requires_padding) ==
                  sizeof(explicit_requires_padding));
    static_assert(sizeof(explicit_requires_padding) > 5);

    auto [data, in, out] = zpp::bits::data_in_out();
    out(explicit_requires_padding{std::byte{0x25}, 0x1337}).or_throw();

    EXPECT_EQ(data.size(), std::size_t{5});
    EXPECT_EQ(hexlify(data),
              "25"
              "37130000");

    explicit_requires_padding s;
    in(s).or_throw();
    EXPECT_EQ(in.position(), std::size_t{5});
    EXPECT_EQ(s.b, std::byte{0x25});
    EXPECT_EQ(s.i32, 0x1337);
}

#if ZPP_BITS_AUTODETECT_MEMBERS_MODE > 0
struct explicit_requires_padding_auto
{
    using serialize = zpp::bits::explicit_members<>;
    std::byte b{};
    std::int32_t i32{};
};

static_assert(sizeof(requires_padding) ==
              sizeof(explicit_requires_padding_auto));

TEST(byte_serializable, explicit_requires_padding_auto)
{
    static_assert(!zpp::bits::concepts::byte_serializable<
                  explicit_requires_padding_auto>);
    static_assert(sizeof(requires_padding) ==
                  sizeof(explicit_requires_padding_auto));
    static_assert(sizeof(explicit_requires_padding_auto) > 5);

    auto [data, in, out] = zpp::bits::data_in_out();
    out(explicit_requires_padding_auto{std::byte{0x25}, 0x1337})
        .or_throw();

    EXPECT_EQ(data.size(), std::size_t{5});
    EXPECT_EQ(hexlify(data),
              "25"
              "37130000");

    explicit_requires_padding_auto s;
    in(s).or_throw();
    EXPECT_EQ(in.position(), std::size_t{5});
    EXPECT_EQ(s.b, std::byte{0x25});
    EXPECT_EQ(s.i32, 0x1337);
}
#endif

} // namespace test_byte_serializable
