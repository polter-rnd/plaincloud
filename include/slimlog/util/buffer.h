/**
 * @file buffer.h
 * @brief Contains contiguous memory buffer classes.
 */

#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

#ifdef SLIMLOG_FMTLIB
#if __has_include(<fmt/base.h>)
#include <fmt/base.h>
#else
#include <fmt/core.h>
#endif
#else
#include "slimlog/util/types.h"

#include <iterator>
#endif

namespace SlimLog::Util {

#ifdef SLIMLOG_FMTLIB
/**
 * @brief Defines an alias for `fmt::detail::buffer<T>`.
 */
template<typename T>
using Buffer = fmt::detail::buffer<T>;
#else
/**
 * @brief A contiguous memory buffer with optional growth capabilities.
 *
 * Provides a base class for buffers that manage a contiguous block of memory.
 * Derived classes can implement growth strategies if dynamic resizing is needed.
 *
 * @tparam T The type of elements stored in the buffer.
 */
template<typename T>
class Buffer {
public:
    /// @cond
    using value_type = T; // NOLINT(readability-identifier-naming)
    using const_reference = const T&; // NOLINT(readability-identifier-naming)
    /// @endcond

    Buffer(const Buffer&) = delete;
    void operator=(const Buffer&) = delete;

    /**
     * @brief Returns a pointer to the beginning of the buffer.
     *
     * @return Pointer to the beginning of the buffer.
     */
    auto begin() noexcept -> T*
    {
        return m_ptr;
    }

    /**
     * @brief Returns a pointer to past-the-end of the buffer.
     *
     * @return Pointer to past-the-end of the buffer.
     */
    auto end() noexcept -> T*
    {
        return std::next(m_ptr, m_size);
    }

    /**
     * @brief Returns a constant pointer to the beginning of the buffer.
     *
     * @return Constant pointer to the beginning of the buffer.
     */
    [[nodiscard]] auto begin() const noexcept -> const T*
    {
        return m_ptr;
    }

    /**
     * @brief Returns a constant pointer to past-the-end of the buffer.
     *
     * @return Constant pointer to past-the-end of the buffer.
     */
    [[nodiscard]] auto end() const noexcept -> const T*
    {
        return m_ptr + m_size;
    }

    /**
     * @brief Returns the size of this buffer.
     *
     * @return Size of the buffer.
     */
    [[nodiscard]] constexpr auto size() const noexcept -> std::size_t
    {
        return m_size;
    }

    /**
     * @brief Returns the capacity of this buffer.
     *
     * @return Capacity of the buffer.
     */
    [[nodiscard]] constexpr auto capacity() const noexcept -> std::size_t
    {
        return m_capacity;
    }

    /**
     * @brief Returns a pointer to the buffer data (not null-terminated).
     *
     * @return Pointer to the buffer data.
     */
    [[nodiscard]] constexpr auto data() noexcept -> T*
    {
        return m_ptr;
    }

    /**
     * @brief Returns a constant pointer to the buffer data (not null-terminated).
     *
     * @return Constant pointer to the buffer data.
     */
    [[nodiscard]] constexpr auto data() const noexcept -> const T*
    {
        return m_ptr;
    }

    /**
     * @brief Clears this buffer.
     */
    void clear()
    {
        m_size = 0;
    }

    /**
     * @brief Tries to resize the buffer to contain `count` elements.
     *
     * If T is a POD type, the new elements may not be initialized.
     *
     * @param count Desired size of the buffer.
     */
    constexpr void try_resize(std::size_t count)
    {
        try_reserve(count);
        m_size = count <= m_capacity ? count : m_capacity;
    }

    /**
     * @brief Tries to increase the buffer capacity to `new_capacity`.
     *
     * It can increase the capacity by a smaller amount than requested
     * but guarantees there is space for at least one additional element
     * either by increasing the capacity or by flushing the buffer if it is full.
     *
     * @param new_capacity Desired capacity of the buffer.
     */
    constexpr void try_reserve(std::size_t new_capacity)
    {
        if (new_capacity > m_capacity) {
            m_grow(*this, new_capacity);
        }
    }

    /**
     * @brief Adds an element to the end of the buffer.
     *
     * Resizes the buffer if necessary to accommodate the new element.
     *
     * @param value The value to be added.
     */
    constexpr void push_back(const T& value)
    {
        try_reserve(m_size + 1);
        *std::next(m_ptr, m_size++) = value;
    }

