#pragma once

#include "merchant_ship.hpp"
#include "item.hpp"
#include "ship_config.hpp"

namespace minesweeper::core {

struct MerchantPedestal {
    ItemId itemId = ItemId::None;
    std::string name;
    std::string description;
    int cost = 30;
    Vector2 offset = { 0.0f, 0.0f }; // World offset relative to ship position
    float holdTimer = 0.0f;          // 0.0f to 0.5f
    bool isHovered = false;
    bool isSold = false;
    float radius = 18.0f;
};

// ============================================================================
// Shop Freighter Ship Class (Docked Merchant)
// ============================================================================
class ShopShip : public MerchantShip {
public:
    ShopInventory inventory;
    int shopTier = 0;
    int itemCapacity = 0;
    std::string typeId = "shop1";
    ShipConfig config;

    std::vector<MerchantPedestal> pedestals;
    float safeZoneRadius = 120.0f; // 3-tile safe radius (approx 120px)
    Vector2 hopperOffset = { 0.0f, 32.0f };
    Vector2 hopperSize = { 42.0f, 20.0f };
    std::string currentBanter;
    float banterTimer = 0.0f;

    ShopShip();
    ShopShip(Texture2D texture, Vector2 anchor, const std::string& shipName = "SHOP");
    ShopShip(Texture2D texture, Vector2 anchor, const ShipConfig& config);

    void setupThrusters();
    void initializeInventory();
    void setupPedestals();
    void updatePedestals(float dt, Vector2 mouseWorldPos, bool isMouseDown, uint64_t& teamFunds, PlayerInventory& playerInventory);
    bool isInsideSafeZone(Vector2 worldPos) const;
    Rectangle getHopperWorldRect() const;
    void triggerBanter(const std::string& line, float duration = 3.5f);

    void update(float dt) override;
    void draw(const char* label = nullptr, Color tint = WHITE, bool speaking = false) const override;
    Vector2 getNosePosition() const override;
};

} // namespace minesweeper::core
