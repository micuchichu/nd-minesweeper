#include "ship.hpp"
#include "ui/theme.hpp"
#include <cmath>
#include <cstdlib>
#include <algorithm>

namespace minesweeper::core {

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
    , isMerchant(false)
    , anchorPosition{ 0.0f, 0.0f }
    , restAngle(0.0f)
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
    , isMerchant(false)
    , anchorPosition{ 0.0f, 0.0f }
    , restAngle(0.0f)
{
}

void Ship::reset(Vector2 newPos, float newAngle) {
    position = newPos;
    velocity = { 0.0f, 0.0f };
    angle = newAngle;
    isMoving = false;
    isInitialized = true;
    exhaust.clear();
}

void Ship::update(Vector2 targetPos, float dt) {
    if (!isInitialized) {
        position = { targetPos.x - 50.0f, targetPos.y };
        velocity = { 0.0f, 0.0f };
        angle = 0.0f;
        isInitialized = true;
        exhaust.clear();
    }

    // 1. Calculate direction to target (aim point)
    Vector2 toTarget = { targetPos.x - position.x, targetPos.y - position.y };
    float distToTarget = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y);

    const float targetDist = 70.0f;  // Keep 70px follow distance requested by user
    const float maxSpeed = speed;
    const float slowRadius = 140.0f;
    const float maxAccel = 2200.0f;

    // Desired resting position maintains 'range' distance from target
    Vector2 idealPos;
    if (distToTarget > 0.001f) {
        Vector2 dirFromTarget = { -toTarget.x / distToTarget, -toTarget.y / distToTarget };
        idealPos = { targetPos.x + dirFromTarget.x * targetDist, targetPos.y + dirFromTarget.y * targetDist };
    } else {
        idealPos = { targetPos.x - targetDist, targetPos.y };
    }

    Vector2 toIdeal = { idealPos.x - position.x, idealPos.y - position.y };
    float distToIdeal = std::sqrt(toIdeal.x * toIdeal.x + toIdeal.y * toIdeal.y);

    // Smooth quadratic arrival deceleration curve
    float desiredSpeed = 0.0f;
    if (distToIdeal > slowRadius) {
        desiredSpeed = maxSpeed;
    } else if (distToIdeal > 0.5f) {
        float t = distToIdeal / slowRadius;
        desiredSpeed = maxSpeed * (t * (2.0f - t));
    }

    Vector2 desiredVel = { 0.0f, 0.0f };
    if (distToIdeal > 0.001f && desiredSpeed > 0.0f) {
        desiredVel = { (toIdeal.x / distToIdeal) * desiredSpeed, (toIdeal.y / distToIdeal) * desiredSpeed };
    }

    // Steering acceleration force
    Vector2 accel = { (desiredVel.x - velocity.x) * 10.0f, (desiredVel.y - velocity.y) * 10.0f };
    float accelMag = std::sqrt(accel.x * accel.x + accel.y * accel.y);
    if (accelMag > maxAccel) {
        accel.x = (accel.x / accelMag) * maxAccel;
        accel.y = (accel.y / accelMag) * maxAccel;
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

    // 2. Rotate so eyes (yellow lines at top of sprite) face the target
    if (distToTarget > 0.001f) {
        float dirX = toTarget.x / distToTarget;
        float dirY = toTarget.y / distToTarget;
        float desiredAngle = std::atan2(dirY, dirX) * RAD2DEG + 90.0f;
        float diffAngle = desiredAngle - angle;
        while (diffAngle < -180.0f) diffAngle += 360.0f;
        while (diffAngle > 180.0f) diffAngle -= 360.0f;
        angle += diffAngle * std::min(1.0f, 18.0f * dt);
    }

    // 3. Trailing exhaust particle emitters at (4, 12) & (11, 12)
    float theta = angle * DEG2RAD;
    float cosA = std::cos(theta);
    float sinA = std::sin(theta);

    float lxLeft = -3.5f * scale;
    float lxRight = 3.5f * scale;
    float ly = 4.5f * scale;

    Vector2 leftThrust = {
        position.x + (lxLeft * cosA - ly * sinA),
        position.y + (lxLeft * sinA + ly * cosA)
    };
    Vector2 rightThrust = {
        position.x + (lxRight * cosA - ly * sinA),
        position.y + (lxRight * sinA + ly * cosA)
    };
    Vector2 rear = { -sinA, cosA };
    Vector2 right = { cosA, sinA };

    if (curSpeed > 25.0f) {
        emitTimer += dt * (curSpeed / maxSpeed);
        while (emitTimer >= 0.02f) {
            emitTimer -= 0.02f;
            if (exhaust.size() < 128) {
                float pSpeed = 40.0f + static_cast<float>(rand() % 40);
                float spread = ((rand() % 100) - 50) * 0.004f;
                Vector2 pVel = { (rear.x + right.x * spread) * pSpeed, (rear.y + right.y * spread) * pSpeed };
                Color c1 = (rand() % 2 == 0) ? ui::Colors::Amber400 : ui::Colors::Orange500;
                Color c2 = (rand() % 2 == 0) ? ui::Colors::Yellow400 : ui::Colors::Amber400;
                exhaust.push_back({ leftThrust, pVel, 0.28f, 0.28f, 2.4f, c1 });
                exhaust.push_back({ rightThrust, pVel, 0.28f, 0.28f, 2.4f, c2 });
            }
        }
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

void Ship::updateMerchant(float dt) {
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
    const float slowRadius = 90.0f;
    const float maxAccel = 1200.0f;

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
        if (accelMag > maxAccel) {
            accel.x = (accel.x / accelMag) * maxAccel;
            accel.y = (accel.y / accelMag) * maxAccel;
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

    float theta = angle * DEG2RAD;
    float cosA = std::cos(theta);
    float sinA = std::sin(theta);

    Vector2 rear = { -cosA, -sinA };
    Vector2 perp = { -sinA, cosA };

    float lxTop = -25.0f * scale;
    float lyTop = -11.0f * scale;
    float lxMid = -27.0f * scale;
    float lyMid = 0.0f;
    float lxBot = -25.0f * scale;
    float lyBot = 11.0f * scale;

    Vector2 topThrust = { position.x + (lxTop * cosA - lyTop * sinA), position.y + (lxTop * sinA + lyTop * cosA) };
    Vector2 midThrust = { position.x + (lxMid * cosA - lyMid * sinA), position.y + (lxMid * sinA + lyMid * cosA) };
    Vector2 botThrust = { position.x + (lxBot * cosA - lyBot * sinA), position.y + (lxBot * sinA + lyBot * cosA) };

    float curSpeed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
    if (curSpeed > 15.0f) {
        emitTimer += dt * (curSpeed / maxSpeed);
        while (emitTimer >= 0.02f) {
            emitTimer -= 0.02f;
            if (exhaust.size() < 128) {
                float pSpeed = 35.0f + static_cast<float>(rand() % 35);
                float spread1 = ((rand() % 100) - 50) * 0.005f;
                float spread2 = ((rand() % 100) - 50) * 0.005f;
                float spread3 = ((rand() % 100) - 50) * 0.005f;
                Vector2 v1 = { (rear.x + perp.x * spread1) * pSpeed, (rear.y + perp.y * spread1) * pSpeed };
                Vector2 v2 = { (rear.x + perp.x * spread2) * pSpeed, (rear.y + perp.y * spread2) * pSpeed };
                Vector2 v3 = { (rear.x + perp.x * spread3) * pSpeed, (rear.y + perp.y * spread3) * pSpeed };
                Color cAmber = (rand() % 2 == 0) ? ui::Colors::Amber400 : ui::Colors::Orange500;
                Color cPurple = (rand() % 2 == 0) ? ui::Colors::Purple400 : ui::Colors::Purple500;
                Color cCyan = (rand() % 2 == 0) ? ui::Colors::Cyan400 : ui::Colors::Amber300;
                exhaust.push_back({ topThrust, v1, 0.28f, 0.28f, 2.6f, cAmber });
                exhaust.push_back({ midThrust, v2, 0.32f, 0.32f, 3.0f, cPurple });
                exhaust.push_back({ botThrust, v3, 0.28f, 0.28f, 2.6f, cCyan });
            }
        }
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

bool Ship::resolveCollision(Ship& a, Ship& b, float restitution) {
    Vector2 delta = { b.position.x - a.position.x, b.position.y - a.position.y };
    float distSq = delta.x * delta.x + delta.y * delta.y;
    float minDist = a.collisionRadius + b.collisionRadius;

    if (distSq >= minDist * minDist) {
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

    // 1. Positional Separation proportional to inverse mass
    float totalMass = a.mass + b.mass;
    if (totalMass > 0.0001f) {
        float aFraction = b.mass / totalMass;
        float bFraction = a.mass / totalMass;

        a.position.x -= normal.x * overlap * aFraction;
        a.position.y -= normal.y * overlap * aFraction;
        b.position.x += normal.x * overlap * bFraction;
        b.position.y += normal.y * overlap * bFraction;
    }

    // 2. Velocity Impulse Exchange
    Vector2 relVel = { b.velocity.x - a.velocity.x, b.velocity.y - a.velocity.y };
    float velAlongNormal = relVel.x * normal.x + relVel.y * normal.y;

    if (velAlongNormal < 0.0f) {
        float invMassA = (a.mass > 0.0001f) ? (1.0f / a.mass) : 0.0f;
        float invMassB = (b.mass > 0.0001f) ? (1.0f / b.mass) : 0.0f;
        float invMassSum = invMassA + invMassB;

        if (invMassSum > 0.0001f) {
            float impulseScalar = -(1.0f + restitution) * velAlongNormal / invMassSum;
            Vector2 impulse = { normal.x * impulseScalar, normal.y * impulseScalar };

            a.velocity.x -= impulse.x * invMassA;
            a.velocity.y -= impulse.y * invMassA;
            b.velocity.x += impulse.x * invMassB;
            b.velocity.y += impulse.y * invMassB;
        }
    } else {
        // Overlapping with non-closing velocity: add small repulsive nudge
        float push = 30.0f;
        float invMassA = (a.mass > 0.0001f) ? (1.0f / a.mass) : 0.0f;
        float invMassB = (b.mass > 0.0001f) ? (1.0f / b.mass) : 0.0f;
        a.velocity.x -= normal.x * push * invMassA;
        a.velocity.y -= normal.y * push * invMassA;
        b.velocity.x += normal.x * push * invMassB;
        b.velocity.y += normal.y * push * invMassB;
    }

    return true;
}

void Ship::draw(const char* label, Color tint, bool speaking) const {
    bool drewTexture = false;
    if (texture.id != 0) {
        Rectangle src = { 0.0f, 0.0f, static_cast<float>(texture.width), static_cast<float>(texture.height) };
        float w = static_cast<float>(texture.width) * scale;
        float h = static_cast<float>(texture.height) * scale;
        Vector2 origin = { w * 0.5f, h * 0.5f };

        if (isMerchant) {
            float hoverY = (!isMoving) ? (std::sin(static_cast<float>(GetTime()) * 1.8f) * 2.0f) : 0.0f;
            Vector2 drawPos = { position.x, position.y + hoverY };

            // 1. Hovering Drop Shadow
            Vector2 shadowOffset = { 3.5f * scale, 5.0f * scale };
            Rectangle shadowOuter = { drawPos.x + shadowOffset.x, drawPos.y + shadowOffset.y, w * 1.05f, h * 1.05f };
            Vector2 originOuter = { shadowOuter.width * 0.5f, shadowOuter.height * 0.5f };
            DrawTexturePro(texture, src, shadowOuter, originOuter, angle, Fade(BLACK, 0.20f));
            Rectangle shadowDest = { drawPos.x + shadowOffset.x, drawPos.y + shadowOffset.y, w, h };
            DrawTexturePro(texture, src, shadowDest, origin, angle, Fade(BLACK, 0.40f));

            float theta = angle * DEG2RAD;
            float cosA = std::cos(theta);
            float sinA = std::sin(theta);

            Vector2 rear = { -cosA, -sinA };
            Vector2 perp = { -sinA, cosA };

            float lxTop = -25.0f * scale;
            float lyTop = -11.0f * scale;
            float lxMid = -27.0f * scale;
            float lyMid = 0.0f;
            float lxBot = -25.0f * scale;
            float lyBot = 11.0f * scale;

            Vector2 topThrust = { drawPos.x + (lxTop * cosA - lyTop * sinA), drawPos.y + (lxTop * sinA + lyTop * cosA) };
            Vector2 midThrust = { drawPos.x + (lxMid * cosA - lyMid * sinA), drawPos.y + (lxMid * sinA + lyMid * cosA) };
            Vector2 botThrust = { drawPos.x + (lxBot * cosA - lyBot * sinA), drawPos.y + (lxBot * sinA + lyBot * cosA) };

            float t = static_cast<float>(GetTime());
            if (isMoving) {
                float flk1 = 5.0f + 4.0f * std::sin(t * 38.0f);
                float flk2 = 7.0f + 5.0f * std::cos(t * 44.0f);
                float flk3 = 5.0f + 4.0f * std::sin(t * 36.0f);

                DrawTriangle({ topThrust.x + rear.x * (flk1 * scale), topThrust.y + rear.y * (flk1 * scale) },
                             { topThrust.x + perp.x * (1.8f * scale), topThrust.y + perp.y * (1.8f * scale) },
                             { topThrust.x - perp.x * (1.8f * scale), topThrust.y - perp.y * (1.8f * scale) }, ui::Colors::Orange500);
                DrawTriangle({ midThrust.x + rear.x * (flk2 * scale), midThrust.y + rear.y * (flk2 * scale) },
                             { midThrust.x + perp.x * (2.2f * scale), midThrust.y + perp.y * (2.2f * scale) },
                             { midThrust.x - perp.x * (2.2f * scale), midThrust.y - perp.y * (2.2f * scale) }, ui::Colors::Purple500);
                DrawTriangle({ botThrust.x + rear.x * (flk3 * scale), botThrust.y + rear.y * (flk3 * scale) },
                             { botThrust.x + perp.x * (1.8f * scale), botThrust.y + perp.y * (1.8f * scale) },
                             { botThrust.x - perp.x * (1.8f * scale), botThrust.y - perp.y * (1.8f * scale) }, ui::Colors::Cyan500);

                DrawTriangle({ topThrust.x + rear.x * (flk1 * 0.5f * scale), topThrust.y + rear.y * (flk1 * 0.5f * scale) },
                             { topThrust.x + perp.x * (1.1f * scale), topThrust.y + perp.y * (1.1f * scale) },
                             { topThrust.x - perp.x * (1.1f * scale), topThrust.y - perp.y * (1.1f * scale) }, ui::Colors::Amber300);
                DrawTriangle({ midThrust.x + rear.x * (flk2 * 0.5f * scale), midThrust.y + rear.y * (flk2 * 0.5f * scale) },
                             { midThrust.x + perp.x * (1.3f * scale), midThrust.y + perp.y * (1.3f * scale) },
                             { midThrust.x - perp.x * (1.3f * scale), midThrust.y - perp.y * (1.3f * scale) }, ui::Colors::Purple300);
                DrawTriangle({ botThrust.x + rear.x * (flk3 * 0.5f * scale), botThrust.y + rear.y * (flk3 * 0.5f * scale) },
                             { botThrust.x + perp.x * (1.1f * scale), botThrust.y + perp.y * (1.1f * scale) },
                             { botThrust.x - perp.x * (1.1f * scale), botThrust.y - perp.y * (1.1f * scale) }, ui::Colors::Cyan300);
            } else {
                float idlePulse = 0.5f + 0.5f * std::sin(t * 5.0f);
                DrawCircleV(topThrust, 1.4f * scale + idlePulse * 0.6f, Fade(ui::Colors::Amber400, 0.8f));
                DrawCircleV(midThrust, 1.8f * scale + idlePulse * 0.8f, Fade(ui::Colors::Purple400, 0.85f));
                DrawCircleV(botThrust, 1.4f * scale + idlePulse * 0.6f, Fade(ui::Colors::Cyan400, 0.8f));
            }

            DrawTexturePro(texture, src, { drawPos.x, drawPos.y, w, h }, origin, angle, WHITE);
            drewTexture = true;

            const char* displayName = (label && label[0] != '\0') ? label : (!name.empty() ? name.c_str() : "SHOP");
            int nameW = MeasureText(displayName, 12);
            float badgeY = drawPos.y + (h * 0.5f) + 6.0f;
            Rectangle badge = { drawPos.x - static_cast<float>(nameW + 16) * 0.5f, badgeY, static_cast<float>(nameW + 16), 16.0f };
            DrawRectangleRec(badge, Fade(BLACK, 0.85f));
            DrawRectangleLinesEx(badge, 1.0f, ui::Colors::Amber400);
            DrawText(displayName, static_cast<int>(badge.x + 8), static_cast<int>(badge.y + 2), 12, ui::Colors::Amber300);
            return;
        }

        // Standard Scout / Player Ship
        Rectangle dest = { position.x, position.y, w, h };

        // 1. Hovering Drop Shadow
        Vector2 shadowOffset = { 2.5f * scale, 3.5f * scale };
        Rectangle shadowOuter = { position.x + shadowOffset.x, position.y + shadowOffset.y, w * 1.06f, h * 1.06f };
        Vector2 originOuter = { shadowOuter.width * 0.5f, shadowOuter.height * 0.5f };
        DrawTexturePro(texture, src, shadowOuter, originOuter, angle, Fade(BLACK, 0.18f));
        Rectangle shadowDest = { position.x + shadowOffset.x, position.y + shadowOffset.y, w, h };
        DrawTexturePro(texture, src, shadowDest, origin, angle, Fade(BLACK, 0.38f));

        float theta = angle * DEG2RAD;
        float cosA = std::cos(theta);
        float sinA = std::sin(theta);

        // Center nozzles at (4, 12) and (11, 12)
        float lxLeft = -3.5f * scale;
        float lxRight = 3.5f * scale;
        float ly = 4.5f * scale;

        Vector2 leftThrust = {
            position.x + (lxLeft * cosA - ly * sinA),
            position.y + (lxLeft * sinA + ly * cosA)
        };
        Vector2 rightThrust = {
            position.x + (lxRight * cosA - ly * sinA),
            position.y + (lxRight * sinA + ly * cosA)
        };

        Vector2 rear = { -sinA, cosA };
        Vector2 flameDir = { cosA, sinA };

        float t = static_cast<float>(GetTime());
        if (isMoving) {
            float flicker1 = 4.0f + 3.0f * std::sin(t * 40.0f);
            float flicker2 = 4.0f + 3.0f * std::cos(t * 46.0f);

            Vector2 leftTip = { leftThrust.x + rear.x * (flicker1 * scale), leftThrust.y + rear.y * (flicker1 * scale) };
            Vector2 rightTip = { rightThrust.x + rear.x * (flicker2 * scale), rightThrust.y + rear.y * (flicker2 * scale) };

            float flameW = 1.6f * scale;
            // Outer plasma plume
            DrawTriangle(leftTip, { leftThrust.x + flameDir.x * flameW, leftThrust.y + flameDir.y * flameW }, { leftThrust.x - flameDir.x * flameW, leftThrust.y - flameDir.y * flameW }, ui::Colors::Orange500);
            DrawTriangle(rightTip, { rightThrust.x + flameDir.x * flameW, rightThrust.y + flameDir.y * flameW }, { rightThrust.x - flameDir.x * flameW, rightThrust.y - flameDir.y * flameW }, ui::Colors::Orange500);

            // Inner hot core
            Vector2 leftCoreTip = { leftThrust.x + rear.x * (flicker1 * 0.55f * scale), leftThrust.y + rear.y * (flicker1 * 0.55f * scale) };
            Vector2 rightCoreTip = { rightThrust.x + rear.x * (flicker2 * 0.55f * scale), rightThrust.y + rear.y * (flicker2 * 0.55f * scale) };
            float coreW = 1.0f * scale;
            DrawTriangle(leftCoreTip, { leftThrust.x + flameDir.x * coreW, leftThrust.y + flameDir.y * coreW }, { leftThrust.x - flameDir.x * coreW, leftThrust.y - flameDir.y * coreW }, ui::Colors::Amber300);
            DrawTriangle(rightCoreTip, { rightThrust.x + flameDir.x * coreW, rightThrust.y + flameDir.y * coreW }, { rightThrust.x - flameDir.x * coreW, rightThrust.y - flameDir.y * coreW }, ui::Colors::Amber300);
        } else {
            float idlePulse = 0.4f + 0.4f * std::sin(t * 6.0f);
            DrawCircleV(leftThrust, 1.2f * scale + idlePulse, Fade(ui::Colors::Amber500, 0.75f));
            DrawCircleV(rightThrust, 1.2f * scale + idlePulse, Fade(ui::Colors::Amber500, 0.75f));
        }

        // Draw Ship Sprite rotated around center
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

void Ship::drawExhaust() const {
    for (const auto& p : exhaust) {
        float alpha = p.life / p.maxLife;
        Color c = p.color;
        c.a = static_cast<unsigned char>(alpha * 200.0f);
        float sz = p.size * (0.4f + 0.6f * alpha);
        DrawRectanglePro({ p.pos.x, p.pos.y, sz, sz }, { sz * 0.5f, sz * 0.5f }, 45.0f, c);
    }
}

Vector2 Ship::getNosePosition() const {
    float theta = angle * DEG2RAD;
    float sinA = std::sin(theta);
    float cosA = std::cos(theta);
    if (isMerchant) {
        return {
            position.x + cosA * (28.0f * scale),
            position.y + sinA * (28.0f * scale)
        };
    }
    return {
        position.x + sinA * (8.0f * scale),
        position.y - cosA * (8.0f * scale)
    };
}

} // namespace minesweeper::core
