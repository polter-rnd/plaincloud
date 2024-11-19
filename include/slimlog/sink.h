/**
 * @file sink.h
 * @brief Contains the declaration of Sink and SinkDriver classes.
 */

#pragma once

#include <slimlog/format.h>
#include <slimlog/location.h>
#include <slimlog/pattern.h>
#include <slimlog/policy.h>
#include <slimlog/record.h>
#include <slimlog/util/os.h>

#include <cstdint>
#include <initializer_list>
#include <memory>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace SlimLog {

enum class Level : std::uint8_t;

/**
 * @brief Base abstract sink class.
 *
 * A sink represents a logging backend that processes and outputs log messages.
 *
 * @tparam Logger The logger class type intended for use with this sink.
 */
template<typename Logger>
class Sink : public std::enable_shared_from_this<Sink<Logger>> {
public:
    /** @brief String type for log messages. */
    using StringType = typename Logger::StringType;
    /** @brief String view type for log categories. */
    using StringViewType = typename Logger::StringViewType;
    /** @brief Character type for log messages. */
    using CharType = typename Logger::CharType;
    /** @brief Buffer type used for formatting log messages. */
    using FormatBufferType = FormatBuffer<CharType, Logger::BufferSize, std::allocator<CharType>>;
    /** @brief Log record type. */
    using RecordType = Record<CharType, StringType>;

    /**
     * @brief Constructs a new Sink object.
     *
     * Accepts optional pattern and log level arguments to customize message formatting.
     *
     * Usage example:
     * ```cpp
     * Log::Logger log("main", Log::Level::Info);
     * log.add_sink<Log::OStreamSink>(
            std::cout,
            "({category}) [{level}] {file}|{line}: {message}",
            std::make_pair(Log::Level::Trace, "Trace"),
            std::make_pair(Log::Level::Debug, "Debug"),
            std::make_pair(Log::Level::Info, "Info"),
            std::make_pair(Log::Level::Warning, "Warn"),
            std::make_pair(Log::Level::Error, "Error"),
            std::make_pair(Log::Level::Fatal, "Fatal"));
     * ```
     *
     * @tparam Args Argument types for the pattern and log levels.
     * @param args Optional pattern and list of log levels.
     */
    template<typename... Args>
    explicit Sink(Args&&... args)
        // NOLINTNEXTLINE(*-array-to-pointer-decay,*-no-array-decay)
        : m_pattern(std::forward<Args>(args)...)
    {
    }

    /** @brief Copy constructor. */
    Sink(Sink const&) = default;
    /** @brief Move constructor. */
    Sink(Sink&&) noexcept = default;

    /** @brief Assignment operator. */
    auto operator=(Sink const&) -> Sink& = default;
    /** @brief Move assignment operator. */
    auto operator=(Sink&&) noexcept -> Sink& = default;

    /** @brief Destructor. */
    virtual ~Sink() = default;

    /**
     * @brief Sets the log message pattern.
     *
     * Usage example:
     * ```cpp
     * Log::Logger log("test", Log::Level::Info);
     * log.add_sink<Log::OStreamSink>(std::cerr)->set_pattern("(%t) [%l] %F|%L: %m");
     * ```
     *
     * @param pattern Log message pattern.
     * @return Pointer to the self sink object.
     */
    virtual auto set_pattern(StringViewType pattern) -> std::shared_ptr<Sink<Logger>>;

    /**
     * @brief Sets the log level names.
     *
     * Usage example:
     * ```cpp
     * Log::Logger log("test", Log::Level::Info);
     * log.add_sink<Log::OStreamSink>(std::cerr)->set_levels({{Log::Level::Info, "Information"}});
     * ```
     *
     * @param levels List of log levels with corresponding names.
     * @return Pointer to the self sink object.
     */
    virtual auto set_levels(std::initializer_list<std::pair<Level, StringViewType>> levels)
        -> std::shared_ptr<Sink<Logger>>;

    /**
     * @brief Processes a log record.
     *
     * Formats and outputs the log record.
     *
     * @param record The log record to process.
     */
    virtual auto message(RecordType& record) -> void = 0;

