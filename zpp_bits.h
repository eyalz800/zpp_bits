#ifndef ZPP_BITS_H
#define ZPP_BITS_H

#ifndef ZPP_BITS_AUTODETECT_MEMBERS_MODE
#define ZPP_BITS_AUTODETECT_MEMBERS_MODE (0)
#endif

#include <array>
#include <bit>
#include <climits>
#include <concepts>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>
#if __has_include("zpp_throwing.h")
#include "zpp_throwing.h"
#endif

#ifdef __cpp_exceptions
#include <stdexcept>
#endif

namespace zpp::bits
{
using default_size_type = std::uint32_t;

#ifndef __cpp_lib_bit_cast
namespace std
{
using namespace ::std;
template <class ToType,
          class FromType,
          class = enable_if_t<sizeof(ToType) == sizeof(FromType) &&
                              is_trivially_copyable_v<ToType> &&
                              is_trivially_copyable_v<FromType>>>
constexpr ToType bit_cast(FromType const & from) noexcept
{
    return __builtin_bit_cast(ToType, from);
}
} // namespace std
#endif

template <std::size_t Count = std::numeric_limits<std::size_t>::max()>
struct members
{
    constexpr static std::size_t value = Count;
};

namespace concepts
{
namespace detail
{
template <typename Type>
struct is_unique_ptr : std::false_type
{
};

template <typename Type>
struct is_unique_ptr<std::unique_ptr<Type, std::default_delete<Type>>>
    : std::true_type
{
};

template <typename Type>
struct is_shared_ptr : std::false_type
{
};

template <typename Type>
struct is_shared_ptr<std::shared_ptr<Type>> : std::true_type
{
};
} // detail

template <typename Type>
concept byte_type = std::same_as<std::remove_cv_t<Type>, char> ||
                    std::same_as<std::remove_cv_t<Type>, unsigned char> ||
                    std::same_as<std::remove_cv_t<Type>, std::byte>;

template <typename Type>
concept byte_view = byte_type<typename std::remove_cvref_t<Type>::value_type> &&
    requires(Type value)
{
    value.data();
    value.size();
};

template <typename Type>
concept container = requires(Type container)
{
    typename std::remove_cvref_t<Type>::value_type;
    container.size();
    container.begin();
    container.end();
};

template <typename Type>
concept associative_container = container<Type> && requires(Type container)
{
    typename std::remove_cvref_t<Type>::key_type;
};

template <typename Type>
concept tuple = !container<Type> && requires(Type tuple)
{
    std::get<0>(tuple);
    std::get<1>(tuple);
    sizeof(std::tuple_size<std::remove_cvref_t<Type>>);
}
&&!requires(Type tuple)
{
    tuple.index();
};

template <typename Type>
concept variant = requires (Type variant) {
    variant.index();
    std::get_if<0>(&variant);
    std::variant_size_v<std::remove_cvref_t<Type>>;
};

template <typename Type>
concept optional = requires (Type optional) {
    optional.value();
    optional.has_value();
    optional.operator bool();
    optional.operator*();
};

template <typename Type>
concept owning_pointer =
    detail::is_unique_ptr<std::remove_cvref_t<Type>>::value ||
    detail::is_shared_ptr<std::remove_cvref_t<Type>>::value;

template <typename Type>
concept unspecialized =
    !container<Type> && !owning_pointer<Type> && !tuple<Type> &&
    !variant<Type> && !optional<Type> &&
    !std::is_array_v<std::remove_cvref_t<Type>>;

template <typename Type>
concept empty = requires
{
    requires std::is_empty_v<std::remove_cvref_t<Type>>;
};

} // namespace concepts

template <typename Item>
class bytes
{
public:
    using value_type = Item;

    constexpr explicit bytes(std::span<Item> items) :
        m_items(items)
    {
    }

    constexpr Item * data() const
    {
        return m_items.data();
    }

    constexpr std::size_t size_in_bytes() const
    {
        return m_items.size() * sizeof(Item);
    }

    constexpr std::size_t count() const
    {
        return m_items.size();
    }

private:
    static_assert(std::is_trivially_copyable_v<Item>);

    std::span<Item> m_items;
};

template <typename Item>
bytes(std::span<Item>) -> bytes<Item>;

template <typename Item, std::size_t Count>
bytes(Item(&)[Count]) -> bytes<Item>;

template <concepts::container Container>
bytes(Container && container)
    -> bytes<std::remove_reference_t<decltype(container[0])>>;

constexpr auto as_bytes(auto && object)
{
    return bytes(std::span{&object, 1});
}

template <typename Option>
struct option
{
    using zpp_bits_option = void;
    constexpr auto operator()(auto && bits)
    {
        if constexpr (requires {
                          bits.option(static_cast<Option &>(*this));
                      }) {
            bits.option(static_cast<Option &>(*this));
        }
    }
};

template <typename Left, typename Right>
struct aggregated_options
{
    using zpp_bits_option = void;
    constexpr auto operator()(auto && bits)
    {
        left(bits);
        right(bits);
    }

