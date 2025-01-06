/**
 * @file format-inl.h
 * @brief Contains definitions of FormatValue and CachedFormatter classes.
 */

#pragma once

// IWYU pragma: private, include "slimlog/format.h"

#include "slimlog/format.h" // IWYU pragma: associated

#ifdef SLIMLOG_FMTLIB
#if __has_include(<fmt/base.h>)
#include <fmt/base.h>
#else
#include <fmt/core.h>
#endif
#else
#include <array>
#endif

namespace SlimLog {

#ifndef SLIMLOG_FMTLIB
template<typename T, Formattable<T> Char>
FormatValue<T, Char>::FormatValue(const CachedFormatter<T, Char>& formatter, T value)
    : m_formatter(formatter)
    , m_value(std::move(value))
{
}

template<typename T, Formattable<T> Char>
template<typename Context>
auto FormatValue<T, Char>::format(Context& context) const
{
    return m_formatter.format(m_value, context);
}
#endif

template<typename T, Formattable<T> Char>
CachedFormatter<T, Char>::CachedFormatter(std::basic_string_view<Char> fmt)
#ifdef SLIMLOG_FMTLIB
    : m_empty(fmt.empty())
#endif
{
    FormatParseContext<Char> parse_context(std::move(fmt));
    // Suppress buggy GCC warning on fmtlib sources
#if defined(__GNUC__) and not defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif
    Formatter<T, Char>::parse(parse_context);
#if defined(__GNUC__) and not defined(__clang__)
#pragma GCC diagnostic pop
#endif
}

template<typename T, Formattable<T> Char>
template<typename Out>
void CachedFormatter<T, Char>::format(Out& out, T value) const
{
    if (!m_value || value != *m_value) [[unlikely]] {
        m_value = std::move(value);
        m_buffer.clear();
#ifdef SLIMLOG_FMTLIB
        // Shortcut for numeric types without formatting
        if constexpr (std::is_arithmetic_v<T>) {
            if (m_empty) [[likely]] {
                m_buffer.append(fmt::format_int(*m_value));
                out.append(m_buffer);
                return;
            }
        }

        // For libfmt it's possible to create a custom fmt::basic_format_context
        // appending to the buffer directly, which is the most efficient way.
        using Appender = std::conditional_t<
            std::is_same_v<Char, char>,
            fmt::appender,
            std::back_insert_iterator<decltype(m_buffer)>>;
        fmt::basic_format_context<Appender, Char> fmt_context(Appender(m_buffer), {});
        Formatter<T, Char>::format(*m_value, fmt_context);
#else
        // For std::format there is no way to build a custom format context,
        // so we have to use dummy format string (empty string will be omitted),
        // and pass FormatValue with a reference to CachedFormatter as an argument.
        static constexpr std::array<Char, 3> Fmt{'{', '}', '\0'};
        m_buffer.vformat(Fmt.data(), m_buffer.make_format_args(FormatValue(*this, *m_value)));
#endif
    }
    out.append(m_buffer);
}

} // namespace SlimLog
