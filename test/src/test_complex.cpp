#include <complex>

#include "test.h"

namespace test_complex
{

TEST(complex, float)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(std::complex<float>(1.5f, 2.0f)).or_throw();
    EXPECT_EQ(encode_hex(data),
              "0000c03f"
              "00000040");
    std::complex<float> c;
    in(c).or_throw();
    EXPECT_EQ(c, std::complex<float>(1.5f, 2.0f));
}

TEST(complex, integer)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(std::complex<int>(-40, 100)).or_throw();
    EXPECT_EQ(encode_hex(data),
              "d8ffffff"
              "64000000");
    std::complex<int> c;
    in(c).or_throw();
    EXPECT_EQ(c, std::complex<int>(-40, 100));
}

TEST(complex, unsigned_integer)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    out(std::complex<unsigned int>(100, 200)).or_throw();
    EXPECT_EQ(encode_hex(data),
              "64000000"
              "c8000000");
    std::complex<unsigned int> c;
    in(c).or_throw();
    EXPECT_EQ(c, std::complex<unsigned int>(100, 200));
}

}
