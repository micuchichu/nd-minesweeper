#pragma once

#include "board.hpp"
#include <string>
#include <vector>
#include <cstdint>

namespace minesweeper::core {

struct SaveSlotMetadata {
    int slotIndex = 1;
    std::string slotName = "Save Slot";
    bool isEmpty = true;
    int dim = 2;
    int size = 10;
    int bombs = 15;
    uint64_t seed = 12345;
    float timePlayed = 0.0f;
    bool isGameOver = false;
    bool isVictory = false;
    size_t revealedCount = 0;
    size_t flaggedCount = 0;
    size_t totalCells = 100;
    uint64_t scrapCount = 0;
    std::string lastPlayedTime = "";
};

struct GlobalSettings {
    char playerName[16] = "Player";
    char joinIp[64] = "127.0.0.1:7777";
    char hostPort[16] = "7777";
    bool crtEnabled = true;
    uint8_t cursorSkin = 0;
    uint8_t flagSkin = 0;
    uint8_t playerSkin = 0;
    bool randomizeSeed = true;

    // Voice settings
    bool voiceEnabled = true;
    bool voiceProximity = true;
    bool voicePushToTalk = true;
    float voiceVolume = 1.0f;
    float micGain = 1.0f;

    // Display / Graphics
    bool vsyncEnabled = true;
    bool showFPS = false;
    float fpsLimit = 144.0f;
    float guiScale = 1.0f;

    int lastActiveSlot = 1;
    int controlMode = 0; // 0 = Mouse Follower, 1 = Keyboard / Controller
};

class SaveManager {
public:
    static constexpr int NUM_SLOTS = 3;

    SaveManager() = default;
    ~SaveManager() = default;

    void init();

    // Global Settings
    bool loadGlobalSettings(GlobalSettings& outSettings);
    bool saveGlobalSettings(const GlobalSettings& inSettings);

    // Save Slots
    std::vector<SaveSlotMetadata> getSlots();
    bool getSlotMetadata(int slotIndex, SaveSlotMetadata& outMeta);

    bool loadSlot(int slotIndex, core::Board& board, float& timePlayed, uint64_t& scrapCount, std::string& slotName);
    bool saveSlot(int slotIndex, const std::string& slotName, const core::Board& board, float timePlayed, uint64_t scrapCount);
    bool createSlot(int slotIndex, const std::string& slotName, const core::BoardConfig& cfg);
    bool deleteSlot(int slotIndex);

private:
    std::string getSlotPath(int slotIndex) const;
    void migrateLegacySavegameIfNeeded();
};

} // namespace minesweeper::core
