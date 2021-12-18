#include "test.h"

#if ZPP_BITS_AUTODETECT_MEMBERS_MODE > 0

namespace test_detect_members_serialize
{
struct person
{
    std::string name;
    int age{};
};

TEST(detect_members_serialize, sanity)
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

TEST(detect_members_serialize, private_sanity)
{
    private_sanity_test();
}

struct point
{
    using serialize = zpp::bits::members<2>;

    int x{};
    int y{};
};

TEST(detect_members_serialize, point)
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
    int x{};
};

TEST(detect_members_serialize, number)
{
    auto [data, in, out] = zpp::bits::data_in_out();

    out(number{1337}, number{1338}).or_throw();

    number n1, n2;

    in(n1, n2).or_throw();

    EXPECT_EQ(n1.x, 1337);
    EXPECT_EQ(n2.x, 1338);
}

} // namespace test_detect_members_serialize

#endif
