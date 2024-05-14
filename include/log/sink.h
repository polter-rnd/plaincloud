/**
 * @file sink.h
 * @brief Contains definition of Sink and SinkDriver classes.
 */

#pragma once

#include "format.h"
#include "level.h"
#include "location.h"
#include "pattern.h"
#include "policy.h"

#include <atomic>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace PlainCloud::Log {

/**
 * @brief Base abstract sink class.
 *
 * Sink represents a logger back-end.
 * It emits messages to final destination.
 *
 * @tparam String Base string type used for log messages.
 */
template<typename Logger>
class Sink : public std::enable_shared_from_this<Sink<Logger>> {
public:
    using StringType = typename Logger::StringType;
    using CharType = typename Logger::CharType;

    template<typename... Args>
    explicit Sink(Args&&... args)
        // NOLINTNEXTLINE(*-array-to-pointer-decay,*-no-array-decay)
        : m_pattern(std::forward<Args>(args)...)
    {
    }

    Sink(Sink const&) = default;
    Sink(Sink&&) noexcept = default;
    auto operator=(Sink const&) -> Sink& = default;
    auto operator=(Sink&&) noexcept -> Sink& = default;
    virtual ~Sink() = default;

    virtual auto set_pattern(const typename Pattern<CharType>::StringType& pattern)
        -> std::shared_ptr<Sink<Logger>>
    {
        m_pattern.set_pattern(pattern);
        return this->shared_from_this();
    }

    virtual auto set_levels(
        const std::initializer_list<std::pair<Level, typename Pattern<CharType>::StringType>>&
            levels) -> std::shared_ptr<Sink<Logger>>
    {
        m_pattern.set_levels(levels);
        return this->shared_from_this();
    }

    auto format(
        FormatBuffer<CharType>& result,
        const Level level,
        const StringType& category,
        const Location& caller) const -> void
    {
        m_pattern.format(result, level, category, caller);
    }

    /**
     * @brief Emit message.
     *
     * @param level Log level.
     * @param message Log message.
     * @param location Caller location (file, line, function).
     */
    virtual auto message(
        FormatBuffer<CharType>& buffer,
        Level level,
        const StringType& category,
        const StringType& message,
        const Location& location) -> void = 0;

    virtual auto message(
        FormatBuffer<CharType>& buffer,
        Level level,
        const StringType& category,
        const Location& location) -> void = 0;

    /**
     * @brief Flush message cache, if any.
     */
    virtual auto flush() -> void = 0;

private:
    Pattern<CharType> m_pattern;
};

/**
 * @brief Basic log sink driver class.
 *
 * @tparam String Base string type used for log messages.
 * @tparam ThreadingPolicy Threading policy used for operating over sinks
 *                         (e.g. SingleThreadedPolicy or MultiThreadedPolicy).
 *
 * @note Class doesn't have a virtual destructor
 *       as the intended usage scenario is to
 *       use it as a private base class explicitly
 *       moving access functions to public part of a base class.
 */
template<typename String, typename ThreadingPolicy>
class SinkDriver final { };

/**
 * @brief Single-threaded log sink driver.
 *
 * Handles sinks access without any synchronization.
 *
 * @tparam String Base string type used for log messages.
 */
template<typename Logger>
class SinkDriver<Logger, SingleThreadedPolicy> final {
public:
    using CharType = typename Logger::CharType;

    /**
     * @brief Construct a new SinkDriver object
     *
     * @param sinks Sinks to be added upon creation.
     */
    SinkDriver(const std::initializer_list<std::shared_ptr<Sink<Logger>>>& sinks = {})
    {
        m_sinks.reserve(sinks.size());
        for (const auto& sink : sinks) {
            m_sinks.emplace(sink, true);
        }
    }

    /**
     * @brief Add existing sink.
     *
     * @param sink Pointer to the sink.
     * @return \b true if sink was actually inserted.
     * @return \b false if sink is already present in this logger.
     */
    auto add_sink(const std::shared_ptr<Sink<Logger>>& sink) -> bool
    {
        return m_sinks.insert_or_assign(sink, true).second;
    }

    /**
     * @brief Create and emplace a new sink.
     *
     * @tparam T %Sink type (e.g. ConsoleSink)
     * @tparam Args %Sink constructor argument types (deduced from arguments).
     * @param args Any arguments accepted by specified sink constructor.
     * @return Pointer to the created sink.
     */
    template<typename T, typename... Args>
    auto add_sink(Args&&... args) -> std::shared_ptr<Sink<Logger>>
    {
        return m_sinks.insert_or_assign(std::make_shared<T>(std::forward<Args>(args)...), true)
            .first->first;
    }

