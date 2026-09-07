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
    : mass(1.0f)
    , range(150.0f)
    , speed(600.0f)
    , texture{ 0 }
    , skinId(0)
    , position{ 0.0f, 0.0f }
    , velocity{ 0.0f, 0.0f }
    , angle(0.0f)
    , collisionRadius(14.0f)
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
    thrusterColor = cfg.thrusterColor;

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

void Ship::drawThrusters(Vector2 drawPos, float baseAngle) const {
    if (isMoving) {
        // Triangles removed as requested: only particle embers stream when moving (rendered in drawExhaust)
        return;
    }

    float theta = baseAngle * DEG2RAD;
    float cosA = std::cos(theta);
    float sinA = std::sin(theta);
    float t = static_cast<float>(GetTime());

    for (size_t i = 0; i < thrusters.size(); ++i) {
        const auto& th = thrusters[i];

        // 1. Calculate world nozzle position
        Vector2 worldNozzle = {
            drawPos.x + (th.offset.x * cosA - th.offset.y * sinA) * scale,
            drawPos.y + (th.offset.x * sinA + th.offset.y * cosA) * scale
        };

        // While idling: glowing circles at the back of thrusters
        float idlePulse = 0.4f + 0.4f * std::sin(t * 5.0f + static_cast<float>(i) * 1.5f);
        float r = (th.nozzleWidth * 0.65f * scale) + idlePulse;
        DrawCircleV(worldNozzle, r, Fade(th.outerColor, 0.85f));
        DrawCircleV(worldNozzle, r * 0.5f, Fade(th.innerColor, 0.65f));
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

bool Ship::resolveCollision(Ship& a, Ship& b, float restitution) {
    Vector2 delta = { b.position.x - a.position.x, b.position.y - a.position.y };
    float distSq = delta.x * delta.x + delta.y * delta.y;
    float minDist = a.collisionRadius + b.collisionRadius;

    if (distSq >= minDist * minDist || minDist <= 0.0001f) {
        return false;
    }

    float dist = std::sqrt(distSq);
    Vector2 normal;
    if (dist > 0.0001f) {
        normal = { delta.x / dist, delta.y / dist };
    } else {
        normal = { 1.0f, 0.0f };
        dist = 0.0001f;
    }

    float overlap = minDist - dist;

    // 1. Positional Separation with Baumgarte stabilization & slop threshold
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

    // 2. Velocity Impulse Exchange with Controlled Bumper Dynamics
    Vector2 relVel = { b.velocity.x - a.velocity.x, b.velocity.y - a.velocity.y };
    float velAlongNormal = relVel.x * normal.x + relVel.y * normal.y;

    if (velAlongNormal < 0.0f) {
        // Controlled bumper: enforce crisp minimum separation speed and cap maximum separation speed
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
        // Soft separation nudge for persistent overlaps, strictly bounded
        float nudgeSpeed = std::min(40.0f, overlap * 25.0f);
        float nudgeImpulse = nudgeSpeed / invMassSum;
        Vector2 impulse = { normal.x * nudgeImpulse, normal.y * nudgeImpulse };
        a.velocity.x -= impulse.x * invMassA;
        a.velocity.y -= impulse.y * invMassA;
        b.velocity.x += impulse.x * invMassB;
        b.velocity.y += impulse.y * invMassB;
    }

    // 3. Clamp maximum post-collision velocity so ships NEVER blast off to deep space
    const float maxAllowedPlayerSpeed = 650.0f;
    float spdA = std::sqrt(a.velocity.x * a.velocity.x + a.velocity.y * a.velocity.y);
    float ceilA = std::max(a.speed, maxAllowedPlayerSpeed);
    if (spdA > ceilA) {
        a.velocity.x = (a.velocity.x / spdA) * ceilA;
        a.velocity.y = (a.velocity.y / spdA) * ceilA;
    }
    float spdB = std::sqrt(b.velocity.x * b.velocity.x + b.velocity.y * b.velocity.y);
    float ceilB = std::max(b.speed, maxAllowedPlayerSpeed);
    if (spdB > ceilB) {
        b.velocity.x = (b.velocity.x / spdB) * ceilB;
        b.velocity.y = (b.velocity.y / spdB) * ceilB;
    }

    // 4. Mark bump recoil timers and give satisfying angular impact twitch
    a.bumpTimer = 0.35f;
    b.bumpTimer = 0.35f;

    float torqueA = (-normal.y * relVel.x + normal.x * relVel.y) * 0.08f;
    a.angle += std::clamp(torqueA, -12.0f, 12.0f);
    float torqueB = (normal.y * relVel.x - normal.x * relVel.y) * 0.08f;
    b.angle += std::clamp(torqueB, -12.0f, 12.0f);

    return true;
}

// ============================================================================
// Scout / Player Ship Implementation
// ============================================================================

ScoutShip::ScoutShip()
    : Ship(1.0f, 150.0f, 600.0f)
{
    scale = 1.8f;
    collisionRadius = 14.0f;
    // Dual plasma thrusters: offset {-3.5f, 4.5f} and {+3.5f, 4.5f}, pointing rearwards {0.0f, 1.0f}
    addThruster({ -3.5f, 4.5f }, { 0.0f, 1.0f }, 3.2f, 5.0f, ui::Colors::Orange500, ui::Colors::Amber300);
    addThruster({  3.5f, 4.5f }, { 0.0f, 1.0f }, 3.2f, 5.0f, ui::Colors::Orange500, ui::Colors::Amber300);
}

ScoutShip::ScoutShip(float m, float r, float s, Texture2D tex, int skin)
    : Ship(m, r, s, tex, skin)
{
    scale = 1.8f;
    collisionRadius = 14.0f;
    addThruster({ -3.5f, 4.5f }, { 0.0f, 1.0f }, 3.2f, 5.0f, ui::Colors::Orange500, ui::Colors::Amber300);
    addThruster({  3.5f, 4.5f }, { 0.0f, 1.0f }, 3.2f, 5.0f, ui::Colors::Orange500, ui::Colors::Amber300);
}

void ScoutShip::update(float dt) {
    update(targetPosition, dt);
}

void ScoutShip::update(Vector2 targetPos, float dt) {
    targetPosition = targetPos;
    if (!isInitialized) {
        position = { targetPos.x - 50.0f, targetPos.y };
        velocity = { 0.0f, 0.0f };
        angle = 0.0f;
        isInitialized = true;
        exhaust.clear();
    }

    if (bumpTimer > 0.0f) {
        bumpTimer -= dt;
        if (bumpTimer < 0.0f) bumpTimer = 0.0f;
    }

    Vector2 toTarget = { targetPos.x - position.x, targetPos.y - position.y };
    float distToTarget = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y);

    const float targetDist = targetFollowDistance; // 70px
    const float baseMaxSpeed = speed;

    Vector2 idealPos;
    if (distToTarget > 0.001f) {
        Vector2 dirFromTarget = { -toTarget.x / distToTarget, -toTarget.y / distToTarget };
        idealPos = { targetPos.x + dirFromTarget.x * targetDist, targetPos.y + dirFromTarget.y * targetDist };
    } else {
        idealPos = { targetPos.x - targetDist, targetPos.y };
    }

    Vector2 toIdeal = { idealPos.x - position.x, idealPos.y - position.y };
    float distToIdeal = std::sqrt(toIdeal.x * toIdeal.x + toIdeal.y * toIdeal.y);

    // Adaptive return speed and acceleration when knocked away by bumper collisions
    float currentMaxSpeed = baseMaxSpeed;
    float currentMaxAccel = maxAccel;
    float desiredSpeed = 0.0f;

    if (distToIdeal > slowRadius) {
        // Boost return speed and acceleration to quickly recover from bumpers (recovering in ~1.0-1.5s)
        float excessDist = distToIdeal - slowRadius;
        float speedBoost = std::min(baseMaxSpeed * 0.6f, excessDist * 1.5f);
        currentMaxSpeed = baseMaxSpeed + speedBoost;
        currentMaxAccel = maxAccel * (1.0f + std::min(1.8f, excessDist / 100.0f));
        desiredSpeed = currentMaxSpeed;
    } else if (distToIdeal > 0.5f) {
        float t = distToIdeal / slowRadius;
        desiredSpeed = currentMaxSpeed * (t * (2.0f - t));
    }

    Vector2 desiredVel = { 0.0f, 0.0f };
    if (distToIdeal > 0.001f && desiredSpeed > 0.0f) {
        desiredVel = { (toIdeal.x / distToIdeal) * desiredSpeed, (toIdeal.y / distToIdeal) * desiredSpeed };
    }

    // Directional counter-braking drag: if velocity opposes target direction, rapidly brake the outward recoil
    if (distToIdeal > 8.0f) {
        float normToX = toIdeal.x / distToIdeal;
        float normToY = toIdeal.y / distToIdeal;
        float velAlongToIdeal = velocity.x * normToX + velocity.y * normToY;
        if (velAlongToIdeal < 0.0f) {
            float brakeFactor = std::max(0.0f, 1.0f - 6.0f * dt);
            velocity.x *= brakeFactor;
            velocity.y *= brakeFactor;
        }
    }

    // Rapidly bleed off excess bump speed above maximum cruise speed
    float preCurSpeed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
    if (preCurSpeed > currentMaxSpeed) {
        float dragFactor = std::max(0.0f, 1.0f - 7.0f * dt);
        velocity.x *= dragFactor;
        velocity.y *= dragFactor;
    }

    Vector2 accel = { (desiredVel.x - velocity.x) * 12.0f, (desiredVel.y - velocity.y) * 12.0f };
    float accelMag = std::sqrt(accel.x * accel.x + accel.y * accel.y);
    if (accelMag > currentMaxAccel) {
        accel.x = (accel.x / accelMag) * currentMaxAccel;
        accel.y = (accel.y / accelMag) * currentMaxAccel;
    }

    velocity.x += accel.x * dt;
    velocity.y += accel.y * dt;

    float curSpeed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
    if (distToIdeal < 1.0f && curSpeed < 8.0f) {
        velocity = { 0.0f, 0.0f };
        position = idealPos;
    } else {
        position.x += velocity.x * dt;
        position.y += velocity.y * dt;
    }

    isMoving = (curSpeed > 20.0f);

    if (distToTarget > 0.001f) {
        float dirX = toTarget.x / distToTarget;
        float dirY = toTarget.y / distToTarget;
        float desiredAngle = std::atan2(dirY, dirX) * RAD2DEG + 90.0f;
        float diffAngle = desiredAngle - angle;
        while (diffAngle < -180.0f) diffAngle += 360.0f;
        while (diffAngle > 180.0f) diffAngle -= 360.0f;
        angle += diffAngle * std::min(1.0f, 18.0f * dt);
    }

    if (bumpTimer > 0.0f) {
        emitThrusterParticles(dt, 1.4f);
    } else if (curSpeed > 25.0f) {
        emitThrusterParticles(dt, curSpeed / baseMaxSpeed);
    }

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

void ScoutShip::draw(const char* label, Color tint, bool speaking) const {
    bool drewTexture = false;
    if (texture.id != 0) {
        Rectangle src = { 0.0f, 0.0f, static_cast<float>(texture.width), static_cast<float>(texture.height) };
        float w = static_cast<float>(texture.width) * scale;
        float h = static_cast<float>(texture.height) * scale;
        Vector2 origin = { w * 0.5f, h * 0.5f };
        Rectangle dest = { position.x, position.y, w, h };

        // 1. Hovering Drop Shadow
        Vector2 shadowOffset = { 2.5f * scale, 3.5f * scale };
        Rectangle shadowOuter = { position.x + shadowOffset.x, position.y + shadowOffset.y, w * 1.06f, h * 1.06f };
        Vector2 originOuter = { shadowOuter.width * 0.5f, shadowOuter.height * 0.5f };
        DrawTexturePro(texture, src, shadowOuter, originOuter, angle, Fade(BLACK, 0.18f));
        Rectangle shadowDest = { position.x + shadowOffset.x, position.y + shadowOffset.y, w, h };
        DrawTexturePro(texture, src, shadowDest, origin, angle, Fade(BLACK, 0.38f));

        // 2. Configurable Thruster Plumes / Idle Glow
        drawThrusters(position, angle);

        // 3. Hull Texture
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

Vector2 ScoutShip::getNosePosition() const {
    float theta = angle * DEG2RAD;
    float sinA = std::sin(theta);
    float cosA = std::cos(theta);
    return {
        position.x + sinA * (8.0f * scale),
        position.y - cosA * (8.0f * scale)
    };
}

// ============================================================================
// Merchant Base Ship Implementation
// ============================================================================

MerchantShip::MerchantShip()
    : Ship(8.0f, 100.0f, 140.0f)
{
    scale = 1.8f;
    collisionRadius = 26.0f;
}

MerchantShip::MerchantShip(float m, float r, float s, Texture2D tex, int skin)
    : Ship(m, r, s, tex, skin)
{
    scale = 1.8f;
    collisionRadius = 26.0f;
}

void MerchantShip::setAnchor(Vector2 anchor, float anchorAngle) {
    anchorPosition = anchor;
    restAngle = anchorAngle;
    if (!isInitialized) {
        position = anchor;
        angle = anchorAngle;
        velocity = { 0.0f, 0.0f };
        isInitialized = true;
    }
}

void MerchantShip::update(float dt) {
    if (bumpTimer > 0.0f) {
        bumpTimer -= dt;
        if (bumpTimer < 0.0f) bumpTimer = 0.0f;
    }

    if (!isInitialized) {
        position = anchorPosition;
        velocity = { 0.0f, 0.0f };
        angle = restAngle;
        isInitialized = true;
        exhaust.clear();
    }

    Vector2 toAnchor = { anchorPosition.x - position.x, anchorPosition.y - position.y };
    float dist = std::sqrt(toAnchor.x * toAnchor.x + toAnchor.y * toAnchor.y);

    const float maxSpeed = speed;

    if (dist > 0.5f) {
        float desiredSpeed = 0.0f;
        if (dist > slowRadius) {
            desiredSpeed = maxSpeed;
        } else {
            float t = dist / slowRadius;
            desiredSpeed = maxSpeed * (t * (2.0f - t));
        }

        Vector2 desiredVel = { (toAnchor.x / dist) * desiredSpeed, (toAnchor.y / dist) * desiredSpeed };
        Vector2 accel = { (desiredVel.x - velocity.x) * 7.0f, (desiredVel.y - velocity.y) * 7.0f };
        float accelMag = std::sqrt(accel.x * accel.x + accel.y * accel.y);
        if (accelMag > returnAccel) {
            accel.x = (accel.x / accelMag) * returnAccel;
            accel.y = (accel.y / accelMag) * returnAccel;
        }

        velocity.x += accel.x * dt;
        velocity.y += accel.y * dt;

        float curSpeed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
        if (dist < 0.8f && curSpeed < 6.0f) {
            velocity = { 0.0f, 0.0f };
            position = anchorPosition;
        } else {
            position.x += velocity.x * dt;
            position.y += velocity.y * dt;
        }

        isMoving = (curSpeed > 15.0f);

        float targetAngle = restAngle;
        if (dist > 3.0f) {
            float tilt = std::clamp(velocity.y * 0.08f, -15.0f, 15.0f);
            targetAngle = restAngle + tilt;
        }
        float diffAngle = targetAngle - angle;
        while (diffAngle < -180.0f) diffAngle += 360.0f;
        while (diffAngle > 180.0f) diffAngle -= 360.0f;
        angle += diffAngle * std::min(1.0f, 8.0f * dt);
    } else {
        velocity = { 0.0f, 0.0f };
        position = anchorPosition;
        float diffAngle = restAngle - angle;
        while (diffAngle < -180.0f) diffAngle += 360.0f;
        while (diffAngle > 180.0f) diffAngle -= 360.0f;
        angle += diffAngle * std::min(1.0f, 8.0f * dt);
        isMoving = false;
    }

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

// ============================================================================
// Shop Freighter Ship Implementation
// ============================================================================

ShopShip::ShopShip()
    : MerchantShip(8.0f, 160.0f, 140.0f)
{
    scale = 1.8f;
    collisionRadius = 26.0f;
    name = "SHOP";
    color = ui::Colors::Amber400;
    setupThrusters();
}

ShopShip::ShopShip(Texture2D tex, Vector2 anchor, const std::string& shipName)
    : MerchantShip(8.0f, 160.0f, 140.0f, tex, 0)
{
    scale = 1.8f;
    name = shipName.empty() ? "SHOP" : shipName;
    color = ui::Colors::Amber400;
    setAnchor(anchor, 0.0f);
    setupThrusters();
}

ShopShip::ShopShip(Texture2D tex, Vector2 anchor, const ShipConfig& config)
    : MerchantShip(config.mass, config.range, config.speed, tex, 0)
{
    name = config.name.empty() ? "SHOP" : config.name;
    color = ui::Colors::Amber400;
    setAnchor(anchor, 0.0f);
    applyConfig(config);
}

void ShopShip::setupThrusters() {
    ShipConfig cfg = ShipConfig::createDefault(texture.width, texture.height, name);
    applyConfig(cfg);
}

void ShopShip::update(float dt) {
    MerchantShip::update(dt);

    float curSpeed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
    if (curSpeed > 15.0f) {
        emitThrusterParticles(dt, curSpeed / speed);
    }
}

void ShopShip::draw(const char* label, Color tint, bool speaking) const {
    (void)tint;
    (void)speaking;
    if (texture.id == 0) {
        Ship::draw(label, color, false);
        return;
    }

    Rectangle src = { 0.0f, 0.0f, static_cast<float>(texture.width), static_cast<float>(texture.height) };
    float w = static_cast<float>(texture.width) * scale;
    float h = static_cast<float>(texture.height) * scale;
    Vector2 origin = { w * 0.5f, h * 0.5f };

    float hoverY = (!isMoving) ? (std::sin(static_cast<float>(GetTime()) * 1.8f) * 2.0f) : 0.0f;
    Vector2 drawPos = { position.x, position.y + hoverY };

    // 1. Hovering Drop Shadow
    Vector2 shadowOffset = { 3.5f * scale, 5.0f * scale };
    Rectangle shadowOuter = { drawPos.x + shadowOffset.x, drawPos.y + shadowOffset.y, w * 1.05f, h * 1.05f };
    Vector2 originOuter = { shadowOuter.width * 0.5f, shadowOuter.height * 0.5f };
    DrawTexturePro(texture, src, shadowOuter, originOuter, angle, Fade(BLACK, 0.20f));
    Rectangle shadowDest = { drawPos.x + shadowOffset.x, drawPos.y + shadowOffset.y, w, h };
    DrawTexturePro(texture, src, shadowDest, origin, angle, Fade(BLACK, 0.40f));

    // 2. Configurable Multi-Nozzle Thrusters
    drawThrusters(drawPos, angle);

    // 3. Ship Sprite
    DrawTexturePro(texture, src, { drawPos.x, drawPos.y, w, h }, origin, angle, WHITE);

    // 4. Floating Badge
    const char* displayName = (label && label[0] != '\0') ? label : (!name.empty() ? name.c_str() : "SHOP");
    int nameW = MeasureText(displayName, 12);
    float badgeY = drawPos.y + (h * 0.5f) + 6.0f;
    Rectangle badge = { drawPos.x - static_cast<float>(nameW + 16) * 0.5f, badgeY, static_cast<float>(nameW + 16), 16.0f };
    DrawRectangleRec(badge, Fade(BLACK, 0.85f));
    DrawRectangleLinesEx(badge, 1.0f, ui::Colors::Amber400);
    DrawText(displayName, static_cast<int>(badge.x + 8), static_cast<int>(badge.y + 2), 12, ui::Colors::Amber300);
}

Vector2 ShopShip::getNosePosition() const {
    float theta = angle * DEG2RAD;
    float sinA = std::sin(theta);
    float cosA = std::cos(theta);
    float noseOffset = (texture.width > 0) ? (static_cast<float>(texture.width) * 0.5f - 4.0f) : 28.0f;
    return {
        position.x + cosA * (noseOffset * scale),
        position.y + sinA * (noseOffset * scale)
    };
}

} // namespace minesweeper::core
