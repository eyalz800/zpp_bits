#include "test.h"

namespace test_sanity
{
struct person
{
    constexpr static auto serialize(auto & archive, auto & self)
    {
        return archive(self.name, self.age);
    }

    std::string name;
    int age{};
};

TEST(sanity, exception)
{
    auto [data, in, out] = zpp::bits::data_in_out();

    out(person{"Person1", 25}, person{"Person2", 35}).or_throw();

    person p1, p2;

    in(p1, p2).or_throw();

    EXPECT_EQ(p1.name, "Person1");
    EXPECT_EQ(p1.age, 25);

    EXPECT_EQ(p2.name, "Person2");
    EXPECT_EQ(p2.age, 35);
}

TEST(sanity, return_value)
{
    auto [data, in, out] = zpp::bits::data_in_out();

    auto result = out(person{"Person1", 25}, person{"Person2", 35});
    if (failure(result)) {
        FAIL();
    }

    person p1, p2;

    result = in(p1, p2);
    if (failure(result)) {
        FAIL();
    }

    EXPECT_EQ(p1.name, "Person1");
    EXPECT_EQ(p1.age, 25);

    EXPECT_EQ(p2.name, "Person2");
    EXPECT_EQ(p2.age, 35);
}

#if __has_include("zpp_throwing.h")
TEST(sanity, zpp_throwing)
{
    return zpp::try_catch([&]() -> zpp::throwing<void> {
        auto [data, in, out] = zpp::bits::data_in_out();

        co_await out(person{"Person1", 25}, person{"Person2", 35});

        person p1, p2;

        co_await in(p1, p2);

        EXPECT_EQ(p1.name, "Person1");
        EXPECT_EQ(p1.age, 25);

        EXPECT_EQ(p2.name, "Person2");
        EXPECT_EQ(p2.age, 35);
    }, [&]() {
        FAIL();
    });
}
#endif

struct outside_person
{
    std::string name;
    int age{};
};

constexpr static auto serialize(auto & archive, const outside_person & self)
{
    return archive(self.name, self.age);
}

constexpr static auto serialize(auto & archive, outside_person & self)
{
    return archive(self.name, self.age);
}

TEST(sanity, outside)
{
    auto [data, in, out] = zpp::bits::data_in_out();

    out(person{"Person1", 25}, person{"Person2", 35}).or_throw();

    person p1, p2;

    in(p1, p2).or_throw();

    EXPECT_EQ(p1.name, "Person1");
    EXPECT_EQ(p1.age, 25);

    EXPECT_EQ(p2.name, "Person2");
    EXPECT_EQ(p2.age, 35);
}

} // namespace test_sanity
