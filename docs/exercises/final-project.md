# Final Project: 2D Space Shooter

**Difficulty**: ⭐⭐⭐⭐⭐ Advanced  
**Time**: 3-6 hours

## Project Overview

Build a complete 2D space shooter game that combines all concepts from the exercises:
- Player-controlled spaceship
- Enemy ships with AI
- Projectiles (bullets, lasers)
- Scrolling starfield background
- Particle effects for explosions
- Score and UI display
- Game states (menu, playing, game over)

## Learning Objectives
- Integrate all previous exercise concepts
- Implement game logic and state management
- Handle collisions
- Create visual effects
- Build a complete game loop
- Polish a finished project

## Project Requirements

### Core Features (Must Have)

1. **Player Ship**
   - Keyboard controls (WASD or arrow keys)
   - Fire projectiles (spacebar)
   - Animated thrust/engine effect
   - Health system (3-5 hit points)
   - Invincibility frames after getting hit

2. **Enemies**
   - 2-3 different enemy types
   - Simple AI (move in patterns)
   - Fire at player
   - Spawn in waves

3. **Projectiles**
   - Player bullets (fast, small)
   - Enemy bullets (varied by enemy type)
   - Collision detection
   - Destroy on impact or off-screen

4. **Background**
   - Scrolling starfield (multiple layers for parallax)
   - Seamless wrapping

5. **Visual Effects**
   - Explosion particles when enemies die
   - Muzzle flash when shooting
   - Screen shake on big explosions

6. **UI**
   - Score display
   - Health/lives indicator
   - High score

7. **Game States**
   - Title screen
   - Gameplay
   - Game over screen
   - Ability to restart

### Bonus Features (Nice to Have)

- Power-ups (faster fire rate, shield, spread shot)
- Boss enemy
- Sound effects (simulated with visual cues)
- Combo system
- Multiple levels with increasing difficulty
- Background music visualization

## Architecture Design

### Recommended Structure

```cpp
// main.cpp - Entry point and main loop

// Game.h/cpp - Main game logic
class Game {
    enum class State { Menu, Playing, GameOver };
    State currentState;
    
    Player player;
    std::vector<Enemy> enemies;
    std::vector<Bullet> bullets;
    std::vector<Particle> particles;
    Background background;
    UI ui;
    
    int score;
    int wave;
    
    void Update(float deltaTime);
    void Render(Dx11Device& device);
    void HandleInput(const InputState& input);
};

// Player.h/cpp
class Player {
    Vec2 position;
    Vec2 velocity;
    float rotation;
    int health;
    float fireTimer;
    bool invincible;
    float invincibleTimer;
    
    void Update(float deltaTime, const InputState& input);
    void TakeDamage();
    bool CanFire() const;
};

// Enemy.h/cpp
class Enemy {
    enum class Type { Basic, Fast, Heavy };
    Type type;
    Vec2 position;
    Vec2 velocity;
    int health;
    float fireTimer;
    
    void Update(float deltaTime, Vec2 playerPos);
    void Fire(std::vector<Bullet>& bullets);
};

// Bullet.h/cpp
struct Bullet {
    Vec2 position;
    Vec2 velocity;
    bool fromPlayer;
    float lifetime;
};

// Particle.h/cpp
struct Particle {
    Vec2 position;
    Vec2 velocity;
    Vec4 color;
    float lifetime;
    float size;
};

// Collision.h/cpp
namespace Collision {
    bool CircleCircle(Vec2 p1, float r1, Vec2 p2, float r2);
    bool RectRect(Rect r1, Rect r2);
}

// Background.h/cpp
class Background {
    struct StarLayer {
        std::vector<Vec2> stars;
        float speed;
        float depth;
    };
    std::vector<StarLayer> layers;
    
    void Update(float deltaTime);
    void Render(SpriteBatch& batch);
};
```

## Implementation Steps

### Phase 1: Setup (30-60 min)
1. Create project structure with classes
2. Set up sprite batch system from Exercise 8
3. Create or find sprite assets:
   - Player ship sprite
   - Enemy sprites (2-3 types)
   - Bullet sprites
   - UI elements
4. Load textures and set up rendering

### Phase 2: Player (45-60 min)
1. Implement player movement
   - WASD/arrow key controls
   - Smooth acceleration/deceleration
   - Screen wrapping or boundaries
