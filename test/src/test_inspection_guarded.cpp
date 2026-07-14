#include "test.h"

// Tests for the inspection_guarded concept fix (issue #207).
//
// GCC 16's stricter C++26 structured binding decomposition would attempt to
// decompose types with explicit archive-based serializers (like optional_ptr,
// or types with private fields and custom serialize functions), causing
// compilation failures. The fix adds has_explicit_serialize<Type> to
// inspection_guarded so the structured binding path is never taken for such
// types.
//
// The exhaustive round-trip coverage lives in test_optional_ptr.cpp and
// test_issue_207_repro.cpp; this file focuses on the concept-level guarantees
// and one representative round-trip per distinct type category.

namespace test_inspection_guarded
{

// --- Compile-time concept checks ---

// optional_ptr has an archive-based explicit serializer and must be guarded.
static_assert(zpp::bits::concepts::has_explicit_serialize<zpp::bits::optional_ptr<int>>);
static_assert(zpp::bits::concepts::inspection_guarded<zpp::bits::optional_ptr<int>>);

// std::unique_ptr remains guarded via owning_pointer (unchanged).
static_assert(zpp::bits::concepts::owning_pointer<std::unique_ptr<int>>);
static_assert(zpp::bits::concepts::inspection_guarded<std::unique_ptr<int>>);

// Plain aggregates must NOT be inspection_guarded (structured binding path
// must still apply to ordinary structs).
struct plain_point { int x; int y; };
static_assert(!zpp::bits::concepts::inspection_guarded<plain_point>);

// --- Case 1: optional_ptr as a struct member (the root case from #207) ---
// Using a non-recursive struct to avoid GCC inlining limits on self-referencing types.

struct box
{
    int id{};
    zpp::bits::optional_ptr<int> value;
};

auto serialize(const box &) -> zpp::bits::members<2>;

TEST(inspection_guarded, optional_ptr_member_with_value)
{
    auto [data, in, out] = zpp::bits::data_in_out();

    box b{42, zpp::bits::optional_ptr<int>{std::make_unique<int>(1337)}};
    out(b).or_throw();

    box result;
    in(result).or_throw();

    EXPECT_EQ(result.id, 42);
    ASSERT_TRUE(result.value != nullptr);
    EXPECT_EQ(*result.value, 1337);
}

TEST(inspection_guarded, optional_ptr_member_null)
{
    auto [data, in, out] = zpp::bits::data_in_out();

    box b{7, nullptr};
    out(b).or_throw();

    box result{100, std::make_unique<int>(99)};
    in(result).or_throw();

    EXPECT_EQ(result.id, 7);
    EXPECT_TRUE(result.value == nullptr);
}

// --- Case 2: private fields with a friend free-function archive serializer ---
// (Issue #207 second commenter: these types failed without the fix even when
//  a custom serializer was provided, because inspection_guarded didn't cover
//  them and structured binding decomposition of private members was attempted.)

class private_with_free_serializer
{
public:
    private_with_free_serializer() = default;
    private_with_free_serializer(int x, int y) : x_(x), y_(y) {}

    int x() const { return x_; }
    int y() const { return y_; }

    template <typename Archive>
    friend auto serialize(Archive & archive, private_with_free_serializer & self)
    {
        return archive(self.x_, self.y_);
    }

    template <typename Archive>
    friend auto serialize(Archive & archive,
                          const private_with_free_serializer & self)
    {
        return archive(self.x_, self.y_);
    }

private:
    int x_{};
    int y_{};
};

static_assert(zpp::bits::concepts::has_explicit_serialize<private_with_free_serializer>);
static_assert(zpp::bits::concepts::inspection_guarded<private_with_free_serializer>);

TEST(inspection_guarded, private_fields_free_serializer_roundtrip)
{
    auto [data, in, out] = zpp::bits::data_in_out();

    private_with_free_serializer p{1337, 1338};
    out(p).or_throw();

    private_with_free_serializer result;
    in(result).or_throw();

    EXPECT_EQ(result.x(), 1337);
    EXPECT_EQ(result.y(), 1338);
}

// --- Case 3: private fields with a static member serialize function ---

class private_with_static_serializer
{
public:
    private_with_static_serializer() = default;
    explicit private_with_static_serializer(std::string s, int n)
        : data_(std::move(s)), count_(n)
    {
    }

    const std::string & data() const { return data_; }
    int count() const { return count_; }

    template <typename Archive>
    static auto serialize(Archive & archive, private_with_static_serializer & self)
    {
        return archive(self.data_, self.count_);
    }

    template <typename Archive>
    static auto serialize(Archive & archive,
                          const private_with_static_serializer & self)
    {
        return archive(self.data_, self.count_);
    }

private:
    std::string data_;
    int count_{};
};

static_assert(zpp::bits::concepts::has_explicit_serialize<private_with_static_serializer>);
static_assert(zpp::bits::concepts::inspection_guarded<private_with_static_serializer>);

TEST(inspection_guarded, private_fields_static_serializer_roundtrip)
{
    auto [data, in, out] = zpp::bits::data_in_out();

    private_with_static_serializer s{"hello", 42};
    out(s).or_throw();

    private_with_static_serializer result;
    in(result).or_throw();

    EXPECT_EQ(result.data(), "hello");
    EXPECT_EQ(result.count(), 42);
}

// --- Case 4: optional_ptr<T> where T itself has an explicit serializer ---

class custom_payload
{
public:
    custom_payload() = default;
    explicit custom_payload(int v) : value_(v) {}
    int value() const { return value_; }

    template <typename Archive>
    friend auto serialize(Archive & archive, custom_payload & self)
    {
        return archive(self.value_);
    }

    template <typename Archive>
    friend auto serialize(Archive & archive, const custom_payload & self)
    {
        return archive(self.value_);
    }

private:
    int value_{};
};

TEST(inspection_guarded, optional_ptr_of_explicit_serializer_type)
{
    auto [data, in, out] = zpp::bits::data_in_out();

    zpp::bits::optional_ptr<custom_payload> ptr{std::make_unique<custom_payload>(42)};
    out(ptr).or_throw();

    zpp::bits::optional_ptr<custom_payload> result;
    in(result).or_throw();

    ASSERT_TRUE(result != nullptr);
    EXPECT_EQ(result->value(), 42);
}

// --- Case 5: struct containing a type with explicit serializer as a member ---

struct wrapper
{
    int id{};
    private_with_free_serializer inner;
};

auto serialize(const wrapper &) -> zpp::bits::members<2>;

TEST(inspection_guarded, struct_containing_explicit_serializer_type)
{
    auto [data, in, out] = zpp::bits::data_in_out();

    wrapper w{5, {100, 200}};
    out(w).or_throw();

    wrapper result;
    in(result).or_throw();

    EXPECT_EQ(result.id, 5);
    EXPECT_EQ(result.inner.x(), 100);
    EXPECT_EQ(result.inner.y(), 200);
}

} // namespace test_inspection_guarded