    [[no_unique_address]] Left left;
    [[no_unique_address]] Right right;
};

struct out_append : option<out_append>
{
};
struct out_reserve : option<out_reserve>
{
    constexpr explicit out_reserve(std::size_t size) : size(size)
    {
    }
    std::size_t size{};
};
struct out_resize : option<out_resize>
{
    constexpr explicit out_resize(std::size_t size) : size(size)
    {
    }
    std::size_t size{};
};

template <typename... Types>
aggregated_options(Types...) -> aggregated_options<Types...>;

constexpr auto operator|(auto left, auto right) requires requires
{
    typename decltype(left)::zpp_bits_option;
    typename decltype(right)::zpp_bits_option;
}
{
    return aggregated_options{std::move(left), std::move(right)};
}

template <typename SizeType, concepts::container Container>
struct sized_container
{
    static_assert(std::is_unsigned_v<SizeType> || std::is_void_v<SizeType>);

    constexpr explicit sized_container(Container & container) :
        container(container)
    {
    }

    constexpr static auto serialize(auto & serializer, auto & self)
    {
        return serializer.template serialize_one<SizeType>(self.container);
    }

    Container & container;
};

template <typename SizeType, typename Container>
constexpr auto sized(Container && container)
{
    return sized_container<SizeType, std::remove_reference_t<Container>>(
        container);
}

template <typename Container>
constexpr auto unsized(Container && container)
{
    return sized_container<void, std::remove_reference_t<Container>>(
        container);
}

constexpr auto success(std::errc code)
{
    return std::errc{} == code;
}

constexpr auto failure(std::errc code)
{
    return std::errc{} != code;
}

struct [[nodiscard]] errc
{
    constexpr errc(std::errc code = {}) : code(code)
    {
    }

#if __has_include("zpp_throwing.h")
    constexpr zpp::throwing<void> operator co_await() const
    {
        if (failure(code)) [[unlikely]] {
            return code;
        }
        return zpp::void_v;
    }
#endif

    constexpr operator std::errc() const
    {
        return code;
    }

#ifdef __cpp_exceptions
    constexpr void or_throw() const
    {
        if (failure(code)) [[unlikely]] {
            throw std::system_error(std::make_error_code(code));
        }
    }
#endif

    std::errc code;
};

constexpr auto success(errc code)
{
    return std::errc{} == code;
}

constexpr auto failure(errc code)
{
    return std::errc{} != code;
}

struct access
{
    template <typename Item>
    constexpr static auto placement_new(void * address,
                                        auto &&... arguments)
    {
        return ::new (address)
            Item(std::forward<decltype(arguments)>(arguments)...);
    }

    template <typename Item>
    constexpr static auto make_unique(auto &&... arguments)
    {
        return std::unique_ptr<Item>(
            new Item(std::forward<decltype(arguments)>(arguments)...));
    }

    template <typename Item>
    constexpr static void destruct(Item & item)
    {
        item.~Item();
    }

