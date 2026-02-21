# Exercise 7: Mouse Interaction

**Difficulty**: ⭐⭐⭐ Intermediate  
**Time**: 30-45 minutes

## Learning Objectives
- Capture mouse input in Win32
- Convert screen coordinates to world coordinates
- Implement draggable sprites
- Handle click detection and hit testing
- Create interactive UI elements

## Background

Making graphics interactive is crucial for games and applications. Mouse input lets users click, drag, and interact with your sprites.

**Coordinate Systems**:
- **Screen Space**: Pixels (0, 0) at top-left, (width, height) at bottom-right
- **NDC Space**: (-1, -1) at bottom-left, (1, 1) at top-right
- We need to convert between them!

## Your Tasks

### Task 7.1: Capture Mouse Events

Modify `Win32Window` to capture mouse input:

```cpp
// In Win32Window.h
void SetOnMouseMove(std::function<void(int, int)> cb) { m_onMouseMove = std::move(cb); }
void SetOnMouseDown(std::function<void(int, int, int)> cb) { m_onMouseDown = std::move(cb); }
void SetOnMouseUp(std::function<void(int, int, int)> cb) { m_onMouseUp = std::move(cb); }

private:
    std::function<void(int, int)> m_onMouseMove;
    std::function<void(int, int, int)> m_onMouseDown;  // x, y, button
    std::function<void(int, int, int)> m_onMouseUp;
```

In `Win32Window::WndProc`, add cases:

```cpp
case WM_MOUSEMOVE:
    if (self && self->m_onMouseMove) {
        int x = LOWORD(lparam);
        int y = HIWORD(lparam);
        self->m_onMouseMove(x, y);
    }
    return 0;

case WM_LBUTTONDOWN:
    if (self && self->m_onMouseDown) {
        int x = LOWORD(lparam);
        int y = HIWORD(lparam);
        self->m_onMouseDown(x, y, 0); // 0 = left button
    }
    return 0;

case WM_LBUTTONUP:
    if (self && self->m_onMouseUp) {
        int x = LOWORD(lparam);
        int y = HIWORD(lparam);
        self->m_onMouseUp(x, y, 0);
    }
    return 0;
```

### Task 7.2: Convert Screen to NDC

Create helper functions:

```cpp
struct Vec2 {
    float x, y;
};

Vec2 ScreenToNDC(int screenX, int screenY, int screenWidth, int screenHeight)
{
    Vec2 ndc;
    ndc.x = (screenX / (float)screenWidth) * 2.0f - 1.0f;
    ndc.y = 1.0f - (screenY / (float)screenHeight) * 2.0f;  // Flip Y
    return ndc;
}
```

**Note**: Screen Y goes down (0 at top), NDC Y goes up (-1 at bottom).

### Task 7.3: Hit Testing

Check if mouse click is inside a sprite:

```cpp
struct Sprite {
    Vec2 position;
    Vec2 size;
    
    bool Contains(Vec2 point) const {
        float left = position.x - size.x * 0.5f;
        float right = position.x + size.x * 0.5f;
        float bottom = position.y - size.y * 0.5f;
        float top = position.y + size.y * 0.5f;
        
        return point.x >= left && point.x <= right &&
               point.y >= bottom && point.y <= top;
    }
};
```

### Task 7.4: Implement Click-to-Move

Make a sprite move to where you click:

```cpp
// In main.cpp
Vec2 spritePos = {0.0f, 0.0f};
Vec2 targetPos = spritePos;

window.SetOnMouseDown([&](int x, int y, int button) {
    targetPos = ScreenToNDC(x, y, 800, 600);
});

// In main loop
Vec2 dir = {targetPos.x - spritePos.x, targetPos.y - spritePos.y};
float distance = sqrtf(dir.x * dir.x + dir.y * dir.y);

if (distance > 0.01f) {
    float speed = 0.02f;
    spritePos.x += dir.x / distance * speed;
    spritePos.y += dir.y / distance * speed;
}

// Use spritePos in transformation matrix
```

### Task 7.5: Implement Drag-and-Drop

Make a sprite draggable with the mouse:

```cpp
bool isDragging = false;
Vec2 dragOffset = {0, 0};

window.SetOnMouseDown([&](int x, int y, int button) {
    Vec2 mouseNDC = ScreenToNDC(x, y, 800, 600);
    if (sprite.Contains(mouseNDC)) {
        isDragging = true;
        dragOffset.x = mouseNDC.x - sprite.position.x;
        dragOffset.y = mouseNDC.y - sprite.position.y;
    }
});

window.SetOnMouseMove([&](int x, int y) {
    if (isDragging) {
        Vec2 mouseNDC = ScreenToNDC(x, y, 800, 600);
        sprite.position.x = mouseNDC.x - dragOffset.x;
        sprite.position.y = mouseNDC.y - dragOffset.y;
    }
});

window.SetOnMouseUp([&](int x, int y, int button) {
    isDragging = false;
});
```

### Task 7.6: Visual Feedback