2. Add shooting mechanic
   - Fire cooldown timer
   - Create bullet entities
   - Muzzle flash effect
3. Add player animation
   - Idle/thrust animation
   - Rotation towards movement direction

### Phase 3: Enemies (60-90 min)
1. Create enemy spawner
   - Spawn at top of screen
   - Different spawn patterns
   - Wave system
2. Implement enemy AI
   - Basic: Move down, occasionally shoot
   - Fast: Zigzag pattern
   - Heavy: Slow, shoot frequently
3. Add enemy shooting
   - Aim at player
   - Different bullet patterns per type

### Phase 4: Collision (30-45 min)
1. Implement collision detection
   - Circle-circle for simple hits
   - Rect-rect for more precision
2. Handle collisions:
   - Bullet vs Enemy → damage enemy, destroy bullet
   - Bullet vs Player → damage player, destroy bullet
   - Player vs Enemy → damage player, destroy enemy
3. Add invincibility frames for player

### Phase 5: Effects (45-60 min)
1. Explosion particles
   - Spawn particles on enemy death
   - Color, velocity, lifetime
   - Fade out over time
2. Screen shake
   - Bigger explosions = more shake
   - Smooth interpolation
3. Other effects
   - Bullet trails
   - Hit flashes

### Phase 6: Background (30 min)
1. Create starfield
   - Multiple layers (near, mid, far)
   - Parallax scrolling
2. Seamless wrapping
   - Stars respawn at top when leaving bottom

### Phase 7: UI and Game States (60 min)
1. Create UI elements
   - Score counter
   - Health display
   - High score
2. Title screen
   - "Press SPACE to start"
   - Show high score
3. Game over screen
   - Final score
   - "Press R to restart"
4. State transitions

### Phase 8: Polish (60+ min)
1. Balance gameplay
   - Enemy health and damage
   - Player health and fire rate
   - Spawn rates
2. Add juice
   - Better particles
   - Color flashes
   - Smooth transitions
3. Test and fix bugs

## Sample Code Snippets

### Collision Detection

```cpp
namespace Collision {
    bool CircleCircle(Vec2 p1, float r1, Vec2 p2, float r2) {
        float dx = p2.x - p1.x;
        float dy = p2.y - p1.y;
        float distSq = dx * dx + dy * dy;
        float radiusSum = r1 + r2;
        return distSq < radiusSum * radiusSum;
    }
}

// Usage
for (auto& bullet : bullets) {
    if (!bullet.fromPlayer) continue;
    
    for (auto& enemy : enemies) {
        if (Collision::CircleCircle(
            bullet.position, 0.02f,
            enemy.position, 0.05f))
        {
            enemy.health--;
            bullet.lifetime = 0;  // Mark for removal
            
            if (enemy.health <= 0) {
                SpawnExplosion(enemy.position);
                score += enemy.GetPoints();
            }
        }
    }
}
```

### Enemy AI

```cpp
void Enemy::Update(float deltaTime, Vec2 playerPos) {
    switch (type) {
        case Type::Basic:
            // Move straight down
            velocity.y = -0.5f;
            break;
            
        case Type::Fast:
            // Zigzag
            velocity.y = -1.0f;
            velocity.x = sin(GetTickCount64() / 200.0f) * 0.3f;
            break;
            
        case Type::Heavy:
            // Move toward player X, down Y
            float dx = playerPos.x - position.x;
            velocity.x = std::max(-0.2f, std::min(0.2f, dx * 0.5f));
            velocity.y = -0.3f;
            break;
    }
    
    position.x += velocity.x * deltaTime * 60.0f;
    position.y += velocity.y * deltaTime * 60.0f;
    
    // Fire timer
    fireTimer -= deltaTime;
    if (fireTimer <= 0) {
        fireTimer = GetFireRate();
        // Fire bullet...
    }
}
```

### Particle System

