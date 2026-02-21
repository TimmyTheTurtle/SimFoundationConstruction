# Exercise 4: Drawing Rectangles

**Difficulty**: ⭐⭐⭐ Intermediate  
**Time**: 30-45 minutes

## Learning Objectives
- Understand why we need index buffers
- Create and use index buffers
- Draw quads (rectangles) efficiently
- Learn about triangle strips vs triangle lists

## Background

A rectangle (quad) needs 4 vertices, but to draw it with triangles, we need 6 vertices (2 triangles). Index buffers let us reuse vertices efficiently.

**Without indices**: 6 vertices (duplicate bottom-left and top-right)
```
Triangle 1: Top-left, Top-right, Bottom-left
Triangle 2: Top-right, Bottom-right, Bottom-left
```

**With indices**: 4 vertices + 6 indices
```
Vertices: [0]=Top-left, [1]=Top-right, [2]=Bottom-right, [3]=Bottom-left
Indices: [0, 1, 3, 1, 2, 3]
```

This saves memory and makes updates easier!

## Your Tasks

### Task 4.1: Define Quad Vertices

Create 4 vertices for a centered rectangle:

```cpp
Vertex quadVertices[] = {
    {-0.5f,  0.5f,  1.0f, 0.0f, 0.0f },  // Top-left (red)
    { 0.5f,  0.5f,  0.0f, 1.0f, 0.0f },  // Top-right (green)
    { 0.5f, -0.5f,  0.0f, 0.0f, 1.0f },  // Bottom-right (blue)
    {-0.5f, -0.5f,  1.0f, 1.0f, 0.0f }   // Bottom-left (yellow)
};
```

### Task 4.2: Define Indices

Create an index array that defines two triangles:

```cpp
uint16_t indices[] = {
    0, 1, 3,  // First triangle (top-left, top-right, bottom-left)
    1, 2, 3   // Second triangle (top-right, bottom-right, bottom-left)
};
```

**Important**: Maintain counter-clockwise winding for both triangles!

### Task 4.3: Create Index Buffer

Add a method to `Dx11Device`:

```cpp
// In Dx11Device.h
Microsoft::WRL::ComPtr<ID3D11Buffer> CreateIndexBuffer(
    const uint16_t* data,
    UINT count
);
```

Implement in `Dx11Device.cpp`:
- Use `D3D11_BIND_INDEX_BUFFER`
- Similar to vertex buffer but with different bind flags

### Task 4.4: Draw with Indices

Replace `Draw()` with `DrawIndexed()`:

```cpp
// Set index buffer
m_context->IASetIndexBuffer(
    indexBuffer.Get(), 
    DXGI_FORMAT_R16_UINT,  // 16-bit unsigned integers
    0
);

// Draw using indices
m_context->DrawIndexed(6, 0, 0); // 6 indices, start at index 0, vertex offset 0
```

### Task 4.5: Multiple Quads

Draw 3 rectangles at different positions:
- Modify the vertex shader to use a constant buffer for position offset
- Or create 3 separate vertex buffers with different positions
- Experiment with different approaches

## Hints

<details>
<summary>Click to reveal hint for Task 4.3</summary>

```cpp
Microsoft::WRL::ComPtr<ID3D11Buffer> Dx11Device::CreateIndexBuffer(
    const uint16_t* data, UINT count)
{
    D3D11_BUFFER_DESC bd{};
    bd.Usage = D3D11_USAGE_IMMUTABLE;
    bd.ByteWidth = count * sizeof(uint16_t);
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    
    D3D11_SUBRESOURCE_DATA initData{};
    initData.pSysMem = data;
    
    Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
    ThrowIfFailed(m_device->CreateBuffer(&bd, &initData, &buffer), 
                  "CreateIndexBuffer failed");
    return buffer;
}
```

</details>

<details>
<summary>Click to reveal hint for winding order visualization</summary>

Draw this on paper to verify winding order:
```
0----1
|  / |
| /  |
3----2
```

First triangle: 0 → 1 → 3 (counter-clockwise when viewed from front)
Second triangle: 1 → 2 → 3 (counter-clockwise when viewed from front)

