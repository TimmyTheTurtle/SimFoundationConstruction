# Exercise 10: 2D Camera

**Difficulty**: ⭐⭐⭐⭐ Advanced  
**Time**: 45-60 minutes

## Learning Objectives
- Implement a 2D camera system
- Understand view and projection matrices
- Add camera pan and zoom
- Follow targets smoothly
- Handle screen-to-world coordinate conversion

## Background

A camera transforms world coordinates to screen coordinates. In 2D:
- **World Space**: The actual positions of your game objects
- **View Space**: After applying camera transform
- **Screen Space**: Final pixel positions

The camera lets you move around large worlds without moving all objects!

## Your Tasks

### Task 10.1: Camera Structure

Create a camera class:

```cpp
class Camera2D {
public:
    Vec2 position;      // Camera center in world space
    float zoom;         // Zoom level (1.0 = normal, 2.0 = 2x zoomed in)
    float aspectRatio;  // Width / height
    
    Camera2D(float aspect) 
        : position{0, 0}, zoom(1.0f), aspectRatio(aspect) {}
    
    // Get view-projection matrix
    void GetMatrix(float outMatrix[4][4]) const;
    
    // Convert screen to world coordinates
    Vec2 ScreenToWorld(Vec2 screenPos, int screenWidth, int screenHeight) const;
    
    // Get visible bounds in world space
    struct Bounds {
        float left, right, top, bottom;
    };
    Bounds GetBounds() const;
};
```

### Task 10.2: Camera Matrix

Implement the camera transformation matrix:

```cpp
void Camera2D::GetMatrix(float outMatrix[4][4]) const
{
    // Create view matrix (inverse of camera transform)
    // 1. Scale by zoom
    // 2. Account for aspect ratio
    // 3. Translate by -position
    
    float scaleX = zoom;
    float scaleY = zoom;
    
    // Adjust for aspect ratio so things aren't stretched
    if (aspectRatio > 1.0f) {
        scaleX /= aspectRatio;  // Wider than tall
    } else {
        scaleY *= aspectRatio;  // Taller than wide
    }
    
    // Build matrix: scale, then translate
    outMatrix[0][0] = scaleX; outMatrix[0][1] = 0;      outMatrix[0][2] = 0; outMatrix[0][3] = 0;
    outMatrix[1][0] = 0;      outMatrix[1][1] = scaleY; outMatrix[1][2] = 0; outMatrix[1][3] = 0;
    outMatrix[2][0] = 0;      outMatrix[2][1] = 0;      outMatrix[2][2] = 1; outMatrix[2][3] = 0;
    outMatrix[3][0] = -position.x * scaleX; 
    outMatrix[3][1] = -position.y * scaleY; 
    outMatrix[3][2] = 0; 
    outMatrix[3][3] = 1;
}
```

### Task 10.3: Update Shaders

Modify your vertex shader to use camera matrix:

```hlsl
cbuffer Camera : register(b0)
{
    float4x4 viewProjection;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    
    // Apply instance transform (position, rotation, scale)
    float c = cos(input.instRot);
    float s = sin(input.instRot);
    float2 scaled = input.pos * input.instSize;
    float2 rotated = float2(
        scaled.x * c - scaled.y * s,
        scaled.x * s + scaled.y * c
    );
    float2 worldPos = rotated + input.instPos;
    
    // Apply camera transform
    float4 pos = float4(worldPos, 0.0f, 1.0f);
    output.pos = mul(pos, viewProjection);
    
    output.uv = /* ... */;
    output.color = input.instColor;
    return output;
}
```

### Task 10.4: Screen to World Conversion

Implement coordinate conversion for mouse input:

```cpp
Vec2 Camera2D::ScreenToWorld(Vec2 screenPos, int screenWidth, int screenHeight) const
{
    // Screen to NDC
    Vec2 ndc;
    ndc.x = (screenPos.x / screenWidth) * 2.0f - 1.0f;
    ndc.y = 1.0f - (screenPos.y / screenHeight) * 2.0f;
    
    // NDC to world (inverse of camera transform)
    Vec2 world;
    
    float scaleX = zoom;
    float scaleY = zoom;
    if (aspectRatio > 1.0f) {
        scaleX /= aspectRatio;
    } else {
        scaleY *= aspectRatio;
    }
    
    world.x = ndc.x / scaleX + position.x;
    world.y = ndc.y / scaleY + position.y;
    
    return world;
}
```

