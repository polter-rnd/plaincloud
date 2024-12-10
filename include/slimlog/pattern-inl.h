/**
 * @file pattern-inl.h
 * @brief Contains the definition of the Pattern class.
 */

#pragma once

// IWYU pragma: private, include <slimlog/pattern.h>

#ifndef SLIMLOG_HEADER_ONLY
#include <slimlog/pattern.h>
#endif

#include <slimlog/format.h>
#include <slimlog/level.h>
#include <slimlog/record.h>
#include <slimlog/util/types.h>
#include <slimlog/util/unicode.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <climits>
#include <concepts>
#if defined(__cpp_unicode_characters) or defined(__cpp_char8_t)
#include <cuchar> // IWYU pragma: keep
#endif
#include <cwchar>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace SlimLog {

/** @cond */
namespace Detail {

// Fallback functions to detect missing ones from stdlib
namespace Fallback {
#ifdef __cpp_char8_t
template<typename... Args>
inline auto mbrtoc8(Args... /*unused*/)
{
    return std::monostate{};
};
#endif
#ifdef __cpp_unicode_characters
template<typename... Args>
inline auto mbrtoc16(Args... /*unused*/)
{
    return std::monostate{};
};
template<typename... Args>
inline auto mbrtoc32(Args... /*unused*/)
{
    return std::monostate{};
};
#endif
} // namespace Fallback

template<typename Char>
struct FromMultibyte {
    auto get(Char* chr, const char* str, std::size_t len) -> int
    {
        using namespace Fallback;
        // NOLINTBEGIN (concurrency-mt-unsafe)
        if constexpr (std::is_same_v<Char, wchar_t>) {
            return handle(mbrtowc(chr, str, len, &m_state));
#ifdef __cpp_char8_t
        } else if constexpr (std::is_same_v<Char, char8_t>) {
            return handle(mbrtoc8(chr, str, len, &m_state));
#endif
#ifdef __cpp_unicode_characters
        } else if constexpr (std::is_same_v<Char, char16_t>) {
            return handle(mbrtoc16(chr, str, len, &m_state));
        } else if constexpr (std::is_same_v<Char, char32_t>) {
            return handle(mbrtoc32(chr, str, len, &m_state));
#endif
        } else {
            static_assert(Util::Types::AlwaysFalse<Char>{}, "Unsupported character type");
            return -1;
        }
        // NOLINTEND (concurrency-mt-unsafe)
    }

    static auto handle(std::size_t res) -> int
    {
        return static_cast<int>(res);
    }

    template<typename T = std::monostate>
    static auto handle(T /*unused*/) -> int
    {
        static_assert(
            Util::Types::AlwaysFalse<Char>{},
            "C++ stdlib does not support conversion to given character type");
        return -1;
    }

private:
    std::mbstate_t m_state = {};
};

template<typename Char, typename StringType>
concept HasConvertString = requires(StringType value) {
    { ConvertString<Char, StringType>{}(value) } -> std::same_as<std::basic_string_view<Char>>;
};
} // namespace Detail
/** @endcond */

template<typename Char>
auto Pattern<Char>::Levels::get(Level level) -> RecordStringView<Char>&
{
    switch (level) {
    case Level::Fatal:
        return m_fatal;
    case Level::Error:
        return m_error;
    case Level::Warning:
        return m_warning;
    case Level::Info:
        return m_info;
    case Level::Debug:
        return m_debug;
    case Level::Trace:
        [[fallthrough]];
    default:
        return m_trace;
    }
}

template<typename Char>
auto Pattern<Char>::Levels::set(Level level, StringViewType name) -> void
{
    switch (level) {
    case Level::Fatal:
        m_fatal = std::move(name);
        break;
    case Level::Error:
        m_error = std::move(name);
        break;
    case Level::Warning:
        m_warning = std::move(name);
        break;
    case Level::Info:
        m_info = std::move(name);
        break;
    case Level::Debug:
        m_debug = std::move(name);
        break;
    case Level::Trace:
        m_trace = std::move(name);
        break;
    }
}

