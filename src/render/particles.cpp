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
        float speed = randomFloat(80.0f, 360.0f);
        float life = randomFloat(0.35f, 0.75f);
        particles.push_back({
            pos,
            { std::cos(angle) * speed, std::sin(angle) * speed - randomFloat(20.0f, 60.0f) },
            color,
            life,
            life,
            randomFloat(4.0f, 8.5f),
            randomFloat(0.0f, 360.0f),
            randomFloat(-200.0f, 200.0f)
        });
    }
}

void ParticleSystem::emitCellUncover(Vector2 pos, int count, Color color) {
    for (int i = 0; i < count; ++i) {
        float angle = randomFloat(0.0f, 6.2831853f);
        float speed = randomFloat(70.0f, 320.0f);
        Vector2 vel = {
            std::cos(angle) * speed,
            std::sin(angle) * speed - randomFloat(30.0f, 110.0f)
        };

        float life = randomFloat(0.5f, 0.95f);
        float size = randomFloat(5.5f, 12.0f);

        // High-visibility palette: sparkling white, electric cyan, bright silver zinc, and boosted cell tint
        Color pCol;
        int variant = rand() % 4;
        if (variant == 0) {
            pCol = Color{ 255, 255, 255, 255 }; // Pure white glint
        } else if (variant == 1) {
            pCol = Color{ 145, 242, 255, 255 }; // Electric neon cyan
        } else if (variant == 2) {
            pCol = Color{ 235, 240, 248, 255 }; // Crisp metallic silver
        } else {
            // Brightened version of the cell/number color
            pCol = Color{
                static_cast<unsigned char>(std::min(255, static_cast<int>(color.r) + 85)),
                static_cast<unsigned char>(std::min(255, static_cast<int>(color.g) + 85)),
                static_cast<unsigned char>(std::min(255, static_cast<int>(color.b) + 85)),
                255
            };
        }

        particles.push_back({
            pos,
            vel,
            pCol,
            life,
            life,
            size,
            randomFloat(0.0f, 360.0f),
            randomFloat(-260.0f, 260.0f)
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
            p.velocity.y += 480.0f * dt;
            p.velocity.x *= 0.94f;
            p.velocity.y *= 0.96f;

            p.position.x += p.velocity.x * dt;
            p.position.y += p.velocity.y * dt;
            p.rotation += p.rotSpeed * dt;

            float alpha = std::clamp(p.life / p.maxLife, 0.0f, 1.0f);
            Color drawCol = p.color;
            drawCol.a = static_cast<unsigned char>(alpha * 255.0f);

            Rectangle rect = { p.position.x, p.position.y, p.size * (particleTexture.id != 0 ? 1.4f : 1.0f), p.size * (particleTexture.id != 0 ? 1.4f : 1.0f) };
            Vector2 origin = { rect.width * 0.5f, rect.height * 0.5f };
            if (particleTexture.id != 0) {
                Rectangle src = { 0.0f, 0.0f, static_cast<float>(particleTexture.width), static_cast<float>(particleTexture.height) };
                DrawTexturePro(particleTexture, src, rect, origin, p.rotation, drawCol);
            } else {
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
