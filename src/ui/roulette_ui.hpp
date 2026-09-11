#pragma once

#include "raylib.h"
#include "core/roulette_ship.hpp"
#include "render/camera_controller.hpp"
#include <string>

namespace minesweeper::ui {

struct RoulettePopupState {
    bool active = false;
    float timer = 0.0f;
    float duration = 3.2f;
    int winningNumber = 0;
    bool won = false;
    uint64_t payout = 0;
    uint64_t betAmount = 0;
};

class RouletteUI {
public:
    RouletteUI() = default;

    // Renders the hovering roulette betting card anchored near the roulette ship.
    // Returns true if a bet, spin, or chip change was handled this frame.
    bool draw(
        core::RouletteShip& ship,
        uint64_t& scrapCount,
        float alpha,
        const render::CameraController& camera,
        int screenW,
        int screenH
    );

    void update(float dt, bool isAnimating);
    void triggerPopup(int number, bool won, uint64_t payout, uint64_t betAmount = 0);
    void updatePopup(float dt);
    void drawPopup(
        const render::CameraController& camera,
        int screenW,
        int screenH,
        Vector2 shipPos,
        float cardAlpha
    );

    bool isPopupActive() const { return popup.active; }

    Rectangle lastCardRect = { 0.0f, 0.0f, 0.0f, 0.0f };

    bool isMouseOverCard() const {
        return (lastCardRect.width > 0.0f && CheckCollisionPointRec(GetMousePosition(), lastCardRect));
    }
    bool isMouseOverCard(Vector2 pos) const {
        return (lastCardRect.width > 0.0f && CheckCollisionPointRec(pos, lastCardRect));
    }

    uint64_t selectedChip = 10;
    uint64_t customBetAmount = 10;
    bool isEditingBet = false;
    std::string betInputBuffer = "10";
    bool requestClose = false;

    float animMoveT = 0.0f; // 0.0f = above ship (idle), 1.0f = below ship (animating)

    RoulettePopupState popup;

private:
    float pulseTimer = 0.0f;
};

} // namespace minesweeper::ui
