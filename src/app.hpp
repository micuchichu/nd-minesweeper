#pragma once

#include "core/board.hpp"
#include "render/raylib_renderer.hpp"
#include "net/network_manager.hpp"
#include "ui/hud.hpp"
#include "ui/menu.hpp"
#include "audio/voice_manager.hpp"
#include "render/scrap_system.hpp"
#include "core/save_manager.hpp"

namespace minesweeper {

enum class AppState {
    Menu,
    InGame
};

class App {
public:
    uint64_t initialLobbyId = 0;
    bool testShopMode = false;
    bool testCustomizeMode = false;

    App();
    ~App();

    void run();

private:
    AppState state = AppState::Menu;
    bool shouldQuit = false;
    float timePlayed = 0.0f;
    uint64_t scrapCount = 0;
    int64_t pendingUncoverCell = -1;
    int64_t currentHoveredCell = -1;
    float mouseAimTimer = 0.0f;
    Vector2 prevMousePos = { -9999.0f, -9999.0f };
    int navLastDirX = 0;
    int navLastDirY = 0;
    float navHoldTimer = 0.0f;
    float navRepeatTimer = 0.0f;

    core::Board board;
    render::RaylibRenderer renderer;
    render::ScrapSystem scrapSystem;
    net::NetworkManager net;
    ui::GameHUD hud;
    ui::MainMenu menu;
    audio::VoiceManager voiceMgr;

    void init();
    void update(float dt);
    void draw();
    void cleanup();

    void handleNetEvents();
    void broadcastLaser(Vector2 from, Vector2 to, uint8_t laserType = 0);
    void startNewGame(int dim, int size, int bombs, uint64_t seed);
    void restartCurrentGame();

    core::SaveManager saveMgr;
    int activeSaveSlot = 1;
    std::string activeSaveName = "World 1";

    void saveCurrentSlot();
    bool loadSaveSlot(int slotIndex);
    void saveSettings();
    void loadSettings();
};

} // namespace minesweeper