    template <typename Type>
    constexpr static auto number_of_members()
    {
        using type = std::remove_cvref_t<Type>;
        if constexpr (std::is_array_v<type>) {
            return std::extent_v<type>;
        } else if constexpr (!std::is_class_v<type>) {
            return 0;
        } else if constexpr (concepts::container<type> && requires {
                                 std::integral_constant<
                                     std::size_t,
                                     type{}.size()>::value != 0;
                             }) {
            return type{}.size();
        } else if constexpr (concepts::tuple<type>) {
            return std::tuple_size_v<type>;
        } else if constexpr (requires {
                                 requires std::same_as<
                                     typename type::serialize,
                                     members<type::serialize::value>>;
                                 requires type::serialize::value !=
                                     std::numeric_limits<
                                         std::size_t>::max();
                             }) {
            return type::serialize::value;
        } else if constexpr (requires(Type && item) {
                                 requires std::same_as<
                                     decltype(serialize(item)),
                                     members<decltype(serialize(
                                         item))::value>>;
                                 requires decltype(serialize(
                                     item))::value !=
                                     std::numeric_limits<
                                         std::size_t>::max();
                             }) {
            return decltype(serialize(std::declval<type>()))::value;
#if ZPP_BITS_AUTODETECT_MEMBERS_MODE > 0
#if ZPP_BITS_AUTODETECT_MEMBERS_MODE == 1
            // clang-format off
        } else if constexpr (requires { [](Type && object) { auto && [a1] = object; }; }) { return 1; } else if constexpr (requires { [](Type && object) { auto && [a1, a2] = object; }; }) { return 2; /*.................................................................................................................*/ } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3] = object; }; }) { return 3; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4] = object; }; }) { return 4; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5] = object; }; }) { return 5; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6] = object; }; }) { return 6; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7] = object; }; }) { return 7; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8] = object; }; }) { return 8; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9] = object; }; }) { return 9; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10] = object; }; }) { return 10; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11] = object; }; }) { return 11; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12] = object; }; }) { return 12; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13] = object; }; }) { return 13; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14] = object; }; }) { return 14; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15] = object; }; }) { return 15; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16] = object; }; }) { return 16; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17] = object; }; }) { return 17; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18] = object; }; }) { return 18; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19] = object; }; }) { return 19; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20] = object; }; }) { return 20; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21] = object; }; }) { return 21; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22] = object; }; }) { return 22; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23] = object; }; }) { return 23; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24] = object; }; }) { return 24; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25] = object; }; }) { return 25; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26] = object; }; }) { return 26; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27] = object; }; }) { return 27; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28] = object; }; }) { return 28; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29] = object; }; }) { return 29; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30] = object; }; }) { return 30; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31] = object; }; }) { return 31; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32] = object; }; }) { return 32; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33] = object; }; }) { return 33; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34] = object; }; }) { return 34; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35] = object; }; }) { return 35; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36] = object; }; }) { return 36; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37] = object; }; }) { return 37; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38] = object; }; }) { return 38; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39] = object; }; }) { return 39; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40] = object; }; }) { return 40; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41] = object; }; }) { return 41; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42] = object; }; }) { return 42; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43] = object; }; }) { return 43; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44] = object; }; }) { return 44; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45] = object; }; }) { return 45; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46] = object; }; }) { return 46; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47] = object; }; }) { return 47; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48] = object; }; }) { return 48; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49] = object; }; }) { return 49; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50] = object; }; }) { return 50;
            // Returns the number of members
            // clang-format on
#else
#error "Invalid value for ZPP_BITS_AUTODETECT_MEMBERS_MODE"
#endif
#endif
        } else {
            return -1;
        }
    }

    constexpr static auto max_visit_members = 50;

    constexpr static decltype(auto) visit_members(
        auto && object,
        auto && visitor) requires(0 <=
                                  number_of_members<decltype(object)>()) &&
        (number_of_members<decltype(object)>() < max_visit_members)
    {
        constexpr auto count = number_of_members<decltype(object)>();

        // clang-format off
        if constexpr (count == 0) { return visitor(); } else if constexpr (count == 1) { auto && [a1] = object; return visitor(a1); } else if constexpr (count == 2) { auto && [a1, a2] = object; return visitor(a1, a2); /*......................................................................................................................................................................................................................................................................*/ } else if constexpr (count == 3) { auto && [a1, a2, a3] = object; return visitor(a1, a2, a3); } else if constexpr (count == 4) { auto && [a1, a2, a3, a4] = object; return visitor(a1, a2, a3, a4); } else if constexpr (count == 5) { auto && [a1, a2, a3, a4, a5] = object; return visitor(a1, a2, a3, a4, a5); } else if constexpr (count == 6) { auto && [a1, a2, a3, a4, a5, a6] = object; return visitor(a1, a2, a3, a4, a5, a6); } else if constexpr (count == 7) { auto && [a1, a2, a3, a4, a5, a6, a7] = object; return visitor(a1, a2, a3, a4, a5, a6, a7); } else if constexpr (count == 8) { auto && [a1, a2, a3, a4, a5, a6, a7, a8] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8); } else if constexpr (count == 9) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9); } else if constexpr (count == 10) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10); } else if constexpr (count == 11) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11); } else if constexpr (count == 12) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12); } else if constexpr (count == 13) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13); } else if constexpr (count == 14) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14); } else if constexpr (count == 15) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15); } else if constexpr (count == 16) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16); } else if constexpr (count == 17) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17); } else if constexpr (count == 18) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18); } else if constexpr (count == 19) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19); } else if constexpr (count == 20) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20); } else if constexpr (count == 21) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21); } else if constexpr (count == 22) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22); } else if constexpr (count == 23) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23); } else if constexpr (count == 24) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24); } else if constexpr (count == 25) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25); } else if constexpr (count == 26) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26); } else if constexpr (count == 27) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27); } else if constexpr (count == 28) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28); } else if constexpr (count == 29) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29); } else if constexpr (count == 30) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30); } else if constexpr (count == 31) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31); } else if constexpr (count == 32) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32); } else if constexpr (count == 33) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33); } else if constexpr (count == 34) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34); } else if constexpr (count == 35) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35); } else if constexpr (count == 36) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36); } else if constexpr (count == 37) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37); } else if constexpr (count == 38) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38); } else if constexpr (count == 39) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39); } else if constexpr (count == 40) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40); } else if constexpr (count == 41) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41); } else if constexpr (count == 42) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42); } else if constexpr (count == 43) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43); } else if constexpr (count == 44) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44); } else if constexpr (count == 45) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45); } else if constexpr (count == 46) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46); } else if constexpr (count == 47) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47); } else if constexpr (count == 48) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48); } else if constexpr (count == 49) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49); } else if constexpr (count == 50) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50);
            // Calls the visitor above with all data members of object.
        }
        // clang-format on
    }

    template <typename Type>
        constexpr static decltype(auto)
        visit_members_types(auto && visitor) requires(0 <= number_of_members<Type>()) &&
        (number_of_members<Type>() < max_visit_members)

    {
        using type = std::remove_cvref_t<Type>;
        constexpr auto count = number_of_members<Type>();

        // clang-format off
        if constexpr (count == 0) { return visitor.template operator()<>(); } else if constexpr (count == 1) { auto f = [&](auto && object) { auto && [a1] = object; return visitor.template operator()<decltype(a1)>(); }; /*......................................................................................................................................................................................................................................................................*/ return decltype(f(std::declval<type>()))(); } else if constexpr (count == 2) { auto f = [&](auto && object) { auto && [a1, a2] = object; return visitor.template operator()<decltype(a1), decltype(a2)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 3) { auto f = [&](auto && object) { auto && [a1, a2, a3] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 4) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 5) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 6) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 7) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 8) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 9) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 10) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 11) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 12) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 13) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 14) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 15) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 16) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 17) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 18) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 19) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 20) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 21) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 22) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 23) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 24) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 25) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 26) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 27) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 28) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 29) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 30) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 31) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 32) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31), decltype(a32)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 33) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31), decltype(a32), decltype(a33)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 34) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31), decltype(a32), decltype(a33), decltype(a34)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 35) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31), decltype(a32), decltype(a33), decltype(a34), decltype(a35)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 36) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31), decltype(a32), decltype(a33), decltype(a34), decltype(a35), decltype(a36)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 37) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31), decltype(a32), decltype(a33), decltype(a34), decltype(a35), decltype(a36), decltype(a37)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 38) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31), decltype(a32), decltype(a33), decltype(a34), decltype(a35), decltype(a36), decltype(a37), decltype(a38)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 39) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31), decltype(a32), decltype(a33), decltype(a34), decltype(a35), decltype(a36), decltype(a37), decltype(a38), decltype(a39)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 40) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31), decltype(a32), decltype(a33), decltype(a34), decltype(a35), decltype(a36), decltype(a37), decltype(a38), decltype(a39), decltype(a40)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 41) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31), decltype(a32), decltype(a33), decltype(a34), decltype(a35), decltype(a36), decltype(a37), decltype(a38), decltype(a39), decltype(a40), decltype(a41)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 42) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31), decltype(a32), decltype(a33), decltype(a34), decltype(a35), decltype(a36), decltype(a37), decltype(a38), decltype(a39), decltype(a40), decltype(a41), decltype(a42)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 43) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31), decltype(a32), decltype(a33), decltype(a34), decltype(a35), decltype(a36), decltype(a37), decltype(a38), decltype(a39), decltype(a40), decltype(a41), decltype(a42), decltype(a43)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 44) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31), decltype(a32), decltype(a33), decltype(a34), decltype(a35), decltype(a36), decltype(a37), decltype(a38), decltype(a39), decltype(a40), decltype(a41), decltype(a42), decltype(a43), decltype(a44)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 45) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31), decltype(a32), decltype(a33), decltype(a34), decltype(a35), decltype(a36), decltype(a37), decltype(a38), decltype(a39), decltype(a40), decltype(a41), decltype(a42), decltype(a43), decltype(a44), decltype(a45)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 46) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31), decltype(a32), decltype(a33), decltype(a34), decltype(a35), decltype(a36), decltype(a37), decltype(a38), decltype(a39), decltype(a40), decltype(a41), decltype(a42), decltype(a43), decltype(a44), decltype(a45), decltype(a46)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 47) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31), decltype(a32), decltype(a33), decltype(a34), decltype(a35), decltype(a36), decltype(a37), decltype(a38), decltype(a39), decltype(a40), decltype(a41), decltype(a42), decltype(a43), decltype(a44), decltype(a45), decltype(a46), decltype(a47)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 48) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31), decltype(a32), decltype(a33), decltype(a34), decltype(a35), decltype(a36), decltype(a37), decltype(a38), decltype(a39), decltype(a40), decltype(a41), decltype(a42), decltype(a43), decltype(a44), decltype(a45), decltype(a46), decltype(a47), decltype(a48)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 49) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31), decltype(a32), decltype(a33), decltype(a34), decltype(a35), decltype(a36), decltype(a37), decltype(a38), decltype(a39), decltype(a40), decltype(a41), decltype(a42), decltype(a43), decltype(a44), decltype(a45), decltype(a46), decltype(a47), decltype(a48), decltype(a49)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 50) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31), decltype(a32), decltype(a33), decltype(a34), decltype(a35), decltype(a36), decltype(a37), decltype(a38), decltype(a39), decltype(a40), decltype(a41), decltype(a42), decltype(a43), decltype(a44), decltype(a45), decltype(a46), decltype(a47), decltype(a48), decltype(a49), decltype(a50)>(); }; return decltype(f(std::declval<type>()))();
            // Returns Visitor<member-types...>();
        }
        // clang-format on
    }

    template <typename Type>
    constexpr static auto byte_serializable()
    {
        constexpr auto members_count = number_of_members<Type>();
        using type = std::remove_cvref_t<Type>;

        if constexpr (members_count < 0) {
            return false;
        } else if constexpr (!std::is_trivially_copyable_v<type>) {
            return false;
        } else if constexpr (
            !requires {
                requires std::integral_constant<
                    int,
                    (std::bit_cast<std::remove_all_extents_t<type>>(
                         std::array<
                             std::byte,
                             sizeof(std::remove_all_extents_t<type>)>()),
                     0)>::value == 0;
            }) {
            return false;
        } else if constexpr (!members_count) {
            return true;
        } else {
            return visit_members_types<type>([]<typename... Types>() {
                if constexpr (concepts::empty<type>) {
                    return std::false_type{};
                } else if constexpr ((... ||
                                      !byte_serializable<Types>())) {
                    return std::false_type{};
                } else if constexpr ((0 + ... + sizeof(Types)) !=
                                     sizeof(type)) {
                    return std::false_type{};
                } else if constexpr ((... || concepts::empty<Types>)) {
                    return std::false_type{};
                } else {
                    return std::true_type{};
                }
            })();
        }
    }
};