template<typename Char>
[[nodiscard]] auto Pattern<Char>::empty() const -> bool
{
    return m_pattern.empty();
}

template<typename Char>
template<typename StringType>
auto Pattern<Char>::format(auto& out, Record<Char, StringType>& record) -> void
{
    constexpr std::size_t MsecInNsec = 1000000;
    constexpr std::size_t UsecInNsec = 1000;

    // Process format placeholders
    for (auto& item : m_placeholders) {
        switch (item.type) {
        case Placeholder::Type::None:
            out.append(std::get<StringViewType>(item.value));
            break;
        case Placeholder::Type::Category:
            format_string(out, item.value, record.category);
            break;
        case Placeholder::Type::Level:
            format_string(out, item.value, m_levels.get(record.level));
            break;
        case Placeholder::Type::File:
            format_string(out, item.value, record.location.filename);
            break;
        case Placeholder::Type::Function:
            format_string(out, item.value, record.location.function);
            break;
        case Placeholder::Type::Line:
            format_generic(out, item.value, record.location.line);
            break;
        case Placeholder::Type::Time:
            format_generic(out, item.value, record.time.local);
            break;
        case Placeholder::Type::Msec:
            format_generic(out, item.value, record.time.nsec / MsecInNsec);
            break;
        case Placeholder::Type::Usec:
            format_generic(out, item.value, record.time.nsec / UsecInNsec);
            break;
        case Placeholder::Type::Nsec:
            format_generic(out, item.value, record.time.nsec);
            break;
        case Placeholder::Type::Thread:
            format_generic(out, item.value, record.thread_id);
            break;
        case Placeholder::Type::Message:
            std::visit(
                Util::Types::Overloaded{
                    [&out, &value = item.value](std::reference_wrapper<StringType> arg) {
                        if constexpr (Detail::HasConvertString<Char, StringType>) {
                            format_string(out, value, ConvertString<Char, StringType>{}(arg.get()));
                        } else {
                            (void)out;
                            (void)value;
                            (void)arg;
                            throw FormatError(
                                "No corresponding Log::ConvertString<> specialization found");
                        }
                    },
                    [&out, &value = item.value](auto&& arg) {
                        format_string(out, value, std::forward<decltype(arg)>(arg));
                    },
                },
                record.message);
            break;
        }
    }
}

template<typename Char>
auto Pattern<Char>::set_pattern(StringViewType pattern) -> void
{
    compile(pattern);
}

template<typename Char>
auto Pattern<Char>::set_levels(std::initializer_list<std::pair<Level, StringViewType>> levels)
    -> void
{
    for (const auto& level : levels) {
        m_levels.set(level.first, level.second);
    }
}

template<typename Char>
void Pattern<Char>::compile(StringViewType pattern)
{
    m_placeholders.clear();
    m_pattern.clear();
    m_pattern.reserve(pattern.size());

    bool inside_placeholder = false;
    for (;;) {
        constexpr std::array<Char, 3> Delimeters{'{', '}', '\0'};
        const auto pos = pattern.find_first_of(Delimeters.data());
        const auto len = pattern.size();

        if (pos == pattern.npos) {
            // Append leftovers, if any
            if (inside_placeholder) {
                throw FormatError("format error: unmatched '{' in pattern string");
            }
            if (!pattern.empty()) {
                m_pattern.append(pattern);
                append_placeholder(Placeholder::Type::None, len);
            }
            break;
        }

        const auto chr = Util::Unicode::to_ascii(pattern[pos]);

        // Handle escaped braces
        if (!inside_placeholder && pos < len - 1
            && chr == Util::Unicode::to_ascii(pattern[pos + 1])) {
            m_pattern.append(pattern.substr(0, pos + 1));
            pattern = pattern.substr(pos + 2);
            append_placeholder(Placeholder::Type::None, pos + 1);
            continue;
        }

        if (!inside_placeholder && chr == '{') {
            // Enter inside placeholder
            m_pattern.append(pattern.substr(0, pos + 1));
            pattern = pattern.substr(pos + 1);
            append_placeholder(Placeholder::Type::None, pos, 1);

            inside_placeholder = true;
        } else if (inside_placeholder && chr == '}') {
            auto placeholder = std::find_if(
                Placeholders::List.begin(), Placeholders::List.end(), [pattern](const auto& item) {
                    return pattern.starts_with(item.second);
                });
            if (placeholder == Placeholders::List.end()) {
                throw FormatError("format error: unknown pattern placeholder found");
            }

            const auto delta = placeholder->second.size();
            m_pattern.append(pattern.substr(delta, pos + 1 - delta));
            pattern = pattern.substr(pos + 1);
            append_placeholder(placeholder->first, pos - delta);

            inside_placeholder = false;
        } else {
            // Unescaped brace found
            throw FormatError(
                std::string("format error: unmatched '") + chr + "' in pattern string");
            break;
        }
    }

    // If no placeholders found, just add message as a default
    if (m_placeholders.empty()) {
        m_placeholders.emplace_back(
            Placeholder::Type::Message, typename Placeholder::StringSpecs{});
    }
}

