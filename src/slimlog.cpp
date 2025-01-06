#include "slimlog/format.h"
#include "slimlog/logger.h"
#include "slimlog/pattern.h"
#include "slimlog/policy.h"
#include "slimlog/record.h"
#include "slimlog/sink.h"
#include "slimlog/sinks/file_sink.h"
#include "slimlog/sinks/null_sink.h"
#include "slimlog/sinks/ostream_sink.h"

#ifndef SLIMLOG_HEADER_ONLY
// IWYU pragma: begin_keep
#include "slimlog/format-inl.h"
#include "slimlog/pattern-inl.h"
#include "slimlog/record-inl.h"
#include "slimlog/sink-inl.h"
#include "slimlog/sinks/file_sink-inl.h"
#include "slimlog/sinks/null_sink-inl.h"
#include "slimlog/sinks/ostream_sink-inl.h"
// IWYU pragma: end_keep
#endif

#include <chrono>
#include <cstddef>
#include <string_view>

namespace SlimLog {
// char
template class SinkDriver<Logger<std::string_view>, SingleThreadedPolicy>;
template class SinkDriver<Logger<std::string_view>, MultiThreadedPolicy>;
template class FileSink<std::string_view>;
template class OStreamSink<std::string_view>;
template class NullSink<std::string_view>;
template class RecordStringView<char>;
template class Pattern<char>;
template class CachedFormatter<std::size_t, char>;
template class CachedFormatter<std::chrono::sys_seconds, char>;
#ifndef SLIMLOG_FMTLIB
template class FormatValue<std::size_t, char>;
template class FormatValue<std::chrono::sys_seconds, char>;
#endif

// wchar_t
template class SinkDriver<Logger<std::wstring_view>, SingleThreadedPolicy>;
template class SinkDriver<Logger<std::wstring_view>, MultiThreadedPolicy>;
template class FileSink<std::wstring_view>;
template class OStreamSink<std::wstring_view>;
template class NullSink<std::wstring_view>;
template class RecordStringView<wchar_t>;
template class Pattern<wchar_t>;
template class CachedFormatter<std::size_t, wchar_t>;
template class CachedFormatter<std::chrono::sys_seconds, wchar_t>;
#ifndef SLIMLOG_FMTLIB
template class FormatValue<std::size_t, wchar_t>;
template class FormatValue<std::chrono::sys_seconds, wchar_t>;
#endif

// char8_t
#ifdef SLIMLOG_CHAR8_T
template class SinkDriver<Logger<std::u8string_view>, SingleThreadedPolicy>;
template class SinkDriver<Logger<std::u8string_view>, MultiThreadedPolicy>;
template class FileSink<std::u8string_view>;
template class OStreamSink<std::u8string_view>;
template class NullSink<std::u8string_view>;
template class RecordStringView<char8_t>;
template class Pattern<char8_t>;
template class CachedFormatter<std::size_t, char8_t>;
template class CachedFormatter<std::chrono::sys_seconds, char8_t>;
#endif

// char16_t
#ifdef SLIMLOG_CHAR16_T
template class SinkDriver<Logger<std::u16string_view>, SingleThreadedPolicy>;
template class SinkDriver<Logger<std::u16string_view>, MultiThreadedPolicy>;
template class FileSink<std::u16string_view>;
template class OStreamSink<std::u16string_view>;
template class NullSink<std::u16string_view>;
template class RecordStringView<char16_t>;
template class Pattern<char16_t>;
template class CachedFormatter<std::size_t, char16_t>;
template class CachedFormatter<std::chrono::sys_seconds, char16_t>;
#endif

// char32_t
#ifdef SLIMLOG_CHAR32_T
template class SinkDriver<Logger<std::u32string_view>, SingleThreadedPolicy>;
template class SinkDriver<Logger<std::u32string_view>, MultiThreadedPolicy>;
template class Sink<std::u32string_view>;
template class FileSink<std::u32string_view>;
template class OStreamSink<std::u32string_view>;
template class NullSink<std::u32string_view>;
template class RecordStringView<char32_t>;
template class Pattern<char32_t>;
template class CachedFormatter<std::size_t, char32_t>;
template class CachedFormatter<std::chrono::sys_seconds, char32_t>;
#endif
} // namespace SlimLog