namespace concepts
{

template <typename Type>
concept byte_serializable = access::byte_serializable<Type>();

template <typename Archive, typename Type>
concept custom_serializable = requires (Archive && archive, Type && type) {
    std::remove_cv_t<Type>::serialize(archive, type);
} || requires (Archive && archive, Type && type) {
    serialize(archive, type);
};

template <typename Archive, typename Type>
concept serialize_as_bytes =
    byte_serializable<Type> && !custom_serializable<Archive, Type>;

} // namespace traits

enum class kind
{
    in,
    out
};

constexpr decltype(auto) visit_members(auto && object, auto && visitor)
{
    return access::visit_members(object, visitor);
}

template <typename Type>
constexpr decltype(auto) visit_members_types(auto && visitor)
{
    return access::visit_members_types<Type>(visitor);
}

template <concepts::byte_view ByteView>
class basic_out
{
public:
    template <typename>
    friend struct option;

    template <typename... Types>
    using template_type = basic_out<Types...>;

    friend access;

    template <typename, concepts::container>
    friend struct sized_container;

    using value_type = typename ByteView::value_type;

    constexpr explicit basic_out(ByteView && view) : m_data(view)
    {
        static_assert(!is_resizable);
    }

    constexpr explicit basic_out(ByteView & view) : m_data(view)
    {
    }

