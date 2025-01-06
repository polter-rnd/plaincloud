/**
 * @file record-inl.h
 * @brief Contains the definition of the RecordStringView class.
 */

#pragma once

// IWYU pragma: private, include "slimlog/record.h"

#include "slimlog/record.h" // IWYU pragma: associated
#include "slimlog/util/unicode.h"

#include <utility>

namespace SlimLog {

template<typename T>
RecordStringView<T>::RecordStringView(const RecordStringView& str_view) noexcept
    : std::basic_string_view<T>(str_view)
    , m_codepoints(str_view.m_codepoints.load(std::memory_order_relaxed))
{
}

template<typename T>
RecordStringView<T>::RecordStringView(RecordStringView&& str_view) noexcept
    : std::basic_string_view<T>(std::move(static_cast<std::basic_string_view<T>&&>(str_view)))
    , m_codepoints(str_view.m_codepoints.load(std::memory_order_relaxed))
{
}

template<typename T>
RecordStringView<T>::RecordStringView(std::basic_string_view<T> str_view) noexcept
    : std::basic_string_view<T>(std::move(str_view))
{
}

template<typename T>
auto RecordStringView<T>::operator=(const RecordStringView& str_view) noexcept -> RecordStringView&
{
    if (this == &str_view) {
        return *this;
    }

    std::basic_string_view<T>::operator=(str_view);
    m_codepoints.store(
        str_view.m_codepoints.load(std::memory_order_relaxed), std::memory_order_relaxed);
    return *this;
}

template<typename T>
auto RecordStringView<T>::operator=(RecordStringView&& str_view) noexcept -> RecordStringView&
{
    if (this == &str_view) {
        return *this;
    }

    std::basic_string_view<T>::operator=(
        std::move(static_cast<std::basic_string_view<T>&&>(str_view)));
    m_codepoints.store(
        str_view.m_codepoints.load(std::memory_order_relaxed), std::memory_order_relaxed);
    return *this;
}

template<typename T>
auto RecordStringView<T>::operator=(std::basic_string_view<T> str_view) noexcept
    -> RecordStringView&
{
    if (this != &str_view) {
        std::basic_string_view<T>::operator=(std::move(str_view));
        m_codepoints.store(std::string_view::npos, std::memory_order_relaxed);
    }
    return *this;
}

template<typename T>
auto RecordStringView<T>::codepoints() -> std::size_t
{
    auto codepoints = m_codepoints.load(std::memory_order_acquire);
    if (codepoints == std::string_view::npos) {
        codepoints = Util::Unicode::count_codepoints(this->data(), this->size());
        m_codepoints.store(codepoints, std::memory_order_release);
    }
    return codepoints;
}

} // namespace SlimLog
