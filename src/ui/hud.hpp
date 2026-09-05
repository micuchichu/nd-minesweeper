#pragma once

#include "../core/board.hpp"
#include "../net/network_manager.hpp"
#include "layout_config.hpp"
#include <cstdint>

namespace minesweeper::ui {

struct HUDActions {
    bool returnToMenu = false;
    bool requestNewGame = false;
    bool restartGame = false;
    bool toggleHost = false;
    bool disconnect = false;
    bool openLayoutEditor = false;
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

    static uint64_t parseSeed(const char* str);

    void init(const core::BoardConfig& cfg);
    HUDActions drawAndProcess(int screenW, int screenH, const core::Board& board, float timePlayed, net::NetworkManager& net, const UILayoutConfig& layout = UILayoutConfig{});
};


} // namespace minesweeper::ui