    constexpr explicit basic_out(ByteView && view, auto options) : m_data(view)
    {
        static_assert(!is_resizable);
        options(*this);
    }

    constexpr explicit basic_out(ByteView & view, auto options) : m_data(view)
    {
        options(*this);
    }

    constexpr auto operator()(auto &&... items)
    {
        return serialize_many(items...);
    }

    constexpr auto serialize_many(auto && first_item, auto &&... items)
    {
        if (auto result = serialize_one(first_item); failure(result))
            [[unlikely]] {
            return result;
        }

        return serialize_many(items...);
    }

    constexpr errc serialize_many()
    {
        return {};
    }

    constexpr std::size_t position() const
    {
        return m_position;
    }

    constexpr void reset(std::size_t position = 0)
    {
        m_position = position;
    }

    constexpr static auto kind()
    {
        return kind::out;
    }

protected:
    constexpr auto option(out_append)
    {
        static_assert(is_resizable);
        m_position = m_data.size();
    }

    constexpr auto option(out_reserve size)
    {
        static_assert(is_resizable);
        m_data.reserve(size.size);
    }

    constexpr auto option(out_resize size)
    {
        static_assert(is_resizable);
        m_data.out_resize(size.size);
        if (m_position > size.size) {
            m_position = size.size;
        }
    }

    constexpr errc serialize_one(concepts::unspecialized auto && item)
    {
        using type = std::remove_cvref_t<decltype(item)>;
        static_assert(!std::is_pointer_v<type>);

        if constexpr (requires { type::serialize(*this, item); }) {
            return type::serialize(*this, item);
        } else if constexpr (requires { serialize(*this, item); }) {
            return serialize(*this, item);
        } else if constexpr (std::is_fundamental_v<type> || std::is_enum_v<type>) {
            auto size = m_data.size();
            if (m_position + sizeof(item) > size) [[unlikely]] {
                if constexpr (is_resizable) {
                    m_data.resize((sizeof(item) + size) * 3 / 2);
                } else {
                    return std::errc::result_out_of_range;
                }
            }
            if (std::is_constant_evaluated()) {
                auto value = std::bit_cast<
                    std::array<std::remove_const_t<value_type>,
                               sizeof(item)>>(item);
                for (std::size_t i = 0; i < sizeof(value); ++i) {
                    m_data[m_position + i] = value[i];
                }
            } else {
                std::memcpy(
                    m_data.data() + m_position, &item, sizeof(item));
            }
            m_position += sizeof(item);
            return {};
        } else if constexpr (requires {
                                 typename std::enable_if_t<std::same_as<
                                     bytes<typename type::value_type>,
                                     type>>;
                             }) {
            auto size = m_data.size();
            auto item_size_in_bytes = item.size_in_bytes();
            if (m_position + item_size_in_bytes > size) [[unlikely]] {
                if constexpr (is_resizable) {
                    m_data.resize((item_size_in_bytes + size) * 3 / 2);
                } else {
                    return std::errc::result_out_of_range;
                }
            }
            if (std::is_constant_evaluated()) {
                auto count = item.count();
                for (std::size_t index = 0; index < count; ++index) {
                    auto value = std::bit_cast<
                        std::array<std::remove_const_t<value_type>,
                                   sizeof(typename type::value_type)>>(
                        item.data()[index]);
                    for (std::size_t i = 0;
                         i < sizeof(typename type::value_type);
                         ++i) {
                        m_data[m_position +
                               index * sizeof(typename type::value_type) +
                               i] = value[i];
                    }
                }
            } else {
                std::memcpy(m_data.data() + m_position,
                            item.data(),
                            item_size_in_bytes);
            }
            m_position += item_size_in_bytes;
            return {};
        } else if constexpr (concepts::byte_serializable<type>) {
            return serialize_one(as_bytes(item));
        } else {
            return visit_members(
                item, [&](auto &&... items) constexpr {
                    return serialize_many(items...);
                });
        }
    }

    template <typename SizeType = default_size_type, std::size_t Count = 0>
    constexpr errc serialize_one(auto(&array)[Count])
    {
        using value_type = std::remove_cvref_t<decltype(array[0])>;

        if constexpr (concepts::serialize_as_bytes<decltype(*this),
                                                   value_type>) {
            return serialize_one(bytes(array));
        } else {
            for (auto & item : array) {
                if (auto result = serialize_one(item); failure(result))
                    [[unlikely]] {
                    return result;
                }
            }
            return {};
        }
    }

