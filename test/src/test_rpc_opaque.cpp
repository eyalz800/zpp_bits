#include "test.h"

namespace test_rpc_opaque
{

using namespace std::literals;
using namespace zpp::bits::literals;

int foo(std::span<const std::byte> input)
{
    int i = 0;
    std::string s;
    zpp::bits::in{input}(i, s).or_throw();

    EXPECT_EQ(i, 1337);
    EXPECT_EQ(s, "hello");
    return 1338;
}

int foo_partial(std::span<const std::byte> & input)
{
    int i = 0;
    zpp::bits::in in{input};
    in(i).or_throw();

    EXPECT_EQ(i, 1337);

    input = in.processed_data();
    return 1338;
}

int bar(int i, std::string s)
{
    EXPECT_EQ(i, 1337);
    EXPECT_EQ(s, "hello");
    return 1339;
}

struct a
{
    int foo(std::span<const std::byte> input)
    {
        int i = 0;
        std::string s;
        zpp::bits::in{input}(i, s).or_throw();

        EXPECT_EQ(i, 1337);
        EXPECT_EQ(s, "hello");
        EXPECT_EQ(i2, 1339);
        return 1340;
    }

#if __has_include("zpp_throwing.h")
    zpp::throwing<int> foo_throwing(std::span<const std::byte> input)
    {
        int i = 0;
        std::string s;
        zpp::bits::in{input}(i, s).or_throw();

        EXPECT_EQ(i, 1337);
        EXPECT_EQ(s, "hello");
        EXPECT_EQ(i2, 1339);
        return 1340;
    }
#endif

    int i2 = 1339;
};

TEST(test_rpc_opaque, opaque_function)
{
    auto [data, in, out] = zpp::bits::data_in_out();

    using rpc = zpp::bits::rpc<
        zpp::bits::bind_opaque<foo, "foo"_sha256_int>,
        zpp::bits::bind<bar, "bar"_sha256_int>
    >;

    auto [client, server] = rpc::client_server(in, out);
    client.request<"foo"_sha256_int>(1337, "hello"s).or_throw();
    server.serve().or_throw();

    EXPECT_EQ((client.response<"foo"_sha256_int>().or_throw()), 1338);
}

TEST(test_rpc, opaque_member_function)
{
    auto [data, in, out] = zpp::bits::data_in_out();

    using rpc = zpp::bits::rpc<
        zpp::bits::bind_opaque<&a::foo, "foo"_sha256_int>,
        zpp::bits::bind<bar, "bar"_sha256_int>
    >;

    a a1;
    auto [client, server] = rpc::client_server(in, out, a1);
    client.request<"foo"_sha256_int>(1337, "hello"s).or_throw();
    server.serve().or_throw();

    EXPECT_EQ((client.response<"foo"_sha256_int>().or_throw()), 1340);
}

TEST(test_rpc_opaque, opaque_function_partial)
{
    auto [data, in, out] = zpp::bits::data_in_out();

    using rpc = zpp::bits::rpc<
        zpp::bits::bind_opaque<foo_partial, "foo_partial"_sha256_int>,
        zpp::bits::bind<bar, "bar"_sha256_int>
    >;

    auto [client, server] = rpc::client_server(in, out);
    client.request<"foo_partial"_sha256_int>(1337, "hello"s).or_throw();
    server.serve().or_throw();

    EXPECT_EQ(in.position(), (2 * sizeof(int)));

    std::string s;
    in(s).or_throw();
    EXPECT_EQ(s, "hello"s);

    EXPECT_EQ((client.response<"foo_partial"_sha256_int>().or_throw()), 1338);
}

#if __has_include("zpp_throwing.h")
TEST(test_rpc, opaque_member_function_throwing)
{
    zpp::try_catch([]() -> zpp::throwing<void> {
        auto [data, in, out] = zpp::bits::data_in_out();

        using rpc = zpp::bits::rpc<
            zpp::bits::bind_opaque<&a::foo_throwing,
                                   "a::foo_throwing"_sha256_int>,
            zpp::bits::bind<bar, "bar"_sha256_int>>;

        a a1;
        auto [client, server] = rpc::client_server(in, out, a1);
        co_await client.request<"a::foo_throwing"_sha256_int>(1337,
                                                              "hello"s);
        co_await server.serve();

        EXPECT_EQ(
            (co_await client.response<"a::foo_throwing"_sha256_int>()),
            1340);
    }, []() {
        FAIL();
    });
}
#endif

} // namespace test_rpc_opaque
