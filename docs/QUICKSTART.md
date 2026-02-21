# Quick Start Guide - 2D DirectX Exercises

## Getting Started

This repository now includes **10 progressive exercises** and a **final project** to teach you 2D graphics programming with DirectX 11.

### Prerequisites
- Windows with Visual Studio or compatible C++ compiler
- CMake 3.20 or higher
- DirectX 11 SDK (included with Windows SDK)
- Basic C++ knowledge

### Current Project Status
The repository contains a working DirectX 11 foundation with:
- ✅ Win32 window management
- ✅ DirectX 11 device initialization
- ✅ Basic rendering loop
- ✅ Window resizing support
- ✅ RAII resource management patterns

## Start Learning

### Step 1: Build the Current Project
```bash
# Configure with CMake
cmake --preset windows-default

# Build
cmake --build build

# Run
build\Debug\SimFoundation.exe
```

You should see a blue window. This is your starting point!

### Step 2: Choose Your Path

#### Path A: Follow Exercises in Order (Recommended)
Start with [Exercise 1: Background Colors](./exercises/01-background-colors.md) and work through all 10 exercises sequentially. Each exercise builds on previous concepts.

**Time commitment**: 1-2 hours per exercise
**Outcome**: Comprehensive understanding of 2D graphics

#### Path B: Jump to Topics of Interest
- Want to draw shapes? → [Exercise 3: Your First Triangle](./exercises/03-first-triangle.md)
- Want textures? → [Exercise 5: Textures](./exercises/05-textures.md)
- Want interaction? → [Exercise 7: Mouse Interaction](./exercises/07-mouse-interaction.md)
- Want to build a game? → [Final Project](./exercises/final-project.md)

**Note**: Jumping ahead requires understanding of previous exercises.

### Step 3: Complete the Final Project
After finishing the exercises, build a complete game: [2D Space Shooter](./exercises/final-project.md)

**Time commitment**: 3-6 hours
**Outcome**: Portfolio-ready game project

## Exercise Overview

| # | Exercise | Concepts | Difficulty | Time |
|---|----------|----------|------------|------|
| 1 | [Background Colors](./exercises/01-background-colors.md) | RGB values, Clear() | ⭐ | 15 min |
| 2 | [Animated Colors](./exercises/02-animated-colors.md) | Time-based animation, sine waves | ⭐⭐ | 30 min |
| 3 | [Your First Triangle](./exercises/03-first-triangle.md) | Vertex buffers, shaders | ⭐⭐⭐ | 60 min |
| 4 | [Drawing Rectangles](./exercises/04-rectangles.md) | Index buffers, quads | ⭐⭐⭐ | 45 min |
| 5 | [Textures](./exercises/05-textures.md) | Image loading, sampling | ⭐⭐⭐⭐ | 90 min |
| 6 | [Transformations](./exercises/06-transformations.md) | Matrices, rotation, scaling | ⭐⭐⭐⭐ | 60 min |
| 7 | [Mouse Interaction](./exercises/07-mouse-interaction.md) | Input handling, hit testing | ⭐⭐⭐ | 45 min |
| 8 | [Multiple Sprites](./exercises/08-multiple-sprites.md) | Instancing, batching | ⭐⭐⭐⭐ | 90 min |
| 9 | [Sprite Animation](./exercises/09-sprite-animation.md) | Sprite sheets, frame animation | ⭐⭐⭐ | 60 min |
| 10 | [2D Camera](./exercises/10-camera.md) | View matrices, zoom, pan | ⭐⭐⭐⭐ | 60 min |
| 💎 | [**Final Project**](./exercises/final-project.md) | Complete game | ⭐⭐⭐⭐⭐ | 3-6 hrs |

**Total time**: ~12-18 hours for all exercises + final project

## What You'll Learn

### Core Graphics Concepts
- DirectX 11 rendering pipeline
- Vertex and pixel shaders (HLSL)
- Buffers: vertex, index, constant
- Textures and samplers
- Transformations and matrices

### Practical Skills
- Drawing 2D shapes and sprites
- Handling user input
- Implementing game mechanics
- Optimizing rendering performance
- Creating visual effects

### Architecture Patterns
- RAII resource management
- Entity-component patterns
- Game state management
- Collision detection
- Camera systems

## Tips for Success

1. **Start Simple**: Don't skip exercises, each builds essential understanding
2. **Experiment**: Try changing values to see what happens
3. **Read Error Messages**: DirectX errors can be cryptic, but they're informative
4. **Use Debug Layer**: Enable D3D11_CREATE_DEVICE_DEBUG for better error messages
5. **Take Breaks**: Complex concepts need time to sink in
6. **Ask Questions**: Comment on the PR or open issues
7. **Build Projects**: Apply what you learn to personal projects

## Common Issues

### Build Errors
- **Missing DirectX SDK**: Install Windows SDK
- **CMake version too old**: Update to 3.20+
- **Linker errors**: Ensure d3d11.lib, dxgi.lib are linked

### Runtime Errors
- **Black screen**: Check shaders compiled correctly
- **Crash on startup**: Enable debug layer for details
- **Nothing renders**: Verify vertex buffer and shader pipeline

### Performance Issues
- **Low FPS**: Too many draw calls, use batching
- **Stuttering**: Use deltaTime for frame-rate independence
- **Memory leaks**: Check ComPtr usage, enable debug layer

## Additional Resources

### Official Documentation
- [DirectX 11 Programming Guide](https://learn.microsoft.com/en-us/windows/win32/direct3d11/dx-graphics-overviews)
- [HLSL Reference](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl)
- [DirectX Graphics Samples](https://github.com/microsoft/DirectX-Graphics-Samples)

### Community Resources
- [LearnOpenGL](https://learnopengl.com/) - Concepts transfer to DirectX
- [RasterTek DirectX Tutorials](http://www.rastertek.com/tutdx11.html)
- [Braynzar Soft DirectX Tutorials](https://www.braynzarsoft.net/viewtutorial/q16390-braynzar-soft-directx-11-tutorials)

### This Repository
- [Architecture Documentation](../adr/arch.md) - RAII patterns and best practices
- [CMakeLists.txt](../../CMakeLists.txt) - Build configuration
- [Source Code](../../src/) - Current implementation

## Need Help?

- **Stuck on an exercise?** Check the hints sections (expandable details)
- **Found a bug?** Open an issue on GitHub
- **Want to contribute?** Submit a PR with improvements
- **Have questions?** Comment on the exercises PR

## Next Steps

Ready to start? Head to the [Exercise README](./README.md) for the full overview, or jump straight to [Exercise 1: Background Colors](./01-background-colors.md)!

Good luck and have fun! 🚀
