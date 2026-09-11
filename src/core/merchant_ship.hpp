#pragma once

#include "ship_base.hpp"

namespace minesweeper::core {

// ============================================================================
// Merchant Base Ship Class
// ============================================================================
class MerchantShip : public Ship {
public:
    float restAngle = 0.0f;
    float returnAccel = 1200.0f;
    float slowRadius = 90.0f;

    MerchantShip();
    MerchantShip(float mass, float range, float speed, Texture2D texture = { 0 }, int skinId = 0);

    void setAnchor(Vector2 anchor, float anchorAngle = 0.0f);

    // Physics & steering for returning to anchor station with damped deceleration
    void update(float dt) override;
    void updateMerchant(float dt) { update(dt); }
};

} // namespace minesweeper::core