If your quad appears inside-out or doesn't show, check winding order!

</details>

<details>
<summary>Click to reveal hint for Task 4.5</summary>

**Option 1**: Use a constant buffer for offset (better, more flexible)
```hlsl
cbuffer Transform : register(b0) {
    float2 offset;
};

VSOutput VSMain(VSInput input) {
    VSOutput output;
    output.pos = float4(input.pos + offset, 0.0f, 1.0f);
    output.color = input.color;
    return output;
}
```

**Option 2**: Bake positions into vertices (simpler for now)
```cpp
Vertex quad1[] = { /* centered at -0.5, 0 */ };
Vertex quad2[] = { /* centered at 0.0, 0 */ };
Vertex quad3[] = { /* centered at 0.5, 0 */ };
```

</details>

## Bonus Challenges

### Challenge 4.6: Texture Coordinates
Add texture coordinates to your vertex structure:
```cpp
struct Vertex {
    float x, y;
    float r, g, b;
    float u, v;  // Texture coordinates
};
```

Standard UV mapping for a quad:
```cpp
// Top-left: (0, 0), Top-right: (1, 0)
// Bottom-left: (0, 1), Bottom-right: (1, 1)
```

Update the input layout and shader accordingly. We'll use these in Exercise 5!

### Challenge 4.7: Triangle Strip
Try using `D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP` instead of `TRIANGLELIST`:
- Requires different vertex/index ordering
- More efficient for connected geometry
- Index array becomes: `[0, 1, 3, 2]`

## Common Issues and Solutions

### Issue: Only one triangle appears
- ✅ Check that you have 6 indices (not 3)
- ✅ Verify index buffer creation succeeded
- ✅ Confirm `DrawIndexed(6, ...)` not `DrawIndexed(3, ...)`

### Issue: Quad appears "inside out"
- ✅ Check winding order of both triangles
- ✅ You may have culling enabled (back-face culling)
- ✅ Try reversing index order

### Issue: Vertices in wrong positions
- ✅ Remember NDC: (-1, -1) is bottom-left, (1, 1) is top-right
- ✅ Y-axis: -1 is bottom, 1 is top (opposite of screen coordinates!)

## Deep Dive: Why Use Indices?

### Memory Savings
For a simple quad:
- **Without indices**: 6 vertices × 20 bytes = 120 bytes
- **With indices**: 4 vertices × 20 bytes + 6 indices × 2 bytes = 92 bytes
- **Savings**: 23%

For a 100×100 grid:
- **Without indices**: ~60,000 vertices
- **With indices**: ~10,000 vertices + ~60,000 indices
- **Savings**: Much more significant!

### Cache Efficiency
Modern GPUs cache transformed vertices. Reusing vertices via indices means:
- Vertex shader runs fewer times
- Better cache hit rate
- Significant performance improvement for complex meshes

## Solution Notes

### Index Buffer Format
DirectX supports:
- `DXGI_FORMAT_R16_UINT`: 16-bit indices (0-65,535) - most common for 2D
- `DXGI_FORMAT_R32_UINT`: 32-bit indices (0-4 billion+) - for huge meshes

Use 16-bit unless you have more than 65,536 unique vertices.

### Primitive Topology
- `D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST`: Each 3 indices = 1 triangle
- `D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP`: Shared edges, more efficient
- `D3D11_PRIMITIVE_TOPOLOGY_LINELIST`: For debug wireframe

### DrawIndexed Parameters
```cpp
DrawIndexed(
    UINT IndexCount,           // Number of indices to use
    UINT StartIndexLocation,   // First index to read from index buffer
    INT BaseVertexLocation     // Added to each index before lookup
);
```

## What You Learned
✅ Creating and using index buffers  
✅ Drawing quads efficiently  
✅ Understanding triangle winding order  
✅ Memory optimization with indexed geometry  
✅ DrawIndexed vs Draw  

## Next Steps
In [Exercise 5: Textures](./05-textures.md), you'll learn to load images and apply them to your quads to create textured sprites!
