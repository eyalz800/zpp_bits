#include "test.h"
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

namespace test_alloc_limit
{

TEST(test_alloc_limit, output)
{
    constexpr auto limit = 128;
    auto [data, out] = data_out(zpp::bits::alloc_limit<limit>());
    static_assert(decltype(out)::allocation_limit == limit);

    out(1,2,3,4).or_throw();
    EXPECT_EQ((out(std::array<int, 50>{})), std::errc::no_buffer_space);
}

TEST(test_alloc_limit, input)
{
    constexpr auto limit = 128;
    auto [data, in] = data_in(zpp::bits::alloc_limit<limit>());
    static_assert(decltype(in)::allocation_limit == limit);

    std::vector<std::uint32_t> vi;
    std::vector<std::byte> vb;
    zpp::bits::out{data}(std::vector<std::uint32_t>{1, 2, 3, 4})
        .or_throw();
    in(vi).or_throw();
    in.reset();

    zpp::bits::out{data}(std::vector<std::uint32_t>(limit / sizeof(std::uint32_t)))
        .or_throw();
    in(vi).or_throw();
    in.reset();

    zpp::bits::out{data}(std::vector<std::uint32_t>(limit / sizeof(std::uint32_t) + 1))
        .or_throw();
    EXPECT_EQ((in(vi)), std::errc::message_size);
    in.reset();

    zpp::bits::out{data}(std::vector<std::byte>(limit)).or_throw();
    in(vb).or_throw();
    in.reset();

    zpp::bits::out{data}(std::vector<std::byte>(limit + 1)).or_throw();
    EXPECT_EQ((in(vb)), std::errc::message_size);
    in.reset();

    zpp::bits::out{data}(std::vector<std::byte>(limit / 4),
                         std::vector<std::byte>(limit / 4),
                         std::vector<std::byte>(limit / 4),
                         std::vector<std::byte>(limit / 4))
        .or_throw();
    in(vb).or_throw();
    in(vb).or_throw();
    in(vb).or_throw();
    in(vb).or_throw();
    in.reset();
}

// Associative containers read an element count from the input and then insert
// that many elements one by one. That count needs to respect the allocation
// limit the same way a resizable container's size prefix does.
template <typename Container>
static auto encode(const Container & container)
{
    std::vector<std::byte> data;
    zpp::bits::out{data}(container).or_throw();
    return data;
}

TEST(test_alloc_limit, input_map)
{
    constexpr auto limit = 128;
    using map_type = std::map<std::uint32_t, std::uint32_t>;
    constexpr auto max_entries = limit / sizeof(map_type::value_type);

    map_type at_limit;
    for (std::uint32_t index = 0; index < max_entries; ++index) {
        at_limit.emplace(index, index);
    }
    auto over_limit = at_limit;
    over_limit.emplace(std::uint32_t(max_entries), 0u);

    map_type restored;

    auto within = encode(at_limit);
    zpp::bits::in{within, zpp::bits::alloc_limit<limit>()}(restored)
        .or_throw();
    EXPECT_EQ(restored.size(), max_entries);

    auto beyond = encode(over_limit);
    EXPECT_EQ((zpp::bits::in{beyond, zpp::bits::alloc_limit<limit>()}(
                  restored)),
              std::errc::message_size);
}

TEST(test_alloc_limit, input_set)
{
    constexpr auto limit = 128;
    using set_type = std::set<std::uint32_t>;
    constexpr auto max_entries = limit / sizeof(set_type::value_type);

    set_type at_limit;
    for (std::uint32_t index = 0; index < max_entries; ++index) {
        at_limit.insert(index);
    }
    auto over_limit = at_limit;
    over_limit.insert(std::uint32_t(max_entries));

    set_type restored;

    auto within = encode(at_limit);
    zpp::bits::in{within, zpp::bits::alloc_limit<limit>()}(restored)
        .or_throw();
    EXPECT_EQ(restored.size(), max_entries);

    auto beyond = encode(over_limit);
    EXPECT_EQ((zpp::bits::in{beyond, zpp::bits::alloc_limit<limit>()}(
                  restored)),
              std::errc::message_size);
}

TEST(test_alloc_limit, input_unordered_map)
{
    constexpr auto limit = 128;
    using map_type = std::unordered_map<std::uint32_t, std::uint32_t>;
    constexpr auto max_entries = limit / sizeof(map_type::value_type);

    map_type at_limit;
    for (std::uint32_t index = 0; index < max_entries; ++index) {
        at_limit.emplace(index, index);
    }
    auto over_limit = at_limit;
    over_limit.emplace(std::uint32_t(max_entries), 0u);

    map_type restored;

    auto within = encode(at_limit);
    zpp::bits::in{within, zpp::bits::alloc_limit<limit>()}(restored)
        .or_throw();
    EXPECT_EQ(restored.size(), max_entries);

    auto beyond = encode(over_limit);
    EXPECT_EQ((zpp::bits::in{beyond, zpp::bits::alloc_limit<limit>()}(
                  restored)),
              std::errc::message_size);
}

TEST(test_alloc_limit, input_unordered_set)
{
    constexpr auto limit = 128;
    using set_type = std::unordered_set<std::uint32_t>;
    constexpr auto max_entries = limit / sizeof(set_type::value_type);

    set_type at_limit;
    for (std::uint32_t index = 0; index < max_entries; ++index) {
        at_limit.insert(index);
    }
    auto over_limit = at_limit;
    over_limit.insert(std::uint32_t(max_entries));

    set_type restored;

    auto within = encode(at_limit);
    zpp::bits::in{within, zpp::bits::alloc_limit<limit>()}(restored)
        .or_throw();
    EXPECT_EQ(restored.size(), max_entries);

    auto beyond = encode(over_limit);
    EXPECT_EQ((zpp::bits::in{beyond, zpp::bits::alloc_limit<limit>()}(
                  restored)),
              std::errc::message_size);
}

TEST(test_alloc_limit, input_associative_without_limit_is_unchanged)
{
    std::map<std::uint32_t, std::uint32_t> reference;
    for (std::uint32_t index = 0; index < 1000; ++index) {
        reference.emplace(index, index);
    }

    auto data = encode(reference);

    std::map<std::uint32_t, std::uint32_t> restored;
    zpp::bits::in{data}(restored).or_throw();
    EXPECT_EQ(restored, reference);
}

} // namespace test_alloc_limit
