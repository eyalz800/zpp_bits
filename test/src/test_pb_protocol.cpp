#include "test.h"

namespace test_pb_protocol
{

using namespace zpp::bits::literals;

struct example
{
    zpp::bits::vint32_t i; // field number == 1

    constexpr auto operator<=>(const example &) const = default;
};

auto serialize(const example &) -> zpp::bits::protocol<zpp::bits::pb{}>;

static_assert(
    zpp::bits::to_bytes<zpp::bits::unsized_t<example>{{150}}>() ==
    "089601"_decode_hex);

static_assert(zpp::bits::from_bytes<"089601"_decode_hex,
                                    zpp::bits::unsized_t<example>>()
                  .i == 150);

struct nested_example
{
    example nested; // field number == 1
};

auto serialize(const nested_example &)
    -> zpp::bits::protocol<zpp::bits::pb{}>;

static_assert(zpp::bits::to_bytes<zpp::bits::unsized_t<nested_example>{
                  {.nested = example{150}}}>() == "0a03089601"_decode_hex);

static_assert(zpp::bits::from_bytes<"0a03089601"_decode_hex,
                                    zpp::bits::unsized_t<nested_example>>()
                  .nested.i == 150);

struct nested_reserved_example
{
    [[no_unique_address]] zpp::bits::pb::reserved _1{}; // field number == 1
    [[no_unique_address]] zpp::bits::pb::reserved _2{}; // field number == 2
    example nested{};                                   // field number == 3
};

auto serialize(const nested_reserved_example &)
    -> zpp::bits::protocol<zpp::bits::pb{}>;

static_assert(sizeof(nested_reserved_example) == sizeof(example));

static_assert(
    zpp::bits::to_bytes<zpp::bits::unsized_t<nested_reserved_example>{
        {.nested = example{150}}}>() == "1a03089601"_decode_hex);

static_assert(
    zpp::bits::from_bytes<"1a03089601"_decode_hex,
                          zpp::bits::unsized_t<nested_reserved_example>>()
        .nested.i == 150);

struct repeated_integers
{
    using serialize = zpp::bits::protocol<zpp::bits::pb{}>;
    std::vector<zpp::bits::vsint32_t> integers;
};

TEST(test_pb_protocol, test_repeated_integers)
{
    auto [data, in, out] = zpp::bits::data_in_out(zpp::bits::no_size{});
    out(repeated_integers{.integers = {1, 2, 3, 4, -1, -2, -3, -4}})
        .or_throw();

    repeated_integers r;
    in(r).or_throw();

    EXPECT_EQ(
        r.integers,
        (std::vector<zpp::bits::vsint32_t>{1, 2, 3, 4, -1, -2, -3, -4}));
}

struct repeated_examples
{
    using serialize = zpp::bits::protocol<zpp::bits::pb{}>;
    std::vector<example> examples;
};

TEST(test_pb_protocol, test_repeated_example)
{
    auto [data, in, out] = zpp::bits::data_in_out(zpp::bits::no_size{});
    out(repeated_examples{.examples = {{1}, {2}, {3}, {4}, {-1}, {-2}, {-3}, {-4}}})
        .or_throw();

    repeated_examples r;
    in(r).or_throw();

    EXPECT_EQ(r.examples,
              (std::vector<example>{
                  {1}, {2}, {3}, {4}, {-1}, {-2}, {-3}, {-4}}));
}

struct monster {
    enum color
    {
        red,
        blue,
        green
    };

    struct vec3
    {
        using serialize = zpp::bits::protocol<zpp::bits::pb{}>;
        float x;
        float y;
        float z;

        bool operator==(const vec3 &) const = default;
    };

    struct weapon
    {
        using serialize = zpp::bits::protocol<zpp::bits::pb{}>;
        std::string name;
        int damage;

        bool operator==(const weapon &) const = default;
    };

    using serialize = zpp::bits::protocol<zpp::bits::pb{}>;

    vec3 pos;
    zpp::bits::vint32_t mana;
    int hp;
    std::string name;
    std::vector<std::uint8_t> inventory;
    color color;
    std::vector<weapon> weapons;
    weapon equipped;
    std::vector<vec3> path;
    bool boss;

