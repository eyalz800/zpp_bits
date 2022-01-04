#include "test.h"
#include <compare>

namespace test_byte_serializable
{

struct integers
{
    int x;
    int y;
};

struct members_integers
{
    using serialize = zpp::bits::members<2>;
    int x;
    int y;
};

struct members_integers_outside
{
    int x;
    int y;
};

auto serialize(const members_integers_outside &) -> zpp::bits::members<2>;

struct members_integers_auto
{
    using serialize = zpp::bits::members<>;
    int x;
    int y;
};

struct members_integers_outside_auto
{
    int x;
    int y;
};

auto serialize(const members_integers_outside_auto &)
    -> zpp::bits::members<>;

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

struct stdarray_custom
{
    constexpr static void serialize(auto & archive, auto & self);

    std::array<int, 5> x;
    int y;
};

struct stdarray_custom_outside
{
    std::array<int, 5> x;
    int y;
};
constexpr void serialize(auto & archive,
                         const stdarray_custom_outside & self);

static_assert(zpp::bits::concepts::byte_serializable<int[5]>);
static_assert(zpp::bits::concepts::byte_serializable<members_integers>);
static_assert(
    zpp::bits::concepts::byte_serializable<members_integers_outside>);
static_assert(!zpp::bits::concepts::byte_serializable<ref>);
static_assert(!zpp::bits::concepts::byte_serializable<pointer>);
static_assert(!zpp::bits::concepts::byte_serializable<vector>);
static_assert(!zpp::bits::concepts::byte_serializable<std::vector<char>>);
static_assert(!zpp::bits::concepts::byte_serializable<std::string>);
static_assert(!zpp::bits::concepts::byte_serializable<std::string_view>);
static_assert(!zpp::bits::concepts::byte_serializable<std::span<char>>);
static_assert(zpp::bits::concepts::byte_serializable<std::array<char, 2>>);

static_assert(!zpp::bits::concepts::byte_serializable<stdarray_custom>);
static_assert(!zpp::bits::concepts::byte_serializable<stdarray_custom_outside>);
static_assert(!zpp::bits::concepts::byte_serializable<std::array<stdarray_custom_outside, 1>>);
static_assert(!zpp::bits::concepts::byte_serializable<std::array<stdarray_custom, 1>>);

#if !ZPP_BITS_AUTODETECT_MEMBERS_MODE
auto serialize(const array &) -> zpp::bits::members<2>;
auto serialize(const stdarray &) -> zpp::bits::members<2>;
auto serialize(const stdarray_array &) -> zpp::bits::members<2>;
auto serialize(const integers &) -> zpp::bits::members<2>;
auto serialize(const integers &) -> zpp::bits::members<2>;
#endif

static_assert(zpp::bits::concepts::byte_serializable<stdarray>);
static_assert(zpp::bits::concepts::byte_serializable<std::array<stdarray, 1>>);

static_assert(zpp::bits::concepts::byte_serializable<array>);
static_assert(zpp::bits::concepts::byte_serializable<stdarray>);
static_assert(!zpp::bits::concepts::serialize_as_bytes<zpp::bits::out<>,
                                                       stdarray_custom>);
static_assert(!zpp::bits::concepts::serialize_as_bytes<zpp::bits::in<>,
                                                       stdarray_custom>);
static_assert(
    !zpp::bits::concepts::serialize_as_bytes<zpp::bits::out<>,
                                             stdarray_custom_outside>);
static_assert(
    !zpp::bits::concepts::serialize_as_bytes<zpp::bits::in<>,
                                             stdarray_custom_outside>);
static_assert(zpp::bits::concepts::byte_serializable<stdarray_array>);
static_assert(zpp::bits::concepts::byte_serializable<integers>);

#if ZPP_BITS_AUTODETECT_MEMBERS_MODE > 0
static_assert(
    zpp::bits::concepts::byte_serializable<members_integers_auto>);
static_assert(
    zpp::bits::concepts::byte_serializable<members_integers_outside_auto>);
#endif

struct requires_padding
{
    std::byte b{};
    std::int32_t i32{};
};

TEST(byte_serializable, requires_padding)
{
    auto [data, in, out] = zpp::bits::data_in_out();

    static_assert(
        !zpp::bits::concepts::byte_serializable<requires_padding>);
    static_assert(sizeof(requires_padding) > 5);

#if ZPP_BITS_AUTODETECT_MEMBERS_MODE > 0
    out(requires_padding{std::byte{0x25}, 0x1337}).or_throw();

    EXPECT_EQ(data.size(), std::size_t{5});
    EXPECT_EQ(encode_hex(data),
              "25"
              "37130000");

    requires_padding s;
    in(s).or_throw();
    if constexpr (!ZPP_BITS_AUTODETECT_MEMBERS_MODE) {
        EXPECT_EQ(in.position(), sizeof(requires_padding));
    } else {
        EXPECT_EQ(in.position(), std::size_t{5});
    }
    EXPECT_EQ(s.b, std::byte{0x25});
    EXPECT_EQ(s.i32, 0x1337);
#endif
}

class inaccessible_requires_padding
{
public:
    inaccessible_requires_padding() = default;
    inaccessible_requires_padding(auto b, auto i32) : b(b), i32(i32)
    {
    }

