#include "test.h"

namespace test_varint
{
using namespace zpp::bits::literals;

static_assert(zpp::bits::to_bytes<zpp::bits::varint<std::uint32_t>{}>() == "00"_decode_hex);
static_assert(zpp::bits::from_bytes<"9601"_decode_hex, zpp::bits::vint32_t>() == 150);
static_assert(zpp::bits::from_bytes<"960100000000000000000000"_decode_hex, zpp::bits::vint32_t>() == 150);
static_assert(zpp::bits::from_bytes<"960100000000000000000000"_decode_hex, zpp::bits::vsize_t>() == 150);
static_assert(zpp::bits::from_bytes<"96818000"_decode_hex, zpp::bits::vint32_t>() == 150);
static_assert(zpp::bits::from_bytes<"96818000"_decode_hex, zpp::bits::vsize_t>() == 150);

static_assert(zpp::bits::to_bytes<zpp::bits::varint{150}>() == "9601"_decode_hex);

static_assert(zpp::bits::varint_size(
                  std::numeric_limits<std::uint32_t>::max()) == 5);

static_assert(zpp::bits::varint_size(
                  std::numeric_limits<std::uint64_t>::max()) == 10);

static_assert(zpp::bits::varint_size(0) == 1);
static_assert(zpp::bits::varint_size(0x7f) == 1);
static_assert(zpp::bits::varint_size(0x80) == 2);
static_assert(zpp::bits::varint_size(0x3fff) == 2);
static_assert(zpp::bits::varint_size(0x1fffff) == 3);
static_assert(zpp::bits::varint_size(0x3fffff) == 4);
static_assert(zpp::bits::varint_size(0xfffffff) == 4);
static_assert(zpp::bits::varint_size(0x1fffffff) == 5);
static_assert(zpp::bits::varint_size(0xffffffff) == 5);
static_assert(zpp::bits::varint_size(-1) == 5);

static_assert(
    zpp::bits::to_bytes<zpp::bits::vint32_t{-1}>() == "ffffffff0f"_decode_hex);

static_assert(zpp::bits::from_bytes<"ffffffff0f"_decode_hex,
                                    zpp::bits::vint32_t>() == -1);

static_assert(zpp::bits::to_bytes<zpp::bits::vint64_t{-1}>() ==
              "ffffffffffffffffff01"_decode_hex);

static_assert(zpp::bits::from_bytes<"ffffffffffffffffff01"_decode_hex,
                                    zpp::bits::vint64_t>() == -1);

static_assert(
    zpp::bits::from_bytes<
    zpp::bits::to_bytes<zpp::bits::vint32_t{0}>(),
                          zpp::bits::vint32_t>() == 0);

static_assert(
    zpp::bits::from_bytes<
    zpp::bits::to_bytes<zpp::bits::vint32_t{1}>(),
                          zpp::bits::vint32_t>() == 1);

static_assert(
    zpp::bits::from_bytes<
    zpp::bits::to_bytes<zpp::bits::vint32_t{-1}>(),
                          zpp::bits::vint32_t>() == -1);

static_assert(
    zpp::bits::from_bytes<
    zpp::bits::to_bytes<zpp::bits::vint32_t{0x7f}>(),
                          zpp::bits::vint32_t>() == 0x7f);

static_assert(
    zpp::bits::from_bytes<
    zpp::bits::to_bytes<zpp::bits::vint64_t{0x80}>(),
                          zpp::bits::vint64_t>() == 0x80);

static_assert(
    zpp::bits::from_bytes<
    zpp::bits::to_bytes<zpp::bits::vint64_t{0}>(),
                          zpp::bits::vint64_t>() == 0);

static_assert(
    zpp::bits::from_bytes<
    zpp::bits::to_bytes<zpp::bits::vint64_t{1}>(),
                          zpp::bits::vint64_t>() == 1);

static_assert(
    zpp::bits::from_bytes<
    zpp::bits::to_bytes<zpp::bits::vint64_t{-1}>(),
                          zpp::bits::vint64_t>() == -1);

static_assert(
    zpp::bits::from_bytes<
    zpp::bits::to_bytes<zpp::bits::vint64_t{0x7f}>(),
                          zpp::bits::vint64_t>() == 0x7f);

static_assert(
    zpp::bits::from_bytes<
    zpp::bits::to_bytes<zpp::bits::vint64_t{0x80}>(),
                          zpp::bits::vint64_t>() == 0x80);

static_assert(zpp::bits::to_bytes<zpp::bits::vsint32_t{0}>() ==
              zpp::bits::to_bytes<zpp::bits::vint32_t{0}>());

static_assert(zpp::bits::to_bytes<zpp::bits::vsint32_t{-1}>() ==
              zpp::bits::to_bytes<zpp::bits::vint32_t{1}>());

static_assert(zpp::bits::to_bytes<zpp::bits::vsint32_t{1}>() ==
              zpp::bits::to_bytes<zpp::bits::vint32_t{2}>());

static_assert(zpp::bits::to_bytes<zpp::bits::vsint32_t{-2}>() ==
              zpp::bits::to_bytes<zpp::bits::vint32_t{3}>());

static_assert(zpp::bits::to_bytes<zpp::bits::vsint32_t{2147483647}>() ==
              zpp::bits::to_bytes<zpp::bits::vuint32_t{4294967294}>());

static_assert(zpp::bits::to_bytes<zpp::bits::vsint32_t{-2147483648}>() ==
              zpp::bits::to_bytes<zpp::bits::vuint32_t{4294967295}>());

static_assert(zpp::bits::to_bytes<zpp::bits::vsint64_t{0}>() ==
              zpp::bits::to_bytes<zpp::bits::vint64_t{0}>());

static_assert(zpp::bits::to_bytes<zpp::bits::vsint64_t{-1}>() ==
              zpp::bits::to_bytes<zpp::bits::vint64_t{1}>());

static_assert(zpp::bits::to_bytes<zpp::bits::vsint64_t{1}>() ==
              zpp::bits::to_bytes<zpp::bits::vint64_t{2}>());

static_assert(zpp::bits::to_bytes<zpp::bits::vsint64_t{-2}>() ==
              zpp::bits::to_bytes<zpp::bits::vint64_t{3}>());

static_assert(zpp::bits::to_bytes<zpp::bits::vsint64_t{2147483647}>() ==
              zpp::bits::to_bytes<zpp::bits::vuint64_t{4294967294}>());

static_assert(zpp::bits::to_bytes<zpp::bits::vsint64_t{-2147483648}>() ==
              zpp::bits::to_bytes<zpp::bits::vuint64_t{4294967295}>());

static_assert(
    zpp::bits::from_bytes<zpp::bits::to_bytes<zpp::bits::vsint32_t{0}>(),
                          zpp::bits::vsint32_t>() == 0);

static_assert(zpp::bits::to_bytes<zpp::bits::vsint32_t{-1}>() ==
              zpp::bits::to_bytes<zpp::bits::vint32_t{1}>());

static_assert(
    zpp::bits::from_bytes<zpp::bits::to_bytes<zpp::bits::vsint32_t{1}>(),
                          zpp::bits::vsint32_t>() == 1);

static_assert(
    zpp::bits::from_bytes<zpp::bits::to_bytes<zpp::bits::vsint32_t{2}>(),
                          zpp::bits::vsint32_t>() == 2);

static_assert(
    zpp::bits::from_bytes<zpp::bits::to_bytes<zpp::bits::vsint32_t{2147483647}>(),
                          zpp::bits::vsint32_t>() == 2147483647);

static_assert(
    zpp::bits::from_bytes<zpp::bits::to_bytes<zpp::bits::vsint32_t{-2}>(),
                          zpp::bits::vsint32_t>() == -2);

static_assert(
    zpp::bits::from_bytes<zpp::bits::to_bytes<zpp::bits::vsint32_t{-2147483648}>(),
                          zpp::bits::vsint32_t>() == -2147483648);
static_assert(
    zpp::bits::to_bytes<zpp::bits::varint<std::byte>{std::byte{0x7f}}>() ==
    "7f"_decode_hex);

TEST(varint, sanity)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(zpp::bits::varint{150}).or_throw();

    zpp::bits::varint i{0};
    in(i).or_throw();

    EXPECT_EQ(i, 150);
}

TEST(varint, representation)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(zpp::bits::varint{150}).or_throw();

    std::array<std::byte, 2> representation{};
    in(representation).or_throw();

    EXPECT_EQ(out.position(), std::size_t{2});
    EXPECT_EQ(representation, "9601"_decode_hex);
}

TEST(varint, varint_size)
{
    auto [data, in, out] = data_in_out(zpp::bits::size_varint{});
    auto o =
        std::vector<zpp::bits::vint32_t>{0x13, 0x80, 0x82, 0x1000, -1};
    out(o).or_throw();

    zpp::bits::vsize_t size;
    zpp::bits::in{data}(size).or_throw();
    EXPECT_EQ(size, o.size());

    std::vector<zpp::bits::vint32_t> v;
    in(v).or_throw();

    EXPECT_EQ(v, o);
}

} // namespace test_varint