    bool operator==(const monster &) const = default;
};

TEST(test_pb_protocol, test_monster)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    monster m = {.pos = {1.0, 2.0, 3.0},
                 .mana = 200,
                 .hp = 1000,
                 .name = "mushroom",
                 .inventory = {1, 2, 3},
                 .color = monster::color::blue,
                 .weapons =
                     {
                         monster::weapon{.name = "sword", .damage = 55},
                         monster::weapon{.name = "spear", .damage = 150},
                     },
                 .equipped =
                     {
                         monster::weapon{.name = "none", .damage = 15},
                     },
                 .path = {monster::vec3{2.0, 3.0, 4.0},
                          monster::vec3{5.0, 6.0, 7.0}},
                 .boss = true};
    out(m).or_throw();

    monster m2;
    in(m2).or_throw();

    EXPECT_EQ(m.pos, m2.pos);
    EXPECT_EQ(m.mana, m2.mana);
    EXPECT_EQ(m.hp, m2.hp);
    EXPECT_EQ(m.name, m2.name);
    EXPECT_EQ(m.inventory, m2.inventory);
    EXPECT_EQ(m.color, m2.color);
    EXPECT_EQ(m.weapons, m2.weapons);
    EXPECT_EQ(m.equipped, m2.equipped);
    EXPECT_EQ(m.path, m2.path);
    EXPECT_EQ(m.boss, m2.boss);
    EXPECT_EQ(m, m2);
}

TEST(test_pb_protocol, test_monster_unsized)
{
    auto [data, in, out] = zpp::bits::data_in_out(zpp::bits::no_size{});
    monster m = {.pos = {1.0, 2.0, 3.0},
                 .mana = 200,
                 .hp = 1000,
                 .name = "mushroom",
                 .inventory = {1, 2, 3},
                 .color = monster::color::blue,
                 .weapons =
                     {
                         monster::weapon{.name = "sword", .damage = 55},
                         monster::weapon{.name = "spear", .damage = 150},
                     },
                 .equipped =
                     {
                         monster::weapon{.name = "none", .damage = 15},
                     },
                 .path = {monster::vec3{2.0, 3.0, 4.0},
                          monster::vec3{5.0, 6.0, 7.0}},
                 .boss = true};
    out(m).or_throw();

    monster m2;
    in(m2).or_throw();

    EXPECT_EQ(m.pos, m2.pos);
    EXPECT_EQ(m.mana, m2.mana);
    EXPECT_EQ(m.hp, m2.hp);
    EXPECT_EQ(m.name, m2.name);
    EXPECT_EQ(m.inventory, m2.inventory);
    EXPECT_EQ(m.color, m2.color);
    EXPECT_EQ(m.weapons, m2.weapons);
    EXPECT_EQ(m.equipped, m2.equipped);
    EXPECT_EQ(m.path, m2.path);
    EXPECT_EQ(m.boss, m2.boss);
    EXPECT_EQ(m, m2);
}

struct person
{
    std::string name; // = 1
    zpp::bits::vint32_t id; // = 2
    std::string email; // = 3

    enum phone_type
    {
        mobile = 0,
        home = 1,
        work = 2,
    };

    struct phone_number
    {
        std::string number; // = 1
        phone_type type; // = 2
    };

    std::vector<phone_number> phones; // = 4
};

struct address_book
{
    std::vector<person> people; // = 1
};

auto serialize(const person &) -> zpp::bits::protocol<zpp::bits::pb{}>;
auto serialize(const person::phone_number &) -> zpp::bits::protocol<zpp::bits::pb{}>;
auto serialize(const address_book &) -> zpp::bits::protocol<zpp::bits::pb{}>;

TEST(test_pb_protocol, person)
{
    constexpr auto data =
        "\n\x08John Doe\x10\xd2\t\x1a\x10jdoe@example.com\"\x0c\n\x08"
        "555-4321\x10\x01"_b;
    static_assert(data.size() == 45);

    person p;
    zpp::bits::in{data, zpp::bits::no_size{}}(p).or_throw();

    EXPECT_EQ(p.name, "John Doe");
    EXPECT_EQ(p.id, 1234);
    EXPECT_EQ(p.email, "jdoe@example.com");
    ASSERT_EQ(p.phones.size(), 1u);
    EXPECT_EQ(p.phones[0].number, "555-4321");
    EXPECT_EQ(p.phones[0].type, person::home);
}

} // namespace test_pb_protocol
