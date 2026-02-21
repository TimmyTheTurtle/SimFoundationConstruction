# Exercise 8: Multiple Sprites

**Difficulty**: ⭐⭐⭐⭐ Advanced  
**Time**: 60-90 minutes

## Learning Objectives
- Render many objects efficiently
- Understand draw call optimization
- Implement instanced rendering
- Create a sprite batch system
- Manage sprite data structures

## Background

Drawing thousands of sprites one at a time is inefficient. Each draw call has overhead. We can optimize using:
1. **Batching**: Draw many sprites in one draw call
2. **Instancing**: Use hardware instancing to duplicate geometry
3. **Dynamic buffers**: Update sprite data each frame efficiently

**Performance Comparison**:
- 1000 sprites, individual draws: ~1000 draw calls
- 1000 sprites, batched: ~1 draw call
- Huge performance difference!

## Your Tasks

### Task 8.1: Sprite Structure

Create a data structure to manage multiple sprites:

```cpp
struct SpriteInstance {
    Vec2 position;
    Vec2 size;
    float rotation;
    Vec4 color;      // RGBA tint
    Vec4 texCoords;  // (u_min, v_min, u_max, v_max) for atlas support
};

struct SpriteBatch {
    std::vector<SpriteInstance> sprites;
    ComPtr<ID3D11Buffer> instanceBuffer;
    ComPtr<ID3D11ShaderResourceView> texture;
    size_t capacity;
    
    void Add(const SpriteInstance& sprite);
    void Clear();
    void Draw(Dx11Device& device);
};
```

### Task 8.2: Instance Buffer

Create a dynamic buffer for per-instance data:

```cpp
ComPtr<ID3D11Buffer> CreateInstanceBuffer(size_t maxInstances)
{
    D3D11_BUFFER_DESC bd{};
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = maxInstances * sizeof(SpriteInstance);
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    
    ComPtr<ID3D11Buffer> buffer;
    ThrowIfFailed(m_device->CreateBuffer(&bd, nullptr, &buffer),
                  "CreateInstanceBuffer failed");
    return buffer;
}
```

### Task 8.3: Update Input Layout for Instancing

Extend the input layout:

```cpp
D3D11_INPUT_ELEMENT_DESC layout[] = {
    // Per-vertex data (same for all instances)
    { "POSITION",  0, DXGI_FORMAT_R32G32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0,  8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    
    // Per-instance data (changes for each sprite)
    { "INST_POS",  0, DXGI_FORMAT_R32G32_FLOAT,    1,  0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
    { "INST_SIZE", 0, DXGI_FORMAT_R32G32_FLOAT,    1,  8, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
    { "INST_ROT",  0, DXGI_FORMAT_R32_FLOAT,       1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
    { "INST_COLOR",0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 20, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
    { "INST_UV",   0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 36, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
};
```

**Note**: Buffer slot 0 = vertex buffer, slot 1 = instance buffer

### Task 8.4: Instanced Vertex Shader

Update your shader to use per-instance data:

```hlsl
struct VSInput {
    float2 pos : POSITION;
    float2 uv : TEXCOORD;
    
    // Per-instance attributes
    float2 instPos : INST_POS;
    float2 instSize : INST_SIZE;
    float instRot : INST_ROT;
    float4 instColor : INST_COLOR;
    float4 instUV : INST_UV;
};

struct VSOutput {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};

VSOutput VSMain(VSInput input) {
    VSOutput output;
    
    // Build transformation matrix for this instance
    float c = cos(input.instRot);
    float s = sin(input.instRot);
    
    // Scale
    float2 scaled = input.pos * input.instSize;
    
    // Rotate
    float2 rotated;
    rotated.x = scaled.x * c - scaled.y * s;
    rotated.y = scaled.x * s + scaled.y * c;
    
    // Translate
    float2 worldPos = rotated + input.instPos;
    
    output.pos = float4(worldPos, 0.0f, 1.0f);
    
    // Map vertex UV [0,1] to instance UV range
    output.uv.x = lerp(input.instUV.x, input.instUV.z, input.uv.x);
    output.uv.y = lerp(input.instUV.y, input.instUV.w, input.uv.y);
    
    output.color = input.instColor;
    return output;
}

Texture2D myTexture : register(t0);
SamplerState mySampler : register(s0);

float4 PSMain(VSOutput input) : SV_TARGET {
    return myTexture.Sample(mySampler, input.uv) * input.color;
}
```

### Task 8.5: Draw Instanced

Implement the draw function:

```cpp
void SpriteBatch::Draw(Dx11Device& device)
{
    if (sprites.empty()) return;
    
    // Update instance buffer
    D3D11_MAPPED_SUBRESOURCE mapped;
    device.Context()->Map(instanceBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    memcpy(mapped.pData, sprites.data(), sprites.size() * sizeof(SpriteInstance));
    device.Context()->Unmap(instanceBuffer.Get(), 0);
    
    // Set vertex buffer (quad)
    UINT strides[] = { sizeof(Vertex), sizeof(SpriteInstance) };
    UINT offsets[] = { 0, 0 };
    ID3D11Buffer* buffers[] = { vertexBuffer.Get(), instanceBuffer.Get() };
    device.Context()->IASetVertexBuffers(0, 2, buffers, strides, offsets);
    
    // Set index buffer
    device.Context()->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
    
    // Draw all instances
    device.Context()->DrawIndexedInstanced(
        6,                    // 6 indices per quad
        sprites.size(),       // Number of instances
        0, 0, 0
    );
}
```

