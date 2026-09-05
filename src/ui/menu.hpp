#pragma once

#include <raylib.h>
#include "../net/network_manager.hpp"
#include <string>

namespace minesweeper::ui {

enum class MenuScreen {
    Main,
    Play,
    Host,
    Join,
    Customize,
    Settings
};

enum class CustomizeTab {
    Cursors = 0,
    Flags = 1
};

struct MenuActions {
    bool playSolo = false;
    bool hostGame = false;
    bool joinGame = false;
    bool quit = false;
    uint16_t hostPort = 7777;
    std::string joinAddress = "127.0.0.1:7777";
    bool toggleCRT = false;
};

class MainMenu {
public:
    MenuScreen currentScreen = MenuScreen::Main;
    CustomizeTab activeTab = CustomizeTab::Cursors;

    char playerName[16] = "Player";
    char joinIpBuf[64] = "127.0.0.1:7777";
    char hostPortBuf[16] = "7777";
    std::string statusMessage = "";
    bool crtEnabled = true;

    int cursorSkin = 0;
    int flagSkin = 0;
    int playerSkin = 0;

    // Horizontal Reel state
    float reelScrollX = 0.0f;
    float reelTargetScrollX = 0.0f;
    bool reelIsDragging = false;
    float reelDragStartX = 0.0f;
    float reelDragStartScroll = 0.0f;

    MainMenu();

    MenuActions drawAndProcess(int screenW, int screenH);
};


} // namespace minesweeper::ui
