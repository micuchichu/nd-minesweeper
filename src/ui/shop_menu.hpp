#pragma once

#include "raylib.h"
#include "core/shop_ship.hpp"
#include "core/item.hpp"
#include "render/camera_controller.hpp"
#include <string>

namespace minesweeper::ui {

class ShopMenu {
public:
    ShopMenu() = default;

    // Renders the hovering shop menu card anchored near the specified shop in screen space.
    // Returns true if an item was purchased this frame.
    bool drawHoverMenu(
        core::ShopShip& shop,
        uint64_t& scrapCount,
        core::PlayerInventory& playerInv,
        float alpha,
        const render::CameraController& camera,
        int screenW,
        int screenH
    );

    // Renders the player HUD 5-slot hotbar dock showing equipped items, durability, and active slot.
    // Returns true if the player clicked an item slot or triggered an action.
    bool drawInventoryDock(
        int screenW,
        int screenH,
        core::PlayerInventory& playerInv,
        float alpha
    );

private:
    float pulseTimer = 0.0f;
};

} // namespace minesweeper::ui
