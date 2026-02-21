# Exercise 3: Your First Triangle

**Difficulty**: ⭐⭐⭐ Intermediate  
**Time**: 45-60 minutes

## Learning Objectives
- Create and use vertex buffers
- Write basic HLSL shaders (Vertex and Pixel shaders)
- Understand the graphics pipeline basics
- Set up the Input Assembler stage
- Draw your first geometric primitive

## Background

So far we've only cleared the screen. To draw actual shapes, we need:
1. **Vertex Data**: The points that define our shape
2. **Vertex Buffer**: GPU memory to store those points
3. **Shaders**: Programs that run on the GPU to process vertices and pixels
4. **Pipeline State**: Configuration telling DirectX how to draw

A triangle is the fundamental primitive in 3D graphics. Even complex models are made of triangles!

## The Graphics Pipeline (Simplified)

```
Vertex Data → Vertex Shader → Rasterization → Pixel Shader → Output
```

1. **Vertex Shader**: Transforms vertex positions
2. **Rasterization**: Determines which pixels the triangle covers
3. **Pixel Shader**: Determines the color of each pixel

## Your Tasks

### Task 3.1: Define Triangle Vertices

Create a struct to hold vertex data and define three vertices:

```cpp
struct Vertex {
    float x, y;    // Position in normalized device coordinates
    float r, g, b; // Color
};

// Define triangle vertices (in main.cpp or a new file)
Vertex triangleVertices[] = {
    { 0.0f,  0.5f,  1.0f, 0.0f, 0.0f },  // Top (red)
    { 0.5f, -0.5f,  0.0f, 1.0f, 0.0f },  // Bottom-right (green)
    {-0.5f, -0.5f,  0.0f, 0.0f, 1.0f }   // Bottom-left (blue)
};
```

**Note**: Normalized Device Coordinates (NDC) range from -1 to 1, where (0, 0) is center, (1, 1) is top-right.

### Task 3.2: Create Vertex Buffer

Add a method to `Dx11Device` to create a vertex buffer:

```cpp
// In Dx11Device.h - add this method declaration
Microsoft::WRL::ComPtr<ID3D11Buffer> CreateVertexBuffer(
    const void* data, 
    UINT sizeInBytes
);
```

Implement it in `Dx11Device.cpp`:
- Use `D3D11_BUFFER_DESC` with `D3D11_USAGE_IMMUTABLE`
- Set `BindFlags` to `D3D11_BIND_VERTEX_BUFFER`
- Use `D3D11_SUBRESOURCE_DATA` to provide initial data
- Call `m_device->CreateBuffer()`

### Task 3.3: Write Shaders

Create a new file `shaders/simple.hlsl` with vertex and pixel shaders:

```hlsl
// Vertex Shader
struct VSInput {
    float2 pos : POSITION;
    float3 color : COLOR;
};

struct VSOutput {
    float4 pos : SV_POSITION;
    float3 color : COLOR;
};

VSOutput VSMain(VSInput input) {
    VSOutput output;
    output.pos = float4(input.pos, 0.0f, 1.0f);
    output.color = input.color;
    return output;
}

// Pixel Shader
float4 PSMain(VSOutput input) : SV_TARGET {
    return float4(input.color, 1.0f);
}
```

### Task 3.4: Compile Shaders

Add a method to compile shaders from file:
- Use `D3DCompileFromFile()` (need to link `d3dcompiler.lib`)
- Create vertex shader with `CreateVertexShader()`
- Create pixel shader with `CreatePixelShader()`

### Task 3.5: Create Input Layout

Define how vertex data maps to shader inputs:

```cpp
D3D11_INPUT_ELEMENT_DESC layout[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  8, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};
```

Call `m_device->CreateInputLayout()` with the compiled vertex shader bytecode.

### Task 3.6: Draw the Triangle

In your main loop, after `Clear()`:

```cpp
// Set the vertex buffer
UINT stride = sizeof(Vertex);
UINT offset = 0;
m_context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);

// Set primitive topology
m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

// Set input layout
m_context->IASetInputLayout(inputLayout.Get());

// Set shaders
m_context->VSSetShader(vertexShader.Get(), nullptr, 0);
m_context->PSSetShader(pixelShader.Get(), nullptr, 0);

// Draw
m_context->Draw(3, 0); // 3 vertices, starting at vertex 0
```

