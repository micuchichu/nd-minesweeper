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
    setupPedestals();
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
    setupPedestals();
}

ShopShip::ShopShip(Texture2D tex, Vector2 anchor, const ShipConfig& config)
    : MerchantShip(config.mass, config.range, config.speed, tex, 0)
{
    name = config.name.empty() ? "SHOP" : config.name;
    color = ui::Colors::Amber400;
    this->config = config;
    setAnchor(anchor, 90.0f);
    applyConfig(config);
    shopTier = config.shopTier;
    itemCapacity = config.itemCapacity;
    initializeInventory();
    setupPedestals();
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

void ShopShip::setupPedestals() {
    pedestals.clear();
    // 1. GroundPenetratingWand ($30)
    MerchantPedestal p1;
    p1.itemId = ItemId::GroundPenetratingWand;
    p1.name = "GP-WAND";
    p1.description = "Subsurface sensor. Safely uncovers clean cells, auto-flags mines.";
    p1.cost = 30;
    p1.offset = { -44.0f, 44.0f };
    p1.radius = 18.0f;
    pedestals.push_back(p1);

    // 2. BlastShield ($50)
    MerchantPedestal p2;
    p2.itemId = ItemId::BlastShield;
    p2.name = "BLAST SHIELD";
    p2.description = "Ablative plating. Absorbs 1 lethal detonation with screen shake & knockback.";
    p2.cost = 50;
    p2.offset = { 0.0f, 50.0f };
    p2.radius = 18.0f;
    pedestals.push_back(p2);

    // 3. RadarBeacon ($60)
    MerchantPedestal p3;
    p3.itemId = ItemId::RadarBeacon;
    p3.name = "RADAR BEACON";
    p3.description = "Deployable sonar probe. Pings 5x5 sub-grid for accurate mine count.";
    p3.cost = 60;
    p3.offset = { 44.0f, 44.0f };
    p3.radius = 18.0f;
    pedestals.push_back(p3);
}

bool ShopShip::isInsideSafeZone(Vector2 worldPos) const {
    return Vector2Distance(worldPos, position) <= safeZoneRadius;
}

Rectangle ShopShip::getHopperWorldRect() const {
    return {
        position.x + hopperOffset.x - hopperSize.x * 0.5f,
        position.y + hopperOffset.y - hopperSize.y * 0.5f,
        hopperSize.x,
        hopperSize.y
    };
}

void ShopShip::triggerBanter(const std::string& line, float duration) {
    currentBanter = line;
    banterTimer = duration;
}

void ShopShip::updatePedestals(float dt, Vector2 mouseWorldPos, bool isMouseDown, uint64_t& teamFunds, PlayerInventory& playerInventory) {
    if (banterTimer > 0.0f) {
        banterTimer -= dt;
        if (banterTimer <= 0.0f) currentBanter.clear();
    }

    for (auto& ped : pedestals) {
        Vector2 pPos = { position.x + ped.offset.x, position.y + ped.offset.y };
        float dist = Vector2Distance(mouseWorldPos, pPos);
        ped.isHovered = (dist <= ped.radius);

        if (ped.isHovered && isMouseDown) {
            ped.holdTimer += dt;
            if (ped.holdTimer >= 0.5f) {
                if (teamFunds >= static_cast<uint64_t>(ped.cost) && playerInventory.hasFreeSlot()) {
                    teamFunds -= ped.cost;
                    const Item* it = ItemCatalog::instance().getItem(ped.itemId);
                    if (it) playerInventory.addItem(*it);
                    triggerBanter("Pleasure doing business with you, scavenger.");
                } else if (teamFunds < static_cast<uint64_t>(ped.cost)) {
                    triggerBanter("Insufficient scrap for that hardware.");
                }
                ped.holdTimer = 0.0f;
            }
        } else {
            ped.holdTimer = std::max(0.0f, ped.holdTimer - dt * 2.5f);
        }
    }
}

void ShopShip::setupThrusters() {
    ShipConfig cfg = ShipConfig::createDefault(texture.width, texture.height, name);
    applyConfig(cfg);
    if (shopTier <= 0) shopTier = cfg.shopTier;
    if (itemCapacity <= 0) itemCapacity = cfg.itemCapacity;
}

void ShopShip::update(float dt) {
    MerchantShip::update(dt);

    if (banterTimer > 0.0f) {
        banterTimer -= dt;
        if (banterTimer <= 0.0f) currentBanter.clear();
    }

    float curSpeed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
    if (curSpeed > 15.0f) {
        emitThrusterParticles(dt, curSpeed / speed);
    }
}

void ShopShip::draw(const char* label, Color tint, bool speaking) const {
    (void)tint;
    (void)speaking;

    // 0. Safe Zone Ring
    float szPulse = 0.5f + 0.5f * std::sin(static_cast<float>(GetTime()) * 2.5f);
    DrawCircleLines(static_cast<int>(position.x), static_cast<int>(position.y), safeZoneRadius, Fade(ui::Colors::Green400, 0.20f + 0.10f * szPulse));
    DrawCircle(static_cast<int>(position.x), static_cast<int>(position.y), safeZoneRadius, Fade(ui::Colors::Green400, 0.025f));

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

    // 4. Deposit Hopper
    Rectangle hopperRect = getHopperWorldRect();
    DrawRectangleRec(hopperRect, Color{ 16, 20, 24, 235 });
    DrawRectangleLinesEx(hopperRect, 1.5f, ui::Colors::Amber400);
    for (float hx = hopperRect.x; hx < hopperRect.x + hopperRect.width; hx += 8.0f) {
        DrawLineEx({ hx, hopperRect.y + hopperRect.height }, { hx + 6.0f, hopperRect.y }, 1.2f, Fade(ui::Colors::Amber500, 0.45f));
    }
    int hw = MeasureText("HOPPER", 10);
    DrawText("HOPPER", static_cast<int>(hopperRect.x + (hopperRect.width - hw) * 0.5f), static_cast<int>(hopperRect.y + 4.0f), 10, ui::Colors::Amber300);

    // 5. Physical Pedestals
    for (const auto& ped : pedestals) {
        Vector2 pPos = { position.x + ped.offset.x, position.y + ped.offset.y };

        DrawCircle(static_cast<int>(pPos.x), static_cast<int>(pPos.y + 2.0f), ped.radius, Fade(BLACK, 0.5f));
        DrawCircle(static_cast<int>(pPos.x), static_cast<int>(pPos.y), ped.radius, Color{ 20, 24, 30, 255 });
        DrawCircleLines(static_cast<int>(pPos.x), static_cast<int>(pPos.y), ped.radius, ped.isHovered ? WHITE : ui::Colors::Amber500);

        const char* tag = (ped.itemId == ItemId::BlastShield) ? "SHD" : ((ped.itemId == ItemId::GroundPenetratingWand) ? "WND" : "RDR");
        int tw = MeasureText(tag, 10);
        DrawText(tag, static_cast<int>(pPos.x - tw * 0.5f), static_cast<int>(pPos.y - 5.0f), 10, ped.isHovered ? ui::Colors::Amber200 : ui::Colors::Amber400);

        const char* priceStr = TextFormat("$%d", ped.cost);
        int pw = MeasureText(priceStr, 10);
        Rectangle pBadge = { pPos.x - pw * 0.5f - 4.0f, pPos.y + ped.radius + 2.0f, static_cast<float>(pw + 8), 14.0f };
        DrawRectangleRec(pBadge, Fade(BLACK, 0.85f));
        DrawRectangleLinesEx(pBadge, 1.0f, ui::Colors::Amber500);
        DrawText(priceStr, static_cast<int>(pBadge.x + 4), static_cast<int>(pBadge.y + 2), 10, ui::Colors::Amber300);

        if (ped.holdTimer > 0.0f) {
            float frac = std::clamp(ped.holdTimer / 0.5f, 0.0f, 1.0f);
            float endAngle = -90.0f + 360.0f * frac;
            DrawCircleSector(pPos, ped.radius + 3.0f, -90.0f, endAngle, 32, Fade(ui::Colors::Amber400, 0.40f));
            DrawCircleSectorLines(pPos, ped.radius + 3.0f, -90.0f, endAngle, 32, ui::Colors::Amber300);
        }

        if (ped.isHovered) {
            float ttW = 210.0f;
            float ttH = 76.0f;
            float ttX = pPos.x - ttW * 0.5f;
            float ttY = pPos.y - ped.radius - ttH - 8.0f;
            Rectangle ttRect = { ttX, ttY, ttW, ttH };

            DrawRectangleRec(ttRect, Color{ 10, 10, 12, 245 });
            DrawRectangleLinesEx(ttRect, 1.2f, ui::Colors::Amber400);

            DrawText(ped.name.c_str(), static_cast<int>(ttX + 8), static_cast<int>(ttY + 6), 11, ui::Colors::Amber300);
            DrawText(TextFormat("COST: %d SCRAP", ped.cost), static_cast<int>(ttX + ttW - 85), static_cast<int>(ttY + 6), 10, ui::Colors::Amber400);

            std::string d1 = ped.description.substr(0, 36);
            std::string d2 = (ped.description.size() > 36) ? ped.description.substr(36) : "";
            DrawText(d1.c_str(), static_cast<int>(ttX + 8), static_cast<int>(ttY + 22), 9, ui::Colors::Zinc300);
            if (!d2.empty()) {
                DrawText(d2.c_str(), static_cast<int>(ttX + 8), static_cast<int>(ttY + 34), 9, ui::Colors::Zinc300);
            }

            DrawText("[HOLD LMB (0.5s) TO PURCHASE]", static_cast<int>(ttX + 8), static_cast<int>(ttY + 54), 9, ui::Colors::Green400);
        }
    }

    // 6. Floating Badge
    const char* displayName = (label && label[0] != '\0') ? label : (!name.empty() ? name.c_str() : "SHOP");
    int nameW = MeasureText(displayName, 12);
    float rad = angle * DEG2RAD;
    float halfExtentY = (std::abs(w * std::sin(rad)) + std::abs(h * std::cos(rad))) * 0.5f;
    float badgeY = drawPos.y + halfExtentY + 6.0f;
    Rectangle badge = { drawPos.x - static_cast<float>(nameW + 16) * 0.5f, badgeY, static_cast<float>(nameW + 16), 16.0f };
    DrawRectangleRec(badge, Fade(BLACK, 0.85f));
    DrawRectangleLinesEx(badge, 1.0f, ui::Colors::Amber400);
    DrawText(displayName, static_cast<int>(badge.x + 8), static_cast<int>(badge.y + 2), 12, ui::Colors::Amber300);

    // 7. Contextual Banter Bubble
    if (banterTimer > 0.0f && !currentBanter.empty()) {
        int bw = MeasureText(currentBanter.c_str(), 11);
        float bBoxW = static_cast<float>(bw + 20);
        float bBoxH = 24.0f;
        float bBoxX = drawPos.x - bBoxW * 0.5f;
        float bBoxY = drawPos.y - halfExtentY - 34.0f;
        Rectangle bBox = { bBoxX, bBoxY, bBoxW, bBoxH };

        DrawRectangleRec(bBox, Color{ 10, 12, 16, 240 });
        DrawRectangleLinesEx(bBox, 1.2f, ui::Colors::Amber400);
        DrawTriangle({ drawPos.x - 5.0f, bBoxY + bBoxH }, { drawPos.x + 5.0f, bBoxY + bBoxH }, { drawPos.x, bBoxY + bBoxH + 6.0f }, ui::Colors::Amber400);
        DrawText(currentBanter.c_str(), static_cast<int>(bBoxX + 10), static_cast<int>(bBoxY + 6), 11, ui::Colors::Amber200);
    }
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