template<typename Char>
template<typename StringView>
void Pattern<Char>::format_string(auto& out, const auto& item, StringView&& data)
{
    constexpr auto CountCodepoints = [](StringView& src) {
        if constexpr (std::is_same_v<StringView, StringViewType>) {
            return Util::Unicode::count_codepoints(src.data(), src.size());
        } else {
            return src.codepoints();
        }
    };
    const auto codepoints = CountCodepoints(data);

    if (auto& specs = std::get<typename Placeholder::StringSpecs>(item); specs.width > 0)
        [[unlikely]] {
        write_padded(out, std::forward<StringView>(data), specs, codepoints);
    } else {
        using DataChar = typename std::remove_cvref_t<StringView>::value_type;
        if constexpr (std::is_same_v<DataChar, char> && !std::is_same_v<Char, char>) {
            // NOLINTNEXTLINE (cppcoreguidelines-slicing)
            from_multibyte(out, std::forward<StringView>(data), codepoints);
        } else {
            out.append(std::forward<StringView>(data));
        }
    }
}

template<typename Char>
template<typename T>
void Pattern<Char>::format_generic(auto& out, const auto& item, T data)
{
    std::get<CachedFormatter<T, Char>>(item).format(out, data);
}

template<typename Char>
constexpr auto Pattern<Char>::parse_nonnegative_int(
    const Char*& begin, const Char* end, int error_value) noexcept -> int
{
    unsigned value = 0;
    unsigned prev = 0;
    auto ptr = begin;
    constexpr auto Base = 10ULL;
    while (ptr != end && '0' <= *ptr && *ptr <= '9') {
        prev = value;
        value = value * Base + static_cast<unsigned>(*ptr - '0');
        ++ptr;
    }
    const auto num_digits = ptr - begin;
    begin = ptr;

    constexpr auto Digits10 = static_cast<int>(sizeof(int) * CHAR_BIT * 3 / Base);
    if (num_digits <= Digits10) {
        return static_cast<int>(value);
    }

    // Check for overflow.
    constexpr auto MaxInt = std::numeric_limits<int>::max();
    return num_digits == Digits10 + 1
            && prev * Base + static_cast<unsigned>(*std::prev(ptr) - '0') <= MaxInt
        ? static_cast<int>(value)
        : error_value;
}

