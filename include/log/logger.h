#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <version>

// GCC 11+
// CLang 15+
// libfmt 9.1.0+

namespace PlainCloud::Log {
// Forward declaration of namespace to make IWYU happy
}

#include "format.h"
#include "location.h"

namespace PlainCloud::Log {

enum class Level { Fatal, Error, Warning, Info, Debug, Trace };

template<typename StringT>
class Logger {
public:
    class Sink {
    public:
        Sink() = default;
        Sink(Sink const&) = default;
        Sink(Sink&&) noexcept = default;
        auto operator=(Sink const&) -> Sink& = default;
        auto operator=(Sink&&) noexcept -> Sink& = default;
        virtual ~Sink() = default;

        virtual auto
        emit(Log::Level level, const StringT& message, const Log::Location& location) const -> void
            = 0;
        virtual auto flush() -> void = 0;
    };

    Logger(Logger const&) = delete;
    Logger(Logger&&) = delete;
    auto operator=(Logger const&) -> Logger& = delete;
    auto operator=(Logger&&) -> Logger& = delete;
    ~Logger() = default;

    explicit Logger(
        StringT&& name,
        const Log::Level level = Log::Level::Info,
        const std::initializer_list<std::shared_ptr<Sink>>& sinks = {})
        : m_name(std::move(name))
        , m_level(level)
    {
        m_sinks.reserve(sinks.size());
        for (const auto& sink : sinks) {
            m_sinks.emplace(sink, true);
        }
    }

    auto add_sink(const std::shared_ptr<Sink>& sink) -> bool
    {
        return m_sinks.insert_or_assign(sink, false).second;
    }

    template<template<typename> class T, typename... Args>
    auto add_sink(Args&&... args) -> bool
    {
        return m_sinks
            .insert_or_assign(std::make_shared<T<StringT>>(std::forward<Args>(args)...), false)
            .second;
    }

    auto remove_sink(const std::shared_ptr<Sink>& sink) -> bool
    {
        return m_sinks.erase(sink) == 1;
    }

    auto set_level(Log::Level level) -> void
    {
        m_level = level;
    }

    template<typename T, typename... Args>
        requires std::invocable<T, Args...>
    inline auto emit(
        const Log::Level level,
        const T& callback,
        const Log::Location& location = Log::Location::current(),
        Args&&... args) const -> void
    {
        for (const auto& sink : m_sinks) {
            if (m_level >= level) {
                // NOLINTNEXTLINE(*-array-to-pointer-decay,*-no-array-decay)
                sink.first->emit(level, callback(std::forward<Args>(args)...), location);
            }
        }
    }

    /**
     * @brief Emits and logs new event if the Logger is enabled for specified logging level.
     *
     * @param level Logging level
     * @param location Information about the caller (file, line, function)
     * @param fmt Format string. See `fmt::format` documentation for details.
     * @param args Event message. Use variadic args for `fmt::format`-based formatting.
     */
    template<typename T>
        requires(!std::invocable<T>)
    inline auto emit(
        const Log::Level level,
        const T& message,
        const Log::Location& location = Log::Location::current()) const -> void
    {
        auto callback = [](const T& message) -> const T& { return message; };
        emit(level, callback, location, message);
    }

    template<typename... Args>
    inline void emit(Log::Level level, const Log::Format<Args...>& fmt, Args&&... args) const
    {
        auto callback = [&fmt](Args&&... args) {
            return Log::format(fmt.fmt(), std::forward<Args>(args)...);
        };
        emit(level, callback, fmt.loc(), std::forward<Args>(args)...);
    }

    template<typename... Args>
    inline void emit(Log::Level level, const Log::WideFormat<Args...>& fmt, Args&&... args) const
    {
        auto callback = [&fmt](Args&&... args) {
            return Log::format(fmt.fmt(), std::forward<Args>(args)...);
        };
        emit(level, callback, fmt.loc(), std::forward<Args>(args)...);
    }

    // Informational

    template<typename... Args>
    inline auto info(const Log::Format<Args...>& fmt, Args&&... args) const -> void
    {
        emit(Log::Level::Info, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    inline auto info(const Log::WideFormat<Args...>& fmt, Args&&... args) const -> void
    {
        emit(Log::Level::Info, fmt, std::forward<Args>(args)...);
    }

    template<typename T>
    inline auto info(const T& fmt, const Log::Location& caller = Log::Location::current()) const
        -> void
    {
        emit(Log::Level::Info, fmt, caller);
    }

private:
    std::unordered_map<std::shared_ptr<Sink>, bool> m_sinks;
    StringT m_name;
    Log::Level m_level;
};

template<typename T, size_t N>
Logger(
    const T (&)[N], // NOLINT(*-avoid-c-arrays)
    Log::Level = Log::Level::Info) -> Logger<std::basic_string_view<T>>;

} // namespace PlainCloud::Log
