zpp::bits
=========

[![.github/workflows/actions.yml](https://github.com/eyalz800/zpp_bits/actions/workflows/actions.yml/badge.svg)](https://github.com/eyalz800/zpp_bits/actions/workflows/actions.yml)
[![Build Status](https://dev.azure.com/eyalz800/zpp_bits/_apis/build/status/eyalz800.zpp_bits?branchName=main)](https://dev.azure.com/eyalz800/zpp_bits/_build/latest?definitionId=9&branchName=main)

A modern C++20 binary serialization and RPC library, with just one header file.

This library is a successor to [zpp::serializer](https://github.com/eyalz800/serializer).
The library tries to be simpler for use, but has more or less similar API to its predecessor.

Contents
--------
* [Motivation](#motivation)
* [Introduction](#introduction)
* [Error Handling](#error-handling)
* [Error Codes](#error-codes)
* [Serializing Non-Aggregates](#serializing-non-aggregates)
* [Serializing Private Classes](#serializing-private_classes)
* [Explicit Serialization](#explicit-serialization)
* [Archive Creation](#archive-creation)
* [Constexpr Serialization](#constexpr-serialization)
* [Position Control](#position-control)
* [Standard Library Types Serialization](#standard-library-types-serialization)
* [Serialization as Bytes](#serialization-as-bytes)
* [Variant Types and Version Control](#variant-types-and-version-control)
* [Literal Operators](#literal-operators)
* [Apply to Functions](#apply-to-functions)
* [Remote Procedure Call (RPC)](#remote-procedure-call-rpc)
* [Byte Order Customization](#byte-order-customization)
* [Deserializing View Of Const Bytes](#deserializing-views-of-const-bytes)
* [Pointers as Optionals](#pointers-as-optionals)
* [Reflection](#pointers-as-optionals)
* [Additional Archive Controls](#additional-archive-controls)
* [Variable Length Integers](#variable-length-integers)
* [Protobuf](#protobuf)
* [Advanced Controls](#advanced-controls)
* [Benchmark](#benchmark)

Motivation
----------
* Serialize any object from and to binary form as seamless as possible.
* Provide lightweight remote procedure call (RPC) capabilities
* Be the fastest possible - see the [benchmark](#benchmark)

### The Difference From zpp::serializer
* It is simpler
* Performance improvements
* Almost everything is `constexpr`
* More flexible with serialization of the size of variable length types, opt-out from serializing size.
* Opt-in for [zpp::throwing](https://github.com/eyalz800/zpp_throwing) if header is found.
* More friendly towards freestanding (no exception runtime support).
* Breaks compatibility with anything lower than C++20 (which is why the original library is left intact).
* Better naming for utility classes and namespaces, for instance `zpp::bits` is more easily typed than `zpp::serializer`.
* Modernized polymorphism, based on variant and flexible serialization ids, compared to `zpp::serializer` global
polymorphic types with fixed 8 bytes of sha1 serialization id.
* Lightweight RPC capabilities

Introduction
------------
For many types, enabling serialization is transparent and requires no additional lines of code.
These types are required to be of aggregate type, with non array members.
Here is an example of a `person` class with name and age:
```cpp
struct person
{
    std::string name;
    int age{};
};
```

Example how to serialize the person into and from a vector of bytes:
```cpp
// The `data_in_out` utility function creates a vector of bytes, the input and output archives
// and returns them so we can decompose them easily in one line using structured binding like so:
auto [data, in, out] = zpp::bits::data_in_out();

// Serialize a few people:
out(person{"Person1", 25}, person{"Person2", 35});

// Define our people.
person p1, p2;

// We can now deserialize them either one by one `in(p1)` `in(p2)`, or together, here
// we chose to do it together in one line:
in(p1, p2);
```

This example almost works, we are being warned that we are discarding the return value.
For error checking keep reading.

Error Handling
--------------
We need to check for errors, the library offers multiple ways to do so - a return value
based, exception based, or [zpp::throwing](https://github.com/eyalz800/zpp_throwing) based.

### Using return values
The return value based way for being most explicit, or if you just prefer return values:
```cpp
auto [data, in, out] = zpp::bits::data_in_out();

auto result = out(person{"Person1", 25}, person{"Person2", 35});
if (failure(result)) {
    // `result` is implicitly convertible to `std::errc`.
    // handle the error or return/throw exception.
}

person p1, p2;

result = in(p1, p2);
if (failure(result)) {
    // `result` is implicitly convertible to `std::errc`.
    // handle the error or return/throw exception.
}
```

### Using exceptions
The exceptions based way using `.or_throw()` (read this as "succeed or throw" - hence `or_throw()`):
```cpp
int main()
{
    try {
        auto [data, in, out] = zpp::bits::data_in_out();

        // Check error using `or_throw()` which throws an exception.
        out(person{"Person1", 25}, person{"Person2", 35}).or_throw();

        person p1, p2;

        // Check error using `or_throw()` which throws an exception.
        in(p1, p2).or_throw();

        return 0;
    } catch (const std::exception & error) {
        std::cout << "Failed with error: " << error.what() << '\n';
        return 1;
    } catch (...) {
        std::cout << "Unknown error\n";
        return 1;
    });
}
```

### Using zpp::throwing
Another option is [zpp::throwing](https://github.com/eyalz800/zpp_throwing) where error checking turns into two simple `co_await`s,
to understand how to check for error we provide a full main function:
```cpp
int main()
{
    return zpp::try_catch([]() -> zpp::throwing<int> {
        auto [data, in, out] = zpp::bits::data_in_out();

        // Check error using `co_await`, which suspends the coroutine.
        co_await out(person{"Person1", 25}, person{"Person2", 35});

        person p1, p2;

        // Check error using `co_await`, which suspends the coroutine.
        co_await in(p1, p2);

        co_return 0;
    }, [](zpp::error error) {
        std::cout << "Failed with error: " << error.message() << '\n';
        return 1;
    }, [](/* catch all */) {
        std::cout << "Unknown error\n";
        return 1;
    });
}
```

Error Codes
-----------
All of the above methods, use the following error codes internally and can be checked using the comparison
operator from return value based, or by examining the internal error code of `std::system_error` or `zpp::throwing` depending
which one you used:
1. `std::errc::result_out_of_range` - attempting to write or read from a too short buffer.
2. `std::errc::no_buffer_space` - growing buffer would grow beyond the allocation limits or overflow.
3. `std::errc::value_too_large` - varint (variable length integer) encoding is beyond the representation limits.
4. `std::errc::message_size` - message size is beyond the user defined allocation limits.
5. `std::errc::not_supported` - attempt to call an RPC that is not listed as supported.
6. `std::errc::bad_message` - attempt to read a variant of unrecognized type.
7. `std::errc::invalid_argument` - attempting to serialize null pointer or a value-less variant.
8. `std::errc::protocol_error` - attempt to deserialize an invalid protocol message.

Serializing Non-Aggregates
--------------------------
For most non-aggregate types (or aggregate types with array members),
enabling serialization is a one liner. Here is an example of a non-aggregate
`person` class:
```cpp
struct person
{
    // Add this line to your class with the number of members:
    using serialize = zpp::bits::members<2>; // Two members

    person(auto && ...){/*...*/} // Make non-aggregate.

    std::string name;
    int age{};
};
```
Most of the time types we serialize can work with structured binding, and this library takes advantage
of that, but you need to provide the number of members in your class for this to work using the method above.

This also works with argument dependent lookup, allowing to not modify the source class:
```cpp
namespace my_namespace
{
struct person
{
    person(auto && ...){/*...*/} // Make non-aggregate.

    std::string name;
    int age{};
};

// Add this line somewhere before the actual serialization happens.
auto serialize(const person & person) -> zpp::bits::members<2>;
} // namespace my_namespace
```

In some compilers, *SFINAE* works with `requires expression` under `if constexpr` and `unevaluated lambda expression`. It means
that even with non aggregate types the number of members can be detected automatically in cases where all members are in the same struct.
To opt-in, define `ZPP_BITS_AUTODETECT_MEMBERS_MODE=1`.
```cpp
// Members are detected automatically, no additional change needed.
struct person
{
    person(auto && ...){/*...*/} // Make non-aggregate.

    std::string name;
    int age{};
};
```
This works with `clang 13`, however the portability of this is not clear, since in `gcc` it does not work (it is a hard error) and it explicitly states
in the standard that there is intent not to allow *SFINAE* in similar cases, so it is turned off by default.

Serializing Private Classes
---------------------------
If your data members or default constructor are private, you need to become friend with `zpp::bits::access`
like so:
```cpp
struct private_person
{
    // Add this line to your class.
    friend zpp::bits::access;
    using serialize = zpp::bits::members<2>;

private:
    std::string name;
    int age{};
};
```

Explicit Serialization
----------------------
To enable save & load of any object using explicit serialization, which works
regardless of structured binding compatibility, add the following lines to your class:
```cpp
    constexpr static auto serialize(auto & archive, auto & self)
    {
        return archive(self.object_1, self.object_2, ...);
    }
```
Note that `object_1, object_2, ...` are the non-static data members of your class.

Here is the example of a person class again with explicit serialization function:
```cpp
struct person
{
    constexpr static auto serialize(auto & archive, auto & self)
    {
        return archive(self.name, self.age);
    }

    std::string name;
    int age{};
};
```

Or with argument dependent lookup:
```cpp
namespace my_namespace
{
struct person
{
    std::string name;
    int age{};
};

constexpr auto serialize(auto & archive, person & person)
{
    return archive(person.name, person.age);
}

constexpr auto serialize(auto & archive, const person & person)
{
    return archive(person.name, person.age);
}
} // namespace my_namespace
```

Archive Creation
----------------
Creating input and output archives together and separately from data:
```cpp
// Create both a vector of bytes, input and output archives.
auto [data, in, out] = zpp::bits::data_in_out();

// Create just the input and output archives, and bind them to the
// existing vector of bytes.
std::vector<std::byte> data;
auto [in, out] = zpp::bits::in_out(data);

// Create all of them separately
std::vector<std::byte> data;
zpp::bits::in in(data);
zpp::bits::out out(data);

// When you need just data and in/out
auto [data, in] = zpp::bits::data_in();
auto [data, out] = zpp::bits::data_out();
```

Archives can be created from either one of the byte types:
```cpp
// Either one of these work with the below.
std::vector<std::byte> data;
std::vector<char> data;
std::vector<unsigned char> data;
std::string data;

// Automatically works with either `std::byte`, `char`, `unsigned char`.
zpp::bits::in in(data);
zpp::bits::out out(data);
```

You can also use fixed size data objects such as array, `std::array` and view types such as `std::span`
similar to the above. You just need to make sure there is enough size since they are non resizable:
```cpp
// Either one of these work with the below.
std::byte data[0x1000];
char data[0x1000];
unsigned char data[0x1000];
std::array<std::byte, 0x1000> data;
std::array<char, 0x1000> data;
std::array<unsigned char, 0x1000> data;
std::span<std::byte> data = /*...*/;
std::span<char> data = /*...*/;
std::span<unsigned char> data = /*...*/;

// Automatically works with either `std::byte`, `char`, `unsigned char`.
zpp::bits::in in(data);
zpp::bits::out out(data);
```

When using a vector or string, it automatically grows to the right size, however, with the above
the data is limited to the boundaries of the arrays or spans.

When creating the archive in any of the ways above, it is possible to pass a variadic
number of parameters that control the archive behavior, such as for byte order, default size types,
specifying append behavior and so on. This is discussed in the rest of the README.

Constexpr Serialization
-----------------------
As was said above, the library is almost completely constexpr, here is an example
of using array as data object but also using it in compile time to serialize and deserialize
a tuple of integers:
```cpp
constexpr auto tuple_integers()
{
    std::array<std::byte, 0x1000> data{};
    auto [in, out] = zpp::bits::in_out(data);
    out(std::tuple{1,2,3,4,5}).or_throw();

    std::tuple t{0,0,0,0,0};
    in(t).or_throw();
    return t;
}

// Compile time check.
static_assert(tuple_integers() == std::tuple{1,2,3,4,5});
```

For convenience, the library also provides some simplified serialization functions for
compile time:
```cpp
using namespace zpp::bits::literals;

// Returns an array
// where the first bytes are those of the hello world string and then
// the 1337 as 4 byte integer.
constexpr std::array data =
    zpp::bits::to_bytes<"Hello World!"_s, 1337>();

static_assert(
    zpp::bits::from_bytes<data,
                          zpp::bits::string_literal<char, 12>,
                          int>() == std::tuple{"Hello World!"_s, 1337});
```

Position Control
----------------
Query the position of `in` and `out` using `position()`, in other words
the bytes read and written respectively:
```cpp
std::size_t bytes_read = in.position();
std::size_t bytes_written = out.position();
```

Reset the position backwards or forwards, or to the beginning, use with extreme care:
```cpp
in.reset(); // reset to beginning.
in.reset(position); // reset to position.
in.position() -= sizeof(int); // Go back an integer.
in.position() += sizeof(int); // Go forward an integer.

out.reset(); // reset to beginning.
out.reset(position); // reset to position.
out.position() -= sizeof(int); // Go back an integer.
out.position() += sizeof(int); // Go forward an integer.
```

Standard Library Types Serialization
------------------------------------
When serializing variable length standard library types, such as vectors,
strings and view types such as span and string view, the library
first stores 4 byte integer representing the size, followed by the elements.
```cpp
std::vector v = {1,2,3,4};
out(v);
in(v);
```
The reason why the default size type is of 4 bytes (i.e `std::uint32_t`) is for portability between
different architectures, as well as most programs almost never reach a case of a container being
more than 2^32 items, and it may be unjust to pay the price of 8 bytes size by default.

For specific size types that are not 4 bytes, use `zpp::bits::sized`/`zpp::bits::sized_t` like so:
```cpp
// Using `sized` function:
std::vector<int> v = {1,2,3,4};
out(zpp::bits::sized<std::uint16_t>(v));
in(zpp::bits::sized<std::uint16_t>(v));

// Using `sized_t` type:
zpp::bits::sized_t<std::vector<int>, std::uint16_t> v = {1,2,3,4};
out(v);
in(v);
```

Make sure that the size type is large enough for the serialized object, otherwise less items
will be serialized, according to conversion rules of unsigned types.

You can also choose to not serialize the size at all, like so:
```cpp
// Using `unsized` function:
std::vector<int> v = {1,2,3,4};
out(zpp::bits::unsized(v));
in(zpp::bits::unsized(v));

// Using `unsized_t` type:
zpp::bits::unsized_t<std::vector<int>> v = {1,2,3,4};
out(v);
in(v);
```

For where it is common, there are alias declarations for sized / unsized versions of types, for example,
here are `vector` and `span`, others such as `string`, `string_view`, etc are using the same pattern.
```cpp
zpp::bits::vector1b<T>; // vector with 1 byte size.
zpp::bits::vector2b<T>; // vector with 2 byte size.
zpp::bits::vector4b<T>; // vector with 4 byte size == default std::vector configuration
zpp::bits::vector8b<T>; // vector with 8 byte size.
zpp::bits::static_vector<T>; // unsized vector
zpp::bits::native_vector<T>; // vector with native (size_type) byte size.

zpp::bits::span1b<T>; // span with 1 byte size.
zpp::bits::span2b<T>; // span with 2 byte size.
zpp::bits::span4b<T>; // span with 4 byte size == default std::span configuration
zpp::bits::span8b<T>; // span with 8 byte size.
zpp::bits::static_span<T>; // unsized span
zpp::bits::native_span<T>; // span with native (size_type) byte size.
```

Serialization of fixed size types such as arrays, `std::array`s, `std::tuple`s don't include
any overhead except the elements followed by each other.

Changing the default size type for the whole archive is possible during creation:
```cpp
zpp::bits::in in(data, zpp::bits::size1b{}); // Use 1 byte for size.
zpp::bits::out out(data, zpp::bits::size1b{}); // Use 1 byte for size.

zpp::bits::in in(data, zpp::bits::size2b{}); // Use 2 bytes for size.
zpp::bits::out out(data, zpp::bits::size2b{}); // Use 2 bytes for size.

zpp::bits::in in(data, zpp::bits::size4b{}); // Use 4 bytes for size.
zpp::bits::out out(data, zpp::bits::size4b{}); // Use 4 bytes for size.

zpp::bits::in in(data, zpp::bits::size8b{}); // Use 8 bytes for size.
zpp::bits::out out(data, zpp::bits::size8b{}); // Use 8 bytes for size.

zpp::bits::in in(data, zpp::bits::size_native{}); // Use std::size_t for size.
zpp::bits::out out(data, zpp::bits::size_native{}); // Use std::size_t for size.

zpp::bits::in in(data, zpp::bits::no_size{}); // Don't use size, for very special cases, since it is very limiting.
zpp::bits::out out(data, zpp::bits::no_size{}); // Don't use size, for very special cases, since it is very limiting.

// Can also do it together, for example for 2 bytes size:
auto [data, in, out] = data_in_out(zpp::bits::size2b{});
auto [data, out] = data_out(zpp::bits::size2b{});
auto [data, in] = data_in(zpp::bits::size2b{});
```

Serialization as Bytes
----------------------
Most of the types the library knows how to optimize and serialize objects as bytes.
It is however disabled when using explicit serialization functions.

If you know your type is serializable just as raw bytes, and you are using
explicit serialization, you can opt in and optimize
its serialization to a mere `memcpy`:
```cpp
struct point
{
    int x;
    int y;

    constexpr static auto serialize(auto & archive, auto & self)
    {
        // Serialize as bytes, instead of serializing each
        // member separately. The overall result is the same, but this may be
        // faster sometimes.
        return archive(zpp::bits::as_bytes(self));
    }
};
```

It's also possible to do this directly from a vector or span of trivially copyable types,
this time we use `bytes` instead of `as_bytes` because we convert the contents of the vector
to bytes rather than the vector object itself (the data the vector points to rather than the vector object):
```cpp
std::vector<point> points;
out(zpp::bits::bytes(points));
in(zpp::bits::bytes(points));
```
However in this case the size is not serialized, this may be extended in the future to also
support serializing the size similar to other view types. If you need to serialize as bytes
and want the size, as a workaround it's possible to cast to `std::span<std::byte>`.

Variant Types and Version Control
---------------------------------
While there is no perfect tool to handle backwards compatibility of structures because
of the zero overhead-ness of the serialization, you can use `std::variant` as a way
to version your classes or create a nice polymorphism based dispatching, here is how:
```cpp
namespace v1
{
struct person
{
    using serialize = zpp::bits::members<2>;

    auto get_hobby() const
    {
        return "<none>"sv;
    }

    std::string name;
    int age;
};
} // namespace v1

namespace v2
{
struct person
{
    using serialize = zpp::bits::members<3>;

    auto get_hobby() const
    {
        return std::string_view(hobby);
    }

    std::string name;
    int age;
    std::string hobby;
};
} // namespace v2
```

And then to the serialization itself:
```cpp
auto [data, in, out] = zpp::bits::data_in_out();
out(std::variant<v1::person, v2::person>(v1::person{"Person1", 25}))
    .or_throw();

std::variant<v1::person, v2::person> v;
in(v).or_throw();

std::visit([](auto && person) {
    (void) person.name == "Person1";
    (void) person.age == 25;
    (void) person.get_hobby() == "<none>";
}, v);

out(std::variant<v1::person, v2::person>(
        v2::person{"Person2", 35, "Basketball"}))
    .or_throw();

in(v).or_throw();

std::visit([](auto && person) {
    (void) person.name == "Person2";
    (void) person.age == 35;
    (void) person.get_hobby() == "Basketball";
}, v);
```
The way the variant gets serialized is by serializing its index (0 or 1) as a `std::byte`
before serializing the actual object. This is very efficient, however sometimes
users may want to choose explicit serialization id for that, refer to the point below

To set a custom serialization id, you need to add an additional line inside/outside your
class respectively:
```cpp
using namespace zpp::bits::literals;

// Inside the class, this serializes the full string "v1::person" before you serialize
// the person.
using serialize_id = zpp::bits::id<"v1::person"_s>;

// Outside the class, this serializes the full string "v1::person" before you serialize
// the person.
auto serialize_id(const person &) -> zpp::bits::id<"v1::person"_s>;
```
Note that the serialization ids of types in the variant must match in length, or a
compilation error will issue.

You may also use any sequence of bytes instead of a readable string, as well as an integer
or any literal type, here is an example of how to use a hash of a string as a serialization
id:
```cpp
using namespace zpp::bits::literals;

// Inside:
using serialize_id = zpp::bits::id<"v1::person"_sha1>; // Sha1
using serialize_id = zpp::bits::id<"v1::person"_sha256>; // Sha256

// Outside:
auto serialize_id(const person &) -> zpp::bits::id<"v1::person"_sha1>; // Sha1
auto serialize_id(const person &) -> zpp::bits::id<"v1::person"_sha256>; // Sha256
```

You can also serialize just the first bytes of the hash, like so:
```cpp
// First 4 bytes of hash:
using serialize_id = zpp::bits::id<"v1::person"_sha256, 4>;

// First sizeof(int) bytes of hash:
using serialize_id = zpp::bits::id<"v1::person"_sha256_int>;
```

The type is then converted to bytes at compile time using (... wait for it) `zpp::bits::out`
at compile time, so as long as your literal type is serializable according to the above,
you can use it as a serialization id. The id is serialized to `std::array<std::byte, N>` however
for 1, 2, 4, and 8 bytes its underlying type is `std::byte` `std::uint16_t`, `std::uin32_t` and
`std::uint64_t` respectively for ease of use and efficiency.

If you want to serialize the variant without an id, or if you know that a variant is going to
have a particular ID upon deserialize, you may do it using `zpp::bits::known_id` to wrap your variant:
```cpp
std::variant<v1::person, v2::person> v;

 // Id assumed to be v2::person, and is not serialized / deserialized.
out(zpp::bits::known_id<"v2::person"_sha256_int>(v));
in(zpp::bits::known_id<"v2::person"_sha256_int>(v));

// When deserializing you can pass the id as function parameter, to be able
// to use outside of compile time context. `id_v` stands for "id value".
// In our case 4 bytes translates to a plain std::uint32_t, so any dynamic
// integer could fit as the first parameter to `known_id` below.
in(zpp::bits::known_id(zpp::bits::id_v<"v2::person"_sha256_int>, v));
```

Literal Operators
-----------------
Description of helper literals in the library:
```cpp
using namespace zpp::bits::literals;

"hello"_s // Make a string literal.
"hello"_b // Make a binary data literal.
"hello"_sha1 // Make a sha1 binary data literal.
"hello"_sha256 // Make a sha256 binary data literal.
"hello"_sha1_int // Make a sha1 integer from the first hash bytes.
"hello"_sha256_int // Make a sha256 integer from the first hash bytes.
"01020304"_decode_hex // Decode a hex string into bytes literal.
```

Apply to Functions
------------------
* You can apply input archive contents to a function directly, using
`zpp::bits::apply`, the function must be non-template and have exactly
one overload:
```cpp
int foo(std::string s, int i)
{
    // s == "hello"s;
    // i == 1337;
    return 1338;
}

auto [data, in, out] = zpp::bits::data_in_out();
out("hello"s, 1337).or_throw();

// Call the foo in one of the following ways:

// Exception based:
zpp::bits::apply(foo, in).or_throw() == 1338;

// zpp::throwing based:
co_await zpp::bits::apply(foo, in) == 1338;

// Return value based:
if (auto result = zpp::bits::apply(foo, in);
    failure(result)) {
    // Failure...
} else {
    result.value() == 1338;
}
```
When your function receives no parameters, the effect is just calling the function
without deserialization and the return value is the return value of your function.
When the function returns void, there is no value for the resulting type.

Remote Procedure Call (RPC)
---------------------------
The library also provides a thin RPC (remote procedure call) interface to allow serializing
and deserializing function calls:
```cpp
using namespace std::literals;
using namespace zpp::bits::literals;

int foo(int i, std::string s);
std::string bar(int i, int j);

using rpc = zpp::bits::rpc<
    zpp::bits::bind<foo, "foo"_sha256_int>,
    zpp::bits::bind<bar, "bar"_sha256_int>
>;

auto [data, in, out] = zpp::bits::data_in_out();

// Server and client together:
auto [client, server] = rpc::client_server(in, out);

// Or separately:
rpc::client client{in, out};
rpc::server server{in, out};

// Request from the client:
client.request<"foo"_sha256_int>(1337, "hello"s).or_throw();

// Serve the request from the server:
server.serve().or_throw();

// Read back the response
client.response<"foo"_sha256_int>().or_throw(); // == foo(1337, "hello"s);
```

Regarding error handling, similar to many examples above you can use return value, exceptions,
or `zpp::throwing` way for handling errors.
```cpp
// Return value based.
if (auto result = client.request<"foo"_sha256_int>(1337, "hello"s); failure(result)) {
    // Handle the failure.
}
if (auto result = server.serve(); failure(result)) {
    // Handle the failure.
}
if (auto result = client.response<"foo"_sha256_int>(); failure(result)) {
    // Handle the failure.
} else {
    // Use response.value();
}

// Throwing based.
co_await client.request<"foo"_sha256_int>(1337, "hello"s); failure(result));
co_await server.serve();
co_await client.response<"foo"_sha256_int>(); // == foo(1337, "hello"s);
```

It's possible for the IDs of the RPC calls to be skipped, for example of they
are passed out of band, here is how to achieve this:
```cpp
server.serve(id); // id is already known, don't deserialize it.
client.request_body<Id>(arguments...); // request without serializing id.
```

Member functions can also be registered for RPC, however the server needs
to get a reference to the class object during construction, and all of the member
functions must belong to the same class (though namespace scope functions are ok to mix):
```cpp
struct a
{
    int foo(int i, std::string s);
};

std::string bar(int i, int j);

using rpc = zpp::bits::rpc<
    zpp::bits::bind<&a::foo, "a::foo"_sha256_int>,
    zpp::bits::bind<bar, "bar"_sha256_int>
>;

auto [data, in, out] = zpp::bits::data_in_out();

// Our object.
a a1;

// Server and client together:
auto [client, server] = rpc::client_server(in, out, a1);

// Or separately:
rpc::client client{in, out};
rpc::server server{in, out, a1};

// Request from the client:
client.request<"a::foo"_sha256_int>(1337, "hello"s).or_throw();

// Serve the request from the server:
server.serve().or_throw();

// Read back the response
client.response<"a::foo"_sha256_int>().or_throw(); // == a1.foo(1337, "hello"s);
```

The RPC can also work in an opaque mode and let the function itself serialize/deserialize
the data, when binding a function as opaque, using `bind_opaque`:
```cpp
// Each of the following signatures of `foo()` are valid for opaque rpc call:
auto foo(zpp::bits::in<> &, zpp::bits::out<> &);
auto foo(zpp::bits::in<> &);
auto foo(zpp::bits::out<> &);
auto foo(std::span<std::byte> input); // assumes all data is consumed from archive.
auto foo(std::span<std::byte> & input); // resize input in the function to signal how much was consumed.

using rpc = zpp::bits::rpc<
    zpp::bits::bind_opaque<foo, "a::foo"_sha256_int>,
    zpp::bits::bind<bar, "bar"_sha256_int>
>;
```

Byte Order Customization
------------------------
The default byte order used is the native processor/OS selected one.
You may choose another byte order using `zpp::bits::endian` during construction like so:
```cpp
zpp::bits::in in(data, zpp::bits::endian::big{}); // Use big endian
zpp::bits::out out(data, zpp::bits::endian::big{}); // Use big endian

zpp::bits::in in(data, zpp::bits::endian::network{}); // Use big endian (provided for convenience)
zpp::bits::out out(data, zpp::bits::endian::network{}); // Use big endian (provided for convenience)

zpp::bits::in in(data, zpp::bits::endian::little{}); // Use little endian
zpp::bits::out out(data, zpp::bits::endian::little{}); // Use little endian

zpp::bits::in in(data, zpp::bits::endian::swapped{}); // If little use big otherwise little.
zpp::bits::out out(data, zpp::bits::endian::swapped{}); // If little use big otherwise little.

zpp::bits::in in(data, zpp::bits::endian::native{}); // Use the native one (default).
zpp::bits::out out(data, zpp::bits::endian::native{}); // Use the native one (default).

// Can also do it together, for example big endian:
auto [data, in, out] = data_in_out(zpp::bits::endian::big{});
auto [data, out] = data_out(zpp::bits::endian::big{});
auto [data, in] = data_in(zpp::bits::endian::big{});
```

Deserializing Views Of Const Bytes
----------------------------------
On the receiving end (input archive), the library supports view types of const byte types, such
as `std::span<const std::byte>` in order to get a view at a portion of data without copying.
This needs to be carefully used because invalidating iterators of the contained data could cause
a use after free. It is provided to allow the optimization when needed:
```cpp
using namespace std::literals;

auto [data, in, out] = zpp::bits::data_in_out();
out("hello"sv).or_throw();

std::span<const std::byte> s;
in(s).or_throw();

// s.size() == "hello"sv.size()
// std::memcmp("hello"sv.data(), s.data(), "hello"sv.size()) == 0
}
```

There is also an unsized version, which consumes the rest of the archive data
to allow the common use case of header then arbitrary amount of data:
```cpp
auto [data, in, out] = zpp::bits::data_in_out();
out(zpp::bits::unsized("hello"sv)).or_throw();

std::span<const std::byte> s;
in(zpp::bits::unsized(s)).or_throw();

// s.size() == "hello"sv.size()
// std::memcmp("hello"sv.data(), s.data(), "hello"sv.size()) == 0
```

Pointers as Optionals
---------------------
The library does not support serializing null pointer values, however to explicitly support
optional owning pointers, such as to create graphs and complex structures.

In theory it's valid to use `std::optional<std::unique_ptr<T>>`, but it's
recommended to use the specifically made `zpp::bits::optional_ptr<T>` which
optimizes out the boolean that the optional object usually keeps, and uses null pointer
as an invalid state.

Serializing a null pointer value in that case will serialize a zero byte, while
non-null values serialize as a single one byte followed by the bytes of the object.
(i.e, serialization is identical to `std::optional<T>`).

Reflection
----------
As part of the library implementation it was required to implement some reflection types, for
counting members and visiting members, and the library exposes these to the user:
```cpp
struct point
{
    int x;
    int y;
};

#if !ZPP_BITS_AUTODETECT_MEMBERS_MODE
auto serialize(point) -> zpp::bits::members<2>;
#endif

static_assert(zpp::bits::number_of_members<point>() == 2);

constexpr auto sum = zpp::bits::visit_members(
    point{1, 2}, [](auto x, auto y) { return x + y; });

static_assert(sum == 3);

constexpr auto generic_sum = zpp::bits::visit_members(
    point{1, 2}, [](auto... members) { return (0 + ... + members); });

static_assert(generic_sum == 3);

constexpr auto is_two_integers =
    zpp::bits::visit_members_types<point>([]<typename... Types>() {
        if constexpr (std::same_as<std::tuple<Types...>,
                                   std::tuple<int, int>>) {
            return std::true_type{};
        } else {
            return std::false_type{};
        }
    })();

static_assert(is_two_integers);
```
The example above works with or without `ZPP_BITS_AUTODETECT_MEMBERS_MODE=1`, depending
on the `#if`. As noted above, we must rely on specific compiler feature to detect the
number of members which may not be portable.

Additional Archive Controls
---------------------------
Archives can be constructed with additional control options such as `zpp::bits::append{}`
which instructs output archives to set the position to the end of the vector or other data
source. (for input archives this option has no effect)
```cpp
std::vector<std::byte> data;
zpp::bits::out out(data, zpp::bits::append{});
```

It is possible to use multiple controls and to use them also with `data_in_out/data_in/data_out/in_out`:
```cpp
zpp::bits::out out(data, zpp::bits::append{}, zpp::bits::endian::big{});
auto [in, out] = in_out(data, zpp::bits::append{}, zpp::bits::endian::big{});
auto [data, in, out] = data_in_out(zpp::bits::size2b{}, zpp::bits::endian::big{});
```

Allocation size can be limited in case of output archive to a growing buffer
or when using an input archive to limit how long a single length prefixed
message can be to avoid allocation of a very large buffer in advance, using `zpp::bits::alloc_limit<L>{}`.
The intended use is for safety and sanity reasons rather than
accurate allocation measurement:
```cpp
zpp::bits::out out(data, zpp::bits::alloc_limit<0x10000>{});
zpp::bits::in in(data, zpp::bits::alloc_limit<0x10000>{});
auto [in, out] = in_out(data, zpp::bits::alloc_limit<0x10000>{});
auto [data, in, out] = data_in_out(zpp::bits::alloc_limit<0x10000>{});
```

For best correctness, when using growing buffer for output, if the buffer was grown, the buffer is resized
in the end for the exact position of the output archive, this incurs an extra resize
which in most cases is acceptable, but you may avoid this additional resize and recognize
the end of the buffer by using `position()`. You can achieve this by using `zpp::bits::no_fit_size{}`:
```cpp
zpp::bits::out out(data, zpp::bits::no_fit_size{});
```

To control enlarging of output archive vector, you may use `zpp::bits::enlarger<Mul, Div = 1>`:
```cpp
zpp::bits::out out(data, zpp::bits::enlarger<2>{}); // Grow by multiplying size by 2.
zpp::bits::out out(data, zpp::bits::enlarger<3, 2>{}); // Default - Grow by multiplying size by 3 and divide by 2 (enlarge by 1.5).
zpp::bits::out out(data, zpp::bits::exact_enlarger{}); // Grow to exact size every time.
```

By default, for safety, an output archive that uses a growing buffer, checks for overflow
before any buffer grow. For 64 bit systems, this check although cheap, is almost redundant,
as it is almost impossible to overflow a 64 bit integer when it represents a memory size. (i.e,
the memory allocation will fail before the memory comes close to overflow this integer).
If you wish to disable those overflow checks, in favor of performance, use: `zpp::bits::no_enlarge_overflow{}`:
```cpp
zpp::bits::out out(data, zpp::bits::no_enlarge_overflow{}); // Disable overflow check when enlarging.
```

When serializing explicitly it is often required to identify whether the archive is
input or output archive, and it is done via the `archive.kind()` static member function,
and can be done in an `if constexpr`:
```cpp
static constexpr auto serialize(auto & archive, auto & self)
{
    using archive_type = std::remove_cvref_t<decltype(archive)>;

    if constexpr (archive_type::kind() == zpp::bits::kind::in) {
        // Input archive
    } else if constexpr (archive_type::kind() == zpp::bits::kind::out) {
        // Output archive
    } else {
        // No such archive (no need to check for this)
    }
}
```

Variable Length Integers
------------------------
The library provides a type for serializing and deserializing variable
length integers:
```cpp
auto [data, in, out] = zpp::bits::data_in_out();
out(zpp::bits::varint{150}).or_throw();

zpp::bits::varint i{0};
in(i).or_throw();

// i == 150;
```

Here is an example of the encoding at compile time:
```cpp
static_assert(zpp::bits::to_bytes<zpp::bits::varint{150}>() == "9601"_decode_hex);
```

The class template `zpp::bits::varint<T, E = varint_encoding::normal>` is provided
to be able to define any varint integral type or enumeration type,
along with possible encodings `zpp::bits::varint_encoding::normal/zig_zag` (normal is default).

The following alias declarations are provided:
```cpp
using vint32_t = varint<std::int32_t>; // varint of int32 types.
using vint64_t = varint<std::int64_t>; // varint of int64 types.

using vuint32_t = varint<std::uint32_t>; // varint of unsigned int32 types.
using vuint64_t = varint<std::uint64_t>; // varint of unsigned int64 types.

using vsint32_t = varint<std::int32_t, varint_encoding::zig_zag>; // zig zag encoded varint of int32 types.
using vsint64_t = varint<std::int64_t, varint_encoding::zig_zag>; // zig zag encoded varint of int64 types.

using vsize_t = varint<std::size_t>; // varint of std::size_t types.
```

Using varints to serialize sizes by default is also possible during archive creation:
```cpp
auto [data, in, out] = data_in_out(zpp::bits::size_varint{});

zpp::bits::in in(data, zpp::bits::size_varint{}); // Uses varint to encode size.
zpp::bits::out out(data, zpp::bits::size_varint{}); // Uses varint to encode size.
```

Protobuf
--------
The serialization format of this library is not based on any known or accepted format.
Naturally, other languages do not support this format, which makes it near impossible to use
the library for cross programming language communication.

For this reason the library supports the protobuf format
which is available in many languages.

Please note that protobuf support is kind of experimental, which means
it may not include every possible protobuf feature, and it is generally slower
(around 2-5 times slower, mostly on deserialization) than the default format,
which aims to be zero overhead.

Starting with the basic message:
```cpp
struct example
{
    zpp::bits::vint32_t i; // varint of 32 bit, field number is implicitly set to 1,
    // next field is implicitly 2, and so on
};

// Serialize as protobuf protocol (as usual, can also define this inside the class
// with `using serialize = zpp::bits::pb_protocol;`)
auto serialize(const example &) -> zpp::bits::pb_protocol;

// Use archives as usual, specify what kind of size to prefix the message with.
// We chose no size to demonstrate the actual encoding of the message, but in general
// it is recommended to size prefix protobuf messages since they are not self terminating.
auto [data, in, out] = data_in_out(zpp::bits::no_size{});

out(example{.i = 150}).or_throw();

example e;
in(e).or_throw();

// e.i == 150

// Serialize the message without any size prefix, and check the encoding at compile time:
static_assert(
    zpp::bits::to_bytes<zpp::bits::unsized_t<example>{{.i = 150}}>() ==
    "089601"_decode_hex);
```

For the full syntax, which we'll later use to pass more options, use `zpp::bits::protocol`:
```cpp
// Serialize as protobuf protocol (as usual, can also define this inside the class
// with `using serialize = zpp::bits::protocol<zpp::bits::pb{}>;`)
auto serialize(const example &) -> zpp::bits::protocol<zpp::bits::pb{}>;
```

To reserve fields:
```cpp
struct example
{
    [[no_unique_address]] zpp::bits::pb_reserved _1; // field number 1 is reserved.
    zpp::bits::vint32_t i; // field number == 2
    zpp::bits::vsint32_t j; // field number == 3
};
```

To explicitly specify for each member the field number:
```cpp
struct example
{
    zpp::bits::pb_field<zpp::bits::vint32_t, 20> i; // field number == 20
    zpp::bits::pb_field<zpp::bits::vsint32_t, 30> j; // field number == 30

    using serialize = zpp::bits::pb_protocol;
};
```
Accessing the value behind the field is often transparent however if explicitly needed
use `pb_value(<variable>)` to get or assign to the value.

To map members to another field number:
```cpp
struct example
{
    zpp::bits::vint32_t i; // field number == 20
    zpp::bits::vsint32_t j; // field number == 30

    using serialize = zpp::bits::protocol<
        zpp::bits::pb{
            zpp::bits::pb_map<1, 20>{}, // Map first member to field number 20.
            zpp::bits::pb_map<2, 30>{}}>; // Map second member to field number 30.
};
```

Fixed members are simply regular C++ data members:
```cpp
struct example
{
    std::uint32_t i; // fixed unsigned integer 32, field number == 1
};
```

Like with `zpp::bits::members`, for when it is required, you may specify the number of members
in the protocol field with `zpp::bits::pb_members<N>`:
```cpp
struct example
{
    using serialize = zpp::bits::pb_members<1>; // 1 member.

    zpp::bits::vint32_t i; // field number == 1
};
```

The full version of the above involves passing the number of members
as the second parameter to the protocol:
```cpp
struct example
{
    using serialize = zpp::bits::protocol<zpp::bits::pb{}, 1>; // 1 member.

    zpp::bits::vint32_t i; // field number == 1
};
```

Embedded messages are simply nested within the class as data members:
```cpp
struct nested_example
{
    example nested; // field number == 1
};

auto serialize(const nested_example &) -> zpp::bits::pb_protocol;

static_assert(zpp::bits::to_bytes<zpp::bits::unsized_t<nested_example>{
                  {.nested = example{150}}}>() == "0a03089601"_decode_hex);
```

Repeated fields are of the form of owning containers:
```cpp
struct repeating
{
    using serialize = zpp::bits::pb_protocol;

    std::vector<zpp::bits::vint32_t> integers; // field number == 1
    std::string characters; // field number == 2
    std::vector<example> examples; // repeating examples, field number == 3
};
```

Currently all of the fields are optional, which is a good practice, missing fields are dropped and not concatenated to the message, for efficiency.
Any value that is not set in a message leaves the target data member intact, which allows
to implement defaults for data members by using non-static data member initializer or to initialize
the data member before deserializing the message.

Lets take a full `.proto` file and translate it:
```proto
syntax = "proto3";

package tutorial;

message person {
  string name = 1;
  int32 id = 2;
  string email = 3;

  enum phone_type {
    mobile = 0;
    home = 1;
    work = 2;
  }

  message phone_number {
    string number = 1;
    phone_type type = 2;
  }

  repeated phone_number phones = 4;
}

message address_book {
  repeated person people = 1;
}
```

The translated file:
```cpp
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

auto serialize(const person &) -> zpp::bits::pb_protocol;
auto serialize(const person::phone_number &) -> zpp::bits::pb_protocol;
auto serialize(const address_book &) -> zpp::bits::pb_protocol;
```

Derserializing a message that was originally serialized with python:
```python
import addressbook_pb2
person = addressbook_pb2.person()
person.id = 1234
person.name = "John Doe"
person.email = "jdoe@example.com"
phone = person.phones.add()
phone.number = "555-4321"
phone.type = addressbook_pb2.person.home
```

The output we get for `person` is:
```python
name: "John Doe"
id: 1234
email: "jdoe@example.com"
phones {
  number: "555-4321"
  type: home
}
```

Lets serialize it:
```python
person.SerializeToString()
```

The result is:
```python
b'\n\x08John Doe\x10\xd2\t\x1a\x10jdoe@example.com"\x0c\n\x08555-4321\x10\x01'
```

Back to C++:
```cpp
using namespace zpp::bits::literals;

constexpr auto data =
    "\n\x08John Doe\x10\xd2\t\x1a\x10jdoe@example.com\"\x0c\n\x08"
    "555-4321\x10\x01"_b;
static_assert(data.size() == 45);

person p;
zpp::bits::in{data, zpp::bits::no_size{}}(p).or_throw();

// p.name == "John Doe"
// p.id == 1234
// p.email == "jdoe@example.com"
// p.phones.size() == 1
// p.phones[0].number == "555-4321"
// p.phones[0].type == person::home
```

Advanced Controls
-----------------
By default `zpp::bits` inlines aggressively, but to reduce code size, it does not
inline the full decoding of varints (variable length integers).
To configure inlining of the full varint decoding, define `ZPP_BITS_INLINE_DECODE_VARINT=1`.

If you suspect that `zpp::bits` is inlining too much to the point where it badly affects code size,
you may define `ZPP_BITS_INLINE_MODE=0`, which disables all force inlining and observe the results.
Usually it has a negligible effect, but it is provided as is for additional control.

In some compilers, you may find always inline to fail with recursive structures (for example a tree graph).
In these cases it is required to somehow avoid the always inline attribute for the specific structure, a trivial
example would be to use an explicit serialization function, although most times the library detects such occasions and
it is not necessary, but the example is provided just in case:
```cpp
struct node
{
    constexpr static auto serialize(auto & archive, auto & node)
    {
        return archive(node.value, node.nodes);
    }

    int value;
    std::vector<node> nodes;
};
```

Benchmark
---------
### [fraillt/cpp_serializers_benchmark](https://github.com/fraillt/cpp_serializers_benchmark/tree/a4c0ebfb083c3b07ad16adc4301c9d7a7951f46e)
#### GCC 11
| library     | test case                                                  | bin size | data size | ser time | des time |
| ----------- | ---------------------------------------------------------- | -------- | --------- | -------- | -------- |
| zpp_bits    | general                                                    | 52192B   | 8413B     | **733ms**| **693ms**|
| zpp_bits    | fixed buffer                                               | 48000B   | 8413B     | **620ms**| **667ms**|
| bitsery     | general                                                    | 70904B   | 6913B     | 1470ms   | 1524ms   |
| bitsery     | fixed buffer                                               | 53648B   | 6913B     | 927ms    | 1466ms   |
| boost       | general                                                    | 279024B  | 11037B    | 15126ms  | 12724ms  |
| cereal      | general                                                    | 70560B   | 10413B    | 10777ms  | 9088ms   |
| flatbuffers | general                                                    | 70640B   | 14924B    | 8757ms   | 3361ms   |
| handwritten | general                                                    | 47936B   | 10413B    | 1506ms   | 1577ms   |
| handwritten | unsafe                                                     | 47944B   | 10413B    | 1616ms   | 1392ms   |
| iostream    | general                                                    | 53872B   | 8413B     | 11956ms  | 12928ms  |
| msgpack     | general                                                    | 89144B   | 8857B     | 2770ms   | 14033ms  |
| protobuf    | general                                                    | 2077864B | 10018B    | 19929ms  | 20592ms  |
| protobuf    | arena                                                      | 2077872B | 10018B    | 10319ms  | 11787ms  |
| yas         | general                                                    | 61072B   | 10463B    | 2286ms   | 1770ms   |

#### Clang 12.0.1
| library     | test case                                                  | bin size | data size | ser time | des time |
| ----------- | ---------------------------------------------------------- | -------- | --------- | -------- | -------- |
| zpp_bits    | general                                                    | 47128B   | 8413B     | **790ms**| **715ms**|
| zpp_bits    | fixed buffer                                               | 43056B   | 8413B     | **605ms**| **694ms**|
| bitsery     | general                                                    | 53728B   | 6913B     | 2128ms   | 1832ms   |
| bitsery     | fixed buffer                                               | 49248B   | 6913B     | 946ms    | 1941ms   |
| boost       | general                                                    | 237008B  | 11037B    | 16011ms  | 13017ms  |
| cereal      | general                                                    | 61480B   | 10413B    | 9977ms   | 8565ms   |
| flatbuffers | general                                                    | 62512B   | 14924B    | 9812ms   | 3472ms   |
| handwritten | general                                                    | 43112B   | 10413B    | 1391ms   | 1321ms   |
| handwritten | unsafe                                                     | 43120B   | 10413B    | 1393ms   | 1212ms   |
| iostream    | general                                                    | 48632B   | 8413B     | 10992ms  | 12771ms  |
| msgpack     | general                                                    | 77384B   | 8857B     | 3563ms   | 14705ms  |
| protobuf    | general                                                    | 2032712B | 10018B    | 18125ms  | 20211ms  |
| protobuf    | arena                                                      | 2032760B | 10018B    | 9166ms   | 11378ms  |
| yas         | general                                                    | 51000B   | 10463B    | 2114ms   | 1558ms   |

Limitations
-----------
* Serialization of non-owning pointers & raw pointers is not supported, for simplicity and also for security reasons.
* Serialization of null pointers is not supported to avoid the default overhead of stating whether a pointer is null, to
work around this use optional which is more explicit.

Final Words
-----------
I wish that you find this library useful.
Please feel free to submit any issues, make suggestions for improvements, etc.