### Task 8.6: Create Many Sprites

Test with lots of sprites:

```cpp
SpriteBatch batch;
batch.capacity = 10000;
batch.instanceBuffer = device.CreateInstanceBuffer(10000);
batch.texture = device.LoadTexture(L"assets/sprite.png");

// Create 1000 random sprites
for (int i = 0; i < 1000; i++) {
    SpriteInstance sprite;
    sprite.position = { RandFloat(-1, 1), RandFloat(-1, 1) };
    sprite.size = { 0.05f, 0.05f };
    sprite.rotation = RandFloat(0, 6.28f);
    sprite.color = { RandFloat(0, 1), RandFloat(0, 1), RandFloat(0, 1), 1.0f };
    sprite.texCoords = { 0, 0, 1, 1 };  // Full texture
    
    batch.sprites.push_back(sprite);
}

// In render loop
batch.Draw(device);
```

## Hints

<details>
<summary>Click to reveal hint for Task 8.5</summary>

The key differences from regular drawing:
1. Use `DrawIndexedInstanced` instead of `DrawIndexed`
2. Bind TWO vertex buffers: geometry + instance data
3. Update instance buffer each frame with sprite transforms

```cpp
// Regular draw: 1 quad
DrawIndexed(6, 0, 0);

// Instanced draw: N quads
DrawIndexedInstanced(6, instanceCount, 0, 0, 0);
```

</details>

<details>
<summary>Click to reveal hint for optimization</summary>

**Don't update the entire buffer if not all sprites changed**:

```cpp
// Only update dirty sprites
if (batch.dirty) {
    // Update only the range that changed
    device.Context()->Map(buffer, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mapped);
    // Copy only changed portion...
}
```

**Sort sprites by texture** to minimize texture switches:
```cpp
std::sort(sprites.begin(), sprites.end(), 
    [](const Sprite& a, const Sprite& b) {
        return a.textureID < b.textureID;
    });
```

</details>

## Bonus Challenges

### Challenge 8.7: Sprite Culling
Don't draw sprites outside the viewport:
```cpp
bool IsInView(const SpriteInstance& sprite) {
    return sprite.position.x + sprite.size.x > -1.0f &&
           sprite.position.x - sprite.size.x <  1.0f &&
           sprite.position.y + sprite.size.y > -1.0f &&
           sprite.position.y - sprite.size.y <  1.0f;
}

// Only add visible sprites to batch
for (auto& sprite : allSprites) {
    if (IsInView(sprite)) {
        batch.Add(sprite);
    }
}
```

### Challenge 8.8: Texture Atlas
Use a texture atlas to draw different sprites in one batch:
- Create a texture with multiple sprites
- Use `instUV` to select which sprite to show
- All sprites can be drawn in one draw call!

### Challenge 8.9: Particle System
Create a particle system with 10,000+ particles:
- Update particle positions each frame
- Age and fade particles over time
- Add physics (gravity, wind)

### Challenge 8.10: Sprite Sorting
Implement proper sprite sorting for transparency:
- Sort by depth/layer
- Back-to-front for alpha blending
- Front-to-back for opaque objects

## Common Issues and Solutions

### Issue: Sprites don't appear
- ✅ Check instance buffer is bound to slot 1
- ✅ Verify input layout has both vertex and instance elements
- ✅ Ensure `D3D11_INPUT_PER_INSTANCE_DATA` flag is set
- ✅ Check `DrawIndexedInstanced` parameters

### Issue: All sprites look the same
- ✅ Per-instance data not varying
- ✅ Check vertex shader uses instance data
- ✅ Verify instance buffer update

### Issue: Performance worse than individual draws
- ✅ Make sure you're using `D3D11_MAP_WRITE_DISCARD`
- ✅ Don't create/destroy buffers each frame
- ✅ Batch size might be too small (overhead)

## Deep Dive: Batching Strategies

### Strategy 1: Dynamic Batching
- Update buffer every frame
- Good for frequently changing sprites
- Use `D3D11_USAGE_DYNAMIC` + `D3D11_MAP_WRITE_DISCARD`

### Strategy 2: Static Batching
- Create buffer once
- Good for static background elements
- Use `D3D11_USAGE_IMMUTABLE`

### Strategy 3: Hybrid
- Static buffer for background
- Dynamic buffer for moving sprites
- Multiple batches, sorted by update frequency

### Performance Numbers
On a modern GPU:
- Individual draws: ~50,000 draw calls/frame max
- Instanced draws: ~1,000,000 instances/frame easily
- Proper batching: 60 FPS with 100,000+ sprites

### Memory Layout
```
Vertex Buffer (4 vertices):  [v0, v1, v2, v3]
Instance Buffer (N sprites): [s0, s1, s2, ..., sN]
Index Buffer (6 indices):    [0,1,3, 1,2,3]

GPU draws:
  Instance 0: vertices [v0,v1,v2,v3] with data s0
  Instance 1: vertices [v0,v1,v2,v3] with data s1
  ...
```

## What You Learned
✅ Hardware instancing fundamentals  
✅ Efficient sprite batching  
✅ Dynamic buffer updates  
✅ Per-instance vertex attributes  
✅ DrawIndexedInstanced usage  
✅ Performance optimization techniques  

## Next Steps
In [Exercise 9: Sprite Animation](./09-sprite-animation.md), you'll add frame-based animation to your sprites!