    std::strong_ordering operator<=>(
        const inaccessible_requires_padding & other) const = default;

    void dummy()
    {
        (void)b;
        (void)i32;
    }

private:
    using serialize = zpp::bits::members<2>;
    std::byte b{};
    std::int32_t i32{};
};

static_assert(!zpp::bits::concepts::byte_serializable<
              inaccessible_requires_padding>);

class inaccessible_friend_requires_padding
{
public:
    inaccessible_friend_requires_padding() = default;
    inaccessible_friend_requires_padding(auto b, auto i32) : b(b), i32(i32)
    {
    }

    std::strong_ordering
    operator<=>(const inaccessible_friend_requires_padding & other) const =
        default;

private:
    friend zpp::bits::access;
    using serialize = zpp::bits::members<2>;
    std::byte b{};
    std::int32_t i32{};
};

TEST(byte_serializable, inaccessible_friend_requires_padding)
{
    auto [data, in, out] = zpp::bits::data_in_out();

    static_assert(sizeof(requires_padding) ==
                  sizeof(inaccessible_friend_requires_padding));
    static_assert(sizeof(inaccessible_friend_requires_padding) > 5);
    static_assert(!zpp::bits::concepts::byte_serializable<
                  inaccessible_friend_requires_padding>);

    out(inaccessible_friend_requires_padding{std::byte{0x25}, 0x1337})
        .or_throw();

    EXPECT_EQ(data.size(), std::size_t{5});
    EXPECT_EQ(encode_hex(data),
              "25"
              "37130000");

    inaccessible_friend_requires_padding s;
    in(s).or_throw();
    EXPECT_EQ(in.position(), std::size_t{5});
    EXPECT_EQ(
        s,
        (inaccessible_friend_requires_padding{std::byte{0x25}, 0x1337}));
}

class inaccessible_members_requires_padding
{
public:
    inaccessible_members_requires_padding() = default;
    inaccessible_members_requires_padding(auto b, auto i32) :
        b(b), i32(i32)
    {
    }

    std::strong_ordering operator<=>(
        const inaccessible_members_requires_padding & other) const =
        default;

