# Exercise 6: Transformations

**Difficulty**: ⭐⭐⭐⭐ Advanced  
**Time**: 45-60 minutes

## Learning Objectives
- Understand 2D transformation matrices
- Create and use constant buffers
- Implement translation, rotation, and scaling
- Combine transformations in the correct order
- Build a simple transform system

## Background

To move, rotate, or scale sprites, we use transformation matrices. Instead of modifying vertex positions directly, we pass a transformation matrix to the vertex shader.

**Transformation Types**:
- **Translation**: Move (x, y offset)
- **Rotation**: Rotate (angle in radians)
- **Scale**: Resize (sx, sy multipliers)

In 2D, we use 3×3 matrices (or 4×4 for consistency with 3D).

## Your Tasks

### Task 6.1: Create Constant Buffer Structure

Define a struct for transformation data:

```cpp
struct TransformData {
    float matrix[4][4];  // 4×4 matrix (16 floats)
};
```

**Note**: DirectX constant buffers must be 16-byte aligned. A 4×4 matrix (64 bytes) is naturally aligned.

### Task 6.2: Create Constant Buffer

Add a method to create constant buffers:

```cpp
// In Dx11Device.h
template<typename T>
Microsoft::WRL::ComPtr<ID3D11Buffer> CreateConstantBuffer()
{
    D3D11_BUFFER_DESC bd{};
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = sizeof(T);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    
    Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
    ThrowIfFailed(m_device->CreateBuffer(&bd, nullptr, &buffer),
                  "CreateConstantBuffer failed");
    return buffer;
}
```

### Task 6.3: Update Constant Buffer

Add a method to update constant buffer data:

```cpp
template<typename T>
void UpdateConstantBuffer(ID3D11Buffer* buffer, const T& data)
{
    D3D11_MAPPED_SUBRESOURCE mapped;
    ThrowIfFailed(m_context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped),
                  "Map failed");
    memcpy(mapped.pData, &data, sizeof(T));
    m_context->Unmap(buffer, 0);
}
```

### Task 6.4: Implement Matrix Functions

Create helper functions for 2D transformations:

```cpp
// Translation matrix
void MakeTranslation(float matrix[4][4], float x, float y)
{
    // Identity matrix
    matrix[0][0] = 1; matrix[0][1] = 0; matrix[0][2] = 0; matrix[0][3] = 0;
    matrix[1][0] = 0; matrix[1][1] = 1; matrix[1][2] = 0; matrix[1][3] = 0;
    matrix[2][0] = 0; matrix[2][1] = 0; matrix[2][2] = 1; matrix[2][3] = 0;
    matrix[3][0] = x; matrix[3][1] = y; matrix[3][2] = 0; matrix[3][3] = 1;
}

// Rotation matrix (angle in radians)
void MakeRotation(float matrix[4][4], float angle)
{
    float c = cosf(angle);
    float s = sinf(angle);
    
    matrix[0][0] = c; matrix[0][1] = s; matrix[0][2] = 0; matrix[0][3] = 0;
    matrix[1][0] =-s; matrix[1][1] = c; matrix[1][2] = 0; matrix[1][3] = 0;
    matrix[2][0] = 0; matrix[2][1] = 0; matrix[2][2] = 1; matrix[2][3] = 0;
    matrix[3][0] = 0; matrix[3][1] = 0; matrix[3][2] = 0; matrix[3][3] = 1;
}

// Scale matrix
void MakeScale(float matrix[4][4], float sx, float sy)
{
    matrix[0][0] = sx; matrix[0][1] = 0;  matrix[0][2] = 0; matrix[0][3] = 0;
    matrix[1][0] = 0;  matrix[1][1] = sy; matrix[1][2] = 0; matrix[1][3] = 0;
    matrix[2][0] = 0;  matrix[2][1] = 0;  matrix[2][2] = 1; matrix[2][3] = 0;
    matrix[3][0] = 0;  matrix[3][1] = 0;  matrix[3][2] = 0; matrix[3][3] = 1;
}

// Matrix multiplication: result = a * b
void MatrixMultiply(float result[4][4], const float a[4][4], const float b[4][4])
{
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result[i][j] = 0;
            for (int k = 0; k < 4; k++) {
                result[i][j] += a[i][k] * b[k][j];
            }
        }
    }
}
```

### Task 6.5: Update Vertex Shader

Modify your shader to use the transform:

```hlsl
cbuffer Transform : register(b0)
{
    float4x4 transform;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    
    // Apply transformation matrix
    float4 pos = float4(input.pos, 0.0f, 1.0f);
    output.pos = mul(pos, transform);
    
    output.color = input.color;
    output.uv = input.uv;
    return output;
}
```

### Task 6.6: Use Transformations

In your main loop:

```cpp
// Create transform
TransformData transform;
float angle = GetTickCount64() / 1000.0f; // Rotate over time
MakeRotation(transform.matrix, angle);

// Update constant buffer
dx.UpdateConstantBuffer(transformCB.Get(), transform);

// Bind constant buffer to vertex shader slot 0
ID3D11Buffer* cbs[] = { transformCB.Get() };
m_context->VSSetConstantBuffers(0, 1, cbs);

// Draw
dx.DrawQuad();
```

