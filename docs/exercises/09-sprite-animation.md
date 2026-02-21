# Exercise 9: Sprite Animation

**Difficulty**: ⭐⭐⭐ Intermediate  
**Time**: 45-60 minutes

## Learning Objectives
- Understand sprite sheet animation
- Implement frame-based animation
- Create animation controllers
- Handle animation timing and playback
- Support multiple simultaneous animations

## Background

Sprite animation works by displaying different regions of a texture over time. A **sprite sheet** contains all animation frames in a single image, organized in a grid.

**Example**: A walk cycle might be 8 frames arranged horizontally:
```
[Frame0][Frame1][Frame2][Frame3][Frame4][Frame5][Frame6][Frame7]
```

We change the UV coordinates to show different frames.

## Your Tasks

### Task 9.1: Sprite Sheet Structure

Create structures to define animations:

```cpp
struct Animation {
    std::string name;
    int startFrame;
    int frameCount;
    float frameDuration;  // Seconds per frame
    bool loop;
};

struct AnimationController {
    std::vector<Animation> animations;
    int currentAnimation = 0;
    float currentTime = 0.0f;
    int currentFrame = 0;
    
    int framesPerRow;
    int framesPerColumn;
    
    void Update(float deltaTime);
    void Play(const std::string& name);
    Vec4 GetCurrentUV() const;
};
```

### Task 9.2: Calculate UV Coordinates

Given a frame number, calculate its UV coordinates:

```cpp
Vec4 CalculateFrameUV(int frameIndex, int framesPerRow, int framesPerColumn)
{
    int column = frameIndex % framesPerRow;
    int row = frameIndex / framesPerRow;
    
    float uMin = column / (float)framesPerRow;
    float vMin = row / (float)framesPerColumn;
    float uMax = (column + 1) / (float)framesPerRow;
    float vMax = (row + 1) / (float)framesPerColumn;
    
    return { uMin, vMin, uMax, vMax };
}
```

### Task 9.3: Update Animation

Implement animation update logic:

```cpp
void AnimationController::Update(float deltaTime)
{
    if (animations.empty()) return;
    
    Animation& anim = animations[currentAnimation];
    currentTime += deltaTime;
    
    // Check if we need to advance to next frame
    if (currentTime >= anim.frameDuration) {
        currentTime -= anim.frameDuration;
        currentFrame++;
        
        // Handle loop/stop
        if (currentFrame >= anim.frameCount) {
            if (anim.loop) {
                currentFrame = 0;
            } else {
                currentFrame = anim.frameCount - 1;  // Stay on last frame
                currentTime = 0.0f;
            }
        }
    }
}

Vec4 AnimationController::GetCurrentUV() const
{
    if (animations.empty()) return {0, 0, 1, 1};
    
    const Animation& anim = animations[currentAnimation];
    int absoluteFrame = anim.startFrame + currentFrame;
    
    return CalculateFrameUV(absoluteFrame, framesPerRow, framesPerColumn);
}
```

### Task 9.4: Play Animation by Name

```cpp
void AnimationController::Play(const std::string& name)
{
    for (size_t i = 0; i < animations.size(); i++) {
        if (animations[i].name == name) {
            currentAnimation = i;
            currentFrame = 0;
            currentTime = 0.0f;
            return;
        }
    }
}
```

### Task 9.5: Create Test Sprite Sheet

Create a test sprite sheet:
- Make an 8×1 grid (8 frames in a row)
- Each frame shows a different color or simple shape
- Or download a real sprite sheet online

Example using code to generate one:
```cpp
// Generate a 512×64 texture with 8 colored squares
const int width = 512, height = 64;
const int frames = 8;
std::vector<uint32_t> pixels(width * height);

for (int i = 0; i < frames; i++) {
    uint32_t color = 0xFF000000 | 
                     ((i * 32) << 16) |  // Red gradient
                     ((255 - i * 32) << 8) |  // Green gradient
                     (128);  // Blue constant
    
    int startX = i * (width / frames);
    int endX = (i + 1) * (width / frames);
    
    for (int y = 0; y < height; y++) {
        for (int x = startX; x < endX; x++) {
            pixels[y * width + x] = color;
        }
    }
}

// Create texture from pixels...
```

### Task 9.6: Integrate with Sprite Batch

Update your sprite instance to use animation:

```cpp
struct AnimatedSprite {
    SpriteInstance instance;
    AnimationController animator;
    
    void Update(float deltaTime) {
        animator.Update(deltaTime);
        instance.texCoords = animator.GetCurrentUV();
    }
};

// In main loop
float deltaTime = /* calculate from timer */;

for (auto& sprite : animatedSprites) {
    sprite.Update(deltaTime);
}
```

### Task 9.7: Multiple Animations

Create a character with multiple animations:

```cpp
AnimationController CreateCharacterAnimator()
{
    AnimationController animator;
    animator.framesPerRow = 8;
    animator.framesPerColumn = 4;  // 4 rows of animations
    
    // Row 0: Idle (4 frames, looping)
    animator.animations.push_back({
        "idle", 0, 4, 0.2f, true
    });
    
    // Row 1: Walk (8 frames, looping)
    animator.animations.push_back({
        "walk", 8, 8, 0.1f, true
    });
    
    // Row 2: Run (8 frames, looping)
    animator.animations.push_back({
        "run", 16, 8, 0.08f, true
    });
    
    // Row 3: Jump (6 frames, no loop)
    animator.animations.push_back({
        "jump", 24, 6, 0.1f, false
    });
    
    return animator;
}

// Usage
animator.Play("walk");
```