    /**
     * @brief Remove sink.
     *
     * @param sink Pointer to the sink.
     * @return \b true if sink was actually removed.
     * @return \b false if sink does not exist in this logger.
     */
    auto remove_sink(const std::shared_ptr<Sink<Logger>>& sink) -> bool
    {
        return m_sinks.erase(sink) == 1;
    }

    /**
     * @brief Enable or disable sink for this logger.
     *
     * @param sink Pointer to the sink.
     * @param enabled Enabled flag.
     * @return \b true if sink exists and enabled.
     * @return \b false if sink does not exist in this logger.
     */
    auto set_sink_enabled(const std::shared_ptr<Sink<Logger>>& sink, bool enabled) -> bool
    {
        if (const auto itr = m_sinks.find(sink); itr != m_sinks.end()) {
            itr->second = enabled;
            return true;
        }
        return false;
    }

    /**
     * @brief Check if sink is enabled.
     *
     * @param sink Pointer to the sink.
     * @return \b true if sink is enabled.
     * @return \b false if sink is disabled.
     */
    auto sink_enabled(const std::shared_ptr<Sink<Logger>>& sink) -> bool
    {
        if (const auto itr = m_sinks.find(sink); itr != m_sinks.end()) {
            return itr->second;
        }
        return false;
    }

    auto buffer() const -> auto&
    {
        static thread_local FormatBuffer<CharType> buffer;
        return buffer;
    }

    /**
     * @brief Emit new callback-based log message if it fits for specified logging level.
     *
     * Method to emit log message from callback return value convertible to logger string type.
     * Used to postpone formatting or other preparations to the next steps after filtering.
     * Makes logging almost zero-cost in case if it does not fit for current logging level.
     *
     * @tparam Logger %Logger argument type
     * @tparam T Invocable type for the callback. Deduced from argument.
     * @tparam Args Format argument types. Deduced from arguments.
     * @param logger %Logger object.
     * @param level Logging level.
     * @param callback Log callback.
     * @param location Caller location (file, line, function).
     */
    template<typename T, typename... Args>
    auto message(
        FormatBuffer<CharType>& buffer,
        const Logger& logger,
        const Level level,
        const T& callback,
        const Location& location = Location::current(),
        Args&&... args) const -> void
    {
        if (static_cast<Level>(logger.m_level) < level) {
            return;
        }

        /*auto sinks{m_sinks};
        for (auto parent{logger.m_parent}; parent; parent = parent->m_parent) {
            if (static_cast<Level>(parent->m_level) >= level) {
                sinks.insert(parent->m_sinks.m_sinks.cbegin(), parent->m_sinks.m_sinks.cend());
            }
        }*/

        for (const auto& sink : m_sinks) {
            if (sink.second) {
                if constexpr (std::is_invocable_v<T, decltype(buffer), Args...>) {
                    callback(buffer, std::forward<Args>(args)...);
                    sink.first->message(buffer, level, logger.category(), location);
                } else if constexpr (std::is_invocable_v<T, Args...>) {
                    if constexpr (std::is_void_v<typename std::invoke_result_t<T, Args...>>) {
                        callback(std::forward<Args>(args)...);
                        sink.first->message(buffer, level, logger.category(), location);
                    } else {
                        sink.first->message(
                            buffer,
                            level,
                            logger.category(),
                            callback(std::forward<Args>(args)...),
                            location);
                    }
                } else {
                    // NOLINTNEXTLINE(*-array-to-pointer-decay,*-no-array-decay)
                    sink.first->message(buffer, level, logger.category(), callback, location);
                }
                buffer.str(std::basic_string<CharType>{});
            }
        }
    }

    template<typename T, typename... Args>
    auto message(
        const Logger& logger,
        const Level level,
        const T& message,
        const Location& location = Location::current(),
        Args&&... args) const -> void
    {
        this->message(buffer(), logger, level, message, location, std::forward<Args>(args)...);
    }

protected:
    auto cbegin() const noexcept ->
        typename std::unordered_map<std::shared_ptr<Sink<Logger>>, bool>::const_iterator
    {
        return m_sinks.cbegin();
    }

    auto cend() const noexcept ->
        typename std::unordered_map<std::shared_ptr<Sink<Logger>>, bool>::const_iterator
    {
        return m_sinks.cend();
    }

private:
    std::unordered_map<std::shared_ptr<Sink<Logger>>, bool> m_sinks;
};

