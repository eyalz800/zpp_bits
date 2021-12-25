#include "test.h"

TEST(data_sources, vector_bytes)
{
    std::vector<std::byte> data;
    auto [in, out] = zpp::bits::in_out(data);
    out(std::vector{1,2,3,4}).or_throw();

    EXPECT_EQ(hexlify(data),
              "04000000"
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    std::vector<int> v;
    in(v).or_throw();

    EXPECT_EQ(v, (std::vector{1,2,3,4}));
}

TEST(data_sources, vector_unsigned_char)
{
    std::vector<unsigned char> data;
    auto [in, out] = zpp::bits::in_out(data);
    out(std::vector{1,2,3,4}).or_throw();

    EXPECT_EQ(hexlify(data),
              "04000000"
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    std::vector<int> v;
    in(v).or_throw();

    EXPECT_EQ(v, (std::vector{1,2,3,4}));
}

TEST(data_sources, vector_char)
{
    std::vector<char> data;
    auto [in, out] = zpp::bits::in_out(data);
    out(std::vector{1,2,3,4}).or_throw();

    EXPECT_EQ(hexlify(data),
              "04000000"
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    std::vector<int> v;
    in(v).or_throw();

    EXPECT_EQ(v, (std::vector{1,2,3,4}));
}

TEST(data_sources, string)
{
    std::string data;
    auto [in, out] = zpp::bits::in_out(data);
    out(std::vector{1,2,3,4}).or_throw();

    EXPECT_EQ(hexlify(data),
              "04000000"
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    std::vector<int> v;
    in(v).or_throw();

    EXPECT_EQ(v, (std::vector{1,2,3,4}));
}

TEST(data_sources, stdarray)
{
    std::array<std::byte, 128> data;
    auto [in, out] = zpp::bits::in_out(data);
    out(std::vector{1,2,3,4}).or_throw();

    EXPECT_EQ(hexlify(std::span{data.data(), out.position()}),
              "04000000"
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    std::vector<int> v;
    in(v).or_throw();

    EXPECT_EQ(v, (std::vector{1,2,3,4}));
}


TEST(data_sources, array)
{
    std::byte data[128];
    auto [in, out] = zpp::bits::in_out(data);
    out(std::vector{1,2,3,4}).or_throw();

    EXPECT_EQ(hexlify(std::span{std::data(data), out.position()}),
              "04000000"
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    std::vector<int> v;
    in(v).or_throw();

    EXPECT_EQ(v, (std::vector{1,2,3,4}));
}

TEST(data_sources, array_over_span)
{
    std::byte data[128];
    auto [in, out] = zpp::bits::in_out(std::span(data));
    out(std::vector{1,2,3,4}).or_throw();

    EXPECT_EQ(hexlify(std::span{data, out.position()}),
              "04000000"
              "01000000"
              "02000000"
              "03000000"
              "04000000");

    std::vector<int> v;
    in(v).or_throw();

    EXPECT_EQ(v, (std::vector{1,2,3,4}));
}
