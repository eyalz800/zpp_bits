#include "test.h"

namespace test_rpc
{

using namespace std::literals;
using namespace zpp::bits::literals;

int foo(int i, std::string s)
{
    EXPECT_EQ(i, 1337);
    EXPECT_EQ(s, "hello");
    return 1338;
}

std::string foo_str(int i, std::string s)
{
    EXPECT_EQ(i, 1337);
    EXPECT_EQ(s, "hello");
    return "1338"s;
}

std::string foo_str_no_params()
{
    return "1338"s;
}

void foo_void(int i, std::string s)
{
    EXPECT_EQ(i, 1337);
    EXPECT_EQ(s, "hello");
}

int bar(int i, std::string s)
{
    EXPECT_EQ(i, 1337);
    EXPECT_EQ(s, "hello");
    return 1339;
}

struct a
{
    int foo(int i, std::string s)
    {
        EXPECT_EQ(i, 1337);
        EXPECT_EQ(s, "hello");
        EXPECT_EQ(i2, 1339);
        return 1340;
    }

    std::string foo_str(int i, std::string s)
    {
        EXPECT_EQ(i, 1337);
        EXPECT_EQ(s, "hello");
        EXPECT_EQ(i2, 1339);
        return "1340"s;
    }

    void foo_void(int i, std::string s)
    {
        EXPECT_EQ(i, 1337);
        EXPECT_EQ(s, "hello");
        EXPECT_EQ(i2, 1339);
    }


#if __has_include("zpp_throwing.h")
    zpp::throwing<std::string> foo_str_throw(int i, std::string s)
    {
        EXPECT_EQ(i, 1337);
        EXPECT_EQ(s, "hello");
        EXPECT_EQ(i2, 1339);
        co_return "1341"s;
    }

    zpp::throwing<std::string> foo_str_throw_no_params()
    {
        EXPECT_EQ(i2, 1339);
        co_return "1342"s;
    }

    zpp::throwing<void> foo_void_throw(int i, std::string s)
    {
        EXPECT_EQ(i, 1337);
        EXPECT_EQ(s, "hello");
        EXPECT_EQ(i2, 1339);
        co_return;
    }

    zpp::throwing<void> foo_void_throw_no_params()
    {
        EXPECT_EQ(i2, 1339);
        co_return;
    }
#endif

