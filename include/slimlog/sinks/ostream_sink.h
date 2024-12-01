/**
 * @file ostream_sink.h
 * @brief Contains declaration of OStreamSink class.
 */

#pragma once

#include <slimlog/logger.h>
#include <slimlog/sink.h>

#include <ostream>
#include <utility>

namespace SlimLog {

/**
 * @brief Output stream-based sink.
 *
 * This sink writes formatted log messages to an output stream.
 *
 * @tparam Logger The logger class type intended for use with this sink.
 */
template<typename Logger>
class OStreamSink : public Sink<Logger> {
public:
    using typename Sink<Logger>::CharType;
    using typename Sink<Logger>::FormatBufferType;
    using typename Sink<Logger>::RecordType;

    /**
     * @brief Constructs a new OStreamSink object.
     *
     * @tparam Args Argument types for the pattern and log levels.
     * @param ostream Reference to the output stream to be used by the sink.
     * @param args Optional pattern and list of log levels.
     */
    template<typename... Args>
    explicit OStreamSink(const std::basic_ostream<CharType>& ostream, Args&&... args)
        : Sink<Logger>(std::forward<Args>(args)...)
        , m_ostream(ostream.rdbuf())
    {
    }

    /**
     * @brief Constructs a new OStreamSink object.
     *
     * @tparam Args Argument types for the pattern and log levels.
     * @param streambuf Pointer to the output stream buffer to be used by the sink.
     * @param args Optional pattern and list of log levels.
     */
    template<typename... Args>
    explicit OStreamSink(std::basic_streambuf<CharType>* streambuf, Args&&... args)
        : Sink<Logger>(std::forward<Args>(args)...)
        , m_ostream(streambuf)
    {
    }

    /**
     * @brief Processes a log record.
     *
     * Formats the log record and writes it to the output stream.
     *
     * @param record The log record to process.
     */
    auto message(RecordType& record) -> void override;

    /**
     * @brief Flushes the output stream.
     */
    auto flush() -> void override;

private:
    std::basic_ostream<CharType> m_ostream;
};

} // namespace SlimLog

#ifdef SLIMLOG_HEADER_ONLY
#include <slimlog/sinks/ostream_sink-inl.h> // IWYU pragma: keep
#endif
