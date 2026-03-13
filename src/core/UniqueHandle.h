#pragma once

#include <Windows.h>

#include <utility>

// UniqueHandle is the HANDLE analogue of std::unique_ptr: exactly one object
// owns the OS handle, and destruction closes it automatically.
class UniqueHandle
{
public:
    // Default construction means "owns nothing".
    UniqueHandle() noexcept = default;

    // This constructor adopts an already-created raw HANDLE.
    explicit UniqueHandle(HANDLE handle) noexcept
        : handle_(handle)
    {
    }

    ~UniqueHandle() noexcept
    {
        // Destruction funnels through reset() so there is one place that knows
        // how to safely close the currently-owned handle.
        reset();
    }

    // Copying would create two owners for one handle, so we forbid it.
    UniqueHandle(const UniqueHandle&) = delete;
    UniqueHandle& operator=(const UniqueHandle&) = delete;

    // Moving transfers ownership and leaves the source empty.
    UniqueHandle(UniqueHandle&& other) noexcept
        : handle_(other.release())
    {
    }

    UniqueHandle& operator=(UniqueHandle&& other) noexcept
    {
        if (this != &other)
        {
            // release() detaches the source without closing it; reset() then
            // closes our old handle (if any) before adopting the new one.
            reset(other.release());
        }
        return *this;
    }

    [[nodiscard]] HANDLE get() const noexcept
    {
        // get() intentionally does not transfer ownership; it only exposes the
        // underlying HANDLE for APIs that need to inspect or use it.
        return handle_;
    }

    [[nodiscard]] explicit operator bool() const noexcept
    {
        // This lets callers write "if (handle) ..." as a concise validity test.
        return IsValid(handle_);
    }

    [[nodiscard]] HANDLE release() noexcept
    {
        // release() hands ownership back to the caller. The caller now becomes
        // responsible for eventually closing the returned handle.
        HANDLE released = handle_;
        handle_ = nullptr;
        return released;
    }

    void reset(HANDLE handle = nullptr) noexcept
    {
        // If we already own a different valid handle, close it before replacing
        // it. If the new value is the same as the old one, we do nothing.
        if (handle_ != handle && IsValid(handle_))
        {
            CloseHandle(handle_);
        }
        handle_ = handle;
    }

    void swap(UniqueHandle& other) noexcept
    {
        // swap() is useful for generic algorithms and custom ownership shuffles.
        std::swap(handle_, other.handle_);
    }

    [[nodiscard]] static bool IsValid(HANDLE handle) noexcept
    {
        // Win32 uses both nullptr and INVALID_HANDLE_VALUE as sentinel "not a
        // usable handle" values depending on the API, so we treat both as empty.
        return handle != nullptr && handle != INVALID_HANDLE_VALUE;
    }

private:
    HANDLE handle_ = nullptr;
};

inline void swap(UniqueHandle& lhs, UniqueHandle& rhs) noexcept
{
    // Non-member swap enables ADL and keeps the type friendly to generic code.
    lhs.swap(rhs);
}
