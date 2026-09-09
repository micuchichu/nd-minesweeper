#pragma once

#include <raylib.h>
#include "../core/board.hpp"
#include "../net/network_manager.hpp"
#include <cstdint>

namespace minesweeper::ui {

struct HUDActions {
    bool returnToMenu = false;
    bool requestNewGame = false;
    bool restartGame = false;
    bool toggleHost = false;
    bool disconnect = false;
};

class GameHUD {
public:
    int nextDim = 2;
    int nextSize = 10;
    int nextBombs = 10;
    uint64_t nextSeed = 12345;
    bool randomizeSeed = true;
    char seedBuf[32] = "12345";
    bool seedInputActive = false;
    float copiedSeedTimer = 0.0f;

    bool showLargeGridWarning = false;
    bool endModalDismissed = false;

    uint64_t scrapCount = 0;
    float scrapPulseTimer = 0.0f;
    Vector2 scrapBadgeScreenPos = { 150.0f, 35.0f };
    Texture2D scrapTexture{};

    Vector2 getScrapBadgeScreenPos() const { return scrapBadgeScreenPos; }
    void triggerScrapPulse() { scrapPulseTimer = 0.35f; }

    static uint64_t parseSeed(const char* str);

    void init(const core::BoardConfig& cfg);
    HUDActions drawAndProcess(int screenW, int screenH, const core::Board& board, float timePlayed, net::NetworkManager& net, bool isTransmitting = false, bool voiceEnabled = true, bool isPushToTalk = true);

    bool isMouseOver(int screenW, int screenH, float guiScale) const;
    bool isMouseOver(int screenW, int screenH, float guiScale, Vector2 mousePos) const;
};


} // namespace minesweeper::ui