    /**
     * @brief Appends a range of elements to the buffer.
     *
     * Copies elements from the given range [begin, end) into the buffer.
     *
     * @tparam U The type of the input elements.
     * @param begin Pointer to the beginning of the range.
     * @param end Pointer to the end of the range.
     */
    template<typename U>
    void append(const U* begin, const U* end)
    {
        while (begin != end) {
            auto count = Util::Types::to_unsigned(end - begin);
            try_reserve(m_size + count);

            const auto free_cap = m_capacity - m_size;
            if (free_cap < count) {
                count = free_cap;
            }
            if constexpr (std::is_same_v<T, U>) {
                std::uninitialized_copy_n(begin, count, std::next(m_ptr, m_size));
            } else {
                T* out = std::next(m_ptr, m_size);
                for (std::size_t i = 0; i < count; ++i) {
                    *std::next(out, i) = *std::next(begin, i);
                }
            }

            m_size += count;
            std::advance(begin, count);
        }
    }

    /**
     * @brief Accesses element reference by index.
     *
     * @param index Element index.
     * @return Reference to the element.
     */
    template<typename Idx>
    constexpr auto operator[](Idx index) -> T&
    {
        return m_ptr[index];
    }

    /**
     * @brief Accesses element constant reference by index.
     *
     * @param index Element index.
     * @return Constant reference to the element.
     */
    template<typename Idx>
    constexpr auto operator[](Idx index) const -> const T&
    {
        return m_ptr[index];
    }

protected:
    /**
     * @brief Callback to be called on buffer overflow.
     *
     * @param buf Reference to the buffer.
     * @param capacity Desired capacity.
     */
    using GrowCallback = void (*)(Buffer& buf, std::size_t capacity);

    /**
     * @brief Constructs a new Buffer object.
     *
     * @param grow Pointer to grow callback function.
     * @param size Initial buffer size.
     */
    constexpr Buffer(GrowCallback grow, std::size_t size) noexcept
        : m_ptr(nullptr)
        , m_size(size)
        , m_capacity(size)
        , m_grow(grow)
    {
    }

    /**
     * @brief Constructs a new Buffer object.
     *
     * @param grow Pointer to grow callback function.
     * @param ptr Pointer to initial data.
     * @param size Initial buffer size.
     * @param capacity Initial buffer capacity.
     */
    explicit constexpr Buffer(
        GrowCallback grow,
        T* ptr = nullptr,
        std::size_t size = 0,
        std::size_t capacity = 0) noexcept
        : m_ptr(ptr)
        , m_size(size)
        , m_capacity(capacity)
        , m_grow(grow)
    {
    }

    constexpr ~Buffer() = default;

    /**
     * @brief Move constructor.
     */
    Buffer(Buffer&&) = default;

    /**
     * @brief Move assignment operator.
     */
    auto operator=(Buffer&&) -> Buffer& = default;

    /**
     * @brief Sets the buffer data and capacity.
     *
     * @param buf_data Pointer to the new data.
     * @param buf_capacity New capacity.
     */
    constexpr void set(T* buf_data, std::size_t buf_capacity) noexcept
    {
        m_ptr = buf_data;
        m_capacity = buf_capacity;
    }

private:
    T* m_ptr;
    std::size_t m_size;
    std::size_t m_capacity;
    GrowCallback m_grow;
};
#endif

/**
 * @brief A memory buffer with a fixed initial capacity and dynamic growth.
 *
 * Stores elements in a stack-allocated array of size `Size`. If more space is needed,
 * it allocates additional memory on the heap using the specified allocator.
 *
 * Usage example:
 *```cpp
 * auto str = std::string_view{"test string"};
 * auto out = MemoryBuffer<char, 1024>();
 * out.append(str);
 *```
 *
 * @tparam T The type of elements stored in the buffer.
 * @tparam Size The initial capacity of the buffer.
 * @tparam Allocator The allocator used for dynamic memory allocation.
 */
template<typename T, std::size_t Size, typename Allocator = std::allocator<T>>
class MemoryBuffer : public Buffer<T> {
public:
    /// @cond
    using value_type = T; // NOLINT(readability-identifier-naming)
    using const_reference = const T&; // NOLINT(readability-identifier-naming)
    /// @endcond

    /**
     * @brief Constructs a new MemoryBuffer object.
     *
     * @param allocator Allocator for growing the buffer.
     */
    constexpr explicit MemoryBuffer(const Allocator& allocator = Allocator())
#if !defined(SLIMLOG_FMTLIB) || FMT_VERSION >= 110000
        : Buffer<T>(grow)
        , m_allocator(allocator)
#else
        : m_allocator(allocator)
#endif
    {
        this->set(static_cast<T*>(m_store), Size);
        if (std::is_constant_evaluated()) {
            std::fill_n(static_cast<T*>(m_store), Size, T());
        }
    }

    /**
     * @brief Destroys the MemoryBuffer object.
     */
    virtual constexpr ~MemoryBuffer()
    {
        deallocate();
    }

