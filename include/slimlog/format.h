/**
 * @file format.h
 * @brief Contains declarations and definitions of classes related to formatting.
 */

#pragma once

#include "slimlog/location.h"
#include "slimlog/util/buffer.h"
#include "slimlog/util/types.h"

#ifdef SLIMLOG_FMTLIB
#if __has_include(<fmt/base.h>)
#include <fmt/base.h>
#else
#include <fmt/core.h>
#endif
#include <fmt/chrono.h> // IWYU pragma: keep
#include <fmt/format.h> // IWYU pragma: keep
#include <fmt/xchar.h> // IWYU pragma: keep
#else
#include <format>
#endif

#include <concepts>
#include <cstring>
#include <iterator>
#include <memory>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>

namespace SlimLog {

#ifdef SLIMLOG_FMTLIB
/** @brief Alias for \a fmt::basic_format_string. */
template<typename T, typename... Args>
using FormatString = fmt::basic_format_string<T, Args...>;
/** @brief Alias for \a fmt::format_error. */
using FormatError = fmt::format_error;
/** @brief Alias for \a fmt::formatter. */
template<typename T, typename Char>
using Formatter = fmt::formatter<T, Char>;
/** @brief Alias for \a fmt::basic_format_parse_context. */
template<typename Char>
using FormatParseContext = fmt::basic_format_parse_context<Char>;
#else
/** @brief Alias for std::basic_format_string. */
template<typename T, typename... Args>
using FormatString = std::basic_format_string<T, Args...>;
/** @brief Alias for \a std::format_error. */
using FormatError = std::format_error;
/** @brief Alias for \a std::formatter. */
template<typename T, typename Char>
using Formatter = std::formatter<T, Char>;
/** @brief Alias for \a std::basic_format_parse_context. */
template<typename Char>
using FormatParseContext = std::basic_format_parse_context<Char>;
#endif

/**
 * @brief Wrapper class consisting of a format string and location.
 *
 * Allows passing a format string along with the source location information.
 *
 * @tparam Char Character type of the format string.
 * @tparam Args Format argument types.
 */
template<typename Char, typename... Args>
class Format final {
public:
    /**
     * @brief Constructs a new Format object from a format string and location.
     *
     * @tparam T Format string type.
     * @param fmt Format string.
     * @param loc Source code location.
     */
    template<typename T>
        requires(std::constructible_from<FormatString<Char, Args...>, T>
                 || std::same_as<std::decay_t<T>, const Char*>
                 || std::same_as<std::decay_t<T>, Char*>)
    // NOLINTNEXTLINE(*-explicit-conversions)
    consteval Format(T fmt, Location loc = Location::current()) // cppcheck-suppress passedByValue
        : m_fmt(std::move(fmt))
        , m_loc(loc)
    {
    }

    /** @brief Copy constructor. */
    Format(const Format&) = default;
    /** @brief Move constructor. */
    Format(Format&&) noexcept = default;
    /** @brief Destructor. */
    ~Format() = default;

    /** @brief Assignment operator. */
    auto operator=(const Format&) -> Format& = default;
    /** @brief Move assignment operator. */
    auto operator=(Format&&) noexcept -> Format& = default;

    /**
     * @brief Gets the format string.
     *
     * @return The format string.
     */
    [[nodiscard]] constexpr auto fmt() const -> const auto&
    {
        return m_fmt;
    }

    /**
     * @brief Gets the source location.
     *
     * @return The source location.
     */
    [[nodiscard]] constexpr auto loc() const -> const auto&
    {
        return m_loc;
    }

private:
    FormatString<Char, Args...> m_fmt;
    Location m_loc;
};

/**
 * @brief Require that formatting supports particular character and argument types.
 *
 * @tparam Char Character type of the format string.
 * @tparam Args Format argument types.
 */
template<typename Char, typename... Args>
concept Formattable = requires(const Char* fmt, Args... args) {
#ifdef SLIMLOG_FMTLIB
    fmt::format(fmt, args...);
#else
    std::format(fmt, args...);
#endif
};

/**
 * @brief Buffer used for log message formatting.
 *
 * @tparam Char Underlying character type for the string.
 * @tparam BufferSize Size of the buffer.
 * @tparam Allocator Allocator for the buffer data.
 */
template<Formattable Char, std::size_t BufferSize, typename Allocator = std::allocator<Char>>
class FormatBuffer final : public Util::MemoryBuffer<Char, BufferSize, Allocator> {
public:
    using Util::MemoryBuffer<Char, BufferSize, Allocator>::MemoryBuffer;

    /**
     * @brief Appends data to the end of the buffer.
     *
     * @tparam ContiguousRange Type of the source object.
     *
     * @param range Source object containing data to be added to the buffer.
     */
    template<typename ContiguousRange>
    void append(const ContiguousRange& range) // cppcheck-suppress duplInheritedMember
    {
        append(range.data(), std::next(range.data(), range.size()));
    }

    /**
     * @brief Appends data to the end of the buffer.
     *
     * @tparam U Input data type.
     * @param begin Begin input iterator.
     * @param end End input iterator.
     */
    template<typename U>
    void append(const U* begin, const U* end) // cppcheck-suppress duplInheritedMember
    {
        const auto buf_size = this->size();
        const auto count = Util::Types::to_unsigned(end - begin);
        this->resize(buf_size + count);
        std::uninitialized_copy_n(begin, count, std::next(this->begin(), buf_size));
    }

