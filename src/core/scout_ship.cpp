#include "scout_ship.hpp"
#include "ui/theme.hpp"
#include <cmath>
#include <algorithm>

namespace minesweeper::core {

// ============================================================================
// Scout / Player Ship Implementation
// ============================================================================

ScoutShip::ScoutShip()
    : Ship(8.0f, 150.0f, 600.0f)
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

    updateExhaust(dt);
}

void ScoutShip::updateDirect(Vector2 moveInput, float dt, bool hasAim, float aimAngle) {
    if (!isInitialized) {
        velocity = { 0.0f, 0.0f };
        angle = 0.0f;
        isInitialized = true;
        exhaust.clear();
    }

    if (bumpTimer > 0.0f) {
        bumpTimer -= dt;
        if (bumpTimer < 0.0f) bumpTimer = 0.0f;
    }

    float inputLen = std::sqrt(moveInput.x * moveInput.x + moveInput.y * moveInput.y);
    if (inputLen > 1.0f) {
        moveInput.x /= inputLen;
        moveInput.y /= inputLen;
        inputLen = 1.0f;
    }

    float baseMaxSpeed = speed; // 600 px/s
    Vector2 desiredVel = { moveInput.x * baseMaxSpeed, moveInput.y * baseMaxSpeed };

    // Direct movement acceleration & deceleration
    if (inputLen > 0.01f) {
        Vector2 accel = { (desiredVel.x - velocity.x) * 14.0f, (desiredVel.y - velocity.y) * 14.0f };
        float accelMag = std::sqrt(accel.x * accel.x + accel.y * accel.y);
        float limitAccel = maxAccel * 1.5f;
        if (accelMag > limitAccel && accelMag > 0.001f) {
            accel.x = (accel.x / accelMag) * limitAccel;
            accel.y = (accel.y / accelMag) * limitAccel;
        }
        velocity.x += accel.x * dt;
        velocity.y += accel.y * dt;
    } else {
        // Active space braking when no direction keys are held
        float brake = std::max(0.0f, 1.0f - 9.0f * dt);
        velocity.x *= brake;
        velocity.y *= brake;
        if (std::abs(velocity.x) < 2.0f) velocity.x = 0.0f;
        if (std::abs(velocity.y) < 2.0f) velocity.y = 0.0f;
    }

    // Bleed off excess bump/explosion recoil speed
    float curSpeed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
    if (curSpeed > baseMaxSpeed) {
        float dragFactor = std::max(0.0f, 1.0f - 7.0f * dt);
        velocity.x *= dragFactor;
        velocity.y *= dragFactor;
    }

    position.x += velocity.x * dt;
    position.y += velocity.y * dt;

    isMoving = (curSpeed > 20.0f || inputLen > 0.01f);

    // Orientation / rotation handling
    if (hasAim) {
        float diffAngle = aimAngle - angle;
        while (diffAngle < -180.0f) diffAngle += 360.0f;
        while (diffAngle > 180.0f) diffAngle -= 360.0f;
        angle += diffAngle * std::min(1.0f, 18.0f * dt);
    } else if (inputLen > 0.1f && curSpeed > 10.0f) {
        float targetAngle = std::atan2(velocity.y, velocity.x) * RAD2DEG + 90.0f;
        float diffAngle = targetAngle - angle;
        while (diffAngle < -180.0f) diffAngle += 360.0f;
        while (diffAngle > 180.0f) diffAngle -= 360.0f;
        angle += diffAngle * std::min(1.0f, 14.0f * dt);
    }

    if (bumpTimer > 0.0f) {
        emitThrusterParticles(dt, 1.4f);
    } else if (isMoving) {
        emitThrusterParticles(dt, std::min(1.0f, curSpeed / baseMaxSpeed));
    }

    updateExhaust(dt);
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

} // namespace minesweeper::core
