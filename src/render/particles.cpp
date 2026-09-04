#include "particles.hpp"
#include <cmath>
#include <cstdlib>
#include <algorithm>

namespace minesweeper::render {

ParticleSystem::ParticleSystem(size_t reserveCount) {
    particles.reserve(reserveCount);
}

float ParticleSystem::randomFloat(float min, float max) {
    return min + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * (max - min);
}

void ParticleSystem::emitExplosion(Vector2 pos, int count, Color color) {
    for (int i = 0; i < count; ++i) {
        float angle = randomFloat(0.0f, 6.2831853f);
        float speed = randomFloat(400.0f, 4000.0f);
        particles.push_back({
            pos,
            { std::cos(angle) * speed, std::sin(angle) * speed },
            color,
            randomFloat(0.4f, 1.2f),
            1.2f,
            randomFloat(3.0f, 8.0f),
            randomFloat(0.0f, 360.0f),
            randomFloat(-300.0f, 300.0f)
        });
    }
}

void ParticleSystem::emitDebris(Vector2 pos, int count, Color color) {
    for (int i = 0; i < count; ++i) {
        float angle = randomFloat(0.0f, 6.2831853f);
        float speed = randomFloat(150.0f, 600.0f);
        particles.push_back({
            pos,
            { std::cos(angle) * speed, std::sin(angle) * speed },
            color,
            randomFloat(0.2f, 0.5f),
            0.5f,
            randomFloat(2.0f, 5.0f),
            randomFloat(0.0f, 360.0f),
            randomFloat(-200.0f, 200.0f)
        });
    }
}

void ParticleSystem::updateAndDraw(float dt) {
    for (size_t i = 0; i < particles.size();) {
        Particle& p = particles[i];
        p.life -= dt;

        if (p.life <= 0.0f) {
            particles[i] = particles.back();
            particles.pop_back();
        } else {
            p.velocity.y += 800.0f * dt;
            p.velocity.x *= 0.94f;
            p.velocity.y *= 0.98f;

            p.position.x += p.velocity.x * dt;
            p.position.y += p.velocity.y * dt;
            p.rotation += p.rotSpeed * dt;

            float alpha = std::clamp(p.life / p.maxLife, 0.0f, 1.0f);
            Color drawCol = p.color;
            drawCol.a = static_cast<unsigned char>(alpha * 255.0f);

            Rectangle rect = { p.position.x, p.position.y, p.size, p.size };
            Vector2 origin = { p.size * 0.5f, p.size * 0.5f };
            DrawRectanglePro(rect, origin, p.rotation, drawCol);

            ++i;
        }
    }
}

void ParticleSystem::clear() {
    particles.clear();
}

} // namespace minesweeper::render
