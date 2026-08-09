#include "test.h"
#include <cstddef>
#include <string>
#include <vector>

namespace test_pb_unknown_fields
{

// A reader that knows only field 1.
struct one_field
{
    zpp::bits::vint32_t a;

    using serialize = zpp::bits::pb_protocol;
};

// A writer that also has field 2.
struct two_fields
{
    zpp::bits::vint32_t a;
    std::string b;

    using serialize = zpp::bits::pb_protocol;
};

// A message that reserves field 1 and uses field 2.
struct reserved_field
{
    [[no_unique_address]] zpp::bits::pb_reserved _1{};
    zpp::bits::vint32_t b;

    using serialize = zpp::bits::pb_protocol;
};

static auto read(auto & item, const std::vector<std::byte> & data)
{
    zpp::bits::in in{data, zpp::bits::size_varint{}};
    return in(zpp::bits::unsized(item));
}

TEST(test_pb_unknown_fields, newer_writer_to_older_reader)
{
    two_fields newer{.a = 1, .b = "AAAAAAAA"};

    std::vector<std::byte> data;
    zpp::bits::out{data, zpp::bits::size_varint{}}(
        zpp::bits::unsized(newer))
        .or_throw();

    one_field older;
    ASSERT_EQ(read(older, data), std::errc{});
    EXPECT_EQ(older.a.value, 1);
}

TEST(test_pb_unknown_fields, unknown_length_delimited_before_known)
{
    // field 2, length delimited, "AAAA", then field 1 = 7
    std::vector<std::byte> data{
        std::byte{0x12}, std::byte{0x04}, std::byte{0x41}, std::byte{0x41},
        std::byte{0x41}, std::byte{0x41}, std::byte{0x08}, std::byte{0x07}};

    one_field item;
    ASSERT_EQ(read(item, data), std::errc{});
    EXPECT_EQ(item.a.value, 7);
}

TEST(test_pb_unknown_fields, unknown_varint_before_known)
{
    // field 2 varint = 8, then field 1 = 7. The unknown value byte is itself
    // a valid tag for field 1, so a reader that fails to skip it reads 8.
    std::vector<std::byte> data{std::byte{0x10},
                                std::byte{0x08},
                                std::byte{0x08},
                                std::byte{0x07}};

    one_field item;
    ASSERT_EQ(read(item, data), std::errc{});
    EXPECT_EQ(item.a.value, 7);
}

TEST(test_pb_unknown_fields, unknown_fixed_32_before_known)
{
    // field 3 fixed 32, then field 1 = 7
    std::vector<std::byte> data{
        std::byte{0x1d}, std::byte{0xde}, std::byte{0xad}, std::byte{0xbe},
        std::byte{0xef}, std::byte{0x08}, std::byte{0x07}};

    one_field item;
    ASSERT_EQ(read(item, data), std::errc{});
    EXPECT_EQ(item.a.value, 7);
}

TEST(test_pb_unknown_fields, unknown_fixed_64_before_known)
{
    // field 3 fixed 64, then field 1 = 7
    std::vector<std::byte> data{
        std::byte{0x19}, std::byte{0x01}, std::byte{0x02}, std::byte{0x03},
        std::byte{0x04}, std::byte{0x05}, std::byte{0x06}, std::byte{0x07},
        std::byte{0x08}, std::byte{0x08}, std::byte{0x07}};

    one_field item;
    ASSERT_EQ(read(item, data), std::errc{});
    EXPECT_EQ(item.a.value, 7);
}

TEST(test_pb_unknown_fields, unknown_field_payload_is_not_interpreted)
{
    // An unknown length delimited field whose payload would read as
    // "field 1 = 99" if it were mistaken for a tag stream.
    std::vector<std::byte> data{std::byte{0x12},
                                std::byte{0x02},
                                std::byte{0x08},
                                std::byte{0x63}};

    one_field item{.a = 1};
    ASSERT_EQ(read(item, data), std::errc{});
    EXPECT_EQ(item.a.value, 1);
}

TEST(test_pb_unknown_fields, reserved_field_number_is_skipped)
{
    // field 1 (reserved), length delimited, carrying two bytes that would
    // read as "field 2 = 99" if the payload were mistaken for a tag stream,
    // then the real field 2 = 7.
    std::vector<std::byte> data{std::byte{0x0a},
                                std::byte{0x02},
                                std::byte{0x10},
                                std::byte{0x63},
                                std::byte{0x10},
                                std::byte{0x07}};

    reserved_field item;
    ASSERT_EQ(read(item, data), std::errc{});
    EXPECT_EQ(item.b.value, 7);
}

TEST(test_pb_unknown_fields, unknown_field_running_past_the_end_is_rejected)
{
    // field 2, length delimited, claiming 100 bytes that are not there.
    std::vector<std::byte> data{
        std::byte{0x12}, std::byte{0x64}, std::byte{0x41}, std::byte{0x41}};

    one_field item;
    EXPECT_EQ(read(item, data), std::errc::result_out_of_range);
}

TEST(test_pb_unknown_fields, unknown_varint_running_past_the_end_is_rejected)
{
    // field 2 varint whose continuation bits never terminate.
    std::vector<std::byte> data{
        std::byte{0x10}, std::byte{0x80}, std::byte{0x80}, std::byte{0x80}};

    one_field item;
    EXPECT_EQ(read(item, data), std::errc::result_out_of_range);
}

TEST(test_pb_unknown_fields, unsupported_wire_type_is_rejected)
{
    // field 4 with wire type 3, a deprecated group.
    std::vector<std::byte> data{std::byte{0x23}, std::byte{0x01}};

    one_field item;
    EXPECT_EQ(read(item, data), std::errc::protocol_error);
}

TEST(test_pb_unknown_fields, field_number_zero_is_still_rejected)
{
    std::vector<std::byte> data{std::byte{0x00}, std::byte{0x01}};

    one_field item;
    EXPECT_EQ(read(item, data), std::errc::protocol_error);
}

TEST(test_pb_unknown_fields, known_fields_still_round_trip)
{
    two_fields item{.a = 4321, .b = "round trip"};

    std::vector<std::byte> data;
    zpp::bits::out{data, zpp::bits::size_varint{}}(zpp::bits::unsized(item))
        .or_throw();

    two_fields restored;
    ASSERT_EQ(read(restored, data), std::errc{});
    EXPECT_EQ(restored.a.value, item.a.value);
    EXPECT_EQ(restored.b, item.b);
}

} // namespace test_pb_unknown_fields
