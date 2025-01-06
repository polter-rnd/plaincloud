/**
 * @file pattern.h
 * @brief Contains the declaration of the Pattern class.
 */

#pragma once

#include "slimlog/format.h"
#include "slimlog/level.h"
#include "slimlog/record.h"

#include <array>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace SlimLog {

/**
 * @brief Converts a string to a `std::basic_string_view`.
 *
 * This function takes a String object and returns a `std::basic_string_view`
 * of the same character type.
 *
 * @tparam Char Type of the characters in the string.
 * @tparam String String type.
 * @param str The input string object to be converted.
 * @return A `std::basic_string_view` of the same character type as the input string.
 */
template<typename Char, typename String>
struct ConvertString {
    auto operator()(const String&) const -> std::basic_string_view<Char> = delete;
};

/**
 * @brief Represents a log message pattern specifying the message format.
 *
 * This class defines how log messages are formatted using placeholders.
 *
 * @tparam Char The character type for the pattern string.
 */
template<typename Char>
class Pattern {
public:
    /** @brief String view type for pattern strings. */
    using StringViewType = std::basic_string_view<Char>;

    /**
     * @brief Structure for managing log level names.
     */
    struct Levels {
        /**
         * @brief Retrieves the name of the specified log level.
         *
         * @param level The log level.
         * @return A reference to the name of the specified log level.
         */
        auto get(Level level) -> RecordStringView<Char>&;

        /**
         * @brief Sets the name for the specified log level.
         *
         * @param level The log level.
         * @param name The new name for the log level.
         */
        auto set(Level level, StringViewType name) -> void;

    private:
        static constexpr std::array<Char, 6> DefaultTrace{'T', 'R', 'A', 'C', 'E', '\0'};
        static constexpr std::array<Char, 6> DefaultDebug{'D', 'E', 'B', 'U', 'G', '\0'};
        static constexpr std::array<Char, 5> DefaultInfo{'I', 'N', 'F', 'O', '\0'};
        static constexpr std::array<Char, 5> DefaultWarning{'W', 'A', 'R', 'N', '\0'};
        static constexpr std::array<Char, 6> DefaultError{'E', 'R', 'R', 'O', 'R', '\0'};
        static constexpr std::array<Char, 6> DefaultFatal{'F', 'A', 'T', 'A', 'L', '\0'};

        RecordStringView<Char> m_trace{DefaultTrace.data()}; ///< Trace level
        RecordStringView<Char> m_debug{DefaultDebug.data()}; ///< Debug level
        RecordStringView<Char> m_info{DefaultInfo.data()}; ///< Info level
        RecordStringView<Char> m_warning{DefaultWarning.data()}; ///< Warning level
        RecordStringView<Char> m_error{DefaultError.data()}; ///< Error level
        RecordStringView<Char> m_fatal{DefaultFatal.data()}; ///< Fatal level
    };

    /**
     * @brief Placeholder for message pattern components.
     *
     * Specifies available fields and their formatting options.
     */
    struct Placeholder {
        /** @brief Log pattern field types. */
        enum class Type : std::uint8_t {
            None,
            Category,
            Level,
            File,
            Line,
            Message,
            Function,
            Time,
            Msec,
            Usec,
            Nsec,
            Thread
        };

        /** @brief Parameters for string fields. */
        struct StringSpecs {
            /** @brief Field alignment options. */
            enum class Align : std::uint8_t { None, Left, Right, Center };

            std::size_t width = 0; ///< Field width.
            Align align = Align::None; ///< Field alignment.
            StringViewType fill = StringSpecs::DefaultFill.data(); ///< Fill character.

        private:
            static constexpr std::array<Char, 2> DefaultFill{' ', '\0'};
        };

        Type type = Type::None; ///< Placeholder type.
        std::variant<
            StringViewType,
            StringSpecs,
            CachedFormatter<std::size_t, Char>,
            CachedFormatter<std::chrono::sys_seconds, Char>>
            value = StringViewType{}; ///< Placeholder value.
    };