    template <typename SizeType = default_size_type>
    constexpr errc serialize_one(concepts::container auto && container)
    {
        using type = std::remove_cvref_t<decltype(container)>;
        using value_type = typename type::value_type;

        if constexpr (!std::is_void_v<SizeType> &&
                      (
                          concepts::associative_container<
                              decltype(container)> ||
                          requires(type container) {
                              container.resize(1);
                          } ||
                          requires (type container) {
                              container = {container.data(), 1};
                          })) {
            if (auto result =
                    serialize_one(static_cast<SizeType>(container.size()));
                failure(result)) [[unlikely]] {
                return result;
            }
        }

        if constexpr (concepts::serialize_as_bytes<decltype(*this),
                                                   value_type> &&
                      std::is_base_of_v<std::random_access_iterator_tag,
                                        typename std::iterator_traits<
                                            typename type::iterator>::
                                            iterator_category> &&
                      requires { container.data(); }) {
            return serialize_one(bytes(container));
        } else {
            for (auto & item : container) {
                if (auto result = serialize_one(item); failure(result))
                    [[unlikely]] {
                    return result;
                }
            }
            return {};
        }
    }

    constexpr errc serialize_one(concepts::tuple auto && tuple)
    {
        return serialize_one(tuple,
                             std::make_index_sequence<std::tuple_size_v<
                                 std::remove_cvref_t<decltype(tuple)>>>());
    }

    template <std::size_t... Indices>
    constexpr errc serialize_one(concepts::tuple auto && tuple,
                                 std::index_sequence<Indices...>)
    {
        return serialize_many(std::get<Indices>(tuple)...);
    }

    constexpr errc serialize_one(concepts::optional auto && optional)
    {
        if (optional.has_value()) {
            return serialize_many(std::byte(true), *optional);
        } else {
            return serialize_one(std::byte(false));
        }
    }

    constexpr errc serialize_one(concepts::variant auto && variant)
    {
        using type = std::remove_cvref_t<decltype(variant)>;
        static_assert(std::variant_size_v<type> < 0xff);

        auto variant_index = variant.index();
        if (std::variant_npos == variant_index) [[unlikely]] {
            return std::errc::invalid_argument;
        }

        return std::visit(
            [index = static_cast<std::byte>(variant_index & 0xff),
             this](auto & object) { return this->serialize_many(index, object); },
            variant);
    }

    constexpr errc serialize_one(concepts::owning_pointer auto && pointer)
    {
        if (nullptr == pointer) [[unlikely]] {
            return std::errc::invalid_argument;
        }

        return serialize_one(*pointer);
    }

    constexpr ~basic_out() = default;

    constexpr static bool is_resizable = requires(ByteView view)
    {
        view.resize(1);
    };

    using view_type =
        std::conditional_t<is_resizable,
                           ByteView &,
                           std::span<typename ByteView::value_type>>;

    view_type m_data{};
    std::size_t m_position{};
};

template <concepts::byte_view ByteView = std::vector<std::byte>>
class out : public basic_out<ByteView>
{
public:
    template <typename... Types>
    using template_type = out<Types...>;

    friend access;

    using basic_out<ByteView>::basic_out;

    constexpr auto operator()(auto &&... items)
    {
        if constexpr (is_resizable) {
            auto end = m_data.size();
            auto result = serialize_many(items...);
            if (m_position > end) {
                m_data.resize(m_position);
            }
            return result;
        } else {
            return serialize_many(items...);
        }
    }

private:
    using basic_out<ByteView>::is_resizable;
    using basic_out<ByteView>::serialize_many;
    using basic_out<ByteView>::m_data;
    using basic_out<ByteView>::m_position;
};

template <typename Type>
out(Type &) -> out<Type>;

template <typename Type>
out(Type &&) -> out<Type>;

template <concepts::byte_view ByteView = std::vector<std::byte>>
class in
{
public:
    template <typename... Types>
    using template_type = in<Types...>;

    template <typename>
    friend struct option;

    friend access;

    template <typename, concepts::container>
    friend struct sized_container;

    using value_type =
        std::conditional_t<std::is_const_v<std::remove_reference_t<
                               decltype(std::declval<ByteView>()[1])>>,
                           std::add_const_t<typename ByteView::value_type>,
                           typename ByteView::value_type>;

    constexpr explicit in(ByteView && view) : m_data(view)
    {
        static_assert(!is_resizable);
    }

    constexpr explicit in(ByteView & view) : m_data(view)
    {
    }

    constexpr explicit in(ByteView && view, auto options) : m_data(view)
    {
        static_assert(!is_resizable);
        options(*this);
    }

    constexpr explicit in(ByteView & view, auto options) : m_data(view)
    {
        options(*this);
    }

    constexpr auto operator()(auto &&... items)
    {
        return serialize_many(items...);
    }

    constexpr std::size_t position() const
    {
        return m_position;
    }

    constexpr void reset(std::size_t position = 0)
    {
        m_position = position;
    }

    constexpr static auto kind()
    {
        return kind::out;
    }

private:
    constexpr auto serialize_many(auto && first_item, auto &&... items)
    {
        if (auto result = serialize_one(first_item); failure(result))
            [[unlikely]] {
            return result;
        }

        return serialize_many(items...);
    }

    constexpr errc serialize_many()
    {
        return {};
    }