    /**
     * @brief Flushes any buffered log messages.
     */
    virtual auto flush() -> void = 0;

protected:
    /**
     * @brief Formats a log record according to the pattern.
     *
     * @param result Buffer to store the formatted message.
     * @param record The log record to format.
     */
    auto format(FormatBufferType& result, RecordType& record) -> void;

private:
    Pattern<CharType> m_pattern;
};

/**
 * @brief Sink driver for the logger.
 *
 * Manages sinks with or without synchronization,
 * depending on the threading policy.
 *
 * @tparam Logger Base logger type used for log messages.
 * @tparam ThreadingPolicy Singlethreaded or multithreaded policy.
 */
template<typename Logger, typename ThreadingPolicy>
class SinkDriver final {
public:
    /** @brief Character type for log messages. */
    using CharType = typename Logger::CharType;
    /** @brief String view type for log category. */
    using StringViewType = typename Logger::StringViewType;
    /** @brief Buffer type used for log message formatting. */
    using FormatBufferType = typename Sink<Logger>::FormatBufferType;
    /** @brief Log record type. */
    using RecordType = typename Sink<Logger>::RecordType;

    /**
     * @brief Constructs a new SinkDriver object.
     *
     * @param logger Pointer to the logger.
     * @param parent Pointer to the parent instance (if any).
     */
    explicit SinkDriver(const Logger* logger, SinkDriver* parent = nullptr);

    SinkDriver(const SinkDriver&) = delete;
    SinkDriver(SinkDriver&&) = delete;
    auto operator=(const SinkDriver&) -> SinkDriver& = delete;
    auto operator=(SinkDriver&&) -> SinkDriver& = delete;

    /**
     * @brief Destroys the SinkDriver object.
     */
    ~SinkDriver();

    /**
     * @brief Adds an existing sink.
     *
     * @param sink Pointer to the sink.
     * @return \b true if the sink was actually inserted.
     * @return \b false if the sink is already present in this logger.
     */
    auto add_sink(const std::shared_ptr<Sink<Logger>>& sink) -> bool;

    /**
     * @brief Creates and emplaces a new sink.
     *
     * @tparam T Sink type (e.g., ConsoleSink).
     * @tparam Args Sink constructor argument types.
     * @param args Any arguments accepted by the specified sink constructor.
     * @return Shared pointer to the created sink or `nullptr` in case of failure.
     */
    template<typename T, typename... Args>
    auto add_sink(Args&&... args) -> std::shared_ptr<Sink<Logger>>
    {
        auto sink = std::make_shared<T>(std::forward<Args>(args)...);
        return add_sink(sink) ? sink : nullptr;
    }

    /**
     * @brief Removes a sink.
     *
     * @param sink Pointer to the sink.
     * @return \b true if the sink was actually removed.
     * @return \b false if the sink does not exist in this logger.
     */
    auto remove_sink(const std::shared_ptr<Sink<Logger>>& sink) -> bool;

    /**
     * @brief Enables or disables a sink for this logger.
     *
     * @param sink Pointer to the sink.
     * @param enabled Enabled flag.
     * @return \b true if the sink exists and is enabled.
     * @return \b false if the sink does not exist in this logger.
     */
    auto set_sink_enabled(const std::shared_ptr<Sink<Logger>>& sink, bool enabled) -> bool;

    /**
     * @brief Checks if a sink is enabled.
     *
     * @param sink Pointer to the sink.
     * @return \b true if the sink is enabled.
     * @return \b false if the sink is disabled.
     */
    auto sink_enabled(const std::shared_ptr<Sink<Logger>>& sink) -> bool;