## Hints

<details>
<summary>Click to reveal hint for deltaTime calculation</summary>

Calculate time between frames:

```cpp
static LARGE_INTEGER lastTime = {0};
static LARGE_INTEGER frequency = {0};

if (frequency.QuadPart == 0) {
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&lastTime);
}

LARGE_INTEGER currentTime;
QueryPerformanceCounter(&currentTime);

float deltaTime = (currentTime.QuadPart - lastTime.QuadPart) / 
                  (float)frequency.QuadPart;
lastTime = currentTime;

// Cap deltaTime to prevent huge jumps
if (deltaTime > 0.1f) deltaTime = 0.1f;
```

</details>

<details>
<summary>Click to reveal hint for sprite sheet layout</summary>

Common sprite sheet layouts:
```
Horizontal strip:
[0][1][2][3][4][5][6][7]

Grid (4×2):
[0][1][2][3]
[4][5][6][7]

Multiple animations (8×4):
[idle0][idle1][idle2][idle3][...][idle7]
[walk0][walk1][walk2][walk3][...][walk7]
[run0 ][run1 ][run2 ][run3 ][...][run7 ]
[jump0][jump1][jump2][jump3][...][jump7]
```

Choose based on your needs!

</details>

<details>
<summary>Click to reveal hint for smooth transitions</summary>

For smooth animation transitions, blend between animations:

```cpp
struct BlendedAnimator {
    AnimationController from;
    AnimationController to;
    float blendTime;
    float blendDuration;
    
    void CrossFade(const std::string& toAnim, float duration) {
        from = currentAnimator;
        to = currentAnimator;
        to.Play(toAnim);
        blendTime = 0.0f;
        blendDuration = duration;
    }
    
    void Update(float deltaTime) {
        from.Update(deltaTime);
        to.Update(deltaTime);
        blendTime += deltaTime;
        
        if (blendTime >= blendDuration) {
            currentAnimator = to;
        }
    }
};
```

</details>

## Bonus Challenges

### Challenge 9.8: Animation Events
Trigger events at specific frames (e.g., play footstep sound on frame 3):

```cpp
struct AnimationEvent {
    int frame;
    std::function<void()> callback;
};

// Add to Animation struct
std::vector<AnimationEvent> events;
```

### Challenge 9.9: Ping-Pong Animation
Play animation forward then backward:
```cpp
bool reverse = false;
if (currentFrame >= anim.frameCount - 1) reverse = true;
if (currentFrame <= 0) reverse = false;
currentFrame += reverse ? -1 : 1;
```

### Challenge 9.10: Variable Frame Duration
Allow different durations per frame:
```cpp
struct Frame {
    int index;
    float duration;
};

std::vector<Frame> frames;
```

## Common Issues and Solutions

### Issue: Animation too fast/slow
- ✅ Adjust `frameDuration` value
- ✅ Check deltaTime is in seconds (not milliseconds)
- ✅ Ensure Update() is called once per frame

### Issue: UV coordinates wrong
- ✅ Check framesPerRow and framesPerColumn values
- ✅ Verify integer division: use `(float)` casts
- ✅ Test with a single frame first

### Issue: Animation doesn't loop
- ✅ Check `loop` flag is true
- ✅ Verify frame reset: `currentFrame = 0` when looping
- ✅ Ensure currentTime wraps correctly

### Issue: Seams visible between frames
- ✅ Add small UV padding: `uMax -= 0.001f`
- ✅ Use point filtering for pixel-perfect sprites
- ✅ Ensure sprite sheet has no padding between frames

## Deep Dive: Advanced Animation Techniques

### State Machine
For complex character controllers:
```cpp
enum class State {
    Idle, Walking, Running, Jumping, Falling
};

State currentState;

void UpdateState(float deltaTime) {
    switch (currentState) {
        case State::Idle:
            if (inputX != 0) currentState = State::Walking;
            if (jumpPressed) currentState = State::Jumping;
            break;
        case State::Walking:
            if (inputX == 0) currentState = State::Idle;
            if (runPressed) currentState = State::Running;
            break;
        // ...
    }
    
    // Map states to animations
    static const char* stateAnims[] = {
        "idle", "walk", "run", "jump", "fall"
    };
    animator.Play(stateAnims[(int)currentState]);
}
```

### Additive Animations
Blend multiple animations (e.g., walk + talk):
- Base animation: walk cycle
- Additive animation: mouth movement
- Combine UVs or use multiple sprites

### Skeletal Animation
For more complex characters:
- Define bone hierarchy
- Store bone transforms per frame
- More flexible than sprite sheets
- Requires more complex system (beyond this exercise)

## What You Learned
✅ Sprite sheet basics  
✅ Frame-based animation  
✅ Animation timing and playback  
✅ Animation controllers  
✅ Delta time calculation  
✅ Multiple animation support  

## Next Steps
In [Exercise 10: 2D Camera](./10-camera.md), you'll implement a camera system to pan and zoom around your 2D world!
