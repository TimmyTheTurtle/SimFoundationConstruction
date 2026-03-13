#pragma once
#include <wrl/client.h>
#include <d3d11.h>
#include <dxgi.h>

// These options let tests request a WARP device while the sample application
// continues to default to normal hardware creation.
struct Dx11DeviceOptions
{
    D3D_DRIVER_TYPE driverType{ D3D_DRIVER_TYPE_HARDWARE };
    bool enableDebugLayer{
#if defined(_DEBUG)
        true
#else
        false
#endif
    };
};

// Dx11Device is a tiny RAII wrapper around the minimum D3D11 objects needed to
// clear and present a swap chain. The goal here is clarity, not a full engine.
class Dx11Device
{
public:
    Dx11Device(HWND hwnd, int width, int height, const Dx11DeviceOptions& options = {});
    ~Dx11Device();
    Dx11Device(const Dx11Device&) = delete;
    Dx11Device& operator=(const Dx11Device&) = delete;

    void Resize(int width, int height);
    void Clear(float r, float g, float b, float a);
    void Present(bool vsync);

private:
    // Whenever the swap chain changes size, the back-buffer texture changes
    // too, so we rebuild the render-target view through this helper.
    void CreateBackBufferRTV();

    // ComPtr gives us COM lifetime management without manual Release calls.
    Microsoft::WRL::ComPtr<ID3D11Device>           m_device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext>    m_context;
    Microsoft::WRL::ComPtr<IDXGISwapChain>         m_swapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_rtv;

    // We cache the window handle and dimensions so resize logic can recreate
    // the viewport with the most recent size.
    HWND m_hwnd{};
    int  m_width{};
    int  m_height{};
};
