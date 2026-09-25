#pragma once

#include "raylib.h"
#include <vector>
#include <cstdint>

namespace minesweeper::render {

enum class ParticleType : uint8_t {
    Debris = 0,     // Tumbling angular shard / fragment
    Spark,          // High-speed directional streak / electric spark
    PlasmaMote,     // Soft radiant glowing orb (energy pulse)
    Sparkle,        // 4-pointed twinkling star / glint
    Smoke,          // Expanding soft vapor / smoke puff
    Shockwave       // Expanding thin energy ring
};

struct Particle {
    Vector2 position;
    Vector2 velocity;
    Color color;
    float life;
    float maxLife;
    float size;
    float rotation;
    float rotSpeed;
    ParticleType type = ParticleType::Debris;
    float extra = 0.0f; // Extra parameter: maxRadius for shockwave, phase for sparkle/mote
};

class ParticleSystem {
public:
    std::vector<Particle> particles;

    ParticleSystem(size_t reserveCount = 4096);

    void emitExplosion(Vector2 pos, int count, Color color);
    void emitDebris(Vector2 pos, int count, Color color);
    void emitSparks(Vector2 pos, int count, Color color, float speedMin = 150.0f, float speedMax = 650.0f);
    void emitPlasmaMotes(Vector2 pos, int count, Color color, float speedMin = 25.0f, float speedMax = 140.0f);
    void emitSparkles(Vector2 pos, int count, Color color, float spreadRadius = 24.0f);
    void emitSmoke(Vector2 pos, int count, Color color, float speedMin = 10.0f, float speedMax = 50.0f);
    void emitShockwave(Vector2 pos, float maxRadius, Color color, float duration = 0.35f);

    // High-level composite effects
    void emitDefusalFX(Vector2 pos, float cellSize = 30.0f);
    void emitWarpSpawnFX(Vector2 pos, Color themeColor);

    void updateAndDraw(float dt);
    void clear();

private:
    float randomFloat(float min, float max);
};

} // namespace minesweeper::render