    /**
     * @brief Placeholder field names.
     */
    struct Placeholders {
    private:
        static constexpr std::array<Char, 9> Category{'c', 'a', 't', 'e', 'g', 'o', 'r', 'y', '\0'};
        static constexpr std::array<Char, 6> Level{'l', 'e', 'v', 'e', 'l', '\0'};
        static constexpr std::array<Char, 5> File{'f', 'i', 'l', 'e', '\0'};
        static constexpr std::array<Char, 5> Line{'l', 'i', 'n', 'e', '\0'};
        static constexpr std::array<Char, 9> Function{'f', 'u', 'n', 'c', 't', 'i', 'o', 'n', '\0'};
        static constexpr std::array<Char, 5> Time{'t', 'i', 'm', 'e', '\0'};
        static constexpr std::array<Char, 5> Msec{'m', 's', 'e', 'c', '\0'};
        static constexpr std::array<Char, 5> Usec{'u', 's', 'e', 'c', '\0'};
        static constexpr std::array<Char, 5> Nsec{'n', 's', 'e', 'c', '\0'};
        static constexpr std::array<Char, 7> Thread{'t', 'h', 'r', 'e', 'a', 'd', '\0'};
        static constexpr std::array<Char, 8> Message{'m', 'e', 's', 's', 'a', 'g', 'e', '\0'};

    public:
        /** @brief List of placeholder names. */
        static constexpr std::array<std::pair<typename Placeholder::Type, StringViewType>, 11> List{
            {{Placeholder::Type::Category, Placeholders::Category.data()},
             {Placeholder::Type::Level, Placeholders::Level.data()},
             {Placeholder::Type::File, Placeholders::File.data()},
             {Placeholder::Type::Line, Placeholders::Line.data()},
             {Placeholder::Type::Function, Placeholders::Function.data()},
             {Placeholder::Type::Time, Placeholders::Time.data()},
             {Placeholder::Type::Msec, Placeholders::Msec.data()},
             {Placeholder::Type::Usec, Placeholders::Usec.data()},
             {Placeholder::Type::Nsec, Placeholders::Nsec.data()},
             {Placeholder::Type::Thread, Placeholders::Thread.data()},
             {Placeholder::Type::Message, Placeholders::Message.data()}}};
    };

    /**
     * @brief Constructs a new Pattern object.
     *
     * Initializes with a pattern string and optional log level pairs.
     *
     * Usage example:
     * ```cpp
     * Log::Pattern<char> pattern(
     *       "({category}) [{level}] {file}|{line}: {message}",
     *       std::make_pair(Log::Level::Trace, "Trace"),
     *       std::make_pair(Log::Level::Debug, "Debug"),
     *       std::make_pair(Log::Level::Info, "Info"),
     *       std::make_pair(Log::Level::Warning, "Warn"),
     *       std::make_pair(Log::Level::Error, "Error"),
     *       std::make_pair(Log::Level::Fatal, "Fatal"));
     * ```
     *
     * @tparam Args Argument types for log level pairs.
     * @param pattern Pattern string.
     * @param args Log level and name pairs.
     */
    template<typename... Args>
    explicit Pattern(StringViewType pattern = {}, Args&&... args)
    {
        set_levels({std::forward<Args>(args)...});
        compile(pattern);
    }

    /**
     * @brief Checks if the pattern is empty.
     *
     * @return \b true if the pattern is an empty string.
     * @return \b false if the pattern is not an empty string.
     */
    [[nodiscard]] auto empty() const -> bool;

    /**
     * @brief Formats a message according to the pattern.
     *
     * This function formats a log message based on the specified pattern.
     *
     * @tparam StringType %Logger string type.
     * @param out Buffer storing the raw message to be overwritten with the result.
     * @param record Log record.
     */
    template<typename StringType>
    auto format(auto& out, Record<Char, StringType>& record) -> void;

    /**
     * @brief Sets the message pattern.
     *
     * This function sets the pattern used for formatting log messages.
     *
     * Usage example:
     * ```cpp
     * Log::Logger log("test", Log::Level::Info);
     * log.add_sink<Log::OStreamSink>(std::cerr)->set_pattern("(%t) [%l] %F|%L: %m");
     * ```
     *
     * @param pattern Message pattern.
     */
    auto set_pattern(StringViewType pattern) -> void;

    /**
     * @brief Sets the log level names.
     *
     * This function sets a name for each log level.
     *
     * Usage example:
     * ```cpp
     * Log::Logger log("test", Log::Level::Info);
     * log.add_sink<Log::OStreamSink>(std::cerr)->set_levels({{Log::Level::Info, "Information"}});
     * ```
     *
     * @param levels Initializer list of log level pairs.
     */
    auto set_levels(std::initializer_list<std::pair<Level, StringViewType>> levels) -> void;

protected:
    /**
     * @brief Compiles the pattern string into a fast-lookup representation.
     *
     * This function processes the provided pattern string and converts it into
     * an internal representation that allows for efficient formatting of log messages.
     *
     * @param pattern The pattern string to be compiled.
     */
    void compile(StringViewType pattern);