template<typename Char>
constexpr auto Pattern<Char>::parse_align(
    const Char* begin, const Char* end, Placeholder::StringSpecs& specs) -> const Char*
{
    auto align = Placeholder::StringSpecs::Align::None;
    auto* ptr = std::next(begin, Util::Unicode::code_point_length(begin));
    if (end - ptr <= 0) {
        ptr = begin;
    }

    for (;;) {
        switch (Util::Unicode::to_ascii(*ptr)) {
        case '<':
            align = Placeholder::StringSpecs::Align::Left;
            break;
        case '>':
            align = Placeholder::StringSpecs::Align::Right;
            break;
        case '^':
            align = Placeholder::StringSpecs::Align::Center;
            break;
        }
        if (align != Placeholder::StringSpecs::Align::None) {
            if (ptr != begin) {
                // Actually this check is redundant, cause using '{' or '}'
                // as a fill character will cause parsing failure earlier.
                switch (Util::Unicode::to_ascii(*begin)) {
                case '}':
                    return begin;
                case '{':
                    throw FormatError("format: invalid fill character '{'\n");
                }
                specs.fill
                    = std::basic_string_view<Char>(begin, Util::Types::to_unsigned(ptr - begin));
                begin = std::next(ptr);
            } else {
                std::advance(begin, 1);
            }
            break;
        }
        if (ptr == begin) {
            break;
        }
        ptr = begin;
    }
    specs.align = align;

    return begin;
}

template<typename Char>
void Pattern<Char>::append_placeholder(Placeholder::Type type, std::size_t count, std::size_t shift)
{
    auto data = StringViewType{m_pattern}.substr(m_pattern.size() - count - shift, count);
    switch (type) {
    case Placeholder::Type::None:
        if (!m_placeholders.empty() && m_placeholders.back().type == type) {
            // In case of raw text, we can safely merge current chunk with the last one
            auto& format = std::get<StringViewType>(m_placeholders.back().value);
            format = StringViewType{format.data(), format.size() + data.size()};
        } else {
            // Otherwise just add new string placeholder
            m_placeholders.emplace_back(type, data);
        }
        break;
    case Placeholder::Type::Category:
    case Placeholder::Type::Level:
    case Placeholder::Type::File:
    case Placeholder::Type::Function:
    case Placeholder::Type::Message:
        m_placeholders.emplace_back(type, get_string_specs(data));
        break;
    case Placeholder::Type::Line:
    case Placeholder::Type::Thread:
    case Placeholder::Type::Msec:
    case Placeholder::Type::Usec:
    case Placeholder::Type::Nsec:
        m_placeholders.emplace_back(type, CachedFormatter<std::size_t, Char>(data));
        break;
    case Placeholder::Type::Time:
        m_placeholders.emplace_back(type, CachedFormatter<std::chrono::sys_seconds, Char>(data));
        break;
    }
};

template<typename Char>
auto Pattern<Char>::get_string_specs(StringViewType value) -> Placeholder::StringSpecs
{
    typename Placeholder::StringSpecs specs = {};
    if (!value.empty()) {
        const auto* begin = value.data();
        const auto* end = std::next(begin, value.size());
        const auto* fmt = parse_align(begin, end, specs);
        if (auto chr = Util::Unicode::to_ascii(*fmt); chr != '}') {
            const int width = parse_nonnegative_int(fmt, std::prev(end), -1);
            if (width == -1) {
                throw FormatError("format field width is too big");
            }
            chr = Util::Unicode::to_ascii(*fmt);
            switch (chr) {
            case '}':
                break;
            case 's':
                if (Util::Unicode::to_ascii(*std::next(fmt)) != '}') {
                    throw FormatError("missing '}' in format string");
                }
                break;
            default:
                throw FormatError(
                    std::string("wrong format type '") + chr + "' for the string field");
            }
            specs.width = width;
        }
    }
    return specs;
}

