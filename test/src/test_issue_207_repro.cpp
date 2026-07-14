#include "test.h"
#include <mutex>
#include <shared_mutex>
#include <memory>

// Reproduction of issue #207: "optional_ptr and gcc 16.1.0"
// Source: https://godbolt.org/z/aG1dahGqE
//
// The person struct uses an explicit archive-based serialize to control which
// fields are serialized (excluding std::mutex, std::shared_mutex, etc.).
// It also holds zpp::bits::optional_ptr<person> members.
//
// On GCC 16.1 with C++26 (__cpp_structured_bindings >= 202411L), the library
// attempts structured binding decomposition of optional_ptr<person> inside
// number_of_members<optional_ptr<person>>() because optional_ptr is not
// covered by inspection_guarded. This fails to compile since optional_ptr
// (derived from unique_ptr) is neither an aggregate nor tuple-like.

namespace test_issue_207_repro
{

struct person
{
    constexpr static auto serialize(auto & archive, auto & self)
    {
        return archive(self.name, self.children, self.grandchildren);
    }

    std::string                                           name;
    std::mutex                                            m_name;
    std::vector<std::optional<std::unique_ptr<person>>>   children;
    std::vector<zpp::bits::optional_ptr<person>>          grandchildren;
    std::shared_mutex                                     m_children;
    std::shared_ptr<std::string>                          dog;
};

TEST(issue_207_repro, person_roundtrip)
{
    person p;
    p.name = "bill";
    p.children.push_back(std::make_unique<person>());
    p.children.back().value()->name = "sam";
    p.grandchildren.push_back(std::make_unique<person>());
    p.grandchildren.back()->name = "grace";
    p.dog = std::make_shared<std::string>("growler");

    std::vector<std::byte> data;
    zpp::bits::out out{data};
    out(p).or_throw();

    person p2;
    zpp::bits::in in{data};
    in(p2).or_throw();

    EXPECT_EQ(p2.name, "bill");
    ASSERT_EQ(p2.children.size(), 1u);
    ASSERT_TRUE(p2.children[0].has_value());
    EXPECT_EQ((*p2.children[0])->name, "sam");
    ASSERT_EQ(p2.grandchildren.size(), 1u);
    EXPECT_EQ(p2.grandchildren[0]->name, "grace");
}

TEST(issue_207_repro, optional_ptr_member_null_and_nonnull)
{
    person p;
    p.name = "alice";
    p.grandchildren.push_back(nullptr);
    p.grandchildren.push_back(std::make_unique<person>());
    p.grandchildren.back()->name = "bob";

    std::vector<std::byte> data;
    zpp::bits::out out{data};
    out(p).or_throw();

    person p2;
    zpp::bits::in in{data};
    in(p2).or_throw();

    EXPECT_EQ(p2.name, "alice");
    ASSERT_EQ(p2.grandchildren.size(), 2u);
    EXPECT_TRUE(p2.grandchildren[0] == nullptr);
    ASSERT_TRUE(p2.grandchildren[1] != nullptr);
    EXPECT_EQ(p2.grandchildren[1]->name, "bob");
}

} // namespace test_issue_207_repro
