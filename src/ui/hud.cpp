#include "hud.hpp"
#include "widgets.hpp"
#include "theme.hpp"
#include "../net/steam_manager.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <algorithm>

namespace minesweeper::ui {

uint64_t GameHUD::parseSeed(const char* str) {
    if (!str || str[0] == '\0') return 0;
    while (*str == ' ' || *str == '\t') str++;
    if (*str == '\0') return 0;

    bool allDigits = true;
    for (const char* p = str; *p; ++p) {
        if (*p < '0' || *p > '9') {
            allDigits = false;
            break;
        }
    }
    if (allDigits) {
        return std::strtoull(str, nullptr, 10);
    }
    // FNV-1a 64-bit hash for arbitrary alphanumeric strings
    uint64_t hash = 14695981039346656037ULL;
    for (const char* p = str; *p; ++p) {
        hash ^= static_cast<uint64_t>(static_cast<unsigned char>(*p));
        hash *= 1099511628211ULL;
    }
    return hash;
}

void GameHUD::init(const core::BoardConfig& cfg) {
    nextDim = cfg.dim;
    nextSize = cfg.size;
    nextBombs = cfg.bombs;
    nextSeed = cfg.seed;
    std::snprintf(seedBuf, sizeof(seedBuf), "%llu", nextSeed);
    showLargeGridWarning = false;
    endModalDismissed = false;
}

HUDActions GameHUD::drawAndProcess(int screenW, int screenH, const core::Board& board, float timePlayed, net::NetworkManager& net) {
    HUDActions actions;

    float topBarH = 70.0f;
    float topBarY = 0.0f;

    // 1. Top Header Bar
    DrawRectangle(0, static_cast<int>(topBarY), screenW, static_cast<int>(topBarH), Colors::Zinc900Translucent);
    DrawRectangle(0, static_cast<int>(topBarY + topBarH - 1), screenW, 1, Colors::Zinc800);

    float btnH = 36.0f;
    float btnY = topBarY + (topBarH - btnH) * 0.5f;

    if (Widgets::button("<- MENU", { 20, btnY, 96, btnH }, Colors::Zinc800, Colors::Zinc700, false, 16)) {
        actions.returnToMenu = true;
    }

    const char* dimTag = (board.config.dim == 2) ? "2D STANDARD" : ((board.config.dim == 3) ? "3D SLICES" : "4D HYPERCUBE");
    DrawText(dimTag, 130, static_cast<int>(topBarY + (topBarH - 20) * 0.5f), 19, Colors::Zinc300);

    // Mines Remaining Counter
    int remaining = board.config.bombs - static_cast<int>(board.flaggedCount);
    const char* bombStr = TextFormat("MINES: %d", remaining);
    DrawText(bombStr, static_cast<int>(screenW * 0.5f - 130), static_cast<int>(topBarY + (topBarH - 20) * 0.5f), 20, Colors::Red500);

    // Timer Display
    int minutes = static_cast<int>(timePlayed) / 60;
    float seconds = std::fmod(timePlayed, 60.0f);
    const char* timeStr = TextFormat("TIME: %02d:%04.1f", minutes, seconds);
    DrawText(timeStr, static_cast<int>(screenW * 0.5f + 25), static_cast<int>(topBarY + (topBarH - 20) * 0.5f), 20, Colors::Green500);

    // Active Seed Display & Click-to-Copy Pill
    if (copiedSeedTimer > 0.0f) {
        copiedSeedTimer -= GetFrameTime();
    }
    const char* seedPillText = (copiedSeedTimer > 0.0f)
        ? "COPIED!"
        : TextFormat("SEED: %llu", board.config.seed);

    int pillTextW = MeasureText(seedPillText, 13);
    float pillW = static_cast<float>(pillTextW + 18);
    float pillH = 28.0f;
    float pillX = static_cast<float>(screenW * 0.5f + 180.0f);
    float pillY = topBarY + (topBarH - pillH) * 0.5f;

    float maxPillX = (net.role == net::NetRole::Offline)
        ? (screenW - 145.0f - pillW)
        : (screenW - 285.0f - pillW);

    if (pillX <= maxPillX) {
        Rectangle pillRect = { pillX, pillY, pillW, pillH };
        Color pillHover = Colors::Green500;
        if (Widgets::button(seedPillText, pillRect, Colors::Zinc900, pillHover, false, 13)) {
            SetClipboardText(TextFormat("%llu", board.config.seed));
            copiedSeedTimer = 1.5f;
        }
    }

    // Multiplayer Status & Steam Invite Button
    bool steamActive = net::SteamManager::instance().isSteamActive();
    float netBtnY = topBarY + (topBarH - 34.0f) * 0.5f;
    float netTextY = topBarY + (topBarH - 16.0f) * 0.5f;

    if (net.role == net::NetRole::Host) {
        if (steamActive) {
            if (Widgets::button("INVITE (STEAM)", { static_cast<float>(screenW - 275), netBtnY, 135, 34 }, Colors::Zinc800, Colors::Green500, false, 13)) {
                net::SteamManager::instance().openInviteOverlay();
            }
        } else {
            DrawText("HOSTING: Port 7777", screenW - 270, static_cast<int>(netTextY), 16, Colors::Green500);
        }
        if (Widgets::button("DISCONNECT", { static_cast<float>(screenW - 130), netBtnY, 110, 34 }, Colors::Zinc800, Colors::Red500, false, 14)) {
            actions.disconnect = true;
        }
    } else if (net.role == net::NetRole::Client) {
        if (steamActive) {
            if (Widgets::button("INVITE (STEAM)", { static_cast<float>(screenW - 275), netBtnY, 135, 34 }, Colors::Zinc800, Colors::Green500, false, 13)) {
                net::SteamManager::instance().openInviteOverlay();
            }
        } else {
            DrawText("CONNECTED", screenW - 240, static_cast<int>(netTextY), 16, Colors::Green500);
        }
        if (Widgets::button("DISCONNECT", { static_cast<float>(screenW - 130), netBtnY, 110, 34 }, Colors::Zinc800, Colors::Red500, false, 14)) {
            actions.disconnect = true;
        }
    } else {
        if (steamActive) {
            if (Widgets::button("STEAM FRIENDS", { static_cast<float>(screenW - 275), netBtnY, 135, 34 }, Colors::Zinc800, Colors::Zinc700, false, 13)) {
                net::SteamManager::instance().openFriendsOverlay();
            }
        }
        if (Widgets::button("HOST GAME", { static_cast<float>(screenW - 130), netBtnY, 110, 34 }, Colors::Zinc800, Colors::Green600, false, 14)) {
            actions.toggleHost = true;
        }
    }

    // 2. Bottom Footer Control Bar (Uncrammed Two-Row Layout)
    float footerH = 92.0f;
    float footerY = screenH - footerH;
    DrawRectangle(0, static_cast<int>(footerY), screenW, static_cast<int>(footerH), Colors::Zinc900Translucent);
    DrawRectangle(0, static_cast<int>(footerY), screenW, 1, Colors::Zinc800);
    DrawRectangle(16, static_cast<int>(footerY + 46.0f), screenW - 32, 1, Fade(Colors::Zinc800, 0.7f));

    bool isClient = (net.role == net::NetRole::Client);

    // --- ROW 1: Board Configuration (DIM, SIZE, BOMBS, Density Stats, Shift Hint) ---
    float row1Y = footerY + 11.0f;
    float curX = 20.0f;

    Widgets::spinner("DIM", { curX, row1Y }, nextDim, 2, 4, isClient, 38);
    curX += 160.0f;

    Widgets::spinner("SIZE", { curX, row1Y }, nextSize, 4, 200, isClient, 44);
    curX += 166.0f;

    // Calculate max bombs based on nextDim and nextSize
    uint64_t maxCells = 1;
    for (int i = 0; i < nextDim; ++i) maxCells *= nextSize;
    int maxAllowedBombs = static_cast<int>(std::min<uint64_t>(maxCells - 1, 100000));
    nextBombs = std::clamp(nextBombs, 1, maxAllowedBombs);

    Widgets::spinner("BOMBS", { curX, row1Y }, nextBombs, 1, maxAllowedBombs, isClient, 64);
    curX += 184.0f;

    // Live Grid Density & Cell Count Badge
    float density = (maxCells > 0) ? (static_cast<float>(nextBombs) / static_cast<float>(maxCells) * 100.0f) : 0.0f;
    const char* densityStr = TextFormat("(%llu cells, %.1f%% mines)", maxCells, density);
    if (curX + MeasureText(densityStr, 14) < screenW - 170) {
        DrawText(densityStr, static_cast<int>(curX + 6), static_cast<int>(row1Y + 6), 14, Colors::Zinc500);
    }

    // Shift Key Hint on far right of Row 1
    const char* shiftHint = "(Hold SHIFT: +/- 10)";
    int hintW = MeasureText(shiftHint, 13);
    DrawText(shiftHint, screenW - hintW - 20, static_cast<int>(row1Y + 6), 13, Colors::Zinc600);

    // --- ROW 2: Action & Seed Controls (NEW GAME, RANDOM SEED, SEED INPUT, REROLL, FPS) ---
    float row2Y = footerY + 53.0f;

    curX = 20.0f;

    float btnW = 120.0f;
    if (Widgets::button("NEW GAME", { curX, row2Y, btnW, 30 }, Colors::Zinc800, Colors::Green500, isClient, 15)) {
        if (randomizeSeed) {
            nextSeed = (static_cast<uint64_t>(GetTime() * 100000.0) ^ 0x01252D8F21ULL);
            if (nextSeed == 0) nextSeed = 12345;
            std::snprintf(seedBuf, sizeof(seedBuf), "%llu", nextSeed);
        } else {
            nextSeed = parseSeed(seedBuf);
            if (nextSeed == 0) nextSeed = 12345;
            std::snprintf(seedBuf, sizeof(seedBuf), "%llu", nextSeed);
        }

        if (maxCells >= 100000) {
            showLargeGridWarning = true;
        } else {
            actions.requestNewGame = true;
        }
    }
    curX += btnW + 18.0f;

    // Subtle vertical separator between NEW GAME and Seed Controls
    DrawRectangle(static_cast<int>(curX - 9.0f), static_cast<int>(row2Y + 2.0f), 1, 26, Colors::Zinc700);

    // RANDOM SEED Checkbox
    Widgets::checkbox("RANDOM SEED", { curX, row2Y + 4.0f }, randomizeSeed, isClient);
    curX += 140.0f;

    // SEED: Label
    int lblW = MeasureText("SEED:", 16);
    DrawText("SEED:", static_cast<int>(curX), static_cast<int>(row2Y + 6.0f), 16, isClient ? Colors::Zinc600 : (randomizeSeed ? Colors::Zinc500 : Colors::Zinc300));
    curX += lblW + 10.0f;

    // SEED Text Input Box
    float rightReserved = 160.0f; // Reserved for REROLL button + FPS text + padding
    float boxW = std::clamp(screenW - curX - rightReserved, 160.0f, 260.0f);
    bool wasActive = seedInputActive;
    Widgets::textInput({ curX, row2Y, boxW, 30 }, seedBuf, sizeof(seedBuf), seedInputActive, "Seed (number or text)", true, 15);

    // If user clicked or focused into seed box, automatically switch off RANDOM
    if (!wasActive && seedInputActive) {
        randomizeSeed = false;
    }

    if (seedInputActive) {
        nextSeed = parseSeed(seedBuf);
    } else if (wasActive) {
        nextSeed = parseSeed(seedBuf);
        std::snprintf(seedBuf, sizeof(seedBuf), "%llu", nextSeed);
    }
    curX += boxW + 8.0f;

    // REROLL Button
    float rndW = 68.0f;
    if (Widgets::button("REROLL", { curX, row2Y, rndW, 30 }, Colors::Zinc800, Colors::Green500, isClient, 13)) {
        nextSeed = (static_cast<uint64_t>(GetTime() * 100000.0) ^ 0x01252D8F21ULL);
        if (nextSeed == 0) nextSeed = 12345;
        std::snprintf(seedBuf, sizeof(seedBuf), "%llu", nextSeed);
        randomizeSeed = false; // User explicitly rolled a seed to play!
        seedInputActive = false;
    }

    // Clean FPS counter anchored to bottom-right corner of footer
    const char* fpsStr = TextFormat("%d FPS", GetFPS());
    int fpsW = MeasureText(fpsStr, 13);
    DrawText(fpsStr, screenW - fpsW - 20, static_cast<int>(row2Y + 8.0f), 13, Colors::Zinc600);

    // 3. Large Grid Warning Modal
    if (showLargeGridWarning) {
        const char* msg = TextFormat("Generating %llu cells may take a moment. Proceed?", maxCells);
        int resp = Widgets::modal(screenW, screenH, "LARGE GRID WARNING", msg, "GENERATE", "CANCEL", true);
        if (resp == 1) {
            showLargeGridWarning = false;
            actions.requestNewGame = true;
        } else if (resp == 2 || resp == 3) {
            showLargeGridWarning = false;
        }
    }

    // 4. Game Over / Victory Modal
    if (board.isGameOver || board.isVictory) {
        if (!endModalDismissed) {
            const char* title = board.isVictory ? "VICTORY!" : "GAME OVER";
            const char* msg = "";
            const char* btnText = "PLAY AGAIN (R)";
            bool btnLocked = false;

            if (isClient) {
                btnText = "WAITING FOR HOST...";
                btnLocked = true;
                msg = board.isVictory
                    ? TextFormat("Cleared all safe cells in %02d:%04.1f!\nSeed: %llu\nWaiting for host to restart.", minutes, seconds, board.config.seed)
                    : TextFormat("A mine was detonated!\nSeed: %llu\nWaiting for host to restart.", board.config.seed);
            } else {
                msg = board.isVictory
                    ? TextFormat("Cleared all safe cells in %02d:%04.1f!\nSeed: %llu", minutes, seconds, board.config.seed)
                    : TextFormat("You detonated a mine!\nSeed: %llu\nPress R or Play Again.", board.config.seed);
            }

            int resp = Widgets::modal(screenW, screenH, title, msg, btnText, nullptr, true, btnLocked);
            if (resp == 1 || (!isClient && IsKeyPressed(KEY_R))) {
                if (!isClient) {
                    actions.restartGame = true;
                }
            } else if (resp == 3) {
                // User closed modal with 'X' or Escape to inspect board
                endModalDismissed = true;
            }
        } else {
            // Modal is dismissed/minimized: display an unobtrusive floating badge so user can re-open it
            const char* bannerText = board.isVictory
                ? (isClient ? "VICTORY (Click to View Stats)" : "VICTORY (Click to View Stats / R to Restart)")
                : (isClient ? "GAME OVER (Click to View Stats)" : "GAME OVER (Click to View Stats / R to Restart)");

            int bW = MeasureText(bannerText, 16) + 36;
            Rectangle badgeRect = { (screenW - bW) * 0.5f, 78.0f, static_cast<float>(bW), 32.0f };

            Color badgeCol = board.isVictory ? Colors::Green500 : Colors::Red500;
            if (Widgets::button(bannerText, badgeRect, Colors::Zinc900, badgeCol, false, 15)) {
                endModalDismissed = false; // Re-open stats modal!
            }

            if (!isClient && IsKeyPressed(KEY_R)) {
                actions.restartGame = true;
            }
        }
    }

    return actions;
}

} // namespace minesweeper::ui
