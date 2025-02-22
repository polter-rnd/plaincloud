/**
 * @file os.h
 * @brief Provides OS-specific functions.
 */

#pragma once

#if !defined(_WIN32) || defined(__STDC_WANT_SECURE_LIB__)
// In addition to <ctime> below for localtime_r() on POSIX and localtime_s() on Windows
#include <time.h> // IWYU pragma: keep
#endif

#include <chrono>
#include <ctime>
#include <tuple>
#include <utility>

// Used by <chrono> in C++20
// IWYU pragma: no_include <ratio>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h> // for GetCurrentThreadId
#else
#include <unistd.h>
#ifdef __linux__
#include <sys/syscall.h> // use gettid() syscall under linux to get thread id
#elif defined(_AIX)
#include <pthread.h> // for pthread_getthrds_np
#elif defined(__DragonFly__) || defined(__FreeBSD__)
#include <pthread_np.h> // for pthread_getthreadid_np
#elif defined(__NetBSD__)
#include <lwp.h> // for _lwp_self
#elif defined(__sun)
#include <thread.h> // for thr_self
#elif defined(__APPLE__)
#include <AvailabilityMacros.h> // for MAC_OS_X_VERSION_MAX_ALLOWED
#include <pthread.h> // for pthread_threadid_np
#endif
#endif

namespace SlimLog::Util::OS {

/**
 * @brief Retrieves the current thread ID across different platforms.
 *
 * Utilizes platform-specific APIs to obtain a unique identifier for the current thread.
 * If no platform-specific method is available, it falls back to hashing
 * `std::this_thread::get_id()`.
 *
 * @return The current thread ID as a `std::size_t`.
 */
[[nodiscard]] inline auto thread_id() noexcept -> std::size_t
{
    static thread_local std::size_t cached_tid;

    if (cached_tid == 0) {
#ifdef _WIN32
        cached_tid = static_cast<size_t>(::GetCurrentThreadId());
#elif defined(__linux__)
#if defined(__ANDROID__) && defined(__ANDROID_API__) && (__ANDROID_API__ < 21)
#define SYS_gettid __NR_gettid
#endif
        cached_tid = static_cast<size_t>(::syscall(SYS_gettid)); // NOLINT(*-vararg)
#elif defined(_AIX)
        struct __pthrdsinfo buf;
        int reg_size = 0;
        pthread_t pt = pthread_self();
        int retval
            = pthread_getthrds_np(&pt, PTHRDSINFO_QUERY_TID, &buf, sizeof(buf), NULL, &reg_size);
        int tid = (!retval) ? buf.__pi_tid : 0;
        cached_tid = static_cast<std::size_t>(tid);
#elif defined(__DragonFly__) || defined(__FreeBSD__)
        cached_tid = static_cast<std::size_t>(::pthread_getthreadid_np());
#elif defined(__NetBSD__)
        cached_tid = static_cast<std::size_t>(::_lwp_self());
#elif defined(__OpenBSD__)
        cached_tid = static_cast<std::size_t>(::getthrid());
#elif defined(__sun)
        cached_tid = static_cast<std::size_t>(::thr_self());
#elif defined(__APPLE__)
        uint64_t tid;
// There is no pthread_threadid_np prior to Mac OS X 10.6, and it is not supported on any PPC,
// including 10.6.8 Rosetta. __POWERPC__ is Apple-specific define encompassing ppc and ppc64.
#ifdef MAC_OS_X_VERSION_MAX_ALLOWED
        {
#if (MAC_OS_X_VERSION_MAX_ALLOWED < 1060) || defined(__POWERPC__)
            tid = ::pthread_mach_thread_np(::pthread_self());
#elif MAC_OS_X_VERSION_MIN_REQUIRED < 1060
            if (&pthread_threadid_np) {
                ::pthread_threadid_np(nullptr, &tid);
            } else {
                tid = ::pthread_mach_thread_np(::pthread_self());
            }
#else
            ::pthread_threadid_np(nullptr, &tid);
#endif
        }
#else
        ::pthread_threadid_np(nullptr, &tid);
#endif
        cached_tid = static_cast<std::size_t>(tid);
#else // Default to standard C++11 (other Unix)
        cached_tid
            = static_cast<std::size_t>(std::hash<std::thread::id>()(std::this_thread::get_id()));
#endif
    }

    return cached_tid;
}

/**
 * @brief Gets the local time and nanoseconds component.
 *
 * Fetches the current local time, along with the additional nanoseconds,
 * providing higher resolution time data.
 *
 * @tparam TimePoint The type used to represent the time point (e.g., `std::tm`).
 * @return A pair consisting of the local time and the nanoseconds part.
 */
[[nodiscard]] inline auto local_time() -> std::pair<std::chrono::sys_seconds, std::size_t>
{
    static thread_local std::chrono::sys_seconds cached_local;
    static thread_local std::time_t cached_time;

    std::timespec curtime{};
#ifdef __linux__
    std::ignore = ::clock_gettime(CLOCK_REALTIME_COARSE, &curtime);
#else
    std::ignore = std::timespec_get(&curtime, TIME_UTC);
#endif

    if (curtime.tv_sec != cached_time) {
        cached_time = curtime.tv_sec;

        std::tm local_tm{};
#ifdef _WIN32
#ifdef __STDC_WANT_SECURE_LIB__
        std::ignore = ::localtime_s(&local_tm, &cached_time);
#else
        // MSVC is known to use thread-local buffer
        local_tm = *std::localtime(&cached_time);
#endif
#else
        std::ignore = ::localtime_r(&cached_time, &local_tm);
#endif

        constexpr int TmEpoch = 1900;
        cached_local = std::chrono::sys_days(std::chrono::year_month_day(
                           std::chrono::year(local_tm.tm_year + TmEpoch),
                           std::chrono::month(local_tm.tm_mon),
                           std::chrono::day(local_tm.tm_mday)))
            + std::chrono::hours(local_tm.tm_hour) + std::chrono::minutes(local_tm.tm_min)
            + std::chrono::seconds(local_tm.tm_sec);
    }

    return std::make_pair(cached_local, static_cast<std::size_t>(curtime.tv_nsec));
}

} // namespace SlimLog::Util::OS
