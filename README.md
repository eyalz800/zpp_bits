zpp::bits
=========

[![Build Status](https://dev.azure.com/eyalz800/zpp_bits/_apis/build/status/eyalz800.zpp_bits?branchName=main)](https://dev.azure.com/eyalz800/zpp_bits/_build/latest?definitionId=9&branchName=main)

A modern C++20 binary serialization library, with just one header file.

This library is a successor to [zpp::serializer](https://github.com/eyalz800/serializer).
The library tries to be simpler for use, but has more or less similar API to its predecessor.

Motivation
----------
Provide a single, simple header file, that would enable one to:
* Enable save & load any STL container / string / utility into and from a binary form, in a zero overhead approach.
* Enable save & load any object, by adding only a few lines to any class, without breaking existing code.
* Enable save & load the dynamic type of any object, by a simple one-liner.

### The Difference From zpp::serializer
* It is simpler
* Performance improvements
* Almost everything is `constexpr`
* More flexible with serialization of the size of variable length types, opt-out from serializing size.
* Opt-in for [zpp::throwing](https://github.com/eyalz800/zpp_throwing) if header is found.
* More friendly towards freestanding (no exception runtime support).
* Breaks compatibility with anything lower than C++20 (which is why the original library is left intact).
* Better naming for utility classes and namespaces, for instance `zpp::bits` is more easily typed than `zpp::serializer`.
* For now, dropped support for polymorphic serialization, seeking a more modern way to do it.

Contents
--------
* For most types, enabling serialization is *just one line*. Here is an example of a `person` class with name
and age:
```cpp
struct person
{
    // Add this line to your class with the number of members:
    using serialize = zpp::bits::members<2>; // Two members

    std::string name;
    int age{};
};
```
Most of the time types we serialize can work with structured binding, and this library takes advantage
of that, but you need to provide the number of members in your class for this to work using the method above.

* In some compilers, *SFINAE* works with `requires expression` under `if constexpr` and `unevaluated lambda expression`. It means
that even the number of members can be detected automatically in most cases. To opt-in,
define `ZPP_BITS_AUTODETECT_MEMBERS_MODE=1`.
```cpp
// Members are detected automatically, no additional change needed.
struct person
{
    std::string name;
    int age{};
};
```
This works with `clang 13`, however the portability of this is not clear, since in `gcc` it does not work (it is a hard error) and it explicitly states
in the standard that there is intent not to allow *SFINAE* in similar cases, so it is turned off by default.

* If your data members or default constructor are private, you need to become friend with `zpp::bits::access`
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

* To enable save & load of any object, even ones without structured binding, add the following lines to your class
```cpp
    constexpr static auto serialize(auto & archive, auto & self)
    {
        return archive(self.object_1, self.object_2, ...);
    }
```
Note that `object_1, object_2, ...` are the non-static data members of your class.

* Here is the example of a person class again with explicit serialization function:
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

* Example how to serialize the person into and from a vector of bytes:
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
We need to check for errors, the library offers multiple ways to do so - a return value
based, exception based, or [zpp::throwing](https://github.com/eyalz800/zpp_throwing) based.

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

Another option is [zpp::throwing](https://github.com/eyalz800/zpp_throwing) it turns into two simple `co_await`s,
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

* Constructing input and output archives together and separately from data:
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

* Archives can be constructed from either one of the byte types:
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
You can also use fixed size data objects such as `std::array` and view types such as `std::span`
similar to the above. You just need to make sure there is enough size since they are non resizable.

* As was said above, the library is almost completely constexpr, here is an example
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

* When using a vector, it automatically grows to the right size, however, you
can also output and input from a span, in which case your memory size is
limited by the memory span:
```cpp
zpp::bits::in in(std::span{pointer, size});
zpp::bits::out out(std::span{pointer, size});
```

* Query the position of `in` and `out` using `position()`, in other words
the bytes read and written respectively:
```cpp
std::size_t bytes_read = in.position();
std::size_t bytes_written = out.position();
```

* Reset the position backwards or forwards, or to the beginning:
```cpp
in.reset(); // reset to beginning.
in.reset(position); // reset to position.

out.reset(); // reset to beginning.
out.reset(position); // reset to position.
```

* Serializing STL containers and strings, first stores a 4 byte size, then the elements:
```cpp
std::vector v = {1,2,3,4};
out(v);
in(v);
```
The reason why the default size type is of 4 bytes (i.e `std::uint32_t`) is that most programs
almost never reach a case of a container being more than ~4 billion items, and it may be unjust to
pay the price of 8 bytes size by default.

* For specific size types that are not 4 bytes, use `zpp::bits::sized` like so:
```cpp
std::vector<int> v = {1,2,3,4};
out(zpp::bits::sized<std::uint16_t>(v));
in(zpp::bits::sized<std::uint16_t>(v));
```

Make sure that the size type is large enough for the serialized object, otherwise less items
will be serialized, according to conversion rules of unsigned types.

* You can also choose to not serialize the size at all, like so:
```cpp
std::vector<int> v = {1,2,3,4};
out(zpp::bits::unsized(v));
in(zpp::bits::unsized(v));
```

* Serialization using argument dependent lookup is also possible, using both
the automatic member serialization way or with fully defined serialization functions.

With automatic member serialization:
```cpp
namespace my_namespace
{
struct adl
{
    int x;
    int y;
};

constexpr auto serialize(const adl & adl) -> zpp::bits::members<2>;
} // namespace my_namespace
```

With fully defined serialization functions:
```cpp
namespace my_namespace
{
struct adl
{
    int x;
    int y;
};

constexpr auto serialize(auto & archive, adl & adl)
{
    return archive(adl.x, adl.y);
}

constexpr auto serialize(auto & archive, const adl & adl)
{
    return archive(adl.x, adl.y);
}
} // namespace my_namespace
```

* If you know your type is serializable just as raw bytes, you can opt in and optimize
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
It is however done automatically if your class is using member based serialization with `zpp::bits::members`,
rather than an explicit serialization function.

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

* As part of the library implementation it was required to implement some reflection types, for
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

* For convenience, the library also provides some simplified serialization functions for
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

* This should cover most of the basic stuff, more documentation may come in the future.

Limitations
-----------
* Currently there is no explicit tool to handle backwards compatibility of structures, the only
overhead that is generated is also part of the data structures anyway, which is size of variable length types,
which is the active member of a variant, and whether an optional holds a value, which does not leave much metadata
for backwards compatibility. However, a neat way to achieve some versioning is by using `std::variant`, and versioning
your types this way `std::variant<version1::type, version2::type, version3::type, ...>` and then use `std::visit()`
to get called with the right version.
* Serialization of non-owning pointers & raw pointers is not supported, for simplicity and also for security reasons.
* Serialization of null pointers is not supported to avoid the default overhead of stating whether a pointer is null, to
work around this use optional which is more explicit.

Final Words
-----------
I wish that you find this library useful.
Please feel free to submit any issues, make suggestions for improvements, etc.

