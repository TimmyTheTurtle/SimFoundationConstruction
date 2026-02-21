# Exercise 1: Background Colors

**Difficulty**: ⭐ Beginner  
**Time**: 10-15 minutes

## Learning Objectives
- Understand RGB color representation (values from 0.0 to 1.0)
- Learn how the `Clear()` function works
- Modify the application's background color
- Understand the rendering loop

## Background

In DirectX 11, colors are represented using floating-point values from 0.0 to 1.0 for each channel (Red, Green, Blue, Alpha). The `Clear()` function fills the entire render target with a solid color.

Current code in `main.cpp` clears the screen to a blue color:
```cpp
dx.Clear(0.1f, 0.2f, 0.4f, 1.0f);  // Dark blue
```

## Your Tasks

### Task 1.1: Change to Different Colors
Modify the `Clear()` call to display these colors:

1. **Pure Red**: `(1.0, 0.0, 0.0, 1.0)`
2. **Pure Green**: `(0.0, 1.0, 0.0, 1.0)`
3. **Pure Blue**: `(0.0, 0.0, 1.0, 1.0)`
4. **White**: `(1.0, 1.0, 1.0, 1.0)`
5. **Black**: `(0.0, 0.0, 0.0, 1.0)`
6. **Yellow**: `(1.0, 1.0, 0.0, 1.0)`
7. **Magenta**: `(1.0, 0.0, 1.0, 1.0)`
8. **Cyan**: `(0.0, 1.0, 1.0, 1.0)`

### Task 1.2: Create Custom Colors
Experiment and create these effects:
- A pleasant sky blue
- A warm sunset orange
- A deep purple
- A forest green

### Task 1.3: Understand Alpha
Try changing the alpha value (4th parameter):
- What happens when alpha is 0.5?
- What happens when alpha is 0.0?
- Why might alpha not seem to do anything right now?

## Hints

<details>
<summary>Click to reveal hint for Task 1.1</summary>

Find the line in `main.cpp` that looks like:
```cpp
dx.Clear(0.1f, 0.2f, 0.4f, 1.0f);
```

Change the first three numbers (R, G, B). The fourth number (Alpha) should stay at 1.0 for now.

</details>

<details>
<summary>Click to reveal hint for Task 1.2</summary>

Mix different amounts of red, green, and blue:
- Orange = mostly red + some green
- Purple = red + blue
- Forest green = mostly green + a bit of blue

Use values between 0.0 and 1.0.

</details>

<details>
<summary>Click to reveal hint for Task 1.3</summary>

Alpha controls transparency, but you need something behind the background to see through to! Right now, there's nothing behind it, so alpha has no visible effect. We'll use alpha later when we draw objects on top of each other.

</details>

## Solution Notes

### RGB Color Space
- **Red = (1, 0, 0)**: Maximum red, no green or blue
- **Yellow = (1, 1, 0)**: Red + Green = Yellow
- **White = (1, 1, 1)**: All colors at maximum

### The Rendering Loop
Notice that `Clear()` is called every frame in the while loop. This means:
- The screen is cleared 60+ times per second (with vsync)
- Any drawing you do needs to happen every frame
- Changes to Clear() values happen immediately

### Common Beginner Mistakes
❌ Using values > 1.0 (they get clamped to 1.0)  
❌ Using integers like `255` instead of floats like `1.0`  
❌ Forgetting to rebuild the project after changes  

## What You Learned
✅ RGB color representation in DirectX (0.0 to 1.0 range)  
✅ How to modify the Clear() function  
✅ The concept of the rendering loop  
✅ Alpha channel basics  

## Next Steps
In [Exercise 2: Animated Colors](./02-animated-colors.md), you'll make the background color change over time using math and time values!
