#include "test.h"
#include <cstddef>
#include <cstdlib>
#include <deque>
#include <new>
#include <string>
#include <variant>
#include <vector>

namespace test_hardening
{

// An allocator that records the largest allocation it was ever asked for, so
// that a test can tell "rejected the input before allocating" apart from
// "allocated first, failed afterwards".
inline std::size_t largest_request = 0;

inline void reset_largest_request()
{
    largest_request = 0;
}

template <typename Type>
struct recording_allocator
{
    using value_type = Type;

    recording_allocator() = default;

    template <typename Other>
    constexpr recording_allocator(const recording_allocator<Other> &)
    {
    }

    Type * allocate(std::size_t count)
    {
        auto bytes = count * sizeof(Type);
        if (bytes > largest_request) {
            largest_request = bytes;
        }
        // Refuse anything absurd rather than actually committing it, so a
        // regression fails the test instead of taking the machine down.
        if (bytes > (std::size_t{1} << 30)) {
            throw std::bad_alloc{};
        }
        auto pointer = std::malloc(bytes ? bytes : 1);
        if (!pointer) {
            throw std::bad_alloc{};
        }
        return static_cast<Type *>(pointer);
    }

    void deallocate(Type * pointer, std::size_t)
    {
        std::free(pointer);
    }

    template <typename Other>
    constexpr bool operator==(const recording_allocator<Other> &) const
    {
        return true;
    }
};

using recording_string =
    std::basic_string<char, std::char_traits<char>, recording_allocator<char>>;

template <typename Type>
using recording_vector = std::vector<Type, recording_allocator<Type>>;

// A protobuf message with a repeated varint field. Repeated varints take the
// reserve() path of the protobuf field reader.
struct pb_repeated_varint
{
    recording_vector<zpp::bits::vint32_t> values;

    using serialize = zpp::bits::pb_protocol;
};

// A protobuf message with a length delimited string field, which takes the
// resize() path of the protobuf field reader.
struct pb_string
{
    recording_string value;

    using serialize = zpp::bits::pb_protocol;
};

// A repeated field held in a container without reserve(), which reaches the
// end position calculation of the protobuf field reader directly.
struct pb_repeated_deque
{
    std::deque<zpp::bits::vint32_t> values;
    zpp::bits::vint32_t tail;

    using serialize = zpp::bits::pb_protocol;
};

// Field 1, wire type 2 (length delimited), followed by a varint length of
// 0xffffffff, and no payload at all.
inline auto oversized_length_delimited_field()
{
    return std::vector<std::byte>{std::byte{0x0a},
                                  std::byte{0xff},
                                  std::byte{0xff},
                                  std::byte{0xff},
                                  std::byte{0xff},
                                  std::byte{0x0f}};
}

TEST(test_hardening, pb_repeated_length_beyond_alloc_limit)
{
    auto data = oversized_length_delimited_field();

    pb_repeated_varint item;
    reset_largest_request();
    zpp::bits::in in{
        data, zpp::bits::size_varint{}, zpp::bits::alloc_limit<128>{}};

    EXPECT_EQ(in(zpp::bits::unsized(item)), std::errc::message_size);
    EXPECT_LE(largest_request, std::size_t{128});
}

TEST(test_hardening, pb_string_length_beyond_alloc_limit)
{
    auto data = oversized_length_delimited_field();

    pb_string item;
    reset_largest_request();
    zpp::bits::in in{
        data, zpp::bits::size_varint{}, zpp::bits::alloc_limit<128>{}};

    EXPECT_EQ(in(zpp::bits::unsized(item)), std::errc::message_size);
    EXPECT_LE(largest_request, std::size_t{128});
}

TEST(test_hardening, pb_repeated_deque_still_round_trips)
{
    pb_repeated_deque item{.values = {1, 2, 300}, .tail = 42};

    std::vector<std::byte> data;
    zpp::bits::out out{data, zpp::bits::size_varint{}};
    out(zpp::bits::unsized(item)).or_throw();

    pb_repeated_deque restored;
    zpp::bits::in in{data, zpp::bits::size_varint{}};
    in(zpp::bits::unsized(restored)).or_throw();

    ASSERT_EQ(restored.values.size(), item.values.size());
    for (std::size_t index = 0; index < item.values.size(); ++index) {
        EXPECT_EQ(restored.values[index].value, item.values[index].value);
    }
    EXPECT_EQ(restored.tail.value, 42);
}

TEST(test_hardening, pb_length_delimited_still_round_trips)
{
    pb_repeated_varint item{.values = {1, 2, 300, 40000}};

    std::vector<std::byte> data;
    zpp::bits::out out{data, zpp::bits::size_varint{}};
    out(zpp::bits::unsized(item)).or_throw();

    pb_repeated_varint restored;
    zpp::bits::in in{data, zpp::bits::size_varint{}};
    in(zpp::bits::unsized(restored)).or_throw();

    ASSERT_EQ(restored.values.size(), item.values.size());
    for (std::size_t index = 0; index < item.values.size(); ++index) {
        EXPECT_EQ(restored.values[index].value, item.values[index].value);
    }
}

TEST(test_hardening, pb_string_still_round_trips)
{
    pb_string item{.value = "hardening"};

    std::vector<std::byte> data;
    zpp::bits::out out{data, zpp::bits::size_varint{}};
    out(zpp::bits::unsized(item)).or_throw();

    pb_string restored;
    zpp::bits::in in{data, zpp::bits::size_varint{}};
    in(zpp::bits::unsized(restored)).or_throw();

    EXPECT_EQ(restored.value, item.value);
}

TEST(test_hardening, variant_rejects_unknown_id)
{
    using variant_type = std::variant<int, std::string, double>;

    // Serialize a valid variant, then corrupt its one byte identifier.
    std::vector<std::byte> data;
    zpp::bits::out{data}(variant_type{42}).or_throw();
    ASSERT_FALSE(data.empty());
    data[0] = std::byte{0x7f};

    variant_type restored;
    zpp::bits::in in{data};
    EXPECT_EQ(in(restored), std::errc::bad_message);
}

TEST(test_hardening, variant_accepts_last_alternative)
{
    using variant_type = std::variant<int, std::string, double>;

    std::vector<std::byte> data;
    zpp::bits::out{data}(variant_type{2.5}).or_throw();

    variant_type restored;
    zpp::bits::in in{data};
    in(restored).or_throw();

    ASSERT_EQ(restored.index(), 2u);
    EXPECT_EQ(std::get<2>(restored), 2.5);
}

} // namespace test_hardening
