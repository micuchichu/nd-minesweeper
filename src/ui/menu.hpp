#pragma once

#include <raylib.h>
#include "../net/network_manager.hpp"
#include "../audio/voice_manager.hpp"
#include "../core/types.hpp"
#include <string>

namespace minesweeper::core {
    class SaveManager;
}

namespace minesweeper::ui {

enum class MenuScreen {
    Main,
    Play, // Save / World selection
    NewSave,
    HostConfirm,
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
    Audio = 1,
    Controls = 2
};

struct MenuActions {
    bool playSolo = false;
    bool hostGame = false;
    bool joinGame = false;
    bool quit = false;
    uint16_t hostPort = 7777;
    std::string joinAddress = "127.0.0.1:7777";
    int selectedSlot = 1;
    bool startNewInSlot = false;
    core::BoardConfig newSlotConfig;
    std::string newSlotName = "";
    bool toggleCRT = false;
    bool vsyncChanged = false;
    bool fpsLimitChanged = false;
    bool guiScaleChanged = false;
    bool controlModeChanged = false;
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
    int controlMode = 0; // 0 = Mouse Follower, 1 = Keyboard / Controller

    int cursorSkin = 0;
    int flagSkin = 0;
    int playerSkin = 0;

    uint64_t scrapCount = 0;
    Texture2D scrapTexture{};

    core::SaveManager* saveManager = nullptr;
    int selectedSlot = 1;
    int confirmingDeleteSlot = 0;

    // New Save Configuration fields
    char newSaveNameBuf[32] = "World 1";
    int newSaveDim = 2;
    int newSaveSize = 10;
    int newSaveBombs = 15;
    uint64_t newSaveSeed = 12345;
    char newSaveSeedBuf[32] = "12345";
    bool newSaveSeedActive = false;
    bool newSaveNameActive = false;
    bool newSaveLaunchAsHost = false;

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