    constexpr errc serialize_one(concepts::unspecialized auto && item)
    {
        using type = std::remove_cvref_t<decltype(item)>;
        static_assert(!std::is_pointer_v<type>);

        if constexpr (requires { type::serialize(*this, item); }) {
            return type::serialize(*this, item);
        } else if constexpr (requires { serialize(*this, item); }) {
            return serialize(*this, item);
        } else if constexpr (std::is_fundamental_v<type> || std::is_enum_v<type>) {
            auto size = m_data.size();
            if (m_position + sizeof(item) > size) [[unlikely]] {
                return std::errc::result_out_of_range;
            }
            if (std::is_constant_evaluated()) {
                std::array<std::remove_const_t<value_type>, sizeof(item)>
                    value;
                for (std::size_t i = 0; i < sizeof(value); ++i) {
                    value[i] = value_type(m_data[m_position + i]);
                }
                item = std::bit_cast<type>(value);
            } else {
                std::memcpy(
                    &item, m_data.data() + m_position, sizeof(item));
            }
            m_position += sizeof(item);
            return {};
        } else if constexpr (requires {
                                 typename std::enable_if_t<std::same_as<
                                     bytes<typename type::value_type>,
                                     type>>;
                             }) {
            auto size = m_data.size();
            auto item_size_in_bytes = item.size_in_bytes();
            if (m_position + item_size_in_bytes > size) [[unlikely]] {
                return std::errc::result_out_of_range;
            }
            if (std::is_constant_evaluated()) {
                std::size_t count = item.count();
                for (std::size_t index = 0; index < count; ++index) {
                    std::array<std::remove_const_t<value_type>,
                               sizeof(typename type::value_type)>
                        value;
                    for (std::size_t i = 0;
                         i < sizeof(typename type::value_type);
                         ++i) {
                        value[i] = value_type(
                            m_data[m_position +
                                   index *
                                       sizeof(typename type::value_type) +
                                   i]);
                    }
                    item.data()[index] =
                        std::bit_cast<typename type::value_type>(value);
                }
            } else {
                std::memcpy(item.data(),
                            m_data.data() + m_position,
                            item_size_in_bytes);
            }
            m_position += item_size_in_bytes;
            return {};
        } else if constexpr (concepts::byte_serializable<type>) {
            return serialize_one(as_bytes(item));
        } else {
            return visit_members(
                item, [&](auto &&... items) constexpr {
                    return serialize_many(items...);
                });
        }
    }

    template <typename SizeType = default_size_type, std::size_t Count = 0>
    constexpr errc serialize_one(auto(&array)[Count])
    {
        using value_type = std::remove_cvref_t<decltype(array[0])>;

        if constexpr (concepts::serialize_as_bytes<decltype(*this),
                                                   value_type>) {
            return serialize_one(bytes(array));
        } else {
            for (auto & item : array) {
                if (auto result = serialize_one(item); failure(result))
                    [[unlikely]] {
                    return result;
                }
            }
            return {};
        }
    }

    template <typename SizeType = default_size_type>
    constexpr errc
    serialize_one(concepts::container auto && container)
    {
        using type = std::remove_cvref_t<decltype(container)>;
        using value_type = typename type::value_type;

        if constexpr (!std::is_void_v<SizeType> &&
                      (
                          requires (type container) { container.resize(1); } ||
                          requires (type container) {
                              container = {container.data(), 1};
                          })) {
            SizeType size{};
            if (auto result = serialize_one(size); failure(result))
                [[unlikely]] {
                return result;
            }

            if constexpr (requires(type container) {
                              container.resize(size);
                          }) {
                container.resize(size);
            } else {
                if (size > container.size()) [[unlikely]] {
                    return std::errc::value_too_large;
                }

                container = {container.data(), size};
            }
        }

        if constexpr (concepts::serialize_as_bytes<decltype(*this),
                                                   value_type> &&
                      std::is_base_of_v<std::random_access_iterator_tag,
                                        typename std::iterator_traits<
                                            typename type::iterator>::
                                            iterator_category> &&
                      requires { container.data(); }) {
            return serialize_one(bytes(container));
        } else {
            for (auto & item : container) {
                if (auto result = serialize_one(item); failure(result))
                    [[unlikely]] {
                    return result;
                }
            }
            return {};
        }
    }

    template <typename SizeType = default_size_type>
    constexpr errc
    serialize_one(concepts::associative_container auto && container)
    {
        using type = typename std::remove_cvref_t<decltype(container)>;

        SizeType size{};

        if constexpr (!std::is_void_v<SizeType>) {
            if (auto result = serialize_one(size); failure(result))
                [[unlikely]] {
                return result;
            }
        } else {
            size = container.size();
        }

        container.clear();

        for (SizeType index{}; index < size; ++index) {
            if constexpr (requires { typename type::mapped_type; }) {
                using value_type = std::pair<typename type::key_type,
                                             typename type::mapped_type>;
                std::aligned_storage_t<sizeof(value_type),
                                       alignof(value_type)>
                    storage;

                std::unique_ptr<value_type, void (*)(value_type *)> object(
                    access::placement_new<value_type>(
                        std::addressof(storage)),
                    [](auto pointer) { access::destruct(*pointer); });
                if (auto result = serialize_one(*object);
                    failure(result)) [[unlikely]] {
                    return result;
                }

                container.insert(std::move(*object));
            } else {
                using value_type = typename type::value_type;

                std::aligned_storage_t<sizeof(value_type),
                                       alignof(value_type)>
                    storage;

                std::unique_ptr<value_type, void (*)(value_type *)> object(
                    access::placement_new<value_type>(
                        std::addressof(storage)),
                    [](auto pointer) { access::destruct(*pointer); });
                if (auto result = serialize_one(*object);
                    failure(result)) [[unlikely]] {
                    return result;
                }

                container.insert(std::move(*object));
            }
        }

        return {};
    }

