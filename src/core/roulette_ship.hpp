#pragma once

#include "merchant_ship.hpp"
#include "roulette_types.hpp"

namespace minesweeper::core {

struct ShipConfig;

// ============================================================================
// Dedicated Roulette Casino Ship Class
// ============================================================================
class RouletteShip : public MerchantShip {
public:
    RouletteState roulette;

    // Post-spin result display & alignment back to rest dock
    float resultDisplayTimer = 0.0f;
    float resultDisplayDuration = 2.5f;
    bool isAligning = false;
    float alignTimer = 0.0f;
    float alignDuration = 1.2f;
    float alignStartAngle = 90.0f;
    float alignTargetAngle = 90.0f;

    RouletteShip();
    RouletteShip(Texture2D texture, Vector2 anchor, const std::string& shipName = "roulette");
    RouletteShip(Texture2D texture, Vector2 anchor, const ShipConfig& config);

    void setupThrusters();
    void startSpin(int winningNumber = -1, float duration = 4.5f);
    void alignBackToDock();
    void emitVortexParticles(float dt, float spinIntensity);

    void update(float dt) override;
    void draw(const char* label = nullptr, Color tint = WHITE, bool speaking = false) const override;
    Vector2 getNosePosition() const override;
};

} // namespace minesweeper::core
