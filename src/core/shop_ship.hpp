#pragma once

#include "merchant_ship.hpp"

namespace minesweeper::core {

struct ShipConfig;

// ============================================================================
// Shop Freighter Ship Class (Docked Merchant)
// ============================================================================
class ShopShip : public MerchantShip {
public:
    ShopShip();
    ShopShip(Texture2D texture, Vector2 anchor, const std::string& shipName = "SHOP");
    ShopShip(Texture2D texture, Vector2 anchor, const ShipConfig& config);

    void setupThrusters();
    void update(float dt) override;
    void draw(const char* label = nullptr, Color tint = WHITE, bool speaking = false) const override;
    Vector2 getNosePosition() const override;
};

} // namespace minesweeper::core