    /**
     * @brief Emits a new callback-based log message if it fits the specified logging level.
     *
     * Emits a log message from the callback return value convertible to the logger string type.
     * Postpones formatting or other preparations to the next steps after filtering.
     * Makes logging almost zero-cost if it does not fit the current logging level.
     *
     * @tparam Logger Logger argument type.
     * @tparam T Invocable type for the callback. Deduced from the argument.
     * @tparam Args Format argument types. Deduced from the arguments.
     * @param level Logging level.
     * @param callback Log callback or message.
     * @param category Logger category.
     * @param location Caller location (file, line, function).
     * @param args Format arguments.
     */
    template<typename T, typename... Args>
    auto message(
        Level level,
        T&& callback,
        StringViewType category,
        Location location = Location::current(), // cppcheck-suppress passedByValue
        Args&&... args) const -> void
    {
        FormatBufferType buffer; // NOLINT(misc-const-correctness)
        RecordType record
            = {level,
               {location.file_name(),
                location.function_name(),
                static_cast<std::size_t>(location.line())},
               std::move(category),
               Util::OS::thread_id()};
        std::tie(record.time.local, record.time.nsec)
            = Util::OS::local_time<RecordTime::TimePoint>();

        // Flag to check that message has been evaluated
        bool evaluated = false;

        const typename ThreadingPolicy::ReadLock lock(m_mutex);
        for (const auto& [sink, logger] : m_effective_sinks) {
            if (!logger->level_enabled(level)) [[unlikely]] {
                continue;
            }

            if (!evaluated) [[unlikely]] {
                evaluated = true;

                using BufferRefType = std::add_lvalue_reference_t<FormatBufferType>;
                if constexpr (std::is_invocable_v<T, BufferRefType, Args...>) {
                    // Callable with buffer argument: message will be stored in buffer.
                    callback(buffer, std::forward<Args>(args)...);
                    record.message = RecordStringView{buffer.data(), buffer.size()};
                } else if constexpr (std::is_invocable_v<T, Args...>) {
                    using RetType = typename std::invoke_result_t<T, Args...>;
                    if constexpr (std::is_void_v<RetType>) {
                        // Void callable without arguments: there is no message, just a callback
                        callback(std::forward<Args>(args)...);
                        break;
                    } else {
                        // Non-void callable without arguments: message is the return value
                        auto message = callback(std::forward<Args>(args)...);
                        if constexpr (std::is_convertible_v<RetType, RecordStringView<CharType>>) {
                            record.message = RecordStringView<CharType>{std::move(message)};
                        } else {
                            record.message = message;
                        }
                    }
                } else if constexpr (std::is_convertible_v<T, RecordStringView<CharType>>) {
                    // Non-invocable argument: argument is the message itself
                    // get rid of extra constructor
                    // NOLINTNEXTLINE(*-array-to-pointer-decay,*-no-array-decay)
                    record.message = RecordStringView<CharType>{std::forward<T>(callback)};
                } else {
                    record.message = callback;
                }
            }

            sink->message(record);
        }
    }

protected:
    /**
     * @brief Returns a pointer to the parent sink (or `nullptr` if none).
     *
     * @return Pointer to the parent sink.
     */
    auto parent() -> SinkDriver*;

    /**
     * @brief Sets the parent sink object.
     *
     * @param parent Pointer to the parent sink driver.
     */
    auto set_parent(SinkDriver* parent) -> void;

    /**
     * @brief Adds a child sink driver.
     *
     * @param child Pointer to the child sink driver.
     */
    auto add_child(SinkDriver* child) -> void;
    /**
     * @brief Removes a child sink driver.
     *
     * @param child Pointer to the child sink driver.
     */
    auto remove_child(SinkDriver* child) -> void;

private:
    /**
     * @brief Updates the effective sinks for the current sink driver and its children.
     */
    auto update_effective_sinks() -> void;

    const Logger* m_logger;
    SinkDriver* m_parent;
    std::unordered_set<SinkDriver*> m_children;
    std::unordered_map<Sink<Logger>*, const Logger*> m_effective_sinks;
    std::unordered_map<std::shared_ptr<Sink<Logger>>, bool> m_sinks;
    mutable ThreadingPolicy::Mutex m_mutex;
}; // namespace SlimLog

} // namespace SlimLog

#ifdef SLIMLOG_HEADER_ONLY
#include <slimlog/sink-inl.h>
#endif