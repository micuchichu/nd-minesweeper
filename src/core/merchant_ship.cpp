#include "merchant_ship.hpp"
#include <cmath>
#include <algorithm>

namespace minesweeper::core {

// ============================================================================
// Merchant Base Ship Implementation
// ============================================================================

MerchantShip::MerchantShip()
    : Ship(8.0f, 100.0f, 140.0f)
{
    scale = 1.8f;
    collisionRadius = 26.0f;
    bumpable = false;
    isAnchored = true;
    maxLeashDist = std::clamp(350.0f / std::sqrt(std::max(1.0f, mass)), 30.0f, 120.0f);
}

MerchantShip::MerchantShip(float m, float r, float s, Texture2D tex, int skin)
    : Ship(m, r, s, tex, skin)
{
    scale = 1.8f;
    collisionRadius = 26.0f;
    bumpable = false;
    isAnchored = true;
    maxLeashDist = std::clamp(350.0f / std::sqrt(std::max(1.0f, mass)), 30.0f, 120.0f);
}

void MerchantShip::setAnchor(Vector2 anchor, float anchorAngle) {
    anchorPosition = anchor;
    restAngle = anchorAngle;
    isAnchored = true;
    maxLeashDist = std::clamp(350.0f / std::sqrt(std::max(1.0f, mass)), 30.0f, 120.0f);
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
        float t = std::min(1.0f, dist / slowRadius);
        float baseDesiredSpeed = (dist > slowRadius) ? maxSpeed : maxSpeed * (t * (2.0f - t));
        float normDist = dist / std::max(10.0f, slowRadius);
        float tension = 1.0f + normDist * normDist;
        float desiredSpeed = std::min(600.0f, baseDesiredSpeed * tension);

        Vector2 desiredVel = { (toAnchor.x / dist) * desiredSpeed, (toAnchor.y / dist) * desiredSpeed };
        float effectiveReturnAccel = returnAccel * tension;

        Vector2 accel = { (desiredVel.x - velocity.x) * 8.0f, (desiredVel.y - velocity.y) * 8.0f };
        float accelMag = std::sqrt(accel.x * accel.x + accel.y * accel.y);
        if (accelMag > effectiveReturnAccel) {
            accel.x = (accel.x / accelMag) * effectiveReturnAccel;
            accel.y = (accel.y / accelMag) * effectiveReturnAccel;
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

        // Hard clamp at maxLeashDist to prevent escaping anchor mooring
        if (maxLeashDist > 0.0f) {
            Vector2 curToAnchor = { anchorPosition.x - position.x, anchorPosition.y - position.y };
            float curDist = std::sqrt(curToAnchor.x * curToAnchor.x + curToAnchor.y * curToAnchor.y);
            if (curDist > maxLeashDist) {
                position.x = anchorPosition.x - (curToAnchor.x / curDist) * maxLeashDist;
                position.y = anchorPosition.y - (curToAnchor.y / curDist) * maxLeashDist;
                float vDotOut = (-velocity.x * curToAnchor.x - velocity.y * curToAnchor.y) / (curDist * curDist);
                if (vDotOut > 0.0f) {
                    velocity.x -= (-curToAnchor.x / curDist) * (vDotOut * curDist);
                    velocity.y -= (-curToAnchor.y / curDist) * (vDotOut * curDist);
                }
            }
        }

        isMoving = (curSpeed > 15.0f);

        float targetAngle = restAngle;
        if (dist > 3.0f) {
            float rad = restAngle * DEG2RAD;
            float lateralVel = -velocity.x * std::sin(rad) + velocity.y * std::cos(rad);
            float tilt = std::clamp(lateralVel * 0.08f, -15.0f, 15.0f);
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

} // namespace minesweeper::core