    /**
     * @brief Formats a log message with compile-time argument checks.
     *
     * @tparam Args Format argument types.
     * @param fmt Format string.
     * @param args Format arguments.
     */
    template<typename... Args>
        requires Formattable<Char, Args...>
    auto format(FormatString<Char, std::type_identity_t<Args>...> fmt, Args&&... args) -> void
    {
#ifdef SLIMLOG_FMTLIB
        if constexpr (std::is_same_v<Char, char>) {
            fmt::format_to(fmt::appender(*this), std::move(fmt), std::forward<Args>(args)...);
        } else {
            fmt::format_to(
                std::back_inserter(*this),
#if FMT_VERSION < 110000
                static_cast<fmt::basic_string_view<Char>>(std::move(fmt)),
#else
                std::move(fmt),
#endif
                std::forward<Args>(args)...);
        }
#else
        std::format_to(std::back_inserter(*this), std::move(fmt), std::forward<Args>(args)...);
#endif
    }

    /**
     * @brief Formats a log message using argument storage.
     *
     * @tparam Args Format argument types. Deduced from arguments.
     * @param fmt Format string.
     * @param args Format argument storage (see make_format_args).
     * @throws FormatError if fmt is not a valid format string for the provided arguments.
     */
    template<typename Args>
    auto vformat(std::basic_string_view<Char> fmt, Args&& args) -> void
    {
#ifdef SLIMLOG_FMTLIB
        if constexpr (std::is_same_v<Char, char>) {
            fmt::vformat_to(fmt::appender(*this), std::move(fmt), std::forward<Args>(args));
        } else {
            fmt::vformat_to(std::back_inserter(*this), std::move(fmt), std::forward<Args>(args));
        }
#else
        std::vformat_to(std::back_inserter(*this), std::move(fmt), std::forward<Args>(args));
#endif
    }

    /**
     * @brief Returns an object that stores an array of formatting arguments.
     *
     * @tparam Char Character type of the format string.
     * @tparam Args Format argument types.
     * @param args Format arguments.
     * @return Format argument storage.
     */
    template<typename... Args>
    static constexpr auto make_format_args(Args... args) -> auto
    {
#if defined(SLIMLOG_FMTLIB) and FMT_VERSION >= 110000
        return fmt::make_format_args<fmt::buffered_context<Char>>(args...);
#elif defined(SLIMLOG_FMTLIB)
        return fmt::make_format_args<fmt::buffer_context<Char>>(args...);
#else
        if constexpr (std::is_same_v<Char, char>) {
            return std::make_format_args(args...);
        } else if constexpr (std::is_same_v<Char, wchar_t>) {
            return std::make_wformat_args(args...);
        } else {
            static_assert(
                Util::Types::AlwaysFalse<Char>{},
                "std::vformat_to() supports only `char` or `wchar_t` character types");
        }
#endif
    }
};

#ifndef SLIMLOG_FMTLIB
template<typename T, Formattable<T> Char>
class CachedFormatter;

/**
 * @brief Wrapper for formatting a value with a cached formatter.
 *
 * Stores the value and reference to the cached formatter (see CachedFormatter).
 * Used in combination with `std::formatter<FormatValue<T, Char>, Char>` specialization.
 *
 * @tparam T Value type.
 * @tparam Char Output character type.
 */
template<typename T, Formattable<T> Char>
class FormatValue final {
public:
    /**
     * @brief Constructs a new FormatValue object from a reference to the formatter and value.
     *
     * @param formatter Reference to the formatter.
     * @param value Value to be formatted.
     */
    explicit FormatValue(const CachedFormatter<T, Char>& formatter, T value);
    ~FormatValue() = default;

    FormatValue(const FormatValue&) = delete;
    FormatValue(FormatValue&&) = delete;
    auto operator=(const FormatValue&) -> FormatValue& = delete;
    auto operator=(FormatValue&&) noexcept -> FormatValue& = delete;

    /**
     * @brief Formats the value with the cached formatter.
     *
     * @tparam Context Format context type (`std::format_context`).
     * @param context Format context.
     * @return Output iterator to the resulting buffer.
     */
    template<typename Context>
    auto format(Context& context) const;

private:
    const CachedFormatter<T, Char>& m_formatter;
    T m_value;
};
#endif

/**
 * @brief Wrapper to parse the format string and store the format context.
 *
 * Used to parse format string only once and then cache the formatter.
 *
 * @tparam T Value type.
 * @tparam Char Output character type.
 */
template<typename T, Formattable<T> Char>
class CachedFormatter final : Formatter<T, Char> {
public:
    using Formatter<T, Char>::format;

    /**
     * @brief Constructs a new CachedFormatter object from a format string.
     *
     * @param fmt Format string.
     */
    explicit CachedFormatter(std::basic_string_view<Char> fmt);

    /**
     * @brief Formats the value and writes to the output buffer.
     *
     * @tparam Out Output buffer type (see MemoryBuffer).
     * @param out Output buffer.
     * @param value Value to be formatted.
     */
    template<typename Out>
    void format(Out& out, T value) const;

private:
#ifdef SLIMLOG_FMTLIB
    bool m_empty;
#endif
    mutable std::optional<T> m_value;
    mutable FormatBuffer<Char, 32> m_buffer;
};

} // namespace SlimLog

#ifndef SLIMLOG_FMTLIB
/** @cond */
template<typename T, SlimLog::Formattable<T> Char>
struct std::formatter<SlimLog::FormatValue<T, Char>, Char> { // NOLINT(cert-dcl58-cpp)
    constexpr auto parse(SlimLog::FormatParseContext<Char>& context)
    {
        return context.begin();
    }

    template<typename Context>
    auto format(const SlimLog::FormatValue<T, Char>& wrapper, Context& context) const
    {
        return wrapper.format(context);
    }
};
/** @endcond */
#endif

#ifdef SLIMLOG_HEADER_ONLY
#include "slimlog/format-inl.h" // IWYU pragma: keep
#endif