/**
 * @brief Multi-threaded log sink driver.
 *
 * Handles sinks access with locking a mutex.
 *
 * @tparam String Base string type used for log messages.
 */
template<
    typename Logger,
    typename Mutex,
    typename ReadLock,
    typename WriteLock,
    std::memory_order LoadOrder,
    std::memory_order StoreOrder>
class SinkDriver<Logger, MultiThreadedPolicy<Mutex, ReadLock, WriteLock, LoadOrder, StoreOrder>>
    final {
public:
    /**
     * @brief Construct a new SinkDriver object
     *
     * @param sinks Sinks to be added upon creation.
     */
    SinkDriver(const std::initializer_list<std::shared_ptr<Sink<Logger>>>& sinks = {})
        : m_sinks(sinks)
    {
    }

    /**
     * @brief Add existing sink.
     *
     * @param sink Pointer to the sink.
     * @return \b true if sink was actually inserted.
     * @return \b false if sink is already present in this logger.
     */
    auto add_sink(const std::shared_ptr<Sink<Logger>>& sink) -> bool
    {
        WriteLock lock(m_mutex);
        return m_sinks.add_sink(sink);
    }

    /**
     * @brief Create and emplace a new sink.
     *
     * @tparam T %Sink type (e.g. ConsoleSink)
     * @tparam Args %Sink constructor argument types (deduced from arguments).
     * @param args Any arguments accepted by specified sink constructor.
     * @return Pointer to the created sink.
     */
    template<typename T, typename... Args>
    auto add_sink(Args&&... args) -> std::shared_ptr<Sink<Logger>>
    {
        WriteLock lock(m_mutex);
        return m_sinks.template add_sink<T>(std::forward<Args>(args)...);
    }

    /**
     * @brief Remove sink.
     *
     * @param sink Pointer to the sink.
     * @return \b true if sink was actually removed.
     * @return \b false if sink does not exist in this logger.
     */
    auto remove_sink(const std::shared_ptr<Sink<Logger>>& sink) -> bool
    {
        WriteLock lock(m_mutex);
        return m_sinks.remove_sink(sink);
    }

    /**
     * @brief Enable or disable sink for this logger.
     *
     * @param sink Pointer to the sink.
     * @param enabled Enabled flag.
     * @return \b true if sink exists and enabled.
     * @return \b false if sink does not exist in this logger.
     */
    auto set_sink_enabled(const std::shared_ptr<Sink<Logger>>& sink, bool enabled) -> bool
    {
        WriteLock lock(m_mutex);
        return m_sinks.set_sink_enabled(sink, enabled);
    }

    /**
     * @brief Check if sink is enabled.
     *
     * @param sink Pointer to the sink.
     * @return \b true if sink is enabled.
     * @return \b false if sink is disabled.
     */
    auto sink_enabled(const std::shared_ptr<Sink<Logger>>& sink) -> bool
    {
        ReadLock lock(m_mutex);
        return m_sinks.sink_enabled(sink);
    }

    auto buffer() const -> FormatBuffer<typename Logger::CharType>&
    {
        static FormatBuffer<typename Logger::CharType> buffer;
        return buffer;
    }

    /**
     * @brief Emit new callback-based log message if it fits for specified logging level.
     *
     * Method to emit log message from callback return value convertible to logger string type.
     * Used to postpone formatting or other preparations to the next steps after filtering.
     * Makes logging almost zero-cost in case if it does not fit for current logging level.
     *
     * @tparam Logger %Logger argument type
     * @tparam T Invocable type for the callback. Deduced from argument.
     * @tparam Args Format argument types. Deduced from arguments.
     * @param logger %Logger object.
     * @param level Logging level.
     * @param message Log message or callback.
     * @param location Caller location (file, line, function).
     */
    template<typename T, typename... Args>
    auto message(
        const Logger& logger,
        const Level level,
        const T& message,
        const Location& location = Location::current(),
        Args&&... args) const -> void
    {
        ReadLock lock(m_mutex);
        m_sinks.message(buffer(), logger, level, message, location, std::forward<Args>(args)...);
    }

private:
    mutable Mutex m_mutex;
    SinkDriver<Logger, SingleThreadedPolicy> m_sinks;

    friend class SinkDriver<Logger, SingleThreadedPolicy>;
};

} // namespace PlainCloud::Log
