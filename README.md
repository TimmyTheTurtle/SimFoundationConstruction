# SimFoundation - DirectX 11 Learning Project

A clean, modern DirectX 11 foundation project for learning 2D and 3D graphics programming.

## 🎮 New: 2D Graphics Exercises!

This repository now includes **comprehensive beginner-friendly exercises** for learning 2D graphics with DirectX 11!

📚 **[Start Learning → Quick Start Guide](./docs/QUICKSTART.md)**

### What's Included

- **10 Progressive Exercises**: From basic colors to advanced camera systems
- **Final Project**: Build a complete 2D space shooter game
- **Detailed Documentation**: Step-by-step instructions with hints and solutions
- **Modern C++20**: Best practices with RAII patterns
- **Production-Ready Code**: Clean architecture ready for your projects

[View All Exercises →](./docs/exercises/README.md)

## 🚀 Quick Start

### Build and Run

```bash
# Configure
cmake --preset windows-default

# Build
cmake --build build

# Run
build\Debug\SimFoundation.exe
```

You should see a blue window - this is your starting point!

### Start Learning

1. **Complete beginner?** Start with [Exercise 1: Background Colors](./docs/exercises/01-background-colors.md)
2. **Want hands-on?** Jump to [Exercise 3: Your First Triangle](./docs/exercises/03-first-triangle.md)
3. **Ready for a project?** Try the [Final Project: 2D Space Shooter](./docs/exercises/final-project.md)

## 📋 Features

### Current Implementation

- ✅ Win32 window management with resize support
- ✅ DirectX 11 device initialization
- ✅ Swap chain and render target setup
- ✅ Basic rendering loop (60 FPS with vsync)
- ✅ RAII resource management
- ✅ Clean, modular architecture

### Exercise Topics

1. **Basics**: Colors, animation, time-based rendering
2. **Geometry**: Vertices, buffers, shaders, primitives
3. **Textures**: Loading images, sampling, UVs
4. **Transforms**: Matrices, rotation, scaling, translation
5. **Interaction**: Mouse/keyboard input, hit testing
6. **Optimization**: Instancing, batching, culling
7. **Effects**: Particles, animations, camera systems

## 📁 Project Structure

```
SimFoundationConstruction/
├── src/
│   ├── app/          # Application entry point
│   ├── platform/     # Win32 window management
│   ├── gfx/          # DirectX 11 rendering
│   └── sim/          # Simulation/game logic
├── docs/
│   ├── exercises/    # 10 exercises + final project
│   ├── adr/          # Architecture decision records
│   └── QUICKSTART.md # Quick start guide
├── tests/            # Unit tests (GoogleTest)
└── CMakeLists.txt    # Build configuration
```

## 🎯 Learning Path

### Beginner Track (6-8 hours)
- Exercises 1-5: Basics through textures
- Build simple sprite viewer

### Intermediate Track (10-12 hours)
- Exercises 6-8: Transforms, input, batching
- Build interactive sprite demo

### Advanced Track (15-20 hours)
- Exercises 9-10: Animation, camera
- Complete final project (space shooter)

## 🛠️ Requirements

- **OS**: Windows 10/11
- **Compiler**: Visual Studio 2019+ or clang-cl
- **CMake**: 3.20 or higher
- **SDK**: Windows SDK (for DirectX 11)
- **C++ Standard**: C++20

## 📖 Documentation

- **[Quick Start Guide](./docs/QUICKSTART.md)** - Get started quickly
- **[Exercise Overview](./docs/exercises/README.md)** - All exercises and final project
- **[Architecture Guide](./docs/adr/arch.md)** - RAII patterns and best practices

## 🧪 Testing

```bash
# Build tests
cmake --build build --target SimFoundationTests

# Run tests
ctest --test-dir build
```

## 🤝 Contributing

Improvements and additions are welcome! Some ideas:

- Additional exercises (3D graphics, advanced shaders)
- Example projects beyond the space shooter
- Performance benchmarks
- Cross-platform support (Linux/macOS with alternative APIs)

## 📝 License

This is an educational project. Feel free to use it for learning and personal projects.

## 🙏 Acknowledgments

- Built on modern DirectX 11 best practices
- Architecture inspired by professional game engines
- Exercises designed for progressive learning

## 📬 Support

- **Questions?** Open an issue
- **Found a bug?** Submit a PR
- **Want to share your project?** We'd love to see it!

---

**Ready to start?** Head to the [Quick Start Guide](./docs/QUICKSTART.md) and begin your journey into graphics programming! 🚀
