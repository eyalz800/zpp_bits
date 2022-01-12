#include "test.h"

namespace test_alloc_limit
{

TEST(test_alloc_limit, output)
{
    constexpr auto limit = 128;
    auto [data, out] = data_out(zpp::bits::alloc_limit<limit>());
    static_assert(decltype(out)::allocation_limit == limit);

    out(1,2,3,4).or_throw();
    EXPECT_EQ((out(std::array<int, 50>{})), std::errc::no_buffer_space);
}

TEST(test_alloc_limit, input)
{
    constexpr auto limit = 128;
    auto [data, in] = data_in(zpp::bits::alloc_limit<limit>());
    static_assert(decltype(in)::allocation_limit == limit);

    std::vector<std::uint32_t> vi;
    std::vector<std::byte> vb;
    zpp::bits::out{data}(std::vector<std::uint32_t>{1, 2, 3, 4})
        .or_throw();
    in(vi).or_throw();
    in.reset();

    zpp::bits::out{data}(std::vector<std::uint32_t>(limit / sizeof(std::uint32_t)))
        .or_throw();
    in(vi).or_throw();
    in.reset();

    zpp::bits::out{data}(std::vector<std::uint32_t>(limit / sizeof(std::uint32_t) + 1))
        .or_throw();
    EXPECT_EQ((in(vi)), std::errc::message_size);
    in.reset();

    zpp::bits::out{data}(std::vector<std::byte>(limit)).or_throw();
    in(vb).or_throw();
    in.reset();

    zpp::bits::out{data}(std::vector<std::byte>(limit + 1)).or_throw();
    EXPECT_EQ((in(vb)), std::errc::message_size);
    in.reset();

    zpp::bits::out{data}(std::vector<std::byte>(limit / 4),
                         std::vector<std::byte>(limit / 4),
                         std::vector<std::byte>(limit / 4),
                         std::vector<std::byte>(limit / 4))
        .or_throw();
    in(vb).or_throw();
    in(vb).or_throw();
    in(vb).or_throw();
    in(vb).or_throw();
    in.reset();
}

} // namespace test_alloc_limit
