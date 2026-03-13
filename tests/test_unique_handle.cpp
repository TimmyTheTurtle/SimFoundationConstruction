#include <gtest/gtest.h>

#include <Windows.h>

#include <type_traits>
#include <utility>

#include "core/UniqueHandle.h"

namespace {

// We create real Win32 event handles in these tests because the behavior we
// care about lives at the operating-system level: valid handle, invalid handle,
// double-close avoidance, and ownership transfer.
HANDLE CreateTestEvent()
{
    HANDLE handle = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    EXPECT_NE(handle, nullptr);
    return handle;
}

// After a UniqueHandle releases ownership, the original raw handle should no
// longer be usable. This helper checks that the OS agrees.
void ExpectInvalidHandle(HANDLE handle)
{
    DWORD flags = 0;
    SetLastError(ERROR_SUCCESS);
    EXPECT_FALSE(GetHandleInformation(handle, &flags));
    EXPECT_EQ(GetLastError(), static_cast<DWORD>(ERROR_INVALID_HANDLE));
}

} // namespace

// The type traits act as a compile-time contract for the class design: move-only.
static_assert(!std::is_copy_constructible_v<UniqueHandle>);
static_assert(!std::is_copy_assignable_v<UniqueHandle>);
static_assert(std::is_move_constructible_v<UniqueHandle>);
static_assert(std::is_move_assignable_v<UniqueHandle>);

TEST(UniqueHandle, DefaultConstructedHandleIsEmpty)
{
    // The default state should behave like "owns nothing".
    UniqueHandle handle;

    EXPECT_FALSE(handle);
    EXPECT_EQ(handle.get(), nullptr);
}

TEST(UniqueHandle, InvalidHandleValueIsTreatedAsEmpty)
{
    // Some Win32 APIs use INVALID_HANDLE_VALUE instead of nullptr to signal
    // failure. Our wrapper treats both as "not an owned handle".
    UniqueHandle handle(INVALID_HANDLE_VALUE);

    EXPECT_FALSE(handle);
    EXPECT_EQ(handle.get(), INVALID_HANDLE_VALUE);

    handle.reset();
    EXPECT_FALSE(handle);
    EXPECT_EQ(handle.get(), nullptr);
}

TEST(UniqueHandle, ClosesOwnedHandleOnScopeExit)
{
    // The central RAII promise: once the wrapper dies, the kernel handle should
    // already be closed without any manual cleanup code.
    const HANDLE rawHandle = CreateTestEvent();
    ASSERT_NE(rawHandle, nullptr);

    {
        UniqueHandle handle(rawHandle);
        ASSERT_TRUE(handle);
        EXPECT_EQ(handle.get(), rawHandle);
    }

    ExpectInvalidHandle(rawHandle);
}

TEST(UniqueHandle, MoveConstructorTransfersOwnership)
{
    // A move constructor should transfer responsibility from source to
    // destination without duplicating the underlying OS handle.
    const HANDLE rawHandle = CreateTestEvent();
    ASSERT_NE(rawHandle, nullptr);

    UniqueHandle original(rawHandle);
    UniqueHandle moved(std::move(original));

    EXPECT_FALSE(original);
    EXPECT_EQ(original.get(), nullptr);
    EXPECT_TRUE(moved);
    EXPECT_EQ(moved.get(), rawHandle);

    moved.reset();
    ExpectInvalidHandle(rawHandle);
}

TEST(UniqueHandle, MoveAssignmentClosesOldHandleAndTakesOwnership)
{
    // Move assignment has two jobs: clean up the destination's old handle and
    // then adopt the source's handle.
    const HANDLE firstRawHandle = CreateTestEvent();
    const HANDLE secondRawHandle = CreateTestEvent();
    ASSERT_NE(firstRawHandle, nullptr);
    ASSERT_NE(secondRawHandle, nullptr);

    UniqueHandle first(firstRawHandle);
    UniqueHandle second(secondRawHandle);

    second = std::move(first);

    EXPECT_FALSE(first);
    EXPECT_EQ(first.get(), nullptr);
    EXPECT_TRUE(second);
    EXPECT_EQ(second.get(), firstRawHandle);

    ExpectInvalidHandle(secondRawHandle);

    second.reset();
    ExpectInvalidHandle(firstRawHandle);
}

TEST(UniqueHandle, ResetClosesPreviousHandleAndAdoptsNewOne)
{
    // reset(new_handle) is the explicit "replace what I own" operation.
    const HANDLE firstRawHandle = CreateTestEvent();
    const HANDLE secondRawHandle = CreateTestEvent();
    ASSERT_NE(firstRawHandle, nullptr);
    ASSERT_NE(secondRawHandle, nullptr);

    UniqueHandle handle(firstRawHandle);
    handle.reset(secondRawHandle);

    EXPECT_EQ(handle.get(), secondRawHandle);
    ExpectInvalidHandle(firstRawHandle);

    handle.reset();
    EXPECT_FALSE(handle);
    EXPECT_EQ(handle.get(), nullptr);
    ExpectInvalidHandle(secondRawHandle);
}

TEST(UniqueHandle, ReleaseHandsOwnershipBackToCaller)
{
    // release() intentionally escapes RAII and returns to raw-handle land. Once
    // we do that, the caller becomes responsible for CloseHandle again.
    const HANDLE rawHandle = CreateTestEvent();
    ASSERT_NE(rawHandle, nullptr);

    UniqueHandle handle(rawHandle);
    HANDLE released = handle.release();

    EXPECT_EQ(released, rawHandle);
    EXPECT_FALSE(handle);
    EXPECT_EQ(handle.get(), nullptr);

    DWORD flags = 0;
    EXPECT_TRUE(GetHandleInformation(released, &flags));
    EXPECT_TRUE(CloseHandle(released));
}

TEST(UniqueHandle, HandleCountReturnsToBaselineAfterRepeatedScopes)
{
    // This is a small leak test: repeated short-lived wrappers should not leave
    // the process with more open handles than it started with.
    DWORD before = 0;
    DWORD after = 0;

    ASSERT_TRUE(GetProcessHandleCount(GetCurrentProcess(), &before));

    for (int index = 0; index < 64; ++index)
    {
        UniqueHandle handle(CreateTestEvent());
        ASSERT_TRUE(handle);
    }

    ASSERT_TRUE(GetProcessHandleCount(GetCurrentProcess(), &after));
    EXPECT_EQ(after, before);
}