    /**
     * @brief Formats a string according to the specifications.
     *
     * This function formats the source string based on the provided specifications,
     * including alignment and fill character, and appends the result to the given buffer.
     *
     * @tparam StringView Source string type (can be std::string_view or RecordStringView).
     * @param out Buffer where the formatted string will be appended.
     * @param item Variant holding either StringSpecs or RecordStringView.
     * @param data Source string to be formatted.
     */
    template<typename StringView>
        requires(
            std::same_as<std::remove_cvref_t<StringView>, RecordStringView<Char>>
            || std::same_as<std::remove_cvref_t<StringView>, RecordStringView<char>>)
    static void format_string(auto& out, const auto& item, StringView&& data);

    /**
     * @brief Formats data according to the cached format context.
     *
     * This function formats the source data based on the format context from CachedFormatter,
     * including alignment and fill character, and appends the out to the given buffer.
     *
     * @tparam T Data type.
     * @param out Buffer where the formatted data will be appended.
     * @param item Variant holding CachedFormatter<T, Char>, where T is the data type.
     * @param data Source data to be formatted.
     */
    template<typename T>
    static void format_generic(auto& out, const auto& item, T data);

private:
    /**
     * @brief Converts a string to a non-negative integer.
     *
     * This function parses a string and converts it to a non-negative integer.
     *
     * @param begin Reference to a pointer to the beginning of the string.
     *              The pointer will be advanced by the number of characters processed.
     * @param end Pointer to the past-the-end character of the string.
     * @param error_value The default value to return in case of a conversion error.
     * @return The parsed integer, or error_value if the conversion fails.
     */
    static constexpr auto
    parse_nonnegative_int(const Char*& begin, const Char* end, int error_value) noexcept -> int;

    /**
     * @brief Parses alignment (^, <, >) and fill character from the formatting field.
     *
     * This function parses the alignment and fill character from the given string
     * and updates the provided specs structure with the parsed values.
     *
     * @param begin Pointer to the beginning of the string.
     * @param end Pointer to the past-the-end of the string.
     * @param specs Reference to the output specs structure to be updated.
     * @return Pointer to the past-the-end of the processed characters.
     */
    static constexpr auto
    parse_align(const Char* begin, const Char* end, Placeholder::StringSpecs& specs) -> const Char*;

    /**
     * @brief Append a pattern placeholder to the list of placeholders.
     *
     * This function parses a placeholder from the end of the string and appends it to the list
     * of placeholders.
     *
     * @param type Placeholder type.
     * @param count Placeholder length.
     * @param shift Margin from the end of the pattern string.
     */
    void append_placeholder(Placeholder::Type type, std::size_t count, std::size_t shift = 0);

    /**
     * @brief Parses the string specifications from the formatting field.
     *
     * This function extracts the string specifications, such as width, alignment, and fill
     * character, from the given placeholder field value.
     *
     * @param value The string value of the placeholder field.
     * @return A Placeholder::StringSpecs object containing the parsed specifications.
     */
    static auto get_string_specs(StringViewType value) -> Placeholder::StringSpecs;

    /**
     * @brief Writes the source string to the destination buffer.
     *
     * @tparam StringView String view type, convertible to `std::basic_string_view`.
     * @param dst Destination buffer where the string will be written.
     * @param src Source string view to be written.
     */
    template<typename StringView>
    constexpr static void write_string(auto& dst, StringView&& src);

    /**
     * @brief Writes the source string to the destination buffer with specific alignment.
     *
     * This function writes the source string to the destination buffer, applying the specified
     * alignment and fill character.
     *
     * @tparam StringView String view type, convertible to `std::basic_string_view`.
     * @param dst Destination buffer where the string will be written.
     * @param src Source string view to be written.
     * @param specs String specifications, including alignment and fill character.
     */
    template<typename StringView>
    constexpr static void
    write_string_padded(auto& dst, StringView&& src, const Placeholder::StringSpecs& specs);

    std::basic_string<Char> m_pattern;
    std::vector<Placeholder> m_placeholders;
    Levels m_levels;
};

} // namespace SlimLog

#ifdef SLIMLOG_HEADER_ONLY
#include "slimlog/pattern-inl.h" // IWYU pragma: keep
#endif
