/**
 * @file types.h
 * @brief Provides various utility classes and functions.
 */

#pragma once

#include <type_traits>

namespace PlainCloud::Util::Types {

/**
 * @brief Defines a helper type for the `std::visit` function.
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

} // namespace PlainCloud::Util::Types
