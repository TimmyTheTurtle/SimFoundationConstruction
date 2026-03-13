// Scoped QPC timer (debug output)
#include "ScopedQpcTimer.h"

#include <atomic>
#include <mutex>
#include <string>

namespace {

struct ScopedQpcTimerRuntime {
    // A value of 0 means "frequency has not been queried yet".
    std::atomic<LONGLONG> cached_frequency{0};

    // Only the cold path takes this mutex. Once the frequency is cached, normal
    // timer construction stays lock-free.
    std::mutex frequency_mutex;

    // These atomics make the hot path cheap while still letting tests swap in
    // fake functions without rewriting production code.
    std::atomic<scoped_qpc_timer::testing::QueryPerformanceFrequencyFn> query_performance_frequency{
        ::QueryPerformanceFrequency};
    std::atomic<scoped_qpc_timer::testing::QueryPerformanceCounterFn> query_performance_counter{
        ::QueryPerformanceCounter};
    std::atomic<scoped_qpc_timer::testing::OutputDebugStringAFn> output_debug_string{
        ::OutputDebugStringA};
};

ScopedQpcTimerRuntime& GetRuntime()
{
    // One shared runtime keeps the cached frequency and the current hook set.
    static ScopedQpcTimerRuntime runtime;
    return runtime;
}

LARGE_INTEGER MakeLargeInteger(LONGLONG value)
{
    // LARGE_INTEGER is the Win32 carrier type for QPC values, so this helper
    // lets us convert from our cached integer representation back to that type.
    LARGE_INTEGER large_integer{};
    large_integer.QuadPart = value;
    return large_integer;
}

LARGE_INTEGER GetCachedFrequency()
{
    ScopedQpcTimerRuntime& runtime = GetRuntime();

    // Fast path: most timers should see an already-cached frequency and avoid
    // taking the mutex entirely.
    LONGLONG cached_frequency = runtime.cached_frequency.load(std::memory_order_acquire);

    if (cached_frequency == 0)
    {
        // Slow path: the first timer (or a test after Reset()) initializes the
        // cache exactly once while holding the mutex.
        std::lock_guard<std::mutex> lock(runtime.frequency_mutex);
        cached_frequency = runtime.cached_frequency.load(std::memory_order_relaxed);

        if (cached_frequency == 0)
        {
            LARGE_INTEGER frequency{};
            // QueryPerformanceFrequency is relatively expensive compared with a
            // simple load, so we do it once and reuse the answer thereafter.
            runtime.query_performance_frequency.load(std::memory_order_relaxed)(&frequency);
            cached_frequency = frequency.QuadPart;
            runtime.cached_frequency.store(cached_frequency, std::memory_order_release);
        }
    }

    return MakeLargeInteger(cached_frequency);
}

} // namespace

ScopedQpcTimer::ScopedQpcTimer(const char* label)
    : label_(label), freq_(GetCachedFrequency())
{
    // Construction marks the beginning of the measured span.
    GetRuntime().query_performance_counter.load(std::memory_order_relaxed)(&start_);
}

ScopedQpcTimer::~ScopedQpcTimer() noexcept
{
    // Destruction marks the end of the measured span. Because destructors run
    // during normal scope exit, early returns, and exception unwinding, this
    // gives us a very reliable "always finishes" timing primitive.
    LARGE_INTEGER end{};
    GetRuntime().query_performance_counter.load(std::memory_order_relaxed)(&end);

    const double ms =
        (end.QuadPart - start_.QuadPart) * 1000.0 / static_cast<double>(freq_.QuadPart);

    // OutputDebugStringA is convenient for lightweight instrumentation because
    // debuggers and tools can read it without us building a full logging stack.
    const std::string message = std::string(label_) + ": " + std::to_string(ms) + " ms\n";
    GetRuntime().output_debug_string.load(std::memory_order_relaxed)(message.c_str());
}

namespace scoped_qpc_timer::testing {

void SetHooks(const Hooks& hooks)
{
    ScopedQpcTimerRuntime& runtime = GetRuntime();

    // Tests can pass nullptr to mean "fall back to the real API".
    runtime.query_performance_frequency.store(
        hooks.query_performance_frequency != nullptr ? hooks.query_performance_frequency
                                                     : ::QueryPerformanceFrequency,
        std::memory_order_relaxed);
    runtime.query_performance_counter.store(
        hooks.query_performance_counter != nullptr ? hooks.query_performance_counter
                                                   : ::QueryPerformanceCounter,
        std::memory_order_relaxed);
    runtime.output_debug_string.store(
        hooks.output_debug_string != nullptr ? hooks.output_debug_string : ::OutputDebugStringA,
        std::memory_order_relaxed);

    // A hook change must invalidate the cached frequency, otherwise a later
    // test could observe the previous test's fake frequency value.
    runtime.cached_frequency.store(0, std::memory_order_release);
}

void Reset()
{
    // Reset simply reinstalls the default production hooks.
    SetHooks(Hooks{});
}

} // namespace scoped_qpc_timer::testing
