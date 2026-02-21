# Exercise 5: Textures

**Difficulty**: ⭐⭐⭐⭐ Advanced  
**Time**: 60-90 minutes

## Learning Objectives
- Load images from files into textures
- Create shader resource views (SRVs)
- Create and configure samplers
- Sample textures in pixel shaders
- Understand texture coordinates (UVs)

## Background

Textures are images stored in GPU memory. Instead of vertex colors, we sample pixels from the texture based on UV coordinates. This is fundamental for sprites, UI, and almost all game graphics!

**Texture Coordinates (UVs)**:
- U: Horizontal (0.0 = left, 1.0 = right)
- V: Vertical (0.0 = top, 1.0 = bottom)
- Each vertex has a UV coordinate
- Pixel shader interpolates UVs and samples the texture

## Your Tasks

### Task 5.1: Update Vertex Structure

Add texture coordinates:

```cpp
struct Vertex {
    float x, y;       // Position
    float r, g, b;    // Color (can be multiplied with texture)
    float u, v;       // Texture coordinates
};

// Example quad with UVs
Vertex quadVertices[] = {
    {-0.5f,  0.5f,  1.0f, 1.0f, 1.0f,  0.0f, 0.0f },  // Top-left
    { 0.5f,  0.5f,  1.0f, 1.0f, 1.0f,  1.0f, 0.0f },  // Top-right
    { 0.5f, -0.5f,  1.0f, 1.0f, 1.0f,  1.0f, 1.0f },  // Bottom-right
    {-0.5f, -0.5f,  1.0f, 1.0f, 1.0f,  0.0f, 1.0f }   // Bottom-left
};
```

### Task 5.2: Update Shaders

Modify your HLSL to use textures:

```hlsl
// Add texture and sampler declarations
Texture2D myTexture : register(t0);
SamplerState mySampler : register(s0);

struct VSInput {
    float2 pos : POSITION;
    float3 color : COLOR;
    float2 uv : TEXCOORD;
};

struct VSOutput {
    float4 pos : SV_POSITION;
    float3 color : COLOR;
    float2 uv : TEXCOORD;
};

VSOutput VSMain(VSInput input) {
    VSOutput output;
    output.pos = float4(input.pos, 0.0f, 1.0f);
    output.color = input.color;
    output.uv = input.uv;
    return output;
}

float4 PSMain(VSOutput input) : SV_TARGET {
    // Sample texture and multiply by vertex color
    float4 texColor = myTexture.Sample(mySampler, input.uv);
    return texColor * float4(input.color, 1.0f);
}
```

### Task 5.3: Load Texture from File

Create a method to load a texture. For simplicity, use WIC (Windows Imaging Component):

```cpp
// In Dx11Device.h
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> LoadTexture(
    const wchar_t* filename
);
```

You'll need to:
1. Use WIC to load the image file (PNG, JPG, BMP, etc.)
2. Create a D3D11 texture from the image data
3. Create a shader resource view (SRV) for the texture

**Note**: This is the most complex part. See hints for implementation!

### Task 5.4: Create a Sampler

Samplers control how textures are filtered and addressed:

```cpp
// In Dx11Device.cpp
Microsoft::WRL::ComPtr<ID3D11SamplerState> CreateSampler()
{
    D3D11_SAMPLER_DESC samplerDesc{};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR; // Bilinear filtering
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    
    Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler;
    ThrowIfFailed(m_device->CreateSamplerState(&samplerDesc, &sampler),
                  "CreateSamplerState failed");
    return sampler;
}
```

### Task 5.5: Bind Texture and Sampler

Before drawing:

```cpp
// Bind texture to pixel shader slot 0
ID3D11ShaderResourceView* srvs[] = { textureSRV.Get() };
m_context->PSSetShaderResources(0, 1, srvs);

// Bind sampler to pixel shader slot 0
ID3D11SamplerState* samplers[] = { sampler.Get() };
m_context->PSSetSamplers(0, 1, samplers);
```

### Task 5.6: Test with an Image

Create or download a simple test image:
- Create a 256×256 pixel image in Paint or GIMP
- Draw something recognizable (smiley face, checkerboard, etc.)
- Save as PNG
- Place in your project folder (e.g., `assets/test.png`)
- Load and display it!

## Hints

<details>
<summary>Click to reveal hint for Task 5.3 (WIC Loading)</summary>

This is complex! Here's a simplified version:

