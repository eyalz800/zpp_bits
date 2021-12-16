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

TEST(sanity, main)
{
    // The `data_in_out` utility function creates a vector of bytes, the
    // input and output archives and returns them so we can decompose them
    // easily in one line using structured binding like so:
    auto [data, in, out] = zpp::bits::data_in_out();

    // Serialize a few people:
    out(person{"Person1", 25}, person{"Person2", 35}).or_throw();

    // Define our people.
    person p1, p2;

    // We can now deserialize them either one by one `in(p1)` `in(p2)`, or
    // together, here we chose to do it together in one line:
    in(p1, p2).or_throw();

    EXPECT_EQ(p1.name, "Person1");
    EXPECT_EQ(p1.age, 25);

    EXPECT_EQ(p2.name, "Person2");
    EXPECT_EQ(p2.age, 35);
}

} // namespace test_sanity
