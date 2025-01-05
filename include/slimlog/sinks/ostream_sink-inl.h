/**
 * @file ostream_sink-inl.h
 * @brief Contains definition of OStreamSink class.
 */

#pragma once

// IWYU pragma: private, include <slimlog/sinks/ostream_sink.h>

#include <slimlog/sinks/ostream_sink.h> // IWYU pragma: associated

namespace SlimLog {

template<typename String, typename Char, std::size_t BufferSize, typename Allocator>
auto OStreamSink<String, Char, BufferSize, Allocator>::message(RecordType& record) -> void
{
    FormatBufferType buffer;
    this->format(buffer, record);
    buffer.push_back('\n');
    m_ostream.write(buffer.begin(), buffer.size());
}

template<typename String, typename Char, std::size_t BufferSize, typename Allocator>
auto OStreamSink<String, Char, BufferSize, Allocator>::flush() -> void
{
    m_ostream.flush();
}

} // namespace SlimLog