    constexpr errc serialize_one(concepts::tuple auto && tuple)
    {
        return serialize_one(tuple,
                             std::make_index_sequence<std::tuple_size_v<
                                 std::remove_cvref_t<decltype(tuple)>>>());
    }

    template <std::size_t... Indices>
    constexpr errc serialize_one(concepts::tuple auto && tuple,
                                 std::index_sequence<Indices...>)
    {
        return serialize_many(std::get<Indices>(tuple)...);
    }

    constexpr errc serialize_one(concepts::optional auto && optional)
    {
        using value_type = std::remove_reference_t<decltype(*optional)>;

        std::byte has_value{};
        if (auto result = serialize_one(has_value); failure(result))
            [[unlikely]] {
            return result;
        }

        if (!bool(has_value)) {
            optional = std::nullopt;
            return {};
        }

        if constexpr (std::is_default_constructible_v<value_type>) {
            if (!optional) {
                optional = value_type{};
            }

            if (auto result = serialize_one(*optional); failure(result))
                [[unlikely]] {
                return result;
            }
        } else {
            std::aligned_storage_t<sizeof(value_type), alignof(value_type)>
                storage;

            std::unique_ptr<value_type, void (*)(value_type *)> object(
                access::placement_new<value_type>(std::addressof(storage)),
                [](auto pointer) { access::destruct(*pointer); });

            if (auto result = serialize_one(*optional); failure(result))
                [[unlikely]] {
                return result;
            }

            optional = std::move(*object);
        }

        return {};
    }

    template <typename... Types, template <typename...> typename Variant>
    constexpr errc serialize_one(Variant<Types...> & variant) requires
        concepts::variant<Variant<Types...>>
    {
        static_assert(sizeof...(Types) < 0xff);

        std::byte index{};
        if (auto result = serialize_one(index); failure(result))
            [[unlikely]] {
            return result;
        }

        if (std::size_t(index) >= sizeof...(Types)) [[unlikely]] {
            return std::errc::value_too_large;
        }

        using loader_type =
            errc (*)(decltype(*this), decltype(variant) &);

        constexpr loader_type loaders[] = {[](auto & self, auto & variant) {
            if constexpr (std::is_default_constructible_v<Types>) {
                if (!std::get_if<Types>(&variant)) {
                    variant = Types{};
                }
                return self.serialize_one(*std::get_if<Types>(&variant));
            } else {
                std::aligned_storage_t<sizeof(Types), alignof(Types)> storage;

                std::unique_ptr<Types, void (*)(Types *)> object(
                    access::placement_new<Types>(std::addressof(storage)),
                    [](auto pointer) { access::destruct(*pointer); });

                if (auto result = self.serialize_one(*object);
                    failure(result)) [[unlikely]] {
                    return result;
                }
                variant = std::move(*object);
            }
        }...};

        return loaders[std::size_t(index)](*this, variant);
    }

    constexpr errc serialize_one(concepts::owning_pointer auto && pointer)
    {
        using type = std::remove_reference_t<decltype(*pointer)>;

        auto loaded = access::make_unique<type>();;
        if (auto result = serialize_one(*loaded); failure(result))
            [[unlikely]] {
            return result;
        }

        pointer.reset(loaded.release());
        return {};
    }

    constexpr static bool is_resizable = requires(ByteView view)
    {
        view.resize(1);
    };

    using view_type =
        std::conditional_t<is_resizable,
                           ByteView &,
                           std::span<value_type>>;

    view_type m_data{};
    std::size_t m_position{};
};

constexpr auto input(auto && view, auto &&... option)
{
    return in(std::forward<decltype(view)>(view),
              std::forward<decltype(option)>(option)...);
}

constexpr auto output(auto && view, auto &&... option)
{
    return out(std::forward<decltype(view)>(view),
               std::forward<decltype(option)>(option)...);
}

constexpr auto in_out(auto && view, auto &&... option)
{
    return std::tuple{in(std::forward<decltype(view)>(view),
                         std::forward<decltype(option)>(option)...),
                      out(std::forward<decltype(view)>(view),
                          std::forward<decltype(option)>(option)...)};
}

template <typename ByteType = std::byte>
constexpr auto data_in_out(auto &&... option)
{
    struct data_in_out
    {
        data_in_out(decltype(option) &&... option) :
            input(data, std::forward<decltype(option)>(option)...),
            output(data, std::forward<decltype(option)>(option)...)
        {
        }

        std::vector<ByteType> data;
        in<decltype(data)> input;
        out<decltype(data)> output;
    };
    return data_in_out{std::forward<decltype(option)>(option)...};
}

template <typename ByteType = std::byte>
constexpr auto data_in(auto &&... option)
{
    struct data_in
    {
        data_in(decltype(option) &&... option) :
            input(data, std::forward<decltype(option)>(option)...)
        {
        }

        std::vector<ByteType> data;
        in<decltype(data)> input;
    };
    return data_in{std::forward<decltype(option)>(option)...};
}

template <typename ByteType = std::byte>
constexpr auto data_out(auto &&... option)
{
    struct data_out
    {
        data_out(decltype(option) &&... option) :
            output(data, std::forward<decltype(option)>(option)...)
        {
        }

        std::vector<ByteType> data;
        out<decltype(data)> output;
    };
    return data_out{std::forward<decltype(option)>(option)...};
}

} // namespace zpp::bits

#endif // ZPP_BITS_H