    using serialize = zpp::bits::members<2>;

private:
    friend zpp::bits::access;
    std::byte b{};
    std::int32_t i32{};
};

TEST(byte_serializable, inaccessible_members_requires_padding)
{
    static_assert(sizeof(requires_padding) ==
                  sizeof(inaccessible_members_requires_padding));
    static_assert(sizeof(inaccessible_members_requires_padding) > 5);

    auto [data, in, out] = zpp::bits::data_in_out();
    out(inaccessible_members_requires_padding{std::byte{0x25}, 0x1337})
        .or_throw();

    EXPECT_EQ(data.size(), std::size_t{5});
    EXPECT_EQ(encode_hex(data),
              "25"
              "37130000");

    inaccessible_members_requires_padding s;
    in(s).or_throw();
    EXPECT_EQ(in.position(), std::size_t{5});
    EXPECT_EQ(
        s,
        (inaccessible_members_requires_padding{std::byte{0x25}, 0x1337}));
}

struct members_requires_padding
{
    using serialize = zpp::bits::members<2>;
    std::byte b{};
    std::int32_t i32{};
};

TEST(byte_serializable, members_requires_padding)
{
    static_assert(
        !zpp::bits::concepts::byte_serializable<members_requires_padding>);
    static_assert(sizeof(requires_padding) ==
                  sizeof(members_requires_padding));
    static_assert(sizeof(members_requires_padding) > 5);

    auto [data, in, out] = zpp::bits::data_in_out();
    out(members_requires_padding{std::byte{0x25}, 0x1337}).or_throw();

    EXPECT_EQ(data.size(), std::size_t{5});
    EXPECT_EQ(encode_hex(data),
              "25"
              "37130000");

    members_requires_padding s;
    in(s).or_throw();
    EXPECT_EQ(in.position(), std::size_t{5});
    EXPECT_EQ(s.b, std::byte{0x25});
    EXPECT_EQ(s.i32, 0x1337);
}

struct members_requires_padding_outside
{
    std::byte b{};
    std::int32_t i32{};
};

auto serialize(const members_requires_padding_outside &)
    -> zpp::bits::members<2>;

TEST(byte_serializable, members_requires_padding_outside)
{
    static_assert(!zpp::bits::concepts::byte_serializable<
                  members_requires_padding_outside>);
    static_assert(sizeof(requires_padding) ==
                  sizeof(members_requires_padding_outside));
    static_assert(sizeof(members_requires_padding_outside) > 5);

    auto [data, in, out] = zpp::bits::data_in_out();
    out(members_requires_padding_outside{std::byte{0x25}, 0x1337})
        .or_throw();

    EXPECT_EQ(data.size(), std::size_t{5});
    EXPECT_EQ(encode_hex(data),
              "25"
              "37130000");

    members_requires_padding_outside s;
    in(s).or_throw();
    EXPECT_EQ(in.position(), std::size_t{5});
    EXPECT_EQ(s.b, std::byte{0x25});
    EXPECT_EQ(s.i32, 0x1337);
}

#if ZPP_BITS_AUTODETECT_MEMBERS_MODE > 0
struct members_requires_padding_auto
{
    using serialize = zpp::bits::members<>;
    std::byte b{};
    std::int32_t i32{};
};

TEST(byte_serializable, members_requires_padding_auto)
{
    static_assert(!zpp::bits::concepts::byte_serializable<
                  members_requires_padding_auto>);
    static_assert(sizeof(requires_padding) ==
                  sizeof(members_requires_padding_auto));
    static_assert(sizeof(members_requires_padding_auto) > 5);

    auto [data, in, out] = zpp::bits::data_in_out();
    out(members_requires_padding_auto{std::byte{0x25}, 0x1337}).or_throw();

    EXPECT_EQ(data.size(), std::size_t{5});
    EXPECT_EQ(encode_hex(data),
              "25"
              "37130000");

    members_requires_padding_auto s;
    in(s).or_throw();
    EXPECT_EQ(in.position(), std::size_t{5});
    EXPECT_EQ(s.b, std::byte{0x25});
    EXPECT_EQ(s.i32, 0x1337);
}

struct members_requires_padding_outside_auto
{
    std::byte b{};
    std::int32_t i32{};
};

auto serialize(const members_requires_padding_outside_auto &)
    -> zpp::bits::members<>;

TEST(byte_serializable, members_requires_padding_outside_auto)
{
    static_assert(!zpp::bits::concepts::byte_serializable<
                  members_requires_padding_outside_auto>);
    static_assert(sizeof(requires_padding) ==
                  sizeof(members_requires_padding_outside_auto));
    static_assert(sizeof(members_requires_padding_outside_auto) > 5);

    auto [data, in, out] = zpp::bits::data_in_out();
    out(members_requires_padding_outside_auto{std::byte{0x25}, 0x1337})
        .or_throw();

    EXPECT_EQ(data.size(), std::size_t{5});
    EXPECT_EQ(encode_hex(data),
              "25"
              "37130000");

    members_requires_padding_outside_auto s;
    in(s).or_throw();
    EXPECT_EQ(in.position(), std::size_t{5});
    EXPECT_EQ(s.b, std::byte{0x25});
    EXPECT_EQ(s.i32, 0x1337);
}

#endif // ZPP_BITS_AUTODETECT_MEMBERS_MODE > 0

} // namespace test_byte_serializable
