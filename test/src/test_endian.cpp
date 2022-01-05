#include "test.h"

namespace test_endian
{

struct integers
{
    std::uint8_t i8;
    std::uint16_t i16;
    std::uint32_t i32;
    std::uint64_t i64;
};

TEST(test_endian, big)
{
    auto [data, in, out] =
        data_in_out(zpp::bits::endian::big{});

    out(0x13371337u,
        integers{0x12u, 0x1234u, 0x12345678u, 0x123456789abcdef0u})
        .or_throw();

    std::uint32_t i = {};
    integers ints = {};
    in(i, ints).or_throw();

    EXPECT_EQ(i, 0x13371337u);
    EXPECT_EQ(ints.i8, 0x12u);
    EXPECT_EQ(ints.i16, 0x1234u);
    EXPECT_EQ(ints.i32, 0x12345678u);
    EXPECT_EQ(ints.i64, 0x123456789abcdef0u);
}

TEST(test_endian, big_out)
{
    auto [data, out] = data_out(zpp::bits::endian::big{});

    zpp::bits::in in{data};

    out(0x13371337u,
        integers{0x12u, 0x1234u, 0x12345678u, 0x123456789abcdef0u})
        .or_throw();

    std::uint32_t i = {};
    integers ints = {};
    in(i, ints).or_throw();

    EXPECT_EQ(i, 0x37133713u);
    EXPECT_EQ(ints.i8, 0x12u);
    EXPECT_EQ(ints.i16, 0x3412u);
    EXPECT_EQ(ints.i32, 0x78563412u);
    EXPECT_EQ(ints.i64, 0xf0debc9a78563412u);
}

TEST(test_endian, big_in)
{
    auto [data, in] = data_in(zpp::bits::endian::big{});

    zpp::bits::out out{data};

    out(0x13371337u,
        integers{0x12u, 0x1234u, 0x12345678u, 0x123456789abcdef0u})
        .or_throw();

    std::uint32_t i = {};
    integers ints = {};
    in(i, ints).or_throw();

    EXPECT_EQ(i, 0x37133713u);
    EXPECT_EQ(ints.i8, 0x12u);
    EXPECT_EQ(ints.i16, 0x3412u);
    EXPECT_EQ(ints.i32, 0x78563412u);
    EXPECT_EQ(ints.i64, 0xf0debc9a78563412u);
}

TEST(test_endian, little)
{
    auto [data, in, out] = data_in_out(zpp::bits::endian::little{});

    out(0x13371337u,
        integers{0x12u, 0x1234u, 0x12345678u, 0x123456789abcdef0u})
        .or_throw();

    std::uint32_t i = {};
    integers ints = {};
    in(i, ints).or_throw();

    EXPECT_EQ(i, 0x13371337u);
    EXPECT_EQ(ints.i8, 0x12u);
    EXPECT_EQ(ints.i16, 0x1234u);
    EXPECT_EQ(ints.i32, 0x12345678u);
    EXPECT_EQ(ints.i64, 0x123456789abcdef0u);
}

TEST(test_endian, little_out)
{
    auto [data, out] = data_out(zpp::bits::endian::little{});

    zpp::bits::in in{data};

    out(0x13371337u,
        integers{0x12u, 0x1234u, 0x12345678u, 0x123456789abcdef0u})
        .or_throw();

    std::uint32_t i = {};
    integers ints = {};
    in(i, ints).or_throw();

    EXPECT_EQ(i, 0x13371337u);
    EXPECT_EQ(ints.i8, 0x12u);
    EXPECT_EQ(ints.i16, 0x1234u);
    EXPECT_EQ(ints.i32, 0x12345678u);
    EXPECT_EQ(ints.i64, 0x123456789abcdef0u);
}

TEST(test_endian, little_in)
{
    auto [data, in] = data_in(zpp::bits::endian::little{});

    zpp::bits::out out{data};

    out(0x13371337u,
        integers{0x12u, 0x1234u, 0x12345678u, 0x123456789abcdef0u})
        .or_throw();

    std::uint32_t i = {};
    integers ints = {};
    in(i, ints).or_throw();

    EXPECT_EQ(i, 0x13371337u);
    EXPECT_EQ(ints.i8, 0x12u);
    EXPECT_EQ(ints.i16, 0x1234u);
    EXPECT_EQ(ints.i32, 0x12345678u);
    EXPECT_EQ(ints.i64, 0x123456789abcdef0u);
}

} // namespace test_endian
