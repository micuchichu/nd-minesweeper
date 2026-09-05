#pragma once

#include "core/board.hpp"
#include "render/raylib_renderer.hpp"
#include "net/network_manager.hpp"
#include "ui/hud.hpp"
#include "ui/menu.hpp"
#include "audio/voice_manager.hpp"

namespace minesweeper {

enum class AppState {
    Menu,
    InGame
};

class App {
public:
    uint64_t initialLobbyId = 0;

    App();
    ~App();

    void run();

private:
    AppState state = AppState::Menu;
    bool shouldQuit = false;
    float timePlayed = 0.0f;

    core::Board board;
    render::RaylibRenderer renderer;
    net::NetworkManager net;
    ui::GameHUD hud;
    ui::MainMenu menu;
    audio::VoiceManager voiceMgr;

    void init();
    void update(float dt);
    void draw();
    void cleanup();

    void handleNetEvents();
    void startNewGame(int dim, int size, int bombs, uint64_t seed);
    void restartCurrentGame();

    void saveSettings();
    void loadSettings();
};

} // namespace minesweeper