## Hints

<details>
<summary>Click to reveal hint for Task 3.2</summary>

```cpp
Microsoft::WRL::ComPtr<ID3D11Buffer> Dx11Device::CreateVertexBuffer(
    const void* data, UINT sizeInBytes)
{
    D3D11_BUFFER_DESC bd{};
    bd.Usage = D3D11_USAGE_IMMUTABLE;
    bd.ByteWidth = sizeInBytes;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    
    D3D11_SUBRESOURCE_DATA initData{};
    initData.pSysMem = data;
    
    Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
    ThrowIfFailed(m_device->CreateBuffer(&bd, &initData, &buffer), 
                  "CreateVertexBuffer failed");
    return buffer;
}
```

</details>

<details>
<summary>Click to reveal hint for Task 3.4</summary>

```cpp
ComPtr<ID3DBlob> vsBlob;
ComPtr<ID3DBlob> errorBlob;

HRESULT hr = D3DCompileFromFile(
    L"shaders/simple.hlsl",
    nullptr,
    D3D_COMPILE_STANDARD_FILE_INCLUDE,
    "VSMain",
    "vs_5_0",
    D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
    0,
    &vsBlob,
    &errorBlob
);

if (FAILED(hr)) {
    if (errorBlob) {
        OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    }
    throw std::runtime_error("Shader compilation failed");
}

m_device->CreateVertexShader(
    vsBlob->GetBufferPointer(),
    vsBlob->GetBufferSize(),
    nullptr,
    &vertexShader
);
```

</details>

<details>
<summary>Click to reveal hint for complete workflow</summary>

The typical order in your main.cpp:

```cpp
// 1. Setup (before main loop)
Vertex vertices[] = { /* ... */ };
auto vb = dx.CreateVertexBuffer(vertices, sizeof(vertices));
auto [vs, ps, inputLayout] = dx.CompileAndCreateShaders(L"shaders/simple.hlsl");

// 2. Render loop
while (window.PumpMessages()) {
    dx.Clear(0.1f, 0.1f, 0.1f, 1.0f);
    
    // Set pipeline state
    dx.SetVertexBuffer(vb, sizeof(Vertex));
    dx.SetShaders(vs, ps, inputLayout);
    
    // Draw
    dx.DrawTriangle();
    
    dx.Present(true);
}
```

Consider creating helper methods to encapsulate common operations!

</details>

## Common Issues and Solutions

### Issue: Nothing appears on screen
- ✅ Check that vertices are in NDC range (-1 to 1)
- ✅ Verify winding order (counter-clockwise by default)
- ✅ Make sure Clear color is different from triangle color
- ✅ Check shader compilation succeeded
- ✅ Verify all pipeline state is set before Draw()

### Issue: Shader compilation fails
- ✅ Check the shader file path is correct
- ✅ Look at the error message in `errorBlob`
- ✅ Ensure entry points "VSMain" and "PSMain" match
- ✅ Verify HLSL syntax (semicolons, types)

### Issue: Triangle is wrong color
- ✅ Check vertex color values (0.0 to 1.0 range)
- ✅ Verify input layout matches vertex structure
- ✅ Check semantic names match between VS input and layout

## Solution Notes

### Normalized Device Coordinates (NDC)
- X: -1 (left) to 1 (right)
- Y: -1 (bottom) to 1 (top) 
- Z: 0 (near) to 1 (far) - we're using 0 for 2D
- W: 1 for 2D (used for perspective division in 3D)

### Vertex Winding Order
DirectX uses counter-clockwise winding by default for front-facing triangles:
```
    1 (top)
   / \
  /   \
 3-----2
```
Vertices should be: 1 → 2 → 3 (counter-clockwise)

### Why IMMUTABLE for Vertex Buffer?
- `D3D11_USAGE_IMMUTABLE`: Data never changes - best performance
- Later we'll use `D3D11_USAGE_DYNAMIC` for animated vertices

## What You Learned
✅ Creating vertex buffers  
✅ Writing HLSL shaders  
✅ Compiling shaders at runtime  
✅ Setting up the input layout  
✅ The basic rendering pipeline  
✅ Drawing primitives with Draw()  

## Next Steps
In [Exercise 4: Drawing Rectangles](./04-rectangles.md), you'll learn about index buffers to draw quads more efficiently!