```cpp
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Dx11Device::LoadTexture(
    const wchar_t* filename)
{
    // Create WIC factory
    ComPtr<IWICImagingFactory> wicFactory;
    ThrowIfFailed(CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&wicFactory)
    ), "WIC factory creation failed");
    
    // Create decoder
    ComPtr<IWICBitmapDecoder> decoder;
    ThrowIfFailed(wicFactory->CreateDecoderFromFilename(
        filename,
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnDemand,
        &decoder
    ), "CreateDecoderFromFilename failed");
    
    // Get first frame
    ComPtr<IWICBitmapFrameDecode> frame;
    ThrowIfFailed(decoder->GetFrame(0, &frame), "GetFrame failed");
    
    // Convert to RGBA
    ComPtr<IWICFormatConverter> converter;
    ThrowIfFailed(wicFactory->CreateFormatConverter(&converter), 
                  "CreateFormatConverter failed");
    ThrowIfFailed(converter->Initialize(
        frame.Get(),
        GUID_WICPixelFormat32bppRGBA,
        WICBitmapDitherTypeNone,
        nullptr,
        0.0,
        WICBitmapPaletteTypeCustom
    ), "Converter Initialize failed");
    
    // Get image dimensions
    UINT width, height;
    ThrowIfFailed(converter->GetSize(&width, &height), "GetSize failed");
    
    // Allocate pixel buffer
    std::vector<uint8_t> pixels(width * height * 4);
    ThrowIfFailed(converter->CopyPixels(
        nullptr,
        width * 4,
        (UINT)pixels.size(),
        pixels.data()
    ), "CopyPixels failed");
    
    // Create D3D11 texture
    D3D11_TEXTURE2D_DESC texDesc{};
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_IMMUTABLE;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    
    D3D11_SUBRESOURCE_DATA initData{};
    initData.pSysMem = pixels.data();
    initData.SysMemPitch = width * 4;
    
    ComPtr<ID3D11Texture2D> texture;
    ThrowIfFailed(m_device->CreateTexture2D(&texDesc, &initData, &texture),
                  "CreateTexture2D failed");
    
    // Create SRV
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = texDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    
    ComPtr<ID3D11ShaderResourceView> srv;
    ThrowIfFailed(m_device->CreateShaderResourceView(texture.Get(), &srvDesc, &srv),
                  "CreateShaderResourceView failed");
    
    return srv;
}
```

Don't forget to call `CoInitialize(nullptr)` in main before using WIC!

</details>

<details>
<summary>Click to reveal hint for simpler texture loading</summary>

If WIC is too complex, you can:
1. Use a library like STB Image (single header file)
2. Or create a simple procedural texture in code:

```cpp
// Create a 256×256 checkerboard texture
const int size = 256;
std::vector<uint32_t> pixels(size * size);
for (int y = 0; y < size; y++) {
    for (int x = 0; x < size; x++) {
        bool checker = ((x / 32) + (y / 32)) % 2;
        pixels[y * size + x] = checker ? 0xFFFFFFFF : 0xFF000000;
    }
}
// Then create texture from pixels array...
```

</details>

<details>
<summary>Click to reveal hint for Input Layout update</summary>

Don't forget to update your input layout:

```cpp
D3D11_INPUT_ELEMENT_DESC layout[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,       0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};
```

</details>

## Bonus Challenges

### Challenge 5.7: Alpha Blending
Enable alpha blending to support transparent textures:

```cpp
D3D11_BLEND_DESC blendDesc{};
blendDesc.RenderTarget[0].BlendEnable = TRUE;
blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

ComPtr<ID3D11BlendState> blendState;
m_device->CreateBlendState(&blendDesc, &blendState);
m_context->OMSetBlendState(blendState.Get(), nullptr, 0xFFFFFFFF);
```

### Challenge 5.8: Texture Atlas
Load a texture with multiple sprites and use different UV coordinates to display different parts.

### Challenge 5.9: Point Filtering
Try `D3D11_FILTER_MIN_MAG_MIP_POINT` for pixel-art style rendering.

## Common Issues and Solutions

### Issue: Texture appears black
- ✅ Check that file loaded successfully
- ✅ Verify SRV and sampler are bound before drawing
- ✅ Check shader is sampling texture correctly
- ✅ Ensure UV coordinates are in [0, 1] range

### Issue: Texture appears upside-down
- ✅ WIC loads images with origin at top-left
- ✅ DirectX UVs have (0, 0) at top-left too
- ✅ If upside-down, flip V coordinates: `v = 1.0 - v`

### Issue: Texture looks blurry
- ✅ Using linear filtering (expected)
- ✅ For pixel art, use point filtering
- ✅ Or create mipmaps for better quality

## Deep Dive: Texture Formats

### Common Formats
- `DXGI_FORMAT_R8G8B8A8_UNORM`: Standard 32-bit RGBA
- `DXGI_FORMAT_B8G8R8A8_UNORM`: BGRA (Windows default)
- `DXGI_FORMAT_BC1_UNORM`: DXT1 compression (no alpha)
- `DXGI_FORMAT_BC3_UNORM`: DXT5 compression (with alpha)

### Sampling Filters
- `POINT`: Nearest neighbor (pixelated, fast)
- `LINEAR`: Bilinear interpolation (smooth, slower)
- `ANISOTROPIC`: High quality, expensive

### Address Modes
- `WRAP`: Repeat texture (0-1-0-1-0-1...)
- `CLAMP`: Extend edge pixels
- `MIRROR`: Mirror repeat (0-1-1-0-0-1...)
- `BORDER`: Use border color outside [0, 1]

## What You Learned
✅ Loading images into textures  
✅ Creating shader resource views  
✅ Configuring samplers  
✅ Using texture coordinates  
✅ Sampling textures in shaders  
✅ Basic texture addressing and filtering  

## Next Steps
In [Exercise 6: Transformations](./06-transformations.md), you'll learn to move, rotate, and scale your textured sprites using matrices!