### Task 10.5: Camera Controls

Add keyboard controls for camera movement:

```cpp
// In main loop
const float moveSpeed = 0.05f / camera.zoom;  // Move slower when zoomed in
const float zoomSpeed = 0.1f;

// Pan camera with arrow keys or WASD
if (GetKeyState('W') & 0x8000) camera.position.y += moveSpeed;
if (GetKeyState('S') & 0x8000) camera.position.y -= moveSpeed;
if (GetKeyState('A') & 0x8000) camera.position.x -= moveSpeed;
if (GetKeyState('D') & 0x8000) camera.position.x += moveSpeed;

// Zoom with Q/E or mouse wheel
if (GetKeyState('Q') & 0x8000) camera.zoom *= (1.0f + zoomSpeed);
if (GetKeyState('E') & 0x8000) camera.zoom *= (1.0f - zoomSpeed);

// Clamp zoom
camera.zoom = std::max(0.1f, std::min(camera.zoom, 10.0f));
```

### Task 10.6: Mouse Wheel Zoom

Add mouse wheel support in `Win32Window`:

```cpp
// In Win32Window.h
void SetOnMouseWheel(std::function<void(int)> cb) { m_onMouseWheel = std::move(cb); }

// In WndProc
case WM_MOUSEWHEEL:
    if (self && self->m_onMouseWheel) {
        int delta = GET_WHEEL_DELTA_WPARAM(wparam);
        self->m_onMouseWheel(delta);
    }
    return 0;

// In main.cpp
window.SetOnMouseWheel([&](int delta) {
    float zoomFactor = delta > 0 ? 1.1f : 0.9f;
    camera.zoom *= zoomFactor;
    camera.zoom = std::max(0.1f, std::min(camera.zoom, 10.0f));
});
```

### Task 10.7: Camera Follow

Make camera smoothly follow a target:

```cpp
void Camera2D::Follow(Vec2 target, float smoothness, float deltaTime)
{
    // Lerp towards target
    float t = 1.0f - pow(smoothness, deltaTime);
    position.x += (target.x - position.x) * t;
    position.y += (target.y - position.y) * t;
}

// Usage in main loop
Vec2 playerPos = /* player position */;
camera.Follow(playerPos, 0.01f, deltaTime);
```

## Hints

<details>
<summary>Click to reveal hint for aspect ratio</summary>

Aspect ratio prevents stretching:
```
800×600 window: aspectRatio = 800/600 = 1.333

Without aspect correction:
- Circle becomes oval
- Square becomes rectangle

With aspect correction:
- Shapes maintain proportions
- May see more horizontally or vertically
```

Update aspect ratio on window resize:
```cpp
window.SetOnResize([&](int w, int h) {
    camera.aspectRatio = w / (float)h;
    // Also resize swap chain...
});
```

</details>

<details>
<summary>Click to reveal hint for zoom to mouse</summary>

To zoom towards mouse cursor (not camera center):

```cpp
window.SetOnMouseWheel([&](int delta) {
    // Get mouse position in world space BEFORE zoom
    Vec2 mouseWorld = camera.ScreenToWorld(mouseScreenPos, width, height);
    
    // Apply zoom
    float zoomFactor = delta > 0 ? 1.1f : 0.9f;
    camera.zoom *= zoomFactor;
    
    // Get mouse position in world space AFTER zoom
    Vec2 mouseWorldNew = camera.ScreenToWorld(mouseScreenPos, width, height);
    
    // Adjust camera to keep mouse over same world position
    camera.position.x += mouseWorld.x - mouseWorldNew.x;
    camera.position.y += mouseWorld.y - mouseWorldNew.y;
});
```

</details>

<details>
<summary>Click to reveal hint for camera bounds</summary>

