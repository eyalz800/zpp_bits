#include "test.h"

namespace test_variant
{
using namespace std::literals;
using namespace zpp::bits::literals;

TEST(variant, int)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(std::variant<int, std::string>(0x1337)).or_throw();

    EXPECT_EQ(hexlify(data),
              "00"
              "37130000");

    std::variant<int, std::string> v;
    in(v).or_throw();

    EXPECT_TRUE(std::holds_alternative<int>(v));
    EXPECT_EQ(std::get<int>(v), 0x1337);
}

TEST(variant, string)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(std::variant<int, std::string>("1234")).or_throw();

    EXPECT_EQ(hexlify(data),
              "01"
              "04000000"
              "31323334");

    std::variant<int, std::string> v;
    in(v).or_throw();

    EXPECT_TRUE(std::holds_alternative<std::string>(v));
    EXPECT_EQ(std::get<std::string>(v), "1234");
}

namespace string_versioning::v1
{
struct person
{
    using serialize = zpp::bits::members<2>;
    using serialize_id = zpp::bits::id<"v1::person"_s>;

    auto get_hobby() const
    {
        return "<none>"sv;
    }

    std::string name;
    int age;
};
} // namespace v1

namespace string_versioning::v2
{
struct person
{
    using serialize = zpp::bits::members<3>;
    using serialize_id = zpp::bits::id<"v2::person"_s>;

    auto get_hobby() const
    {
        return std::string_view(hobby);
    }

    std::string name;
    int age;
    std::string hobby;
};
} // namespace v2

TEST(variant, string_versioning_v1)
{
    using namespace string_versioning;
    auto [data, in, out] = zpp::bits::data_in_out();
    out(std::variant<v1::person, v2::person>(v1::person{"Person1", 25}))
        .or_throw();

    EXPECT_EQ(
        hexlify(data),
        hexlify(zpp::bits::to_bytes<"v1::person"_s,
                                    "Person1"_s.size(),
                                    "Person1"_s,
                                    25>()));

    std::variant<v1::person, v2::person> v;
    in(v).or_throw();

    std::visit([](auto && person) {
        EXPECT_EQ(person.name, "Person1");
        EXPECT_EQ(person.age, 25);
        EXPECT_EQ(person.get_hobby(), "<none>"sv);
    }, v);
}

TEST(variant, string_versioning_v2)
{
    using namespace string_versioning;
    auto [data, in, out] = zpp::bits::data_in_out();
    out(std::variant<v1::person, v2::person>(
            v2::person{"Person2", 35, "Basketball"}))
        .or_throw();

    EXPECT_EQ(
        hexlify(data),
        hexlify(zpp::bits::to_bytes<"v2::person"_s,
                                    "Person2"_s.size(),
                                    "Person2"_s,
                                    35,
                                    "Basketball"_s.size(),
                                    "Basketball"_s>()));

    std::variant<v1::person, v2::person> v;
    in(v).or_throw();

    std::visit([](auto && person) {
        EXPECT_EQ(person.name, "Person2");
        EXPECT_EQ(person.age, 35);
        EXPECT_EQ(person.get_hobby(), "Basketball"sv);
    }, v);
}

namespace string_versioning_outside::v1
{
struct person
{
    auto get_hobby() const
    {
        return "<none>"sv;
    }

    std::string name;
    int age;
};

auto serialize(const person &) -> zpp::bits::members<2>;
auto serialize_id(const person &) -> zpp::bits::id<"v1::person"_s>;
} // namespace v1

namespace string_versioning_outside::v2
{
struct person
{
    auto get_hobby() const
    {
        return std::string_view(hobby);
    }

    std::string name;
    int age;
    std::string hobby;
};

auto serialize(const person &) -> zpp::bits::members<3>;
auto serialize_id(const person &) -> zpp::bits::id<"v2::person"_s>;
} // namespace v2

TEST(variant, string_versioning_outside_v1)
{
    using namespace string_versioning_outside;
    auto [data, in, out] = zpp::bits::data_in_out();
    out(std::variant<v1::person, v2::person>(v1::person{"Person1", 25}))
        .or_throw();

    EXPECT_EQ(
        hexlify(data),
        hexlify(zpp::bits::to_bytes<"v1::person"_s,
                                    "Person1"_s.size(),
                                    "Person1"_s,
                                    25>()));

    std::variant<v1::person, v2::person> v;
    in(v).or_throw();

    std::visit([](auto && person) {
        EXPECT_EQ(person.name, "Person1");
        EXPECT_EQ(person.age, 25);
        EXPECT_EQ(person.get_hobby(), "<none>"sv);
    }, v);
}

