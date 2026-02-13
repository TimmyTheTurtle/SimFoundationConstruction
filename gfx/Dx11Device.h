#pragma once
#include <wrl/client.h>
#include <d3d11.h>
#include <dxgi.h>

class Dx11Device
{
public:
    Dx11Device(HWND hwnd, int width, int height);

    Dx11Device(const Dx11Device&) = delete;
    Dx11Device& operator=(const Dx11Device&) = delete;

    void Resize(int width, int height);
    void Clear(float r, float g, float b, float a);
    void Present(bool vsync);

private:
    void CreateBackBufferRTV();

    Microsoft::WRL::ComPtr<ID3D11Device>           m_device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext>    m_context;
    Microsoft::WRL::ComPtr<IDXGISwapChain>         m_swapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_rtv;

    HWND m_hwnd{};
    int  m_width{};
    int  m_height{};
};
