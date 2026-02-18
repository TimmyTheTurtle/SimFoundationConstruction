#include "gfx/Dx11Device.h"
#include <stdexcept>

static void ThrowIfFailed(HRESULT hr, const char* msg)
{
    if (FAILED(hr)) throw std::runtime_error(msg);
}

Dx11Device::Dx11Device(HWND hwnd, int width, int height)
    : m_hwnd(hwnd), m_width(width), m_height(height)
{
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

    UINT createFlags = 0;
#if defined(_DEBUG)
    createFlags |= D3D11_CREATE_DEVICE_DEBUG; // requires Graphics Tools installed
#endif

    D3D_FEATURE_LEVEL flOut{};
    const D3D_FEATURE_LEVEL fls[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0
    };

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        createFlags,
        fls, (UINT)std::size(fls),
        D3D11_SDK_VERSION,
        &scd,
        &m_swapChain,
        &m_device,
        &flOut,
        &m_context);

    ThrowIfFailed(hr, "D3D11CreateDeviceAndSwapChain failed");

    CreateBackBufferRTV();
}

void Dx11Device::CreateBackBufferRTV()
{
    m_rtv.Reset();

    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
    ThrowIfFailed(m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)), "GetBuffer failed");
    ThrowIfFailed(m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, &m_rtv), "CreateRenderTargetView failed");

    ID3D11RenderTargetView* rtvs[] = { m_rtv.Get() };
    m_context->OMSetRenderTargets(1, rtvs, nullptr);

    D3D11_VIEWPORT vp{};
    vp.Width = (float)m_width;
    vp.Height = (float)m_height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    m_context->RSSetViewports(1, &vp);
}

void Dx11Device::Resize(int width, int height)
{
    if (!m_swapChain) return;
    if (width <= 0 || height <= 0) return;

    m_width = width;
    m_height = height;

    m_context->OMSetRenderTargets(0, nullptr, nullptr);
    m_rtv.Reset();

    ThrowIfFailed(m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0), "ResizeBuffers failed");
    CreateBackBufferRTV();
}

void Dx11Device::Clear(float r, float g, float b, float a)
{
    const float color[4] = { r, g, b, a };
    m_context->ClearRenderTargetView(m_rtv.Get(), color);
}

void Dx11Device::Present(bool vsync)
{
    m_swapChain->Present(vsync ? 1 : 0, 0);
}
