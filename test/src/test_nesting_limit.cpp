#include "test.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace test_nesting_limit
{

// The canonical self referencing shape: a node that holds more of itself.
// Every level is one more level of nesting in the encoded message, and one
// more stack frame while deserializing.
struct tree
{
    std::uint8_t value{};
    std::vector<tree> children;
};

// A `unique_ptr` member is always present rather than optional, so a chain of
// these never terminates - deserializing one recurses until the input runs
// out. Two bytes of input buy one more level of recursion.
struct chain
{
    std::uint8_t value{};
    std::unique_ptr<chain> next;
};

// Protobuf messages nest through a fresh archive per level rather than by
// recursing on one archive, so they exercise a different path.
//
// Protobuf itself allows a message to reference itself, but a self
// referencing message does not currently compile here ("deduced return type
// cannot be used before it is defined"), so the nesting below is static.
struct pb_leaf
{
    zpp::bits::vint32_t value;

    using serialize = zpp::bits::pb_protocol;
};

struct pb_middle
{
    pb_leaf leaf;

    using serialize = zpp::bits::pb_protocol;
};

struct pb_root
{
    pb_middle middle;

    using serialize = zpp::bits::pb_protocol;
};

static_assert(zpp::bits::concepts::self_referencing<tree>);
static_assert(zpp::bits::concepts::self_referencing<chain>);

// Builds a tree that is `depth` levels deep, counting the root as level one.
template <typename Type>
static Type nested(std::size_t depth)
{
    Type root;
    auto current = &root;
    for (std::size_t level = 1; level < depth; ++level) {
        current->children.resize(1);
        current = &current->children.front();
    }
    return root;
}

template <typename Type>
static std::size_t depth_of(const Type & root)
{
    std::size_t depth = 1;
    for (auto current = &root; !current->children.empty();
         current = &current->children.front()) {
        ++depth;
    }
    return depth;
}

TEST(test_nesting_limit, costs_nothing_when_not_requested)
{
    using archive = zpp::bits::in<std::span<const std::byte>>;

    static_assert(sizeof(archive) ==
                  sizeof(std::span<const std::byte>) +
                      sizeof(std::size_t));
}

TEST(test_nesting_limit, within_limit_is_accepted)
{
    constexpr auto limit = 4;

    std::vector<std::byte> data;
    zpp::bits::out{data}(nested<tree>(limit)).or_throw();

    tree restored;
    zpp::bits::in in{data, zpp::bits::nesting_limit<limit>()};
    static_assert(decltype(in)::nesting_depth_limit == limit);

    in(restored).or_throw();
    EXPECT_EQ(depth_of(restored), std::size_t(limit));
}

TEST(test_nesting_limit, beyond_limit_is_rejected)
{
    constexpr auto limit = 4;

    std::vector<std::byte> data;
    zpp::bits::out{data}(nested<tree>(limit + 1)).or_throw();

    tree restored;
    zpp::bits::in in{data, zpp::bits::nesting_limit<limit>()};

    EXPECT_EQ(in(restored), std::errc::value_too_large);
}

TEST(test_nesting_limit, output_beyond_limit_is_rejected)
{
    constexpr auto limit = 3;

    auto deep = nested<tree>(limit + 1);

    std::vector<std::byte> data;
    zpp::bits::out out{data, zpp::bits::nesting_limit<limit>()};
    static_assert(decltype(out)::nesting_depth_limit == limit);

    EXPECT_EQ(out(deep), std::errc::value_too_large);
}

TEST(test_nesting_limit, output_within_limit_is_accepted)
{
    constexpr auto limit = 4;

    auto deep = nested<tree>(limit);

    std::vector<std::byte> data;
    zpp::bits::out{data, zpp::bits::nesting_limit<limit>()}(deep).or_throw();
    EXPECT_FALSE(data.empty());
}

// `pb_root` nests three messages deep: root, middle, leaf.
static auto encoded_pb_root()
{
    std::vector<std::byte> data;
    zpp::bits::out{data, zpp::bits::size_varint{}}(
        zpp::bits::unsized(pb_root{.middle = {.leaf = {150}}}))
        .or_throw();
    return data;
}

TEST(test_nesting_limit, protobuf_beyond_limit_is_rejected)
{
    auto data = encoded_pb_root();

    pb_root restored;
    zpp::bits::in in{data,
                     zpp::bits::size_varint{},
                     zpp::bits::nesting_limit<2>()};

    EXPECT_EQ(in(zpp::bits::unsized(restored)),
              std::errc::value_too_large);
}

TEST(test_nesting_limit, protobuf_within_limit_is_accepted)
{
    auto data = encoded_pb_root();

    pb_root restored;
    zpp::bits::in in{data,
                     zpp::bits::size_varint{},
                     zpp::bits::nesting_limit<3>()};

    in(zpp::bits::unsized(restored)).or_throw();
    EXPECT_EQ(restored.middle.leaf.value.value, 150);
}

TEST(test_nesting_limit, protobuf_output_beyond_limit_is_rejected)
{
    std::vector<std::byte> data;
    zpp::bits::out out{
        data, zpp::bits::size_varint{}, zpp::bits::nesting_limit<2>()};

    EXPECT_EQ(out(zpp::bits::unsized(pb_root{.middle = {.leaf = {150}}})),
              std::errc::value_too_large);
}

TEST(test_nesting_limit, deeply_nested_input_is_rejected_without_recursing)
{
    // Without a limit this input recurses roughly 100000 levels deep and
    // exhausts the stack. Two bytes buy one more level.
    std::vector<std::byte> data(200000, std::byte{0x01});

    chain head;
    zpp::bits::in in{data, zpp::bits::nesting_limit<128>()};

    EXPECT_EQ(in(head), std::errc::value_too_large);
}

TEST(test_nesting_limit, unlimited_by_default)
{
    constexpr auto depth = 512;

    std::vector<std::byte> data;
    zpp::bits::out{data}(nested<tree>(depth)).or_throw();

    tree restored;
    zpp::bits::in{data}(restored).or_throw();

    EXPECT_EQ(depth_of(restored), std::size_t(depth));
}

TEST(test_nesting_limit, non_nesting_types_are_unaffected)
{
    // Deeply nested containers are not self referencing, so the limit does
    // not apply to them however deep the type is.
    constexpr auto limit = 2;

    std::vector<std::vector<std::vector<std::uint32_t>>> deep{
        {{1, 2}, {3}}, {{4, 5, 6}}};

    std::vector<std::byte> data;
    zpp::bits::out{data, zpp::bits::nesting_limit<limit>()}(deep).or_throw();

    decltype(deep) restored;
    zpp::bits::in{data, zpp::bits::nesting_limit<limit>()}(restored)
        .or_throw();

    EXPECT_EQ(restored, deep);
}

} // namespace test_nesting_limit
