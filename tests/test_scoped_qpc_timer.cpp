#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "core/ScopedQpcTimer.h"

namespace {

// These tests swap out the real QPC and OutputDebugString functions so we can
// make timing deterministic. That turns a "hard to observe" profiling utility
// into something we can unit test precisely.
using ScopedQpcTimerHooks = scoped_qpc_timer::testing::Hooks;

struct FakeQpcRuntime {
    // frequency is expressed in "ticks per second". Using 1000 keeps the math
    // simple because one tick then equals one millisecond.
    LONGLONG frequency = 1000;

    // We script the counter values the timer should observe at construction and
    // destruction time.
    std::vector<LONGLONG> counters;

    // Every debug message the timer emits is captured here instead of going to
    // the debugger.
    std::vector<std::string> messages;
    std::size_t next_counter_index = 0;
    int frequency_call_count = 0;

    static FakeQpcRuntime*& Current()
    {
        static FakeQpcRuntime* runtime = nullptr;
        return runtime;
    }

    static BOOL WINAPI QueryPerformanceFrequency(LARGE_INTEGER* frequency)
    {
        // Tests use this count to prove that the production code caches the
        // frequency instead of querying it for every scope.
        FakeQpcRuntime& runtime = *Current();
        ++runtime.frequency_call_count;
        frequency->QuadPart = runtime.frequency;
        return TRUE;
    }

    static BOOL WINAPI QueryPerformanceCounter(LARGE_INTEGER* counter)
    {
        // Each call consumes one scripted timestamp from the vector.
        FakeQpcRuntime& runtime = *Current();
        EXPECT_LT(runtime.next_counter_index, runtime.counters.size());

        if (runtime.next_counter_index >= runtime.counters.size())
        {
            counter->QuadPart = 0;
            return FALSE;
        }

        counter->QuadPart = runtime.counters[runtime.next_counter_index++];
        return TRUE;
    }

    static void WINAPI OutputDebugString(LPCSTR message)
    {
        // Instead of writing to a debugger, store the text so the test can
        // inspect the label and computed milliseconds.
        Current()->messages.emplace_back(message);
    }

    void Install()
    {
        // The production code reads its hooks globally, so tests install one
        // fake runtime before exercising the timer.
        Current() = this;
        scoped_qpc_timer::testing::SetHooks(ScopedQpcTimerHooks{
            &QueryPerformanceFrequency, &QueryPerformanceCounter, &OutputDebugString});
    }
};

class ScopedQpcTimerTest : public ::testing::Test {
protected:
    void TearDown() override
    {
        // Resetting here prevents one test's fake clock from affecting the next.
        scoped_qpc_timer::testing::Reset();
        FakeQpcRuntime::Current() = nullptr;
    }
};

void ExpectContains(std::string_view text, std::string_view needle)
{
    // We only care that the message contains the important pieces, not that we
    // match the entire string byte-for-byte.
    EXPECT_NE(text.find(needle), std::string_view::npos);
}

bool TimeRenderScopeAndReturn()
{
    // This helper simulates the "return early from a function" case that makes
    // scoped timers useful in the first place.
    ScopedQpcTimer timer("render_ms");
    return true;
}

} // namespace

// The destructor is the critical cleanup point, so we assert the noexcept part
// of the exercise contract at compile time.
static_assert(std::is_nothrow_destructible_v<ScopedQpcTimer>);

TEST_F(ScopedQpcTimerTest, LogsElapsedMillisecondsOnScopeExit)
{
    // With a frequency of 1000, going from 100 ticks to 125 ticks should log
    // exactly 25 milliseconds.
    FakeQpcRuntime runtime;
    runtime.counters = {100, 125};
    runtime.Install();

    {
        ScopedQpcTimer timer("physics_ms");
    }

    ASSERT_EQ(runtime.messages.size(), 1u);
    EXPECT_EQ(runtime.frequency_call_count, 1);
    ExpectContains(runtime.messages.front(), "physics_ms");
    ExpectContains(runtime.messages.front(), "25.000000 ms");
}

TEST_F(ScopedQpcTimerTest, LogsWhenScopeExitsViaEarlyReturn)
{
    // Returning out of the middle of a function should still trigger the
    // destructor, and therefore still emit a timing message.
    FakeQpcRuntime runtime;
    runtime.counters = {50, 90};
    runtime.Install();

    EXPECT_TRUE(TimeRenderScopeAndReturn());

    ASSERT_EQ(runtime.messages.size(), 1u);
    ExpectContains(runtime.messages.front(), "render_ms");
    ExpectContains(runtime.messages.front(), "40.000000 ms");
}

TEST_F(ScopedQpcTimerTest, CachesFrequencyAcrossMultipleScopes)
{
    // Two separate timers should share the same cached frequency query.
    FakeQpcRuntime runtime;
    runtime.counters = {0, 10, 20, 50};
    runtime.Install();

    {
        ScopedQpcTimer first("physics_ms");
    }
    {
        ScopedQpcTimer second("render_ms");
    }

    ASSERT_EQ(runtime.messages.size(), 2u);
    EXPECT_EQ(runtime.frequency_call_count, 1);
    ExpectContains(runtime.messages.front(), "physics_ms");
    ExpectContains(runtime.messages.back(), "render_ms");
}
