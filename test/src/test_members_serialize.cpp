#include "test.h"

namespace test_members_serialize
{
struct person
{
    using serialize = zpp::bits::members<2>;
    std::string name;
    int age{};
};

TEST(members_serialize, sanity)
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

class private_person
{
    friend zpp::bits::access;
    using serialize = zpp::bits::members<2>;
    std::string name;
    int age{};

    friend void private_sanity_test();
};

void private_sanity_test()
{
    auto [data, in, out] = zpp::bits::data_in_out();
    private_person op1, op2;
    op1.name = "Person1";
    op1.age = 25;
    op2.name = "Person2";
    op2.age = 35;

    out(op1, op2).or_throw();

    private_person p1, p2;

    in(p1, p2).or_throw();

    EXPECT_EQ(p1.name, "Person1");
    EXPECT_EQ(p1.age, 25);

    EXPECT_EQ(p2.name, "Person2");
    EXPECT_EQ(p2.age, 35);
}

TEST(members_serialize, private_sanity)
{
    private_sanity_test();
}

struct outside_person
{
    std::string name;
    int age{};
};

auto serialize(const outside_person &) -> zpp::bits::members<2>;

TEST(members_serialize, outside)
{
    auto [data, in, out] = zpp::bits::data_in_out();

    out(outside_person{"Person1", 25}, outside_person{"Person2", 35})
        .or_throw();

    outside_person p1, p2;

    in(p1, p2).or_throw();

    EXPECT_EQ(p1.name, "Person1");
    EXPECT_EQ(p1.age, 25);

    EXPECT_EQ(p2.name, "Person2");
    EXPECT_EQ(p2.age, 35);
}

struct point
{
    using serialize = zpp::bits::members<2>;

    int x{};
    int y{};
};

TEST(members_serialize, point)
{
    auto [data, in, out] = zpp::bits::data_in_out();

    out(point{1337, 1338}, point{1339, 1340}).or_throw();

    point p1, p2;

    in(p1, p2).or_throw();

    EXPECT_EQ(p1.x, 1337);
    EXPECT_EQ(p1.y, 1338);

    EXPECT_EQ(p2.x, 1339);
    EXPECT_EQ(p2.y, 1340);
}

struct number
{
    using serialize = zpp::bits::members<1>;

    int x{};
};

TEST(members_serialize, number)
{
    auto [data, in, out] = zpp::bits::data_in_out();

    out(number{1337}, number{1338}).or_throw();

    number n1, n2;

    in(n1, n2).or_throw();

    EXPECT_EQ(n1.x, 1337);
    EXPECT_EQ(n2.x, 1338);
}

} // namespace test_members_serialize
