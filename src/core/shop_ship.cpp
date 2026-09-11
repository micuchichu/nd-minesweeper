#include "shop_ship.hpp"
#include "ship_config.hpp"
#include "ui/theme.hpp"
#include <cmath>
#include <algorithm>

namespace minesweeper::core {

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
    setAnchor({ 0.0f, 0.0f }, 90.0f);
}

ShopShip::ShopShip(Texture2D tex, Vector2 anchor, const std::string& shipName)
    : MerchantShip(8.0f, 160.0f, 140.0f, tex, 0)
{
    scale = 1.8f;
    name = shipName.empty() ? "SHOP" : shipName;
    color = ui::Colors::Amber400;
    setAnchor(anchor, 90.0f);
    setupThrusters();
    initializeInventory();
}

ShopShip::ShopShip(Texture2D tex, Vector2 anchor, const ShipConfig& config)
    : MerchantShip(config.mass, config.range, config.speed, tex, 0)
{
    name = config.name.empty() ? "SHOP" : config.name;
    color = ui::Colors::Amber400;
    setAnchor(anchor, 90.0f);
    applyConfig(config);
    shopTier = config.shopTier;
    itemCapacity = config.itemCapacity;
    initializeInventory();
}

void ShopShip::initializeInventory() {
    // If capacity or tier were not specified (e.g. non-positive or legacy default constructor),
    // apply fallback heuristics based on ship dimensions. Otherwise strictly respect the JSON config.
    if (itemCapacity <= 0) {
        if (capsuleLength > 0.0f || name.find("big") != std::string::npos || (texture.height > 48)) {
            itemCapacity = 4;
        } else {
            itemCapacity = 2;
        }
    }
    if (shopTier <= 0) {
        if (capsuleLength > 0.0f || name.find("big") != std::string::npos || (texture.height > 48)) {
            shopTier = 2;
        } else {
            shopTier = 1;
        }
    }
    inventory = ItemCatalog::instance().createInventoryForShop(shopTier, itemCapacity);
}

void ShopShip::setupThrusters() {
    ShipConfig cfg = ShipConfig::createDefault(texture.width, texture.height, name);
    applyConfig(cfg);
    if (shopTier <= 0) shopTier = cfg.shopTier;
    if (itemCapacity <= 0) itemCapacity = cfg.itemCapacity;
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

    float hoverY = !isMoving ? (std::sin(static_cast<float>(GetTime()) * 1.8f) * 2.0f) : 0.0f;
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
    float rad = angle * DEG2RAD;
    float halfExtentY = (std::abs(w * std::sin(rad)) + std::abs(h * std::cos(rad))) * 0.5f;
    float badgeY = drawPos.y + halfExtentY + 6.0f;
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
