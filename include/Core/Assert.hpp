#pragma once

#include "Logging.hpp"

#include <cstdlib>
#include <source_location>
#include <string_view>

#if defined(_MSC_VER)
#define CORE_DEBUG_BREAK() __debugbreak()
#elif defined(__GNUC__) || defined(__clang__)
#define CORE_DEBUG_BREAK() __builtin_trap()
#else
#include <csignal>
#define CORE_DEBUG_BREAK() raise(SIGTRAP)
#endif

namespace memory::core::detail {

// Not noexcept: this is a hard-failure path, and if a future caller wants to
// turn it into a throw instead of abort() (e.g. for a fuzzing harness), that
// shouldn't require touching the macros.
[[noreturn]] inline void AssertFail(std::string_view expr, std::string_view msg,
                                    std::source_location loc) {
  Logger::Instance().Log(LogLevel::Fatal, loc, "Assertion failed: ({}) {}",
                         expr, msg);
  CORE_DEBUG_BREAK();
  std::abort();
}

} // namespace memory::core::detail

// Always active, in debug AND release. Use for conditions that must never
// be violated without immediately corrupting program state — a failed
// munmap/VirtualFree, a null pointer escaping the platform layer, a
// misaligned pointer handed to a pool allocator's free(). If you're ever
// tempted to disable this in release "for performance", that's a sign the
// check belongs in CORE_ASSERT instead, not that VERIFY should be weaker.
#define CORE_VERIFY(cond, msg)                                                 \
  do {                                                                         \
    if (!(cond)) {                                                             \
      ::memory::core::detail::AssertFail(#cond, msg,                           \
                                         std::source_location::current());     \
    }                                                                          \
  } while (0)

// Compiled out entirely in release (NDEBUG) — the condition itself is never
// evaluated, so this costs nothing and won't trigger unused-variable
// warnings on release builds. Use for expensive or redundant sanity checks
// you only want while developing (bounds checks, invariant re-verification,
// canary checks on every allocation, etc).
//
// Define CORE_FORCE_ASSERTS to keep these live in a release build too
// (e.g. a "release with checks" profiling configuration).
#if !defined(NDEBUG) || defined(CORE_FORCE_ASSERTS)
#define CORE_ASSERT(cond, msg) CORE_VERIFY(cond, msg)
#define CORE_ENABLE_ASSERTS 1
#else
#define CORE_ASSERT(cond, msg) ((void)0)
#define CORE_ENABLE_ASSERTS 0
#endif