template<typename Char>
template<typename StringView>
constexpr void Pattern<Char>::write_padded(
    auto& dst, StringView&& src, const Placeholder::StringSpecs& specs, std::size_t codepoints)
{
    const auto spec_width = Util::Types::to_unsigned(specs.width);
    const auto padding = spec_width > codepoints ? spec_width - codepoints : 0;

    // Shifts are encoded as string literals because constexpr is not
    // supported in constexpr functions.
    const char* shifts = "\x1f\x1f\x00\x01";
    const auto left_padding
        = padding >> static_cast<unsigned>(*std::next(shifts, static_cast<int>(specs.align)));
    const auto right_padding = padding - left_padding;

    // Reserve exact amount for data + padding
    dst.reserve(dst.size() + src.size() + padding * specs.fill.size());

    // Lambda for filling with single character or multibyte pattern
    constexpr auto FillPattern
        = [](auto& dst, std::basic_string_view<Char> fill, std::size_t fill_len) {
              const auto* src = fill.data();
              auto block_size = fill.size();
              auto* dest = std::prev(dst.end(), fill_len * block_size);

              if (block_size > 1) {
                  // Copy first block
                  std::copy_n(src, block_size, dest);

                  // Copy other blocks recursively via O(n*log(N)) calls
                  const auto* start = dest;
                  const auto* end = std::next(start, fill_len * block_size);
                  auto* current = std::next(dest, block_size);
                  while (std::next(current, block_size) < end) {
                      std::copy_n(start, block_size, current);
                      std::advance(current, block_size);
                      block_size *= 2;
                  }

                  // Copy the rest
                  std::copy_n(start, end - current, current);
              } else {
                  std::fill_n(dest, fill_len, *src);
              }
          };

    // Fill left padding
    if (left_padding != 0) {
        dst.resize(dst.size() + left_padding * specs.fill.size());
        FillPattern(dst, specs.fill, left_padding);
    }

    // Fill data
    using DataChar = typename std::remove_cvref_t<StringView>::value_type;
    if constexpr (std::is_same_v<DataChar, char> && !std::is_same_v<Char, char>) {
        // NOLINTNEXTLINE (cppcoreguidelines-slicing)
        from_multibyte(dst, std::forward<StringView>(src), codepoints);
    } else {
        dst.append(std::forward<StringView>(src));
    }

    // Fill right padding
    if (right_padding != 0) {
        dst.resize(dst.size() + right_padding * specs.fill.size());
        FillPattern(dst, specs.fill, right_padding);
    }
}

template<typename Char>
void Pattern<Char>::from_multibyte(auto& out, std::string_view data, std::size_t codepoints)
{
    const auto buf_size = out.size();
#if defined(_WIN32) and defined(__STDC_WANT_SECURE_LIB__)
    out.reserve(buf_size + codepoints + 1);
#else
    out.reserve(buf_size + codepoints);
#endif

    Char* dest = std::next(out.begin(), buf_size);
    const char* source = data.data();

    std::size_t written = 0;
    if constexpr (std::is_same_v<Char, wchar_t>) {
        std::mbstate_t state = {};
#if defined(_WIN32) and defined(__STDC_WANT_SECURE_LIB__)
        if (mbsrtowcs_s(&written, dest, codepoints + 1, &source, _TRUNCATE, &state) != 0) {
            throw std::runtime_error("mbsrtowcs_s(): conversion error");
        }
        written -= 1; // Don't take into account null terminator
#else
        // NOLINTNEXTLINE (concurrency-mt-unsafe)
        written = std::mbsrtowcs(dest, &source, codepoints, &state);
        if (written == static_cast<std::size_t>(-1)) {
            throw std::runtime_error("std::mbsrtowcs(): conversion error");
        }
#endif
    } else {
        Char wchr;
        Detail::FromMultibyte<Char> dispatcher;
        for (auto source_size = data.size(); source_size > 0;) {
            const int next = dispatcher.get(&wchr, source, source_size);
            switch (next) {
            case 0:
                // Null character, finish processing
                source_size = 0;
                break;
            case -1:
                // Encoding error occured
                throw std::runtime_error("std::mbrtocN(): conversion error");
                break;
            case -2:
                // Incomplete but valid character, skip it
                break;
            case -3:
                // Next character from surrogate pair was processed
                *dest = wchr;
                written++;
                std::advance(dest, 1);
                break;
            default:
                // Successfuly processed
                *dest = wchr;
                written++;
                std::advance(dest, 1);
                std::advance(source, next);
                source_size -= next;
                break;
            }
        }
    }
    out.resize(buf_size + written);
}

} // namespace SlimLog
