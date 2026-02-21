# Exercise 2: Animated Colors

**Difficulty**: ⭐⭐ Beginner  
**Time**: 20-30 minutes

## Learning Objectives
- Use time to create animations
- Understand sine and cosine functions for smooth oscillations
- Work with `GetTickCount64()` or `QueryPerformanceCounter()`
- Calculate dynamic color values each frame

## Background

Animation in graphics is about changing values over time. By calculating different color values each frame based on elapsed time, we can create smooth, continuous color transitions.

The sine function is particularly useful because it naturally oscillates between -1 and 1, making it perfect for smooth back-and-forth animations.

## Your Tasks

### Task 2.1: Pulsing Red Background
Make the background pulse between black and red:
- Use `GetTickCount64()` to get time in milliseconds
- Use `sin()` to create a pulsing effect
- Convert the sine wave (-1 to 1) to color range (0 to 1)

**Expected Result**: Screen fades from black to red and back smoothly

### Task 2.2: Rainbow Cycle
Create a rainbow effect by cycling through different hues:
- Red → Yellow → Green → Cyan → Blue → Magenta → Red
- Use time-shifted sine waves for each color channel
- Complete one full cycle every few seconds

**Expected Result**: Screen smoothly cycles through rainbow colors

### Task 2.3: Two-Tone Oscillation
Create an oscillation between two custom colors (e.g., blue and orange):
- Define a start color and end color
- Blend between them using a sine wave
- Make the transition speed adjustable

## Code Structure

You'll need to:
1. Get the current time at the start of each frame
2. Calculate color values based on that time
3. Pass those calculated values to `Clear()`

## Hints

<details>
<summary>Click to reveal hint for Task 2.1</summary>

```cpp
// In your main loop, before dx.Clear():
float time = GetTickCount64() / 1000.0f; // Convert to seconds
float red = (sin(time * 2.0f) + 1.0f) * 0.5f; // Oscillates 0.0 to 1.0
dx.Clear(red, 0.0f, 0.0f, 1.0f);
```

The formula `(sin(x) + 1.0) * 0.5` converts sin's range of [-1, 1] to [0, 1].

</details>

<details>
<summary>Click to reveal hint for Task 2.2</summary>

Use phase-shifted sine waves:
```cpp
float time = GetTickCount64() / 1000.0f;
float r = (sin(time) + 1.0f) * 0.5f;
float g = (sin(time + 2.094f) + 1.0f) * 0.5f; // 2.094 ≈ 2π/3
float b = (sin(time + 4.189f) + 1.0f) * 0.5f; // 4.189 ≈ 4π/3
dx.Clear(r, g, b, 1.0f);
```

These phase shifts (120° apart) create smooth color cycling.

</details>

<details>
<summary>Click to reveal hint for Task 2.3</summary>

Linear interpolation (lerp) formula:
```cpp
result = start + (end - start) * t;
```

Where `t` goes from 0 to 1:
```cpp
float t = (sin(time * speed) + 1.0f) * 0.5f;
float r = startR + (endR - startR) * t;
float g = startG + (endG - startG) * t;
float b = startB + (endB - startB) * t;
```

</details>

## Bonus Challenges

### Challenge 2.4: Multiple Layers
Create a more complex animation by combining multiple sine waves with different frequencies:
```cpp
float r = (sin(time) + sin(time * 2.3f) + 2.0f) / 4.0f;
```

### Challenge 2.5: User-Controlled Speed
Add keyboard input to control animation speed:
- Press UP arrow to speed up
- Press DOWN arrow to slow down
- This requires capturing keyboard input in Win32Window (we'll learn this properly in Exercise 7)

## Math Reference

### Sine and Cosine
- `sin(x)` oscillates between -1 and 1
- Period is 2π (approximately 6.28)
- `sin(0) = 0`, `sin(π/2) ≈ sin(1.57) = 1`

### Converting Sine to Color Range
- Sine output: -1 to 1
- Color needs: 0 to 1
- Formula: `(sin(x) + 1) / 2` or `(sin(x) + 1) * 0.5`

### Common Frequencies
- `sin(time)`: ~1 cycle per 6.28 seconds
- `sin(time * 2)`: ~2 cycles per 6.28 seconds
- `sin(time * π)`: ~1 cycle per 2 seconds

## Solution Notes

### Why Use GetTickCount64()?
- Returns milliseconds since system start
- Simple and sufficient for basic animations
- For more precision, use `QueryPerformanceCounter()` (see arch.md)

### Frame-Rate Independence
Notice that using time (not frame count) makes the animation speed consistent regardless of frame rate:
- On a 60 FPS system: animation looks smooth
- On a 144 FPS system: animation runs at same speed, just smoother

This is crucial for professional applications!

### Understanding Phase Shifts
Adding a constant to the time value shifts the wave:
- `sin(time)`: starts at 0
- `sin(time + π/2)`: starts at 1
- `sin(time + π)`: starts at 0 but opposite phase

For RGB rainbow, we want three waves 120° apart (2π/3 ≈ 2.094 radians).

## What You Learned
✅ Time-based animation fundamentals  
✅ Using sine/cosine for smooth oscillations  
✅ Converting between different value ranges  
✅ Creating smooth color transitions  
✅ Frame-rate independent animation  

## Next Steps
In [Exercise 3: Your First Triangle](./03-first-triangle.md), you'll learn to draw actual geometry using vertices and shaders!
