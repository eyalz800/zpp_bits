#ifndef ZPP_BITS_H
#define ZPP_BITS_H

#include <array>
#include <bit>
#include <climits>
#include <concepts>
#include <cstddef>
#include <iterator>
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
    std::tuple_size_v<std::remove_cvref_t<Type>>;
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
    constexpr zpp::throwing<void> operator co_await()
    {
        if (failure(code)) [[unlikely]] {
            return code;
        }
        return zpp::void_v;
    }
#endif

    constexpr operator std::errc()
    {
        return code;
    }

#ifdef __cpp_exceptions
    constexpr void or_throw()
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
};

enum class kind
{
    in,
    out
};

template <concepts::byte_view ByteView>
class basic_out
{
public:
    template <typename>
    friend struct option;

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

    constexpr errc serialize_one(auto && item)
    {
        using type = std::remove_cvref_t<decltype(item)>;
        static_assert(!std::is_pointer_v<type>);

        if constexpr (std::is_fundamental_v<type> || std::is_enum_v<type>) {
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
        } else if constexpr (requires { type::serialize(*this, item); }) {
            return type::serialize(*this, item);
        } else {
            return serialize(*this, item);
        }
    }

    template <typename SizeType = default_size_type, std::size_t Count = 0>
    constexpr errc serialize_one(auto(&array)[Count])
    {
        using value_type = std::remove_cvref_t<decltype(array[0])>;

        if constexpr (std::is_fundamental_v<value_type> ||
                       std::is_enum_v<value_type>) {
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

        if constexpr ((std::is_fundamental_v<value_type> ||
                       std::is_enum_v<value_type>)&&std::
                          is_base_of_v<std::random_access_iterator_tag,
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

    ~basic_out() = default;

    static constexpr bool is_resizable = requires(ByteView view)
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

template <concepts::byte_view ByteView>
class out : public basic_out<ByteView>
{
public:
    using basic_out<ByteView>::basic_out;

    constexpr auto operator()(auto &&... items)
    {
        if constexpr (is_resizable) {
            auto result = serialize_many(items...);
            m_data.resize(m_position);
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

template <concepts::byte_view ByteView>
class in
{
public:
    template <typename>
    friend struct option;

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

    constexpr errc serialize_one(auto && item)
    {
        using type = std::remove_cvref_t<decltype(item)>;
        static_assert(!std::is_pointer_v<type>);

        if constexpr (std::is_fundamental_v<type> || std::is_enum_v<type>) {
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
        } else if constexpr (requires { type::serialize(*this, item); }) {
            return type::serialize(*this, item);
        } else {
            return serialize(*this, item);
        }
    }

    template <typename SizeType = default_size_type, std::size_t Count = 0>
    constexpr errc serialize_one(auto(&array)[Count])
    {
        using value_type = std::remove_cvref_t<decltype(array[0])>;

        if constexpr (std::is_fundamental_v<value_type> ||
                       std::is_enum_v<value_type>) {
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

        if constexpr ((std::is_fundamental_v<value_type> ||
                       std::is_enum_v<value_type>)&&std::
                          is_base_of_v<std::random_access_iterator_tag,
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
