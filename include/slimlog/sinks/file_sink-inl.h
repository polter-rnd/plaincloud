/**
 * @file file_sink-inl.h
 * @brief Contains definition of FileSink class.
 */

#pragma once

// IWYU pragma: private, include "slimlog/sinks/file_sink.h"

#include "slimlog/sinks/file_sink.h" // IWYU pragma: associated

#if defined(_WIN32) && defined(__STDC_WANT_SECURE_LIB__)
// In addition to <cstdio> below for fopen_s() on Windows
#include <stdio.h> // IWYU pragma: keep
#endif

#include <cerrno>
#include <string>
#include <system_error>

namespace SlimLog {

template<typename String, typename Char, std::size_t BufferSize, typename Allocator>
auto FileSink<String, Char, BufferSize, Allocator>::open(std::string_view filename) -> void
{
#if defined(_WIN32) && defined(__STDC_WANT_SECURE_LIB__)
    FILE* fp;
    std::ignore = fopen_s(&fp, std::string(filename).c_str(), "w+");
    m_fp = {fp, std::fclose};
#else
    m_fp = {std::fopen(std::string(filename).c_str(), "w+"), std::fclose};
#endif
    if (!m_fp) {
        throw std::system_error({errno, std::system_category()}, "Error opening log file");
    }
}

template<typename String, typename Char, std::size_t BufferSize, typename Allocator>
auto FileSink<String, Char, BufferSize, Allocator>::message(RecordType& record) -> void
{
    FormatBufferType buffer;
    this->format(buffer, record);
    buffer.push_back('\n');
    if (std::fwrite(buffer.data(), buffer.size(), 1, m_fp.get()) != 1) {
        throw std::system_error({errno, std::system_category()}, "Failed writing to log file");
    }
}

template<typename String, typename Char, std::size_t BufferSize, typename Allocator>
auto FileSink<String, Char, BufferSize, Allocator>::flush() -> void
{
    if (std::fflush(m_fp.get()) != 0) {
        throw std::system_error({errno, std::system_category()}, "Failed flush to log file");
    }
}

} // namespace SlimLog
