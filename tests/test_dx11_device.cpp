#include <gtest/gtest.h>

#include <Windows.h>

#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>

#include "gfx/Dx11Device.h"

namespace {

// D3D11 swap-chain creation needs a real HWND, even in a test. This helper
// builds a tiny hidden window so the graphics code can run without showing UI.
class HiddenTestWindow
{
public:
    HiddenTestWindow(int width, int height)
    {
        // RegisterClassW is process-global, so we do it once and then reuse the
        // same class name for every test window.
        RegisterClassOnce();

        hwnd_ = CreateWindowExW(
            0,
            ClassName(),
            L"SimFoundation Dx11 Test Window",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            width,
            height,
            nullptr,
            nullptr,
            GetModuleHandleW(nullptr),
            nullptr);

        if (!hwnd_)
        {
            throw std::runtime_error("CreateWindowExW failed");
        }
    }

    ~HiddenTestWindow()
    {
        // The test owns the window, so it also owns the destruction step.
        if (hwnd_)
        {
            DestroyWindow(hwnd_);
        }
    }

    HiddenTestWindow(const HiddenTestWindow&) = delete;
    HiddenTestWindow& operator=(const HiddenTestWindow&) = delete;

    [[nodiscard]] HWND hwnd() const noexcept
    {
        return hwnd_;
    }

private:
    static void RegisterClassOnce()
    {
        static std::once_flag registered;
        std::call_once(registered, [] {
            // A minimal window class is enough because we never show the window
            // or run a custom message loop in these tests.
            WNDCLASSW windowClass{};
            windowClass.lpfnWndProc = DefWindowProcW;
            windowClass.hInstance = GetModuleHandleW(nullptr);
            windowClass.lpszClassName = ClassName();

            if (!RegisterClassW(&windowClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            {
                throw std::runtime_error("RegisterClassW failed");
            }
        });
    }

    static const wchar_t* ClassName() noexcept
    {
        return L"SimFoundationDx11TestWindow";
    }

    HWND hwnd_ = nullptr;
};

Dx11DeviceOptions WarpOptions()
{
    // WARP is Microsoft's software rasterizer. It makes graphics tests much
    // more reliable on machines that lack a suitable hardware adapter.
    Dx11DeviceOptions options;
    options.driverType = D3D_DRIVER_TYPE_WARP;
    options.enableDebugLayer = false;
    return options;
}

std::unique_ptr<Dx11Device> TryCreateWarpDevice(HWND hwnd, int width, int height, std::string& error)
{
    // Device creation can fail on some environments, so tests use this helper
    // to convert "throw on failure" into "skip with explanation".
    try
    {
        return std::make_unique<Dx11Device>(hwnd, width, height, WarpOptions());
    }
    catch (const std::exception& ex)
    {
        error = ex.what();
        return nullptr;
    }
}

} // namespace

TEST(Dx11Device, WarpDeviceConstructsClearsPresentsAndResizes)
{
    // This is the happy-path integration test: construct a device, perform a
    // couple of rendering operations, resize, and make sure none of it throws.
    HiddenTestWindow window(128, 128);
    std::string error;
    std::unique_ptr<Dx11Device> device = TryCreateWarpDevice(window.hwnd(), 128, 128, error);
    if (!device)
    {
        GTEST_SKIP() << "Unable to create D3D11 WARP device: " << error;
    }
    ASSERT_NE(device, nullptr);

    EXPECT_NO_THROW(device->Clear(0.1f, 0.2f, 0.3f, 1.0f));
    EXPECT_NO_THROW(device->Present(false));
    EXPECT_NO_THROW(device->Resize(192, 96));
    EXPECT_NO_THROW(device->Clear(0.4f, 0.3f, 0.2f, 1.0f));
    EXPECT_NO_THROW(device->Present(false));
}

TEST(Dx11Device, ZeroSizedResizeRequestsAreSafeNoOps)
{
    // Resize requests can briefly hit zero or negative values when a window is
    // minimized or the platform reports transient geometry. We want those
    // inputs to be harmless no-ops instead of crashing the graphics layer.
    HiddenTestWindow window(96, 96);
    std::string error;
    std::unique_ptr<Dx11Device> device = TryCreateWarpDevice(window.hwnd(), 96, 96, error);
    if (!device)
    {
        GTEST_SKIP() << "Unable to create D3D11 WARP device: " << error;
    }
    ASSERT_NE(device, nullptr);

    EXPECT_NO_THROW(device->Resize(0, 96));
    EXPECT_NO_THROW(device->Resize(96, 0));
    EXPECT_NO_THROW(device->Resize(-1, 96));
    EXPECT_NO_THROW(device->Resize(96, -1));
    EXPECT_NO_THROW(device->Clear(0.0f, 0.0f, 0.0f, 1.0f));
    EXPECT_NO_THROW(device->Present(false));
}
