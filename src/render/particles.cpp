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
            randomFloat(-300.0f, 300.0f),
            false // textured glow ember
        });
    }
}

void ParticleSystem::emitDebris(Vector2 pos, int count, Color color) {
    for (int i = 0; i < count; ++i) {
        float angle = randomFloat(0.0f, 6.2831853f);
        float speed = randomFloat(80.0f, 320.0f);
        float life = randomFloat(0.30f, 0.60f);
        particles.push_back({
            pos,
            { std::cos(angle) * speed, std::sin(angle) * speed - randomFloat(25.0f, 75.0f) },
            color,
            life,
            life,
            randomFloat(3.0f, 7.5f),
            randomFloat(0.0f, 360.0f),
            randomFloat(-400.0f, 400.0f),
            true // solid square debris
        });
    }
}

void ParticleSystem::emitCellUncover(Vector2 pos, int count, Color /*color*/) {
    // Solid square gray debris palette: varying shades of rock/slate/metal tile rubble
    static const Color debrisGrays[] = {
        Color{ 180, 183, 190, 255 }, // Light stone gray
        Color{ 145, 148, 155, 255 }, // Medium slate gray
        Color{ 215, 218, 224, 255 }, // Bright chipped rock gray
        Color{ 110, 113, 120, 255 }, // Dark concrete gray
        Color{ 160, 163, 170, 255 }, // Neutral tile zinc gray
        Color{ 85,  88,  95,  255 }  // Deep granite gray
    };

    for (int i = 0; i < count; ++i) {
        float angle = randomFloat(0.0f, 6.2831853f);
        float speed = randomFloat(90.0f, 400.0f);
        Vector2 vel = {
            std::cos(angle) * speed,
            std::sin(angle) * speed - randomFloat(40.0f, 160.0f) // snappy upward/outward ejection
        };

        float life = randomFloat(0.35f, 0.70f);
        float size = randomFloat(3.5f, 9.0f);
        Color pCol = debrisGrays[rand() % 6];

        particles.push_back({
            pos,
            vel,
            pCol,
            life,
            life,
            size,
            randomFloat(0.0f, 360.0f),
            randomFloat(-500.0f, 500.0f), // fast tumbling angular velocity
            true // solid square shaped debris
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
            if (p.isSquare) {
                // Ballistic debris physics: heavy downward gravity arc and natural air drag
                p.velocity.y += 880.0f * dt;
                p.velocity.x *= 0.97f;
                p.velocity.y *= 0.98f;
            } else {
                // Soft particle physics (explosions, embers)
                p.velocity.y += 480.0f * dt;
                p.velocity.x *= 0.94f;
                p.velocity.y *= 0.96f;
            }

            p.position.x += p.velocity.x * dt;
            p.position.y += p.velocity.y * dt;
            p.rotation += p.rotSpeed * dt;

            // Keep solid opacity for majority of flight, then quickly fade upon landing
            float lifeFrac = p.life / p.maxLife;
            float alpha = p.isSquare ? std::clamp(lifeFrac * 3.33f, 0.0f, 1.0f) : std::clamp(lifeFrac, 0.0f, 1.0f);
            Color drawCol = p.color;
            drawCol.a = static_cast<unsigned char>(alpha * 255.0f);

            Rectangle rect = { p.position.x, p.position.y, p.size, p.size };
            Vector2 origin = { rect.width * 0.5f, rect.height * 0.5f };

            if (!p.isSquare && particleTexture.id != 0) {
                rect.width *= 1.4f;
                rect.height *= 1.4f;
                origin.x = rect.width * 0.5f;
                origin.y = rect.height * 0.5f;
                Rectangle src = { 0.0f, 0.0f, static_cast<float>(particleTexture.width), static_cast<float>(particleTexture.height) };
                DrawTexturePro(particleTexture, src, rect, origin, p.rotation, drawCol);
            } else {
                // Crisp, solid square quad
                DrawRectanglePro(rect, origin, p.rotation, drawCol);
            }

            ++i;
        }
    }
}

void ParticleSystem::clear() {
    particles.clear();
}

} // namespace minesweeper::render
