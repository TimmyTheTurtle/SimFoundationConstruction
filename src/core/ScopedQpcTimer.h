#pragma once

#include <Windows.h>

// RAII timer: construction starts the measurement and destruction finishes it.
// This is exactly why scoped timers are attractive for profiling; the "end"
// event is guaranteed to happen even if a function returns early.
class ScopedQpcTimer {
    // We keep only the data needed to finish the measurement in the destructor:
    // a human-readable label, the cached performance-counter frequency, and the
    // counter value captured at construction time.
    const char* label_;
    LARGE_INTEGER freq_{}, start_{};

public:
    explicit ScopedQpcTimer(const char* label);
    ~ScopedQpcTimer() noexcept;
};

namespace scoped_qpc_timer::testing {

// The test hooks let unit tests replace the real Win32 timing/debug APIs with
// deterministic fakes. Production code never needs to touch these.
using QueryPerformanceFrequencyFn = decltype(&::QueryPerformanceFrequency);
using QueryPerformanceCounterFn = decltype(&::QueryPerformanceCounter);
using OutputDebugStringAFn = decltype(&::OutputDebugStringA);

struct Hooks {
    // By default the hooks point at the real operating-system functions.
    QueryPerformanceFrequencyFn query_performance_frequency = ::QueryPerformanceFrequency;
    QueryPerformanceCounterFn query_performance_counter = ::QueryPerformanceCounter;
    OutputDebugStringAFn output_debug_string = ::OutputDebugStringA;
};

// Install a new set of hooks for the next timers that are created.
void SetHooks(const Hooks& hooks);

// Restore the production hooks so tests do not leak state into one another.
void Reset();

} // namespace scoped_qpc_timer::testing
