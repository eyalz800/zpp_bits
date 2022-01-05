#include "test.h"

namespace test_default_size_type
{

TEST(default_size_type, size1b)
{
    auto [data, in, out] = zpp::bits::data_in_out(zpp::bits::size1b{});
    out(std::vector{1,2,3,4}).or_throw();

    EXPECT_EQ(encode_hex(data),
              "04"
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    std::vector<int> v;
    in(v).or_throw();

    EXPECT_EQ(v, (std::vector{1,2,3,4}));
}

TEST(default_size_type, size2b)
{
    auto [data, in, out] = zpp::bits::data_in_out(zpp::bits::size2b{});
    out(std::vector{1,2,3,4}).or_throw();

    EXPECT_EQ(encode_hex(data),
              "0400"
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    std::vector<int> v;
    in(v).or_throw();

    EXPECT_EQ(v, (std::vector{1,2,3,4}));
}

TEST(default_size_type, size4b)
{
    auto [data, in, out] = zpp::bits::data_in_out(zpp::bits::size4b{});
    out(std::vector{1,2,3,4}).or_throw();

    EXPECT_EQ(encode_hex(data),
              "04000000"
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    std::vector<int> v;
    in(v).or_throw();

    EXPECT_EQ(v, (std::vector{1,2,3,4}));
}

TEST(default_size_type, size8b)
{
    auto [data, in, out] = zpp::bits::data_in_out(zpp::bits::size8b{});
    out(std::vector{1,2,3,4}).or_throw();

    EXPECT_EQ(encode_hex(data),
              "0400000000000000"
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    std::vector<int> v;
    in(v).or_throw();

    EXPECT_EQ(v, (std::vector{1,2,3,4}));
}

TEST(default_size_type, size_native)
{
    auto [data, in, out] = zpp::bits::data_in_out(zpp::bits::size_native{});
    out(std::vector{1,2,3,4}).or_throw();

    if constexpr (sizeof(std::size_t) == 8) {
        EXPECT_EQ(encode_hex(data),
                  "0400000000000000"
                  "01000000"
                  "02000000"
                  "03000000"
                  "04000000");
    } else {
        EXPECT_EQ(encode_hex(data),
                  "04000000"
                  "01000000"
                  "02000000"
                  "03000000"
                  "04000000");
    }

    std::vector<int> v;
    in(v).or_throw();

    EXPECT_EQ(v, (std::vector{1,2,3,4}));
}

} // namespace test_default_size_type