TEST(variant, string_versioning_outside_v2)
{
    using namespace string_versioning_outside;
    auto [data, in, out] = zpp::bits::data_in_out();
    out(std::variant<v1::person, v2::person>(
            v2::person{"Person2", 35, "Basketball"}))
        .or_throw();

    EXPECT_EQ(
        hexlify(data),
        hexlify(zpp::bits::to_bytes<"v2::person"_s,
                                    "Person2"_s.size(),
                                    "Person2"_s,
                                    35,
                                    "Basketball"_s.size(),
                                    "Basketball"_s>()));

    std::variant<v1::person, v2::person> v;
    in(v).or_throw();

    std::visit([](auto && person) {
        EXPECT_EQ(person.name, "Person2");
        EXPECT_EQ(person.age, 35);
        EXPECT_EQ(person.get_hobby(), "Basketball"sv);
    }, v);
}

namespace int_versioning::v1
{
struct person
{
    using serialize = zpp::bits::members<2>;
    using serialize_id = zpp::bits::id<1337>;

    auto get_hobby() const
    {
        return "<none>"sv;
    }

    std::string name;
    int age;
};
} // namespace v1

namespace int_versioning::v2
{
struct person
{
    using serialize = zpp::bits::members<3>;
    using serialize_id = zpp::bits::id<1338>;

    auto get_hobby() const
    {
        return std::string_view(hobby);
    }

    std::string name;
    int age;
    std::string hobby;
};
} // namespace v2

TEST(variant, int_versioning_v1)
{
    using namespace int_versioning;
    auto [data, in, out] = zpp::bits::data_in_out();
    out(std::variant<v1::person, v2::person>(v1::person{"Person1", 25}))
        .or_throw();

    EXPECT_EQ(
        hexlify(data),
        hexlify(zpp::bits::to_bytes<1337,
                                    "Person1"_s.size(),
                                    "Person1"_s,
                                    25>()));

    std::variant<v1::person, v2::person> v;
    in(v).or_throw();

    std::visit([](auto && person) {
        EXPECT_EQ(person.name, "Person1");
        EXPECT_EQ(person.age, 25);
        EXPECT_EQ(person.get_hobby(), "<none>"sv);
    }, v);
}

TEST(variant, int_versioning_v2)
{
    using namespace int_versioning;
    auto [data, in, out] = zpp::bits::data_in_out();
    out(std::variant<v1::person, v2::person>(
            v2::person{"Person2", 35, "Basketball"}))
        .or_throw();

    EXPECT_EQ(
        hexlify(data),
        hexlify(zpp::bits::to_bytes<1338,
                                    "Person2"_s.size(),
                                    "Person2"_s,
                                    35,
                                    "Basketball"_s.size(),
                                    "Basketball"_s>()));

    std::variant<v1::person, v2::person> v;
    in(v).or_throw();

    std::visit([](auto && person) {
        EXPECT_EQ(person.name, "Person2");
        EXPECT_EQ(person.age, 35);
        EXPECT_EQ(person.get_hobby(), "Basketball"sv);
    }, v);
}

namespace int_versioning_outside::v1
{
struct person
{
    auto get_hobby() const
    {
        return "<none>"sv;
    }

    std::string name;
    int age;
};

auto serialize(const person &) -> zpp::bits::members<2>;
auto serialize_id(const person &) -> zpp::bits::id<1337>;
} // namespace v1

namespace int_versioning_outside::v2
{
struct person
{
    auto get_hobby() const
    {
        return std::string_view(hobby);
    }

    std::string name;
    int age;
    std::string hobby;
};

auto serialize(const person &) -> zpp::bits::members<3>;
auto serialize_id(const person &) -> zpp::bits::id<1338>;
} // namespace v2

TEST(variant, int_versioning_outside_v1)
{
    using namespace int_versioning_outside;
    auto [data, in, out] = zpp::bits::data_in_out();
    out(std::variant<v1::person, v2::person>(v1::person{"Person1", 25}))
        .or_throw();

    EXPECT_EQ(
        hexlify(data),
        hexlify(zpp::bits::to_bytes<1337,
                                    "Person1"_s.size(),
                                    "Person1"_s,
                                    25>()));

    std::variant<v1::person, v2::person> v;
    in(v).or_throw();

    std::visit([](auto && person) {
        EXPECT_EQ(person.name, "Person1");
        EXPECT_EQ(person.age, 25);
        EXPECT_EQ(person.get_hobby(), "<none>"sv);
    }, v);
}

TEST(variant, int_versioning_outside_v2)
{
    using namespace int_versioning_outside;
    auto [data, in, out] = zpp::bits::data_in_out();
    out(std::variant<v1::person, v2::person>(
            v2::person{"Person2", 35, "Basketball"}))
        .or_throw();

    EXPECT_EQ(
        hexlify(data),
        hexlify(zpp::bits::to_bytes<1338,
                                    "Person2"_s.size(),
                                    "Person2"_s,
                                    35,
                                    "Basketball"_s.size(),
                                    "Basketball"_s>()));

    std::variant<v1::person, v2::person> v;
    in(v).or_throw();

    std::visit([](auto && person) {
        EXPECT_EQ(person.name, "Person2");
        EXPECT_EQ(person.age, 35);
        EXPECT_EQ(person.get_hobby(), "Basketball"sv);
    }, v);
}
} // namespace test_variant

