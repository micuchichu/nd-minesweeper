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
}

MerchantShip::MerchantShip(float m, float r, float s, Texture2D tex, int skin)
    : Ship(m, r, s, tex, skin)
{
    scale = 1.8f;
    collisionRadius = 26.0f;
    bumpable = false;
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