    /**
     * @brief Constructs a MemoryBuffer object by moving the content of another object to it.
     */
    constexpr MemoryBuffer(MemoryBuffer&& other) noexcept
#if !defined(SLIMLOG_FMTLIB) || FMT_VERSION >= 110000
        : Buffer<T>(grow)
#endif
    {
        move_from(other);
    }

    /**
     * @brief Moves the content of another `MemoryBuffer` object to this one.
     */
    auto operator=(MemoryBuffer&& other) noexcept -> MemoryBuffer&
    {
        assert(this != &other);
        deallocate();
        move_from(other);
        return *this;
    }

    constexpr MemoryBuffer(const MemoryBuffer& other) = delete;
    auto operator=(const MemoryBuffer& other) = delete;

    /**
     * @brief Returns a copy of the allocator associated with this buffer.
     *
     * @return Allocator associated with the buffer.
     */
    [[nodiscard]] auto get_allocator() const -> Allocator
    {
        return m_allocator;
    }

    /**
     * @brief Resizes the buffer to contain `count` elements.
     *
     * If T is a POD type, new elements may not be initialized.
     *
     * @param count Desired buffer size.
     */
    constexpr void resize(std::size_t count)
    {
        this->try_resize(count);
    }

    /**
     * @brief Increases the buffer capacity to `new_capacity`.
     *
     * @param new_capacity Desired buffer capacity.
     */
    void reserve(std::size_t new_capacity)
    {
        this->try_reserve(new_capacity);
    }

    /**
     * @brief Appends a contiguous range of elements to the buffer.
     *
     * Copies the elements from the provided range into the buffer.
     *
     * @tparam ContiguousRange A type representing a contiguous range (e.g., `std::string_view`).
     * @param range The range of elements to append.
     */
    template<typename ContiguousRange>
    void append(const ContiguousRange& range)
    {
        Buffer<T>::append(range.data(), range.data() + range.size());
    }

protected:
#if !defined(SLIMLOG_FMTLIB) || FMT_VERSION >= 110000
    /**
     * @brief Grows the buffer to accommodate additional elements.
     *
     * Allocates new memory with increased capacity and moves existing elements
     * to the new storage. Deallocates old memory if it was dynamically allocated.
     *
     * @param buf Reference to the buffer that needs to grow.
     * @param size The desired minimum capacity.
     */
    static constexpr void grow(Buffer<T>& buf, std::size_t size)
    {
        auto& self = static_cast<MemoryBuffer&>(buf);
#else
    /**
     * @brief Grows the buffer to the desired size.
     *
     * @param size Desired buffer size.
     */
    constexpr void grow(std::size_t size) final
    {
        auto& self = *this;
#endif
        const std::size_t max_size = std::allocator_traits<Allocator>::max_size(self.m_allocator);
        const std::size_t old_capacity = self.capacity();
        std::size_t new_capacity = old_capacity + old_capacity / 2;
        if (size > new_capacity) {
            new_capacity = size;
        } else if (new_capacity > max_size) {
            new_capacity = size > max_size ? size : max_size;
        }
        T* old_data = self.data();
        T* new_data = self.m_allocator.allocate(new_capacity);
        // The following code doesn't throw, so the raw pointer above doesn't leak.
        std::uninitialized_copy_n(old_data, self.size(), new_data);
        self.set(new_data, new_capacity);
        // Deallocate must not throw according to the standard, but even if it does,
        // the buffer already uses the new storage and will deallocate it in destructor.
        if (old_data != static_cast<T*>(self.m_store)) {
            self.m_allocator.deallocate(old_data, old_capacity);
        }
    }

private:
    /**
     * @brief Deallocates the buffer.
     */
    constexpr void deallocate()
    {
        T* data = this->data();
        if (data != static_cast<T*>(m_store)) {
            m_allocator.deallocate(data, this->capacity());
        }
    }

    /**
     * @brief Moves data from another buffer to this buffer.
     *
     * @param other Reference to the other buffer.
     */
    constexpr void move_from(MemoryBuffer& other)
    {
        m_allocator = std::move(other.m_allocator);
        T* data = other.data();
        const std::size_t size = other.size();
        const std::size_t capacity = other.capacity();
        // NOLINTBEGIN(*-array-to-pointer-decay,*-no-array-decay)
        if (data == other.m_store) {
            this->set(m_store, capacity);
            std::uninitialized_copy_n(other.m_store, size, m_store);
        } else {
            this->set(data, capacity);
            // Set pointer to the inline array so that delete is not called
            // when deallocating.
            other.set(other.m_store, 0);
            other.clear();
        }
        // NOLINTEND(*-array-to-pointer-decay,*-no-array-decay)
        this->resize(size);
    }

    T m_store[Size]; // NOLINT(*-avoid-c-arrays)
    Allocator m_allocator;
};

} // namespace SlimLog::Util