Add hover effects:
- Change sprite color when mouse is over it
- Scale up when hovering
- Change cursor icon

```cpp
bool isHovered = false;

window.SetOnMouseMove([&](int x, int y) {
    Vec2 mouseNDC = ScreenToNDC(x, y, 800, 600);
    isHovered = sprite.Contains(mouseNDC);
    
    // Change cursor
    SetCursor(LoadCursor(nullptr, isHovered ? IDC_HAND : IDC_ARROW));
});

// In rendering code
float scale = isHovered ? 1.1f : 1.0f;  // Slightly bigger when hovered
```

## Hints

<details>
<summary>Click to reveal hint for coordinate conversion</summary>

Screen coordinates:
```
(0,0)──────────(width, 0)
  │              │
  │    Screen    │
  │              │
(0,height)──────(width, height)
```

NDC coordinates:
```
(-1,1)─────────(1, 1)
  │              │
  │     NDC      │
  │              │
(-1,-1)────────(1, -1)
```

Conversion formulas:
```cpp
ndc.x = (screen.x / width) * 2 - 1;
ndc.y = 1 - (screen.y / height) * 2;  // Flip Y!
```

</details>

<details>
<summary>Click to reveal hint for smooth movement</summary>

For smoother movement, use linear interpolation (lerp):

```cpp
float t = 0.1f;  // Interpolation factor (0-1)
spritePos.x += (targetPos.x - spritePos.x) * t;
spritePos.y += (targetPos.y - spritePos.y) * t;
```

This creates a smooth "follow" effect!

</details>

<details>
<summary>Click to reveal hint for multiple draggable sprites</summary>

Use a vector of sprites and find which one was clicked:

```cpp
std::vector<Sprite> sprites;
Sprite* draggedSprite = nullptr;

window.SetOnMouseDown([&](int x, int y, int button) {
    Vec2 mouseNDC = ScreenToNDC(x, y, 800, 600);
    
    // Check from back to front (top to bottom)
    for (auto it = sprites.rbegin(); it != sprites.rend(); ++it) {
        if (it->Contains(mouseNDC)) {
            draggedSprite = &(*it);
            // Calculate offset...
            break;
        }
    }
});
```

</details>

## Bonus Challenges

### Challenge 7.7: Right-Click Menu
Detect right mouse button and show different behavior:
```cpp
case WM_RBUTTONDOWN:
    // Handle right-click
```

### Challenge 7.8: Click-Through Detection
For sprites with transparent pixels, only detect clicks on opaque pixels:
- Sample the texture at UV coordinates
- Check alpha value
- Only count as hit if alpha > threshold

### Challenge 7.9: Selection Rectangle
Click and drag to draw a selection rectangle, select all sprites inside it.

### Challenge 7.10: Double-Click Detection
Track click timing to detect double-clicks:
```cpp
static DWORD lastClickTime = 0;
DWORD currentTime = GetTickCount();
if (currentTime - lastClickTime < 500) {
    // Double-click!
}
lastClickTime = currentTime;
```

## Common Issues and Solutions

### Issue: Coordinates seem inverted
- ✅ Screen Y increases downward, NDC Y increases upward
- ✅ Remember to flip Y: `1.0f - (y / height) * 2.0f`

### Issue: Click detection off by a bit
- ✅ Check window client area vs window total size
- ✅ Use client coordinates, not screen coordinates
- ✅ Account for window borders and title bar

### Issue: Dragging feels "sticky"
- ✅ Calculate and store drag offset when clicking
- ✅ Subtract offset during drag: `sprite.pos = mouse - offset`

### Issue: Sprite jumps when starting drag
- ✅ You're not using drag offset correctly
- ✅ Offset should be: `mouse position - sprite position` at click time

## Deep Dive: Input Handling Best Practices

### Event-Driven vs Polling
- **Event-driven**: Handle WM_MOUSE* messages (what we did)
- **Polling**: Call `GetCursorPos()` each frame
- Event-driven is more responsive and efficient

### Mouse Capture
For dragging outside window:
```cpp
SetCapture(hwnd);  // In WM_LBUTTONDOWN
ReleaseCapture();  // In WM_LBUTTONUP
```

### Z-Order for Overlapping Sprites
Render order determines visual layering:
- Render back-to-front for transparency
- Click-test front-to-back for top sprite

### Input State Management
Consider creating an `InputManager` class:
```cpp
class InputManager {
    bool m_mouseButtons[3];
    Vec2 m_mousePos;
    Vec2 m_mouseDelta;
public:
    void Update();
    bool IsMouseDown(int button) const;
    Vec2 GetMousePos() const;
};
```

## What You Learned
✅ Capturing mouse events in Win32  
✅ Converting screen to NDC coordinates  
✅ Hit testing sprites  
✅ Implementing click-to-move  
✅ Drag-and-drop interaction  
✅ Visual feedback for interactivity  

## Next Steps
In [Exercise 8: Multiple Sprites](./08-multiple-sprites.md), you'll learn to efficiently render many sprites with instancing and batching!
