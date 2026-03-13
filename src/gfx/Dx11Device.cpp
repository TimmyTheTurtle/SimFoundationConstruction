#include "gfx/Dx11Device.h"
#include <d3d11sdklayers.h>
#include <iterator>
#include <stdexcept>

static void ThrowIfFailed(HRESULT hr, const char* msg)
{
    // D3D APIs report failures through HRESULT rather than exceptions, so this
    // helper converts "failed HRESULT" into normal C++ control flow.
    if (FAILED(hr)) throw std::runtime_error(msg);
}

Dx11Device::Dx11Device(HWND hwnd, int width, int height, const Dx11DeviceOptions& options)
    : m_hwnd(hwnd), m_width(width), m_height(height)
{
    // This description tells DXGI what kind of back buffers we want and which
    // window will ultimately present them to the screen.
    DXGI_SWAP_CHAIN_DESC scd{};
    scd.BufferCount = 2;
    scd.BufferDesc.Width = width;
    scd.BufferDesc.Height = height;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hwnd;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; // keep simple for now

    UINT createFlags = options.enableDebugLayer ? D3D11_CREATE_DEVICE_DEBUG : 0u;

    // We ask for the newest feature levels we know how to use, then let D3D11
    // pick the first one the chosen driver actually supports.
    D3D_FEATURE_LEVEL flOut{};
    const D3D_FEATURE_LEVEL fls[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0
    };

    // Create the device, immediate context, and swap chain as a single unit so
    // all three agree about the adapter and feature level.
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        options.driverType,
        nullptr,
        createFlags,
        fls, (UINT)std::size(fls),
        D3D11_SDK_VERSION,
        &scd,
        m_swapChain.ReleaseAndGetAddressOf(),
        m_device.ReleaseAndGetAddressOf(),
        &flOut,
        m_context.ReleaseAndGetAddressOf());

    ThrowIfFailed(hr, "D3D11CreateDeviceAndSwapChain failed");

    // The swap chain owns the back-buffer texture; we still need a view so the
    // output-merger stage can bind that texture as a render target.
    CreateBackBufferRTV();
}
Dx11Device::~Dx11Device()
{
    // ClearState releases pipeline bindings so shutdown does not leave objects
    // artificially referenced through the immediate context.
    if (m_context)
        m_context->ClearState();

    // Reset the views and swap chain before the device so live-object reporting
    // can tell us about real leaks instead of shutdown ordering noise.
    m_rtv.Reset();
    m_swapChain.Reset();
    m_context.Reset();

#if defined(_DEBUG)
    Microsoft::WRL::ComPtr<ID3D11Debug> debug;
    m_device.As(&debug);
    m_device.Reset();
    if (debug)
        // In debug builds we ask D3D11 to print any still-live objects so COM
        // leaks are obvious during development.
        debug->ReportLiveDeviceObjects(D3D11_RLDO_SUMMARY | D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
#endif
}

void Dx11Device::CreateBackBufferRTV()
{
    // Recreating the RTV starts by discarding the old one, because it points at
    // the old back-buffer texture.
    m_rtv.Reset();

    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
    ThrowIfFailed(
        m_swapChain->GetBuffer(
            0,
            __uuidof(ID3D11Texture2D),
            reinterpret_cast<void**>(backBuffer.ReleaseAndGetAddressOf())),
        "GetBuffer failed");
    ThrowIfFailed(
        m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, m_rtv.ReleaseAndGetAddressOf()),
        "CreateRenderTargetView failed");

    // Bind the render target so Clear() and later draw calls know where to go.
    ID3D11RenderTargetView* rtvs[] = { m_rtv.Get() };
    m_context->OMSetRenderTargets(1, rtvs, nullptr);

    // The viewport must match the back-buffer size or the rasterizer will clip
    // or scale in surprising ways.
    D3D11_VIEWPORT vp{};
    vp.Width = (float)m_width;
    vp.Height = (float)m_height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    m_context->RSSetViewports(1, &vp);
}

void Dx11Device::Resize(int width, int height)
{
    // A minimized window can report zero dimensions. Treat that as a no-op so
    // we do not ask DXGI to create invalid buffers.
    if (!m_swapChain) return;
    if (width <= 0 || height <= 0) return;

    m_width = width;
    m_height = height;

    // The old render target still references the old buffers, so unbind and
    // release it before ResizeBuffers.
    m_context->OMSetRenderTargets(0, nullptr, nullptr);
    m_rtv.Reset();

    ThrowIfFailed(m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0), "ResizeBuffers failed");

    // After the resize, rebuild the RTV and viewport against the new buffers.
    CreateBackBufferRTV();
}

void Dx11Device::Clear(float r, float g, float b, float a)
{
    // Clear is our simplest rendering operation: paint the whole back buffer
    // with a single color.
    const float color[4] = { r, g, b, a };
    m_context->ClearRenderTargetView(m_rtv.Get(), color);
}

void Dx11Device::Present(bool vsync)
{
    // Present hands the back buffer to DXGI. Passing 1 enables vsync, while 0
    // presents immediately.
    m_swapChain->Present(vsync ? 1 : 0, 0);
}
