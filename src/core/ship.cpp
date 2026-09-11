#include "ship.hpp"
#include "ship_config.hpp"
#include "ui/theme.hpp"
#include <cmath>
#include <cstdlib>
#include <algorithm>

namespace minesweeper::core {

// ============================================================================
// Base Ship Implementation
// ============================================================================

Ship::Ship()
    : mass(8.0f)
    , range(150.0f)
    , speed(600.0f)
    , texture{ 0 }
    , skinId(0)
    , position{ 0.0f, 0.0f }
    , velocity{ 0.0f, 0.0f }
    , angle(0.0f)
    , collisionRadius(14.0f)
    , capsuleLength(0.0f)
    , scale(1.8f)
    , isMoving(false)
    , isInitialized(false)
    , emitTimer(0.0f)
    , color(WHITE)
    , isSpeaking(false)
{
}

Ship::Ship(float m, float r, float s, Texture2D tex, int skin)
    : mass(m)
    , range(r)
    , speed(s)
    , texture(tex)
    , skinId(skin)
    , position{ 0.0f, 0.0f }
    , velocity{ 0.0f, 0.0f }
    , angle(0.0f)
    , collisionRadius(14.0f)
    , capsuleLength(0.0f)
    , scale(1.8f)
    , isMoving(false)
    , isInitialized(false)
    , emitTimer(0.0f)
    , color(WHITE)
    , isSpeaking(false)
{
}

void Ship::addThruster(Vector2 offset, Vector2 direction, float width, float length, Color outer, Color inner) {
    thrusters.push_back({ offset, direction, width, length, outer, inner });
}

void Ship::applyConfig(const ShipConfig& cfg) {
    if (!cfg.name.empty()) {
        name = cfg.name;
    }
    if (cfg.scale > 0.0f) {
        scale = cfg.scale;
    }
    if (cfg.mass > 0.0f) {
        mass = cfg.mass;
    }
    if (cfg.speed > 0.0f) {
        speed = cfg.speed;
    }
    if (cfg.range > 0.0f) {
        range = cfg.range;
    }
    if (cfg.collisionRadius > 0.0f) {
        collisionRadius = cfg.collisionRadius;
    }
    if (cfg.capsuleLength >= 0.0f) {
        capsuleLength = cfg.capsuleLength;
    }
    thrusterColor = cfg.thrusterColor;
    bumpable = cfg.bumpable;

    if (!cfg.thrusters.empty()) {
        thrusters = cfg.thrusters;
        for (auto& th : thrusters) {
            th.outerColor = thrusterColor;
            th.innerColor = Color{
                static_cast<unsigned char>(std::min(255, thrusterColor.r + 50)),
                static_cast<unsigned char>(std::min(255, thrusterColor.g + 50)),
                static_cast<unsigned char>(std::min(255, thrusterColor.b + 50)),
                255
            };
        }
    }
}

void Ship::reset(Vector2 newPos, float newAngle) {
    position = newPos;
    velocity = { 0.0f, 0.0f };
    angle = newAngle;
    isMoving = false;
    isInitialized = true;
    exhaust.clear();
}

void Ship::update(float dt) {
    if (bumpTimer > 0.0f) {
        bumpTimer -= dt;
        if (bumpTimer < 0.0f) bumpTimer = 0.0f;
    }

    float curSpeed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
    if (curSpeed > 0.001f) {
        position.x += velocity.x * dt;
        position.y += velocity.y * dt;
        velocity.x *= std::max(0.0f, 1.0f - 3.0f * dt);
        velocity.y *= std::max(0.0f, 1.0f - 3.0f * dt);
        isMoving = (curSpeed > 10.0f);
    } else {
        isMoving = false;
    }

    if (isMoving) {
        emitThrusterParticles(dt, curSpeed / speed);
    }

    updateExhaust(dt);
}

void Ship::updateExhaust(float dt) {
    for (size_t i = 0; i < exhaust.size(); ) {
        exhaust[i].life -= dt;
        if (exhaust[i].life <= 0.0f) {
            exhaust[i] = exhaust.back();
            exhaust.pop_back();
        } else {
            exhaust[i].pos.x += exhaust[i].vel.x * dt;
            exhaust[i].pos.y += exhaust[i].vel.y * dt;
            exhaust[i].vel.x *= 0.94f;
            exhaust[i].vel.y *= 0.94f;
            ++i;
        }
    }
}

Texture2D Ship::sharedExhaustTexture = { 0 };
Texture2D Ship::sharedGlowTexture = { 0 };

void Ship::setSharedTextures(Texture2D exhaustTex, Texture2D glowTex) {
    sharedExhaustTexture = exhaustTex;
    sharedGlowTexture = glowTex;
}

void Ship::drawThrusters(Vector2 drawPos, float baseAngle) const {
    float theta = baseAngle * DEG2RAD;
    float cosA = std::cos(theta);
    float sinA = std::sin(theta);
    float t = static_cast<float>(GetTime());
    bool hasGlow = (sharedGlowTexture.id != 0);
    Rectangle gSrc = { 0.0f, 0.0f, static_cast<float>(sharedGlowTexture.width), static_cast<float>(sharedGlowTexture.height) };

    for (size_t i = 0; i < thrusters.size(); ++i) {
        const auto& th = thrusters[i];

        // 1. Calculate world nozzle position
        Vector2 worldNozzle = {
            drawPos.x + (th.offset.x * cosA - th.offset.y * sinA) * scale,
            drawPos.y + (th.offset.x * sinA + th.offset.y * cosA) * scale
        };

        // While idling: glowing circles / radial flares at the back of thrusters
        float idlePulse = 0.4f + 0.4f * std::sin(t * 5.0f + static_cast<float>(i) * 1.5f);
        float r = (th.nozzleWidth * 0.65f * scale) + idlePulse;
        float currSpeedNorm = Vector2Length(velocity) / (speed * 0.6f);
        float glowAlpha = std::clamp(1.0f - currSpeedNorm, 0.0f, 1.0f);

        if (glowAlpha > 0.01f) {
            if (hasGlow) {
                float gSize = r * 3.2f;
                Rectangle gDst = { worldNozzle.x, worldNozzle.y, gSize, gSize };
                Vector2 gOrig = { gSize * 0.5f, gSize * 0.5f };
                DrawTexturePro(sharedGlowTexture, gSrc, gDst, gOrig, 0.0f, Fade(Fade(th.outerColor, 0.95f), glowAlpha));
            } else {
                DrawCircleV(worldNozzle, r, Fade(Fade(th.outerColor, 0.85f), glowAlpha));
                DrawCircleV(worldNozzle, r * 0.5f, Fade(Fade(th.innerColor, 0.65f), glowAlpha));
            }
        }
    }
}

void Ship::emitThrusterParticles(float dt, float speedRatio) {
    if (thrusters.empty()) return;

    emitTimer += dt * std::clamp(speedRatio, 0.2f, 1.5f);
    float theta = angle * DEG2RAD;
    float cosA = std::cos(theta);
    float sinA = std::sin(theta);

    while (emitTimer >= 0.02f) {
        emitTimer -= 0.02f;
        if (exhaust.size() >= 160) break;

        for (const auto& th : thrusters) {
            Vector2 worldNozzle = {
                position.x + (th.offset.x * cosA - th.offset.y * sinA) * scale,
                position.y + (th.offset.x * sinA + th.offset.y * cosA) * scale
            };
            Vector2 worldDir = {
                th.direction.x * cosA - th.direction.y * sinA,
                th.direction.x * sinA + th.direction.y * cosA
            };
            Vector2 worldPerp = { -worldDir.y, worldDir.x };

            float pSpeed = 35.0f + static_cast<float>(rand() % 40);
            float spread = ((rand() % 100) - 50) * 0.005f;
            Vector2 pVel = {
                (worldDir.x + worldPerp.x * spread) * pSpeed,
                (worldDir.y + worldPerp.y * spread) * pSpeed
            };

            Color pCol = (rand() % 2 == 0) ? th.outerColor : th.innerColor;
            float sz = th.nozzleWidth * 1.3f;
            exhaust.push_back({ worldNozzle, pVel, 0.28f, 0.28f, sz, pCol });
        }
    }
}

void Ship::draw(const char* label, Color tint, bool speaking) const {
    bool drewTexture = false;
    if (texture.id != 0) {
        Rectangle src = { 0.0f, 0.0f, static_cast<float>(texture.width), static_cast<float>(texture.height) };
        float w = static_cast<float>(texture.width) * scale;
        float h = static_cast<float>(texture.height) * scale;
        Vector2 origin = { w * 0.5f, h * 0.5f };
        Rectangle dest = { position.x, position.y, w, h };

        drawThrusters(position, angle);
        DrawTexturePro(texture, src, dest, origin, angle, WHITE);
        drewTexture = true;
    }

    if (!drewTexture) {
        int px = static_cast<int>(position.x - 5.0f * scale);
        int py = static_cast<int>(position.y - 5.0f * scale);
        DrawRectangle(px + 4, py + 5, static_cast<int>(10 * scale), static_cast<int>(10 * scale), Fade(BLACK, 0.35f));
        DrawRectangle(px, py, static_cast<int>(10 * scale), static_cast<int>(10 * scale), tint);
        DrawRectangleLines(px, py, static_cast<int>(10 * scale), static_cast<int>(10 * scale), WHITE);
    }

    bool activeSpeaking = speaking || isSpeaking;
    if (activeSpeaking) {
        float t = static_cast<float>(GetTime());
        float pulse = (std::sin(t * 12.0f) + 1.0f) * 0.5f;
        Color speakCol = ui::Colors::Green500;
        DrawCircleLines(static_cast<int>(position.x), static_cast<int>(position.y), 15.0f * scale + pulse * 6.0f, Fade(speakCol, 0.85f));
        DrawCircleLines(static_cast<int>(position.x), static_cast<int>(position.y), 22.0f * scale + pulse * 9.0f, Fade(speakCol, 0.45f));
    }

    const char* displayName = (label && label[0] != '\0') ? label : (!name.empty() ? name.c_str() : nullptr);
    if (displayName && displayName[0] != '\0') {
        int nameW = MeasureText(displayName, 12);
        Rectangle badge = { position.x - static_cast<float>(nameW + (activeSpeaking ? 20 : 8)) * 0.5f, position.y + 16.0f * scale, static_cast<float>(nameW + (activeSpeaking ? 20 : 8)), 16.0f };
        DrawRectangleRec(badge, Fade(BLACK, 0.85f));
        DrawRectangleLinesEx(badge, 1.0f, activeSpeaking ? ui::Colors::Green500 : tint);
        if (activeSpeaking) {
            DrawCircle(static_cast<int>(badge.x + 6), static_cast<int>(badge.y + 8), 3.0f, ui::Colors::Green500);
            DrawText(displayName, static_cast<int>(badge.x + 14), static_cast<int>(badge.y + 2), 12, WHITE);
        } else {
            DrawText(displayName, static_cast<int>(badge.x + 4), static_cast<int>(badge.y + 2), 12, WHITE);
        }
    }
}

Vector2 Ship::getNosePosition() const {
    float theta = angle * DEG2RAD;
    float sinA = std::sin(theta);
    float cosA = std::cos(theta);
    return {
        position.x + sinA * (8.0f * scale),
        position.y - cosA * (8.0f * scale)
    };
}

void Ship::drawExhaust() const {
    for (const auto& p : exhaust) {
        float alpha = p.life / p.maxLife;
        Color c = p.color;
        c.a = static_cast<unsigned char>(alpha * 200.0f);
        float sz = p.size * (0.4f + 0.6f * alpha);
        DrawRectanglePro({ p.pos.x, p.pos.y, sz, sz }, { sz * 0.5f, sz * 0.5f }, 45.0f, c);
    }
}

void Ship::getCapsuleSegment(Vector2& outA, Vector2& outB) const {
    if (capsuleLength <= 0.001f) {
        outA = position;
        outB = position;
        return;
    }
    float theta = angle * DEG2RAD;
    float cosA = std::cos(theta);
    float sinA = std::sin(theta);
    float halfL = capsuleLength * 0.5f;
    outA = { position.x - cosA * halfL, position.y - sinA * halfL };
    outB = { position.x + cosA * halfL, position.y + sinA * halfL };
}

float Ship::segmentToSegmentDist(Vector2 p1, Vector2 q1, Vector2 p2, Vector2 q2, Vector2& outC1, Vector2& outC2) {
    Vector2 d1 = { q1.x - p1.x, q1.y - p1.y };
    Vector2 d2 = { q2.x - p2.x, q2.y - p2.y };
    Vector2 r  = { p1.x - p2.x, p1.y - p2.y };

    float a = d1.x * d1.x + d1.y * d1.y; // Squared length of segment S1
    float e = d2.x * d2.x + d2.y * d2.y; // Squared length of segment S2
    float f = d2.x * r.x + d2.y * r.y;

    float s = 0.0f;
    float t = 0.0f;

    const float eps = 1e-5f;

    if (a <= eps && e <= eps) {
        // Both segments degenerate into points
        s = 0.0f;
        t = 0.0f;
    } else if (a <= eps) {
        // S1 is a point
        s = 0.0f;
        t = std::clamp(f / e, 0.0f, 1.0f);
    } else {
        float c = d1.x * r.x + d1.y * r.y;
        if (e <= eps) {
            // S2 is a point
            t = 0.0f;
            s = std::clamp(-c / a, 0.0f, 1.0f);
        } else {
            // General case: both non-degenerate segments
            float b = d1.x * d2.x + d1.y * d2.y;
            float denom = a * e - b * b;

            if (denom > eps) {
                s = std::clamp((b * f - c * e) / denom, 0.0f, 1.0f);
            } else {
                s = 0.0f; // Parallel segments
            }

            t = (b * s + f) / e;

            if (t < 0.0f) {
                t = 0.0f;
                s = std::clamp(-c / a, 0.0f, 1.0f);
            } else if (t > 1.0f) {
                t = 1.0f;
                s = std::clamp((b - c) / a, 0.0f, 1.0f);
            }
        }
    }

    outC1 = { p1.x + d1.x * s, p1.y + d1.y * s };
    outC2 = { p2.x + d2.x * t, p2.y + d2.y * t };

    Vector2 diff = { outC2.x - outC1.x, outC2.y - outC1.y };
    return std::sqrt(diff.x * diff.x + diff.y * diff.y);
}

bool Ship::resolveCollision(Ship& a, Ship& b, float restitution) {
    if (!a.isInitialized || !b.isInitialized) {
        return false;
    }

    Vector2 segA1, segA2;
    Vector2 segB1, segB2;
    a.getCapsuleSegment(segA1, segA2);
    b.getCapsuleSegment(segB1, segB2);

    Vector2 cA, cB;
    float dist = segmentToSegmentDist(segA1, segA2, segB1, segB2, cA, cB);
    float minDist = a.collisionRadius + b.collisionRadius;

    if (dist >= minDist || minDist <= 0.0001f) {
        return false;
    }

    Vector2 normal;
    if (dist > 0.0001f) {
        normal = { (cB.x - cA.x) / dist, (cB.y - cA.y) / dist };
    } else {
        Vector2 posDelta = { b.position.x - a.position.x, b.position.y - a.position.y };
        float pDist = std::sqrt(posDelta.x * posDelta.x + posDelta.y * posDelta.y);
        if (pDist > 0.0001f) {
            normal = { posDelta.x / pDist, posDelta.y / pDist };
        } else {
            normal = { 1.0f, 0.0f };
        }
        dist = 0.0001f;
    }

    float overlap = minDist - dist;

    // Check if both are bumper-enabled (player vs player)
    bool isBumper = (a.bumpable && b.bumpable);

    if (isBumper) {
        // ====================================================================
        // Bumper Collision (Player vs Player: bouncy, twitch, sparks)
        // ====================================================================
        float invMassA = (a.mass > 0.0001f) ? (1.0f / a.mass) : 0.0f;
        float invMassB = (b.mass > 0.0001f) ? (1.0f / b.mass) : 0.0f;
        float invMassSum = invMassA + invMassB;
        if (invMassSum <= 0.0001f) return false;

        const float slop = 0.5f;
        const float percent = 0.85f;
        float penetration = std::max(0.0f, overlap - slop);
        float sep = penetration * percent;

        a.position.x -= normal.x * sep * (invMassA / invMassSum);
        a.position.y -= normal.y * sep * (invMassA / invMassSum);
        b.position.x += normal.x * sep * (invMassB / invMassSum);
        b.position.y += normal.y * sep * (invMassB / invMassSum);

        Vector2 relVel = { b.velocity.x - a.velocity.x, b.velocity.y - a.velocity.y };
        float velAlongNormal = relVel.x * normal.x + relVel.y * normal.y;

        if (velAlongNormal < 0.0f) {
            const float minBumpSpeed = 160.0f;
            const float maxBumpSpeed = 460.0f;
            float desiredSepSpeed = std::clamp(-velAlongNormal * restitution, minBumpSpeed, maxBumpSpeed);
            float impulseScalar = (-velAlongNormal + desiredSepSpeed) / invMassSum;

            Vector2 impulse = { normal.x * impulseScalar, normal.y * impulseScalar };
            a.velocity.x -= impulse.x * invMassA;
            a.velocity.y -= impulse.y * invMassA;
            b.velocity.x += impulse.x * invMassB;
            b.velocity.y += impulse.y * invMassB;
        } else if (overlap > 1.5f && velAlongNormal < 25.0f) {
            float nudgeSpeed = std::min(40.0f, overlap * 25.0f);
            float nudgeImpulse = nudgeSpeed / invMassSum;
            Vector2 impulse = { normal.x * nudgeImpulse, normal.y * nudgeImpulse };
            a.velocity.x -= impulse.x * invMassA;
            a.velocity.y -= impulse.y * invMassA;
            b.velocity.x += impulse.x * invMassB;
            b.velocity.y += impulse.y * invMassB;
        }

        const float maxAllowedPlayerSpeed = 650.0f;
        float spdA = std::sqrt(a.velocity.x * a.velocity.x + a.velocity.y * a.velocity.y);
        float ceilA = std::max(a.speed, maxAllowedPlayerSpeed);
        if (spdA > ceilA) {
            a.velocity.x = (a.velocity.x / spdA) * ceilA;
            a.velocity.y = (a.velocity.y / spdA) * ceilA;
        }
        a.bumpTimer = 0.35f;
        float torqueA = (-normal.y * relVel.x + normal.x * relVel.y) * 0.08f;
        a.angle += std::clamp(torqueA, -12.0f, 12.0f);

        float spdB = std::sqrt(b.velocity.x * b.velocity.x + b.velocity.y * b.velocity.y);
        float ceilB = std::max(b.speed, maxAllowedPlayerSpeed);
        if (spdB > ceilB) {
            b.velocity.x = (b.velocity.x / spdB) * ceilB;
            b.velocity.y = (b.velocity.y / spdB) * ceilB;
        }
        b.bumpTimer = 0.35f;
        float torqueB = (normal.y * relVel.x - normal.x * relVel.y) * 0.08f;
        b.angle += std::clamp(torqueB, -12.0f, 12.0f);

        return true;
    } else {
        // ====================================================================
        // Push Collision (Shop Ships: pushable, but NOT bumpable)
        // ====================================================================
        // Pushing force and displacement are directly proportional to ship mass
        float invMassA = (a.mass > 0.0001f) ? (1.0f / a.mass) : 0.0f;
        float invMassB = (b.mass > 0.0001f) ? (1.0f / b.mass) : 0.0f;
        float invMassSum = invMassA + invMassB;
        if (invMassSum <= 0.0001f) return false;

        const float slop = 0.2f;
        float penetration = std::max(0.0f, overlap - slop);
        float sep = penetration * 0.90f;

        // Positional separation directly proportional to the pushing mass:
        // Ship A displacement is proportional to b.mass (invMassA / invMassSum = mB / (mA + mB))
        // Ship B displacement is proportional to a.mass (invMassB / invMassSum = mA / (mA + mB))
        float sepFractionA = invMassA / invMassSum;
        float sepFractionB = invMassB / invMassSum;

        a.position.x -= normal.x * sep * sepFractionA;
        a.position.y -= normal.y * sep * sepFractionA;
        b.position.x += normal.x * sep * sepFractionB;
        b.position.y += normal.y * sep * sepFractionB;

        // Mass-proportional push force factors (normalized so equal mass = 1.0f)
        float totalMass = a.mass + b.mass;
        float pushScaleB = (totalMass > 0.0001f) ? ((2.0f * a.mass) / totalMass) : 1.0f;
        float pushScaleA = (totalMass > 0.0001f) ? ((2.0f * b.mass) / totalMass) : 1.0f;

        // Velocity push transfer (inelastic push, NO rebound/bumper kick)
        Vector2 relVel = { b.velocity.x - a.velocity.x, b.velocity.y - a.velocity.y };
        float velAlongNormal = relVel.x * normal.x + relVel.y * normal.y;

        if (velAlongNormal < 0.0f) {
            float closingSpeed = -velAlongNormal;
            float basePush = std::clamp(closingSpeed * 0.65f + 18.0f, 10.0f, 200.0f);

            if (a.bumpable && !b.bumpable) {
                // a (player) pushes b (shop): pushing speed directly proportional to a.mass
                float pushSpeed = std::clamp(basePush * pushScaleB, 2.0f, 200.0f);
                b.velocity.x += normal.x * pushSpeed;
                b.velocity.y += normal.y * pushSpeed;

                // Player velocity into the shop is damped proportional to the obstacle's mass
                float dampFactor = std::clamp(0.80f * (pushScaleA * 0.5f), 0.20f, 1.0f);
                a.velocity.x -= normal.x * (closingSpeed * dampFactor);
                a.velocity.y -= normal.y * (closingSpeed * dampFactor);
            } else if (!a.bumpable && b.bumpable) {
                // b (player) pushes a (shop): pushing speed directly proportional to b.mass
                float pushSpeed = std::clamp(basePush * pushScaleA, 2.0f, 200.0f);
                a.velocity.x -= normal.x * pushSpeed;
                a.velocity.y -= normal.y * pushSpeed;

                float dampFactor = std::clamp(0.80f * (pushScaleB * 0.5f), 0.20f, 1.0f);
                b.velocity.x += normal.x * (closingSpeed * dampFactor);
                b.velocity.y += normal.y * (closingSpeed * dampFactor);
            } else {
                // Both are non-bumpable (e.g. shop vs shop): push speeds directly proportional to each other's mass
                float pushSpeedB = std::clamp(basePush * 0.5f * pushScaleB, 2.0f, 160.0f);
                float pushSpeedA = std::clamp(basePush * 0.5f * pushScaleA, 2.0f, 160.0f);
                a.velocity.x -= normal.x * pushSpeedA;
                a.velocity.y -= normal.y * pushSpeedA;
                b.velocity.x += normal.x * pushSpeedB;
                b.velocity.y += normal.y * pushSpeedB;
            }
        } else if (overlap > 0.8f) {
            // Steady pushing when ship presses continuously against another ship
            // Pushing acceleration is directly proportional to pusher mass / target mass
            float baseSteady = std::min(25.0f, overlap * 12.0f);
            if (a.bumpable && !b.bumpable) {
                float forceRatio = std::clamp(a.mass / std::max(0.1f, b.mass), 0.02f, 5.0f);
                float steadyPush = baseSteady * forceRatio;
                b.velocity.x += normal.x * steadyPush;
                b.velocity.y += normal.y * steadyPush;
            } else if (!a.bumpable && b.bumpable) {
                float forceRatio = std::clamp(b.mass / std::max(0.1f, a.mass), 0.02f, 5.0f);
                float steadyPush = baseSteady * forceRatio;
                a.velocity.x -= normal.x * steadyPush;
                a.velocity.y -= normal.y * steadyPush;
            } else {
                float forceRatioB = std::clamp(a.mass / std::max(0.1f, b.mass), 0.02f, 5.0f);
                float forceRatioA = std::clamp(b.mass / std::max(0.1f, a.mass), 0.02f, 5.0f);
                a.velocity.x -= normal.x * (baseSteady * forceRatioA * 0.5f);
                a.velocity.y -= normal.y * (baseSteady * forceRatioA * 0.5f);
                b.velocity.x += normal.x * (baseSteady * forceRatioB * 0.5f);
                b.velocity.y += normal.y * (baseSteady * forceRatioB * 0.5f);
            }
        }

        // Cap shop speed safely
        const float maxShopPushSpeed = 200.0f;
        if (!a.bumpable) {
            float spd = std::sqrt(a.velocity.x * a.velocity.x + a.velocity.y * a.velocity.y);
            if (spd > maxShopPushSpeed) {
                a.velocity.x = (a.velocity.x / spd) * maxShopPushSpeed;
                a.velocity.y = (a.velocity.y / spd) * maxShopPushSpeed;
            }
        }
        if (!b.bumpable) {
            float spd = std::sqrt(b.velocity.x * b.velocity.x + b.velocity.y * b.velocity.y);
            if (spd > maxShopPushSpeed) {
                b.velocity.x = (b.velocity.x / spd) * maxShopPushSpeed;
                b.velocity.y = (b.velocity.y / spd) * maxShopPushSpeed;
            }
        }

        // Neither ship gets bumpTimer set -> NO recoil twitch, NO flaring, NO bouncing away!
        return true;
    }
}

} // namespace minesweeper::core
