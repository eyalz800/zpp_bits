#include "test.h"

// Reproduction test for issue #207:
// "optional_ptr and gcc 16.1.0"
//
// Root cause: types with explicit archive-based serializers (optional_ptr,
// types with private fields + custom serialize) are not covered by the
// inspection_guarded concept. On GCC 16 + C++26, this causes the library to
// attempt structured binding decomposition of those types, failing to compile.
//
// The fix adds has_explicit_serialize<Type> to inspection_guarded.
// These static_asserts verify that invariant holds — they FAIL on unfixed
// code and PASS after the fix, across all compilers and C++ standard versions.

namespace test_issue_207_repro
{

// optional_ptr<T> has a free-function serialize(archive, optional_ptr<T>).
// It must be inspection_guarded so the structured binding path is never taken.
static_assert(zpp::bits::concepts::has_explicit_serialize<zpp::bits::optional_ptr<int>>,
    "optional_ptr must have an explicit serializer detected");

static_assert(zpp::bits::concepts::inspection_guarded<zpp::bits::optional_ptr<int>>,
    "optional_ptr must be inspection_guarded (issue #207: unfixed code fails this)");

// A type with private fields and a friend archive serializer (second case from #207).
class private_custom_serialize
{
public:
    private_custom_serialize() = default;
    private_custom_serialize(int a, int b) : a_(a), b_(b) {}

    template <typename Archive>
    friend auto serialize(Archive & archive, private_custom_serialize & self)
    {
        return archive(self.a_, self.b_);
    }

    template <typename Archive>
    friend auto serialize(Archive & archive, const private_custom_serialize & self)
    {
        return archive(self.a_, self.b_);
    }

private:
    int a_{};
    int b_{};
};

static_assert(zpp::bits::concepts::has_explicit_serialize<private_custom_serialize>,
    "type with friend archive serializer must be detected as has_explicit_serialize");

static_assert(zpp::bits::concepts::inspection_guarded<private_custom_serialize>,
    "type with private fields + explicit serializer must be inspection_guarded "
    "(without fix, structured binding decomposition of private members is attempted)");

// A type with a static member archive serializer.
class static_custom_serialize
{
public:
    static_custom_serialize() = default;
    explicit static_custom_serialize(int v) : v_(v) {}

    template <typename Archive>
    static auto serialize(Archive & archive, static_custom_serialize & self)
    {
        return archive(self.v_);
    }

    template <typename Archive>
    static auto serialize(Archive & archive, const static_custom_serialize & self)
    {
        return archive(self.v_);
    }

private:
    int v_{};
};

static_assert(zpp::bits::concepts::has_explicit_serialize<static_custom_serialize>,
    "type with static member archive serializer must be detected as has_explicit_serialize");

static_assert(zpp::bits::concepts::inspection_guarded<static_custom_serialize>,
    "type with static member archive serializer must be inspection_guarded");

// Sanity: plain aggregates must NOT be inspection_guarded.
// If this fires, the fix over-guards and would break member auto-detection.
struct plain_aggregate { int x; int y; };
static_assert(!zpp::bits::concepts::inspection_guarded<plain_aggregate>,
    "plain aggregates must NOT be inspection_guarded (member detection must still work)");

// Runtime test: ensure optional_ptr still serializes correctly after the fix.
TEST(issue_207_repro, optional_ptr_roundtrip_valid)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(zpp::bits::optional_ptr<int>{std::make_unique<int>(0x1337)}).or_throw();

    zpp::bits::optional_ptr<int> result;
    in(result).or_throw();

    ASSERT_TRUE(result != nullptr);
    EXPECT_EQ(*result, 0x1337);
}

TEST(issue_207_repro, optional_ptr_roundtrip_null)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(zpp::bits::optional_ptr<int>{}).or_throw();

    zpp::bits::optional_ptr<int> result{std::make_unique<int>(99)};
    in(result).or_throw();

    EXPECT_TRUE(result == nullptr);
}

} // namespace test_issue_207_repro
