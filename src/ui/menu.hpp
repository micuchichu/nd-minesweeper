#pragma once

#include <raylib.h>
#include "../net/network_manager.hpp"
#include "../audio/voice_manager.hpp"
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

enum class SettingsTab {
    Graphics = 0,
    Audio = 1
};

struct MenuActions {
    bool playSolo = false;
    bool hostGame = false;
    bool joinGame = false;
    bool quit = false;
    uint16_t hostPort = 7777;
    std::string joinAddress = "127.0.0.1:7777";
    bool toggleCRT = false;
    bool vsyncChanged = false;
    bool fpsLimitChanged = false;
    bool guiScaleChanged = false;
};

class MainMenu {
public:
    MenuScreen currentScreen = MenuScreen::Main;
    CustomizeTab activeTab = CustomizeTab::Cursors;
    SettingsTab activeSettingsTab = SettingsTab::Graphics;

    char playerName[16] = "Player";
    char joinIpBuf[64] = "127.0.0.1:7777";
    char hostPortBuf[16] = "7777";
    std::string statusMessage = "";
    bool crtEnabled = true;
    bool vsyncEnabled = true;
    bool showFPS = false;
    float fpsLimit = 144.0f;
    float guiScale = 1.0f;

    int cursorSkin = 0;
    int flagSkin = 0;
    int playerSkin = 0;

    uint64_t scrapCount = 0;
    Texture2D scrapTexture = {0};

    audio::VoiceSettings voiceSettings;
    float micInputLevel = 0.0f;

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
