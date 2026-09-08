#pragma once

#include "ship_base.hpp"

namespace minesweeper::core {

// ============================================================================
// Scout / Player Ship Class
// ============================================================================
class ScoutShip : public Ship {
public:
    Vector2 targetPosition = { 0.0f, 0.0f };
    float targetFollowDistance = 70.0f;  // 70px follow distance
    float slowRadius = 140.0f;
    float maxAccel = 2200.0f;

    ScoutShip();
    ScoutShip(float mass, float range, float speed, Texture2D texture = { 0 }, int skinId = 0);

    // Physics & steering towards a target position with arrival deceleration (player / remote ships)
    void update(Vector2 targetPos, float dt);
    void update(float dt) override;
    void updateDirect(Vector2 moveInput, float dt, bool hasAim = false, float aimAngle = 0.0f);

    void draw(const char* label = nullptr, Color tint = WHITE, bool speaking = false) const override;
    Vector2 getNosePosition() const override;
};

using PlayerShip = ScoutShip;

} // namespace minesweeper::core