```cpp
void SpawnExplosion(Vec2 position, int particleCount = 20) {
    for (int i = 0; i < particleCount; i++) {
        Particle p;
        p.position = position;
        
        // Random direction
        float angle = (rand() / (float)RAND_MAX) * 6.28f;
        float speed = 0.5f + (rand() / (float)RAND_MAX) * 1.0f;
        p.velocity.x = cos(angle) * speed;
        p.velocity.y = sin(angle) * speed;
        
        // Random color (orange to yellow)
        p.color = {
            1.0f,
            0.5f + (rand() / (float)RAND_MAX) * 0.5f,
            0.0f,
            1.0f
        };
        
        p.lifetime = 0.5f + (rand() / (float)RAND_MAX) * 0.5f;
        p.size = 0.02f + (rand() / (float)RAND_MAX) * 0.03f;
        
        particles.push_back(p);
    }
}

void UpdateParticles(float deltaTime) {
    for (auto& p : particles) {
        p.position.x += p.velocity.x * deltaTime;
        p.position.y += p.velocity.y * deltaTime;
        p.lifetime -= deltaTime;
        
        // Fade out
        p.color.a = p.lifetime / 1.0f;
    }
    
    // Remove dead particles
    particles.erase(
        std::remove_if(particles.begin(), particles.end(),
            [](const Particle& p) { return p.lifetime <= 0; }),
        particles.end()
    );
}
```

## Assets and Resources

### Creating Sprites

**Option 1: Simple colored shapes (code)**
```cpp
// Create a 32×32 triangle for player ship
std::vector<uint32_t> pixels(32 * 32, 0x00000000);  // Transparent
for (int y = 0; y < 32; y++) {
    for (int x = 0; x < 32; x++) {
        // Draw triangle pointing up
        if (abs(x - 16) < (32 - y) / 2) {
            pixels[y * 32 + x] = 0xFF00FF00;  // Green
        }
    }
}
```

**Option 2: Use free assets**
- [OpenGameArt.org](https://opengameart.org)
- Search for "space shooter" or "spaceship sprites"
- Ensure license allows use

**Option 3: Create in pixel art editor**
- [Piskel](https://www.piskelapp.com) (free, web-based)
- Create 32×32 or 64×64 sprites
- Export as PNG

### Recommended Sprite Sizes
- Player ship: 32×32 to 64×64
- Enemies: 32×32 to 48×48
- Bullets: 8×8 to 16×16
- Particles: 4×4 to 8×8

## Testing Checklist

- [ ] Player moves smoothly in all directions
- [ ] Player can fire continuously
- [ ] Enemies spawn in waves
- [ ] Enemies have different behaviors
- [ ] Bullets collide correctly
- [ ] Explosions appear on enemy death
- [ ] Score increases when enemies die
- [ ] Player takes damage and has invincibility
- [ ] Game over triggers at 0 health
- [ ] Can restart after game over
- [ ] Background scrolls smoothly
- [ ] No crashes or memory leaks
- [ ] Runs at 60 FPS with 100+ entities

## Extensions and Improvements

Once the basic game works, consider adding:

1. **Power-Ups**
   ```cpp
   enum class PowerUpType {
       SpeedBoost, Shield, RapidFire, SpreadShot
   };
   ```

2. **Boss Fights**
   - Larger enemy with multiple phases
   - Complex attack patterns
   - Health bar

3. **Procedural Difficulty**
   ```cpp
   float difficultyMultiplier = 1.0f + (wave * 0.1f);
   enemyHealth *= difficultyMultiplier;
   spawnRate *= difficultyMultiplier;
   ```

4. **Persistent High Score**
   - Save to file
   - Display on title screen

5. **Sound**
   - Use XAudio2 for actual sound
   - Or visual indicators (flash, shake)

## What You'll Learn

By completing this project, you will have:
✅ Built a complete game from scratch  
✅ Integrated all DirectX 2D concepts  
✅ Implemented game logic and state management  
✅ Handled real-time collision detection  
✅ Created visual effects and polish  
✅ Debugged complex interactions  
✅ Experienced the full game development cycle  

## Submission/Showcase

When finished:
1. Take screenshots or record a video
2. Write a brief post-mortem:
   - What went well?
   - What was challenging?
   - What would you improve?
3. Share with others or add to your portfolio!

## Congratulations!

You've completed the 2D DirectX graphics course! You now have the skills to:
- Create interactive 2D graphics applications
- Build games with DirectX 11
- Understand the graphics pipeline
- Optimize rendering performance
- Polish visual presentation

**Next Steps**:
- Explore 3D graphics with DirectX
- Learn more advanced effects (shaders, post-processing)
- Study other game engines (Unity, Unreal)
- Build more projects!

Happy coding! 🚀✨