```cpp
Camera2D::Bounds Camera2D::GetBounds() const
{
    Bounds bounds;
    
    float scaleX = zoom;
    float scaleY = zoom;
    if (aspectRatio > 1.0f) {
        scaleX /= aspectRatio;
    } else {
        scaleY *= aspectRatio;
    }
    
    float halfWidth = 1.0f / scaleX;
    float halfHeight = 1.0f / scaleY;
    
    bounds.left = position.x - halfWidth;
    bounds.right = position.x + halfWidth;
    bounds.bottom = position.y - halfHeight;
    bounds.top = position.y + halfHeight;
    
    return bounds;
}
```

</details>

## Bonus Challenges

### Challenge 10.8: Camera Bounds
Prevent camera from going outside world bounds:

```cpp
void Camera2D::ClampToBounds(float worldLeft, float worldRight, 
                              float worldBottom, float worldTop)
{
    Bounds visible = GetBounds();
    
    if (visible.right - visible.left >= worldRight - worldLeft) {
        // World fits in view, center it
        position.x = (worldLeft + worldRight) * 0.5f;
    } else {
        if (visible.left < worldLeft) position.x += worldLeft - visible.left;
        if (visible.right > worldRight) position.x -= visible.right - worldRight;
    }
    
    // Same for Y...
}
```

### Challenge 10.9: Camera Shake
Add screen shake effect:

```cpp
struct CameraShake {
    float intensity;
    float duration;
    float elapsed;
    
    Vec2 GetOffset() {
        if (elapsed >= duration) return {0, 0};
        
        float t = elapsed / duration;
        float strength = intensity * (1.0f - t);
        
        return {
            (rand() / (float)RAND_MAX - 0.5f) * strength,
            (rand() / (float)RAND_MAX - 0.5f) * strength
        };
    }
};

// Apply shake offset to camera position before rendering
```

### Challenge 10.10: Smooth Zoom
Smoothly interpolate zoom instead of instant changes:

```cpp
class Camera2D {
    float targetZoom;
    
    void SetTargetZoom(float zoom) {
        targetZoom = std::max(0.1f, std::min(zoom, 10.0f));
    }
    
    void Update(float deltaTime) {
        // Smooth zoom
        float t = 1.0f - pow(0.001f, deltaTime);
        zoom += (targetZoom - zoom) * t;
    }
};
```

## Common Issues and Solutions

### Issue: Everything appears stretched
- ✅ Check aspect ratio calculation
- ✅ Update aspect ratio on window resize
- ✅ Ensure aspectRatio = width / height (not height / width)

### Issue: Camera doesn't follow properly
- ✅ Update camera position before rendering
- ✅ Use deltaTime for smooth following
- ✅ Check smoothness factor (smaller = slower)

### Issue: Mouse coordinates wrong after zoom
- ✅ Use ScreenToWorld conversion
- ✅ Update conversion to account for zoom and position
- ✅ Test at different zoom levels

### Issue: Matrix not working
- ✅ Check row-major vs column-major
- ✅ Verify matrix multiplication order in shader
- ✅ Ensure constant buffer is updated each frame

## Deep Dive: Camera Mathematics

### View Matrix
Transforms world space to view (camera) space:
```
View = Translate(-cameraPos) * Scale(zoom)
```

### Projection Matrix
For 2D, we typically use orthographic projection:
```
Orthographic: Maps a rectangular region to NDC [-1, 1]
```

Combined as view-projection matrix:
```
VP = View * Projection
```

### Frustum Culling
Only render what's visible:
```cpp
bool IsVisible(Vec2 spritePos, Vec2 spriteSize) {
    auto bounds = camera.GetBounds();
    
    return spritePos.x + spriteSize.x > bounds.left &&
           spritePos.x - spriteSize.x < bounds.right &&
           spritePos.y + spriteSize.y > bounds.bottom &&
           spritePos.y - spriteSize.y < bounds.top;
}
```

## What You Learned
✅ 2D camera implementation  
✅ View and projection matrices  
✅ Camera pan and zoom  
✅ Screen-to-world coordinate conversion  
✅ Smooth camera following  
✅ Aspect ratio handling  

## Next Steps
You've completed all the exercises! Now tackle the [Final Project: 2D Space Shooter](./final-project.md) to combine everything you've learned!
