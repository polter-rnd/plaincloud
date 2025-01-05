/**
 * @file null_sink.h
 * @brief Contains declaration of NullSink class.
 */

#pragma once

#include <slimlog/sink.h>
#include <slimlog/util/types.h>

#include <string_view>
#include <utility>

namespace SlimLog {

/**
 * @brief Null sink for testing and benchmarking.
 *
 * This sink performs no action and is intended for use in performance tests.
 *
 * @tparam Logger The logger class type intended for use with this sink.
 */
template<typename String, typename Char = Util::Types::UnderlyingCharType<String>>
class NullSink : public Sink<String, Char> {
public:
    using typename Sink<String, Char>::RecordType;

    /**
     * @brief Constructs a new NullSink object.
     *
     * @tparam Args Argument types for the pattern and log levels.
     * @param args Optional pattern and list of log levels.
     */
    template<typename... Args>
    explicit NullSink(Args&&... args)
        : Sink<String, Char>(std::forward<Args>(args)...)
    {
    }

    /**
     * @brief Processes a log record.
     *
     * Formats the log record but does not output it.
     *
     * @param record The log record to process.
     */
    auto message(RecordType& record) -> void override;

    /**
     * @brief Flush operation (no-op for NullSink).
     */
    auto flush() -> void override;
};

} // namespace SlimLog

#ifdef SLIMLOG_HEADER_ONLY
#include <slimlog/sinks/null_sink-inl.h> // IWYU pragma: keep
#endif