### Task 6.7: Combine Transformations

Create a sprite that rotates AND moves:

```cpp
float time = GetTickCount64() / 1000.0f;

// Create individual transforms
float trans[4][4], rot[4][4], scale[4][4];
MakeTranslation(trans, sinf(time) * 0.5f, 0.0f);  // Move left-right
MakeRotation(rot, time);                           // Rotate
MakeScale(scale, 0.5f, 0.5f);                     // Make smaller

// Combine: Scale, then Rotate, then Translate (SRT)
float temp[4][4];
MatrixMultiply(temp, scale, rot);
MatrixMultiply(transform.matrix, temp, trans);
```

**Important**: Transformation order matters! Usually: Scale → Rotate → Translate

## Hints

<details>
<summary>Click to reveal hint for matrix memory layout</summary>

DirectX uses **row-major** matrices in C++, but **column-major** in HLSL by default!

When you define:
```cpp
float matrix[4][4];
```

And multiply in HLSL:
```hlsl
output.pos = mul(pos, transform);
```

The shader automatically transposes. If you get unexpected results, try:
```hlsl
output.pos = mul(transform, pos);
```

Or use `row_major` keyword in HLSL:
```hlsl
cbuffer Transform : register(b0)
{
    row_major float4x4 transform;
};
```

</details>

<details>
<summary>Click to reveal hint for transformation order</summary>

**Matrix multiplication is NOT commutative**: A × B ≠ B × A

**Common order**: Scale → Rotate → Translate (SRT)
- Scale first (affects base size)
- Then rotate (around origin)
- Finally translate (move to position)

**Why this order?**
- If you translate first, rotation happens around wrong point
- If you scale after rotation, you might get unexpected skewing

</details>

<details>
<summary>Click to reveal hint for radians vs degrees</summary>

DirectX uses **radians**, not degrees!
- 90° = π/2 ≈ 1.5708 radians
- 180° = π ≈ 3.14159 radians
- 360° = 2π ≈ 6.28318 radians

Conversion:
```cpp
float radians = degrees * (3.14159f / 180.0f);
float degrees = radians * (180.0f / 3.14159f);
```

</details>

## Bonus Challenges

### Challenge 6.8: Orbit Motion
Make a small sprite orbit around a larger sprite:
- Parent sprite rotates slowly
- Child sprite position is offset from parent
- Child rotates around parent

### Challenge 6.9: Use DirectXMath
Replace manual matrix functions with DirectXMath library:
```cpp
#include <DirectXMath.h>
using namespace DirectX;

XMMATRIX trans = XMMatrixTranslation(x, y, 0);
XMMATRIX rot = XMMatrixRotationZ(angle);
XMMATRIX scale = XMMatrixScaling(sx, sy, 1);
XMMATRIX combined = scale * rot * trans;

// Store in constant buffer
XMStoreFloat4x4(&transform.matrix, XMMatrixTranspose(combined));
```

### Challenge 6.10: Pivot Point
Add support for rotating around a custom pivot point (not center).

## Common Issues and Solutions

### Issue: Sprite rotating around wrong point
- ✅ Rotation happens around origin (0, 0)
- ✅ To rotate around center: translate to origin, rotate, translate back
- ✅ Or adjust vertex positions to be centered at origin

### Issue: Matrix has no effect
- ✅ Check constant buffer is bound to correct slot
- ✅ Verify shader uses the matrix in `mul()`
- ✅ Ensure constant buffer is updated every frame

### Issue: Unexpected distortion
- ✅ Check transformation order (SRT usually correct)
- ✅ Verify matrix multiplication order
- ✅ Check for row-major vs column-major issues

## Deep Dive: Transformation Mathematics

### Homogeneous Coordinates
We use 4×4 matrices even for 2D because:
- Enables translation (not possible with 3×3 rotation/scale only)
- Allows perspective projection
- Consistent with 3D graphics

### Matrix Multiplication for Transforms
Multiplying position by matrix:
```
[x', y', z', w'] = [x, y, z, w] × Matrix
```

For 2D with identity Z and W=1:
```
[x', y', 0, 1] = [x, y, 0, 1] × Transform
```

### Transformation Pipeline
```
Local Space → World Space → View Space → Projection Space → Screen Space
```

For 2D:
- **Local Space**: Vertex positions in model
- **World Space**: After applying transform matrix
- **Projection Space**: Normalized device coordinates (NDC)

## What You Learned
✅ Creating and using constant buffers  
✅ 2D transformation matrices  
✅ Translation, rotation, and scaling  
✅ Combining transformations  
✅ Matrix multiplication  
✅ Row-major vs column-major layouts  

## Next Steps
In [Exercise 7: Mouse Interaction](./07-mouse-interaction.md), you'll make your sprites respond to mouse input!