    int i2 = 1339;
};

TEST(test_rpc, normal_function)
{
    auto [data, in, out] = zpp::bits::data_in_out();

    using rpc = zpp::bits::rpc<
        zpp::bits::bind<foo, "foo"_sha256_int>,
        zpp::bits::bind<bar, "bar"_sha256_int>
    >;

    auto [client, server] = rpc::client_server(in, out);
    client.request<"foo"_sha256_int>(1337, "hello"s).or_throw();
    server.serve().or_throw();

    EXPECT_EQ((client.response<"foo"_sha256_int>().or_throw()), 1338);
}

TEST(test_rpc, normal_function_str)
{
    auto [data, in, out] = zpp::bits::data_in_out();

    using rpc = zpp::bits::rpc<
        zpp::bits::bind<foo, "foo"_sha256_int>,
        zpp::bits::bind<foo_str, "foo_str"_sha256_int>,
        zpp::bits::bind<bar, "bar"_sha256_int>
    >;

    auto [client, server] = rpc::client_server(in, out);
    client.request<"foo_str"_sha256_int>(1337, "hello"s).or_throw();
    server.serve().or_throw();

    EXPECT_EQ((client.response<"foo_str"_sha256_int>().or_throw()), "1338"s);
}

TEST(test_rpc, normal_function_str_no_params)
{
    auto [data, in, out] = zpp::bits::data_in_out();

    using rpc = zpp::bits::rpc<
        zpp::bits::bind<foo, "foo"_sha256_int>,
        zpp::bits::bind<foo_str, "foo_str"_sha256_int>,
        zpp::bits::bind<foo_str_no_params, "foo_str_no_params"_sha256_int>,
        zpp::bits::bind<bar, "bar"_sha256_int>
    >;

    auto [client, server] = rpc::client_server(in, out);
    client.request<"foo_str_no_params"_sha256_int>()
        .or_throw();
    server.serve().or_throw();

    EXPECT_EQ((client.response<"foo_str_no_params"_sha256_int>().or_throw()), "1338"s);
}

TEST(test_rpc, normal_function_str_no_id)
{
    auto [data, in, out] = zpp::bits::data_in_out();

    using rpc = zpp::bits::rpc<
        zpp::bits::bind<foo, "foo"_sha256_int>,
        zpp::bits::bind<foo_str, "foo_str"_sha256_int>,
        zpp::bits::bind<bar, "bar"_sha256_int>
    >;

    auto [client, server] = rpc::client_server(in, out);
    client.request_body<"foo_str"_sha256_int>(1337, "hello"s).or_throw();
    server.serve("foo_str"_sha256_int).or_throw();

    EXPECT_EQ((client.response<"foo_str"_sha256_int>().or_throw()), "1338"s);
}

TEST(test_rpc, normal_function_void)
{
    auto [data, in, out] = zpp::bits::data_in_out();

    using rpc = zpp::bits::rpc<
        zpp::bits::bind<foo, "foo"_sha256_int>,
        zpp::bits::bind<foo_str, "foo_str"_sha256_int>,
        zpp::bits::bind<foo_void, "foo_void"_sha256_int>,
        zpp::bits::bind<bar, "bar"_sha256_int>
    >;

    auto [client, server] = rpc::client_server(in, out);
    client.request<"foo_void"_sha256_int>(1337, "hello"s).or_throw();
    server.serve().or_throw();

    client.response<"foo_void"_sha256_int>();
}

TEST(test_rpc, member_function)
{
    auto [data, in, out] = zpp::bits::data_in_out();

    using rpc = zpp::bits::rpc<
        zpp::bits::bind<foo, "foo"_sha256_int>,
        zpp::bits::bind<&a::foo, "a::foo"_sha256_int>,
        zpp::bits::bind<bar, "bar"_sha256_int>
    >;

    a a1;
    auto [client, server] = rpc::client_server(in, out, a1);
    client.request<"a::foo"_sha256_int>(1337, "hello"s).or_throw();
    server.serve().or_throw();

    EXPECT_EQ((client.response<"a::foo"_sha256_int>().or_throw()), 1340);
}

TEST(test_rpc, member_function_str)
{
    auto [data, in, out] = zpp::bits::data_in_out();

    using rpc = zpp::bits::rpc<
        zpp::bits::bind<foo, "foo"_sha256_int>,
        zpp::bits::bind<&a::foo, "a::foo"_sha256_int>,
        zpp::bits::bind<&a::foo_str, "a::foo_str"_sha256_int>,
        zpp::bits::bind<bar, "bar"_sha256_int>
    >;

    a a1;
    auto [client, server] = rpc::client_server(in, out, a1);
    client.request<"a::foo_str"_sha256_int>(1337, "hello"s).or_throw();
    server.serve().or_throw();

    EXPECT_EQ((client.response<"a::foo_str"_sha256_int>().or_throw()),
              "1340");
}

TEST(test_rpc, member_function_void)
{
    auto [data, in, out] = zpp::bits::data_in_out();

    using rpc = zpp::bits::rpc<
        zpp::bits::bind<foo, "foo"_sha256_int>,
        zpp::bits::bind<&a::foo, "a::foo"_sha256_int>,
        zpp::bits::bind<&a::foo_str, "a::foo_str"_sha256_int>,
        zpp::bits::bind<&a::foo_void, "a::foo_void"_sha256_int>,
        zpp::bits::bind<bar, "bar"_sha256_int>
    >;

    a a1;
    auto [client, server] = rpc::client_server(in, out, a1);
    client.request<"a::foo_void"_sha256_int>(1337, "hello"s).or_throw();
    server.serve().or_throw();

    client.response<"a::foo_void"_sha256_int>();
}

TEST(test_rpc, member_function_mix)
{
    auto [data, in, out] = zpp::bits::data_in_out();

    using rpc = zpp::bits::rpc<
        zpp::bits::bind<foo, "foo"_sha256_int>,
        zpp::bits::bind<&a::foo, "a::foo"_sha256_int>,
        zpp::bits::bind<bar, "bar"_sha256_int>
    >;

    a a1;
    auto [client, server] = rpc::client_server(in, out, a1);
    client.request<"foo"_sha256_int>(1337, "hello"s).or_throw();
    server.serve().or_throw();

    EXPECT_EQ((client.response<"foo"_sha256_int>().or_throw()), 1338);
}

#if __has_include("zpp_throwing.h")
TEST(test_rpc, member_function_str_throwing)
{
    zpp::try_catch([]() -> zpp::throwing<void> {
        auto [data, in, out] = zpp::bits::data_in_out();

        using rpc = zpp::bits::rpc<
            zpp::bits::bind<foo, "foo"_sha256_int>,
            zpp::bits::bind<&a::foo, "a::foo"_sha256_int>,
            zpp::bits::bind<&a::foo_str, "a::foo_str"_sha256_int>,
            zpp::bits::bind<bar, "bar"_sha256_int>
        >;

        a a1;
        auto [client, server] = rpc::client_server(in, out, a1);
        co_await client.request<"a::foo_str"_sha256_int>(1337, "hello"s);
        co_await server.serve();

        EXPECT_EQ((co_await client.response<"a::foo_str"_sha256_int>()),
                  "1340");
    }, []() {
        FAIL();
    });
}

TEST(test_rpc, member_function_str_throwing_returns_throwing)
{
    zpp::try_catch([]() -> zpp::throwing<void> {
        auto [data, in, out] = zpp::bits::data_in_out();

        using rpc = zpp::bits::rpc<
            zpp::bits::bind<foo, "foo"_sha256_int>,
            zpp::bits::bind<&a::foo, "a::foo"_sha256_int>,
            zpp::bits::bind<&a::foo_str, "a::foo_str"_sha256_int>,
            zpp::bits::bind<&a::foo_str_throw, "a::foo_str_throw"_sha256_int>,
            zpp::bits::bind<bar, "bar"_sha256_int>
        >;

        a a1;
        auto [client, server] = rpc::client_server(in, out, a1);
        co_await client.request<"a::foo_str_throw"_sha256_int>(1337, "hello"s);
        co_await server.serve();

        EXPECT_EQ((co_await client.response<"a::foo_str_throw"_sha256_int>()),
                  "1341"s);
    }, []() {
        FAIL();
    });
}

TEST(test_rpc, member_function_str_throwing_returns_throwing_no_params)
{
    zpp::try_catch([]() -> zpp::throwing<void> {
        auto [data, in, out] = zpp::bits::data_in_out();

        using rpc = zpp::bits::rpc<
            zpp::bits::bind<foo, "foo"_sha256_int>,
            zpp::bits::bind<&a::foo, "a::foo"_sha256_int>,
            zpp::bits::bind<&a::foo_str, "a::foo_str"_sha256_int>,
            zpp::bits::bind<&a::foo_str_throw, "a::foo_str_throw"_sha256_int>,
            zpp::bits::bind<&a::foo_str_throw_no_params, "a::foo_str_throw_no_params"_sha256_int>,
            zpp::bits::bind<bar, "bar"_sha256_int>
        >;

        a a1;
        auto [client, server] = rpc::client_server(in, out, a1);
        co_await client.request<"a::foo_str_throw_no_params"_sha256_int>();
        co_await server.serve();

        EXPECT_EQ((co_await client.response<"a::foo_str_throw_no_params"_sha256_int>()),
                  "1342"s);
    }, []() {
        FAIL();
    });
}

TEST(test_rpc, member_function_void_throwing_returns_throwing)
{
    zpp::try_catch([]() -> zpp::throwing<void> {
        auto [data, in, out] = zpp::bits::data_in_out();

        using rpc = zpp::bits::rpc<
            zpp::bits::bind<foo, "foo"_sha256_int>,
            zpp::bits::bind<&a::foo, "a::foo"_sha256_int>,
            zpp::bits::bind<&a::foo_str, "a::foo_str"_sha256_int>,
            zpp::bits::bind<&a::foo_str_throw, "a::foo_str_throw"_sha256_int>,
            zpp::bits::bind<&a::foo_void_throw, "a::foo_void_throw"_sha256_int>,
            zpp::bits::bind<bar, "bar"_sha256_int>
        >;

        a a1;
        auto [client, server] = rpc::client_server(in, out, a1);
        co_await client.request<"a::foo_void_throw"_sha256_int>(1337, "hello"s);
        co_await server.serve();

        client.response<"a::foo_void_throw"_sha256_int>();
    }, []() {
        FAIL();
    });
}

TEST(test_rpc, member_function_void_throwing_returns_throwing_no_params)
{
    zpp::try_catch([]() -> zpp::throwing<void> {
        auto [data, in, out] = zpp::bits::data_in_out();

        using rpc = zpp::bits::rpc<
            zpp::bits::bind<foo, "foo"_sha256_int>,
            zpp::bits::bind<&a::foo, "a::foo"_sha256_int>,
            zpp::bits::bind<&a::foo_str, "a::foo_str"_sha256_int>,
            zpp::bits::bind<&a::foo_str_throw, "a::foo_str_throw"_sha256_int>,
            zpp::bits::bind<&a::foo_void_throw, "a::foo_void_throw"_sha256_int>,
            zpp::bits::bind<&a::foo_void_throw_no_params, "a::foo_void_throw_no_params"_sha256_int>,
            zpp::bits::bind<bar, "bar"_sha256_int>
        >;

        a a1;
        auto [client, server] = rpc::client_server(in, out, a1);
        co_await client.request<"a::foo_void_throw_no_params"_sha256_int>();
        co_await server.serve();

        client.response<"a::foo_void_throw_no_params"_sha256_int>();
    }, []() {
        FAIL();
    });
}
#endif

} // namespace test_rpc
