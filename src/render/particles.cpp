#include "particles.hpp"
#include <cmath>
#include <cstdlib>
#include <algorithm>

#ifndef DEG2RAD
#define DEG2RAD 0.017453292519943295f
#endif

namespace minesweeper::render {

ParticleSystem::ParticleSystem(size_t reserveCount) {
    particles.reserve(reserveCount);
}

float ParticleSystem::randomFloat(float min, float max) {
    return min + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * (max - min);
}

void ParticleSystem::emitDebris(Vector2 pos, int count, Color color) {
    for (int i = 0; i < count; ++i) {
        float angle = randomFloat(0.0f, 6.2831853f);
        float speed = randomFloat(120.0f, 520.0f);
        particles.push_back({
            pos,
            { std::cos(angle) * speed, std::sin(angle) * speed },
            color,
            randomFloat(0.25f, 0.65f),
            0.65f,
            randomFloat(2.5f, 6.0f),
            randomFloat(0.0f, 360.0f),
            randomFloat(-250.0f, 250.0f),
            ParticleType::Debris,
            0.0f
        });
    }
}

void ParticleSystem::emitSparks(Vector2 pos, int count, Color color, float speedMin, float speedMax) {
    for (int i = 0; i < count; ++i) {
        float angle = randomFloat(0.0f, 6.2831853f);
        float speed = randomFloat(speedMin, speedMax);
        float life = randomFloat(0.12f, 0.40f);
        particles.push_back({
            pos,
            { std::cos(angle) * speed, std::sin(angle) * speed },
            color,
            life,
            life,
            randomFloat(1.5f, 3.2f),
            0.0f,
            0.0f,
            ParticleType::Spark,
            0.0f
        });
    }
}

void ParticleSystem::emitPlasmaMotes(Vector2 pos, int count, Color color, float speedMin, float speedMax) {
    for (int i = 0; i < count; ++i) {
        float angle = randomFloat(0.0f, 6.2831853f);
        float speed = randomFloat(speedMin, speedMax);
        float life = randomFloat(0.40f, 1.10f);
        particles.push_back({
            pos,
            { std::cos(angle) * speed, std::sin(angle) * speed },
            color,
            life,
            life,
            randomFloat(3.0f, 7.5f),
            0.0f,
            0.0f,
            ParticleType::PlasmaMote,
            randomFloat(0.0f, 6.28f)
        });
    }
}

void ParticleSystem::emitSparkles(Vector2 pos, int count, Color color, float spreadRadius) {
    for (int i = 0; i < count; ++i) {
        float offsetAngle = randomFloat(0.0f, 6.2831853f);
        float offsetDist = randomFloat(0.0f, spreadRadius);
        Vector2 spawnPos = { pos.x + std::cos(offsetAngle) * offsetDist, pos.y + std::sin(offsetAngle) * offsetDist };

        float driftAngle = randomFloat(0.0f, 6.2831853f);
        float driftSpeed = randomFloat(10.0f, 40.0f);
        float life = randomFloat(0.35f, 0.85f);
        particles.push_back({
            spawnPos,
            { std::cos(driftAngle) * driftSpeed, std::sin(driftAngle) * driftSpeed },
            color,
            life,
            life,
            randomFloat(3.0f, 6.5f),
            randomFloat(0.0f, 90.0f),
            randomFloat(-120.0f, 120.0f),
            ParticleType::Sparkle,
            randomFloat(0.0f, 6.28f)
        });
    }
}

void ParticleSystem::emitSmoke(Vector2 pos, int count, Color color, float speedMin, float speedMax) {
    for (int i = 0; i < count; ++i) {
        float angle = randomFloat(0.0f, 6.2831853f);
        float speed = randomFloat(speedMin, speedMax);
        float life = randomFloat(0.50f, 1.25f);
        particles.push_back({
            pos,
            { std::cos(angle) * speed, std::sin(angle) * speed },
            color,
            life,
            life,
            randomFloat(4.0f, 10.0f),
            randomFloat(0.0f, 360.0f),
            randomFloat(-40.0f, 40.0f),
            ParticleType::Smoke,
            0.0f
        });
    }
}

void ParticleSystem::emitShockwave(Vector2 pos, float maxRadius, Color color, float duration) {
    particles.push_back({
        pos,
        { 0.0f, 0.0f },
        color,
        duration,
        duration,
        2.0f,
        0.0f,
        0.0f,
        ParticleType::Shockwave,
        maxRadius
    });
}

void ParticleSystem::emitExplosion(Vector2 pos, int count, Color color) {
    int sparkCount = std::max(6, count * 4 / 10);
    int debrisCount = std::max(4, count * 3 / 10);
    int moteCount = std::max(3, count * 3 / 10);

    emitSparks(pos, sparkCount, color, 250.0f, 900.0f);
    emitDebris(pos, debrisCount, color);
    emitPlasmaMotes(pos, moteCount, color, 30.0f, 200.0f);
    emitShockwave(pos, 55.0f + static_cast<float>(count) * 0.4f, color, 0.32f);
}

void ParticleSystem::emitDefusalFX(Vector2 pos, float cellSize) {
    // 1. Electric sparks arcing outward
    emitSparks(pos, 10, Color{ 103, 232, 249, 255 }, 160.0f, 550.0f); // Cyan300
    // 2. Soft glowing radiant plasma motes
    emitPlasmaMotes(pos, 6, Color{ 34, 211, 238, 255 }, 20.0f, 110.0f); // Cyan400
    emitPlasmaMotes(pos, 4, Color{ 251, 191, 36, 255 }, 15.0f, 85.0f);  // Amber400
    // 3. Venting neutralizing coolant vapor
    emitSmoke(pos, 3, Color{ 148, 163, 184, 180 }, 10.0f, 40.0f);      // Coolant steam
    // 4. Sparkling star glints
    emitSparkles(pos, 4, Color{ 253, 224, 71, 255 }, cellSize * 0.4f);  // Twinkling glints
    // 5. Expanding energy ring
    emitShockwave(pos, cellSize * 0.85f, Color{ 56, 189, 248, 255 }, 0.28f);
}

void ParticleSystem::emitWarpSpawnFX(Vector2 pos, Color themeColor) {
    emitShockwave(pos, 65.0f, themeColor, 0.35f);
    emitShockwave(pos, 35.0f, WHITE, 0.20f);
    emitPlasmaMotes(pos, 14, themeColor, 40.0f, 190.0f);
    emitPlasmaMotes(pos, 6, WHITE, 25.0f, 130.0f);
    emitSparks(pos, 16, themeColor, 220.0f, 750.0f);
    emitSparkles(pos, 8, themeColor, 40.0f);
}

void ParticleSystem::updateAndDraw(float dt) {
    for (size_t i = 0; i < particles.size();) {
        Particle& p = particles[i];
        p.life -= dt;

        if (p.life <= 0.0f) {
            particles[i] = particles.back();
            particles.pop_back();
            continue;
        }

        float alpha = std::clamp(p.life / p.maxLife, 0.0f, 1.0f);

        switch (p.type) {
        case ParticleType::Debris: {
            p.velocity.x *= 0.94f;
            p.velocity.y *= 0.94f;
            p.position.x += p.velocity.x * dt;
            p.position.y += p.velocity.y * dt;
            p.rotation += p.rotSpeed * dt;

            Color drawCol = p.color;
            drawCol.a = static_cast<unsigned char>(alpha * 255.0f);
            Rectangle rect = { p.position.x, p.position.y, p.size, p.size * 0.7f };
            Vector2 origin = { p.size * 0.5f, p.size * 0.35f };
            DrawRectanglePro(rect, origin, p.rotation, drawCol);
            break;
        }

        case ParticleType::Spark: {
            p.velocity.x *= 0.91f;
            p.velocity.y *= 0.91f;
            p.position.x += p.velocity.x * dt;
            p.position.y += p.velocity.y * dt;

            Color drawCol = p.color;
            drawCol.a = static_cast<unsigned char>(alpha * 255.0f);

            float speedSq = p.velocity.x * p.velocity.x + p.velocity.y * p.velocity.y;
            if (speedSq > 64.0f) {
                float speed = std::sqrt(speedSq);
                float streakLen = std::clamp(speed * 0.035f, 2.0f, 20.0f);
                Vector2 tail = { p.position.x - (p.velocity.x / speed) * streakLen,
                                 p.position.y - (p.velocity.y / speed) * streakLen };
                DrawLineEx(p.position, tail, p.size, drawCol);
            }
            DrawCircleV(p.position, p.size * 0.7f, Fade(WHITE, alpha * 0.9f));
            break;
        }

        case ParticleType::PlasmaMote: {
            p.velocity.x *= 0.91f;
            p.velocity.y *= 0.91f;
            p.position.x += p.velocity.x * dt;
            p.position.y += p.velocity.y * dt;
            p.extra += dt * 4.5f;

            float pulse = 0.85f + 0.15f * std::sin(p.extra);
            float radius = p.size * pulse;

            // Radiant soft outer halo
            DrawCircleV(p.position, radius * 2.2f, Fade(p.color, alpha * 0.22f));
            // Mid glow
            DrawCircleV(p.position, radius * 1.3f, Fade(p.color, alpha * 0.55f));
            // Bright hot core
            DrawCircleV(p.position, radius * 0.55f, Fade(WHITE, alpha * 0.92f));
            break;
        }

        case ParticleType::Sparkle: {
            p.velocity.x *= 0.88f;
            p.velocity.y *= 0.88f;
            p.position.x += p.velocity.x * dt;
            p.position.y += p.velocity.y * dt;
            p.rotation += p.rotSpeed * dt;
            p.extra += dt * 6.5f;

            float twinkle = 0.65f + 0.35f * std::sin(p.extra);
            float armLen = p.size * 1.75f * twinkle;
            float thickness = std::max(1.0f, p.size * 0.35f);

            float rad = p.rotation * DEG2RAD;
            Vector2 dirH = { std::cos(rad), std::sin(rad) };
            Vector2 dirV = { -std::sin(rad), std::cos(rad) };

            Color armCol = Fade(p.color, alpha * 0.80f);
            Color coreCol = Fade(WHITE, alpha * 0.95f);

            DrawLineEx({ p.position.x - dirH.x * armLen, p.position.y - dirH.y * armLen },
                       { p.position.x + dirH.x * armLen, p.position.y + dirH.y * armLen }, thickness, armCol);
            DrawLineEx({ p.position.x - dirV.x * armLen, p.position.y - dirV.y * armLen },
                       { p.position.x + dirV.x * armLen, p.position.y + dirV.y * armLen }, thickness, armCol);
            DrawCircleV(p.position, p.size * 0.5f * twinkle, coreCol);
            break;
        }

        case ParticleType::Smoke: {
            p.velocity.x *= 0.93f;
            p.velocity.y *= 0.93f;
            p.velocity.y -= 12.0f * dt;
            p.position.x += p.velocity.x * dt;
            p.position.y += p.velocity.y * dt;
            p.rotation += p.rotSpeed * dt;

            float progress = 1.0f - alpha;
            float currentRadius = p.size * (1.0f + 1.8f * progress);
            float smokeAlpha = alpha * 0.35f;

            DrawCircleV(p.position, currentRadius, Fade(p.color, smokeAlpha));
            float rad = p.rotation * DEG2RAD;
            Vector2 offset = { std::cos(rad) * currentRadius * 0.35f, std::sin(rad) * currentRadius * 0.35f };
            DrawCircleV({ p.position.x + offset.x, p.position.y + offset.y }, currentRadius * 0.75f, Fade(p.color, smokeAlpha * 0.7f));
            break;
        }

        case ParticleType::Shockwave: {
            p.size += (p.extra / p.maxLife) * dt;
            float ringAlpha = alpha * 0.85f;
            DrawCircleLines(static_cast<int>(p.position.x), static_cast<int>(p.position.y), p.size, Fade(p.color, ringAlpha));
            if (p.size > 4.0f) {
                DrawCircleLines(static_cast<int>(p.position.x), static_cast<int>(p.position.y), p.size - 1.5f, Fade(p.color, ringAlpha * 0.5f));
            }
            break;
        }
        }

        ++i;
    }
}

void ParticleSystem::clear() {
    particles.clear();
}

} // namespace minesweeper::render
