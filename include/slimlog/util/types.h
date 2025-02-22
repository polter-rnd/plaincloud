/**
 * @file types.h
 * @brief Provides various utility classes and functions.
 */

#pragma once

#include <cassert>
#include <string_view>
#include <type_traits>

namespace SlimLog::Util::Types {

/**
 * @brief Helper type to create an overloaded function object.
 *
 * Combines multiple lambdas or function objects into a single callable entity.
 * Useful with `std::visit` when working with `std::variant`.
 *
 * Example usage:
 * ```cpp
 * std::visit(Overloaded{
 *     [](auto arg) { std::cout << arg << ' '; },
 *     [](double arg) { std::cout << std::fixed << arg << ' '; },
 *     [](const std::string& arg) { std::cout << std::quoted(arg) << ' '; }
 * }, v);
 * ```
 *
 * @tparam Ts Variant types.
 */
template<class... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};

/**
 * @brief Defines a type that is always false for unconditional static assertions.
 *
 * Example usage:
 * ```cpp
 * static_assert(Util::Types::AlwaysFalse<T>{}, "Assertion failed");
 * ```
 */
template<typename>
struct AlwaysFalse : std::false_type { };

/// @cond
template<typename T>
struct UnderlyingChar {
    static_assert(AlwaysFalse<T>{}, "Unable to deduce the underlying char type");
};
/// @endcond

/**
 * @brief Detects the underlying char type of a string.
 *
 * @tparam T String type.
 */
template<typename T>
    requires std::is_integral_v<typename std::remove_cvref_t<T>::value_type>
struct UnderlyingChar<T> {
    /** @brief Defines the underlying char type, specialization for STL strings. */
    using Type = typename std::remove_cvref_t<T>::value_type;
};

template<typename T>
    requires std::is_array_v<std::remove_cvref_t<T>>
struct UnderlyingChar<T> {
    /** @brief Defines the underlying char type, specialization for char arrays. */
    using Type = typename std::remove_cvref_t<typename std::remove_all_extents_t<T>>;
};

template<typename T>
    requires std::is_integral_v<typename std::remove_pointer_t<T>>
struct UnderlyingChar<T> {
    /** @brief Defines the underlying char type, specialization for char pointers. */
    using Type = typename std::remove_cvref_t<typename std::remove_pointer_t<T>>;
};

/**
 * @brief Provides an alias for UnderlyingChar<T>::Type.
 *
 * @tparam T String type.
 */
template<typename T>
using UnderlyingCharType = typename UnderlyingChar<T>::Type;

/**
 * @brief Casts a nonnegative integer to unsigned.
 *
 * @trapam Int Integer type.
 *
 * @param value Non-negative integer value.
 */
template<typename Int>
constexpr auto to_unsigned(Int value) -> std::make_unsigned_t<Int>
{
    assert(std::is_unsigned_v<Int> || value >= 0);
    return static_cast<std::make_unsigned_t<Int>>(value);
}

} // namespace SlimLog::Util::Types
