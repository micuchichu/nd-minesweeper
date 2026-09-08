#pragma once

#include "raylib.h"
#include <vector>

namespace minesweeper::render {

struct Particle {
    Vector2 position;
    Vector2 velocity;
    Color color;
    float life;
    float maxLife;
    float size;
    float rotation;
    float rotSpeed;
};

class ParticleSystem {
public:
    std::vector<Particle> particles;
    Texture2D particleTexture = { 0 };

    ParticleSystem(size_t reserveCount = 4096);
    void setTexture(Texture2D tex) { particleTexture = tex; }

    void emitExplosion(Vector2 pos, int count, Color color);
    void emitDebris(Vector2 pos, int count, Color color);
    void updateAndDraw(float dt);
    void clear();

private:
    float randomFloat(float min, float max);
};

} // namespace minesweeper::render
