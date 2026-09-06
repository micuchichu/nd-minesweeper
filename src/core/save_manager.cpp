#include "save_manager.hpp"
#include <fstream>
#include <filesystem>
#include <cstring>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <iostream>

namespace minesweeper::core {

namespace {

constexpr uint32_t SLOT_MAGIC = 0x5057534D; // 'MSWP'
constexpr uint32_t SLOT_VERSION = 1;

constexpr uint32_t SETTINGS_MAGIC = 0x53544553; // 'SETS'
constexpr uint32_t SETTINGS_VERSION = 1;

std::string formatTimestamp(int64_t epochSeconds) {
    if (epochSeconds <= 0) return "Never";
    std::time_t t = static_cast<std::time_t>(epochSeconds);
    std::tm tmVal;
#if defined(_WIN32)
    localtime_s(&tmVal, &t);
#else
    localtime_r(&t, &tmVal);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tmVal, "%Y-%m-%d %H:%M");
    return ss.str();
}

// Legacy savegame structure for automatic migration
struct LegacySaveData {
    char joinIp[64];
    char hostPort[16];
    char playerName[16];
    int dim;
    int size;
    int bombs;
    uint64_t seed;
    bool crtEnabled;
    uint8_t cursorSkin;
    bool randomizeSeed;
    uint8_t flagSkin;
    uint8_t playerSkin;
    bool voiceEnabled;
    bool voiceProximity;
    bool voicePushToTalk;
    float voiceVolume;
    float micGain;
    bool vsyncEnabled;
    bool showFPS;
    float fpsLimit;
    float guiScale;
    uint64_t scrapCount;
};

} // namespace

void SaveManager::init() {
    std::filesystem::create_directories("saves");
    migrateLegacySavegameIfNeeded();
}

std::string SaveManager::getSlotPath(int slotIndex) const {
    return "saves/slot_" + std::to_string(slotIndex) + ".dat";
}

void SaveManager::migrateLegacySavegameIfNeeded() {
    std::string settingsPath = "settings.dat";
    std::string legacyPath = "savegame.dat";

    if (!std::filesystem::exists(settingsPath) && std::filesystem::exists(legacyPath)) {
        std::ifstream file(legacyPath, std::ios::binary);
        if (file.is_open()) {
            LegacySaveData leg{};
            file.read(reinterpret_cast<char*>(&leg), sizeof(LegacySaveData));
            std::streamsize bytes = file.gcount();
            file.close();

            if (bytes >= static_cast<std::streamsize>(sizeof(leg.joinIp) + sizeof(leg.hostPort) + sizeof(leg.playerName))) {
                GlobalSettings gs;
                std::memcpy(gs.joinIp, leg.joinIp, sizeof(gs.joinIp));
                std::memcpy(gs.hostPort, leg.hostPort, sizeof(gs.hostPort));
                std::memcpy(gs.playerName, leg.playerName, sizeof(gs.playerName));
                gs.crtEnabled = leg.crtEnabled;
                gs.cursorSkin = leg.cursorSkin;
                gs.randomizeSeed = leg.randomizeSeed;
                gs.flagSkin = leg.flagSkin;
                gs.playerSkin = leg.playerSkin;
                gs.voiceEnabled = leg.voiceEnabled;
                gs.voiceProximity = leg.voiceProximity;
                gs.voicePushToTalk = leg.voicePushToTalk;
                gs.voiceVolume = leg.voiceVolume;
                gs.micGain = leg.micGain;
                gs.vsyncEnabled = leg.vsyncEnabled;
                gs.showFPS = leg.showFPS;
                gs.fpsLimit = leg.fpsLimit;
                gs.guiScale = leg.guiScale;
                gs.lastActiveSlot = 1;
                saveGlobalSettings(gs);

                // Migrate slot 1 if not exists
                if (!std::filesystem::exists(getSlotPath(1))) {
                    Board b;
                    int d = (leg.dim >= 2 && leg.dim <= 4) ? leg.dim : 2;
                    int s = (leg.size >= 4 && leg.size <= 1000) ? leg.size : 10;
                    int bm = (leg.bombs >= 1) ? leg.bombs : 15;
                    uint64_t sd = (leg.seed != 0) ? leg.seed : 12345;
                    b.init(d, s, bm, sd);
                    saveSlot(1, "World 1", b, 0.0f, leg.scrapCount);
                }
            }
        }
    }
}

bool SaveManager::loadGlobalSettings(GlobalSettings& outSettings) {
    std::ifstream file("settings.dat", std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    uint32_t magic = 0;
    uint32_t version = 0;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    file.read(reinterpret_cast<char*>(&version), sizeof(version));

    if (magic != SETTINGS_MAGIC || version != SETTINGS_VERSION) {
        return false;
    }

    file.read(reinterpret_cast<char*>(&outSettings), sizeof(GlobalSettings));
    return file.good();
}

bool SaveManager::saveGlobalSettings(const GlobalSettings& inSettings) {
    std::ofstream file("settings.dat.tmp", std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    uint32_t magic = SETTINGS_MAGIC;
    uint32_t version = SETTINGS_VERSION;
    file.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));
    file.write(reinterpret_cast<const char*>(&inSettings), sizeof(GlobalSettings));
    file.close();

    std::error_code ec;
    std::filesystem::rename("settings.dat.tmp", "settings.dat", ec);
    return !ec;
}

std::vector<SaveSlotMetadata> SaveManager::getSlots() {
    std::vector<SaveSlotMetadata> slots(NUM_SLOTS);
    for (int i = 0; i < NUM_SLOTS; ++i) {
        slots[i].slotIndex = i + 1;
        slots[i].slotName = "Slot " + std::to_string(i + 1);
        getSlotMetadata(i + 1, slots[i]);
    }
    return slots;
}

bool SaveManager::getSlotMetadata(int slotIndex, SaveSlotMetadata& outMeta) {
    outMeta.slotIndex = slotIndex;
    outMeta.isEmpty = true;

    std::string path = getSlotPath(slotIndex);
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    uint32_t magic = 0;
    uint32_t version = 0;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    file.read(reinterpret_cast<char*>(&version), sizeof(version));

    if (magic != SLOT_MAGIC || version != SLOT_VERSION) {
        return false;
    }

    uint32_t nameLen = 0;
    file.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
    if (nameLen > 128) return false;
    std::string name(nameLen, '\0');
    file.read(&name[0], nameLen);
    outMeta.slotName = name;

    int32_t dim = 0, size = 0, bombs = 0;
    uint64_t seed = 0;
    file.read(reinterpret_cast<char*>(&dim), sizeof(dim));
    file.read(reinterpret_cast<char*>(&size), sizeof(size));
    file.read(reinterpret_cast<char*>(&bombs), sizeof(bombs));
    file.read(reinterpret_cast<char*>(&seed), sizeof(seed));

    float timePlayed = 0.0f;
    uint8_t isGameOver = 0, isVictory = 0;
    uint64_t revealedCount = 0, flaggedCount = 0, scrapCount = 0;
    int64_t timestamp = 0;

    file.read(reinterpret_cast<char*>(&timePlayed), sizeof(timePlayed));
    file.read(reinterpret_cast<char*>(&isGameOver), sizeof(isGameOver));
    file.read(reinterpret_cast<char*>(&isVictory), sizeof(isVictory));
    file.read(reinterpret_cast<char*>(&revealedCount), sizeof(revealedCount));
    file.read(reinterpret_cast<char*>(&flaggedCount), sizeof(flaggedCount));
    file.read(reinterpret_cast<char*>(&scrapCount), sizeof(scrapCount));
    file.read(reinterpret_cast<char*>(&timestamp), sizeof(timestamp));

    if (!file.good()) return false;

    outMeta.isEmpty = false;
    outMeta.dim = dim;
    outMeta.size = size;
    outMeta.bombs = bombs;
    outMeta.seed = seed;
    outMeta.timePlayed = timePlayed;
    outMeta.isGameOver = (isGameOver != 0);
    outMeta.isVictory = (isVictory != 0);
    outMeta.revealedCount = static_cast<size_t>(revealedCount);
    outMeta.flaggedCount = static_cast<size_t>(flaggedCount);
    outMeta.scrapCount = scrapCount;
    outMeta.lastPlayedTime = formatTimestamp(timestamp);

    uint64_t total = 1;
    for (int i = 0; i < dim; ++i) total *= size;
    outMeta.totalCells = static_cast<size_t>(total);

    return true;
}

bool SaveManager::loadSlot(int slotIndex, core::Board& board, float& timePlayed, uint64_t& scrapCount, std::string& slotName) {
    std::string path = getSlotPath(slotIndex);
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    uint32_t magic = 0;
    uint32_t version = 0;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    file.read(reinterpret_cast<char*>(&version), sizeof(version));

    if (magic != SLOT_MAGIC || version != SLOT_VERSION) {
        return false;
    }

    uint32_t nameLen = 0;
    file.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
    if (nameLen > 128) return false;
    slotName.resize(nameLen);
    file.read(&slotName[0], nameLen);

    int32_t dim = 0, size = 0, bombs = 0;
    uint64_t seed = 0;
    file.read(reinterpret_cast<char*>(&dim), sizeof(dim));
    file.read(reinterpret_cast<char*>(&size), sizeof(size));
    file.read(reinterpret_cast<char*>(&bombs), sizeof(bombs));
    file.read(reinterpret_cast<char*>(&seed), sizeof(seed));

    uint8_t isGameOver = 0, isVictory = 0;
    uint64_t revCount = 0, flgCount = 0;
    int64_t timestamp = 0;

    file.read(reinterpret_cast<char*>(&timePlayed), sizeof(timePlayed));
    file.read(reinterpret_cast<char*>(&isGameOver), sizeof(isGameOver));
    file.read(reinterpret_cast<char*>(&isVictory), sizeof(isVictory));
    file.read(reinterpret_cast<char*>(&revCount), sizeof(revCount));
    file.read(reinterpret_cast<char*>(&flgCount), sizeof(flgCount));
    file.read(reinterpret_cast<char*>(&scrapCount), sizeof(scrapCount));
    file.read(reinterpret_cast<char*>(&timestamp), sizeof(timestamp));

    // Initialize board layout from seed & dimensions
    board.init(dim, size, bombs, seed);
    board.isGameOver = (isGameOver != 0);
    board.isVictory = (isVictory != 0);

    // Read state words
    uint32_t wordCount = 0;
    file.read(reinterpret_cast<char*>(&wordCount), sizeof(wordCount));
    if (wordCount == board.state.data.size()) {
        file.read(reinterpret_cast<char*>(board.state.data.data()), wordCount * sizeof(uint64_t));
        board.revealedCount = board.state.countRevealed();
        board.flaggedCount = 0;
        for (size_t i = 0; i < board.coord.totalCells; ++i) {
            if (board.state.get(i) == CellState::Flagged) {
                ++board.flaggedCount;
            }
        }
    }

    // Read flag owners
    uint32_t flagCount = 0;
    file.read(reinterpret_cast<char*>(&flagCount), sizeof(flagCount));
    board.flagOwners.clear();
    for (uint32_t i = 0; i < flagCount; ++i) {
        uint64_t idx = 0;
        uint32_t placerId = 0;
        uint8_t skinId = 0;
        file.read(reinterpret_cast<char*>(&idx), sizeof(idx));
        file.read(reinterpret_cast<char*>(&placerId), sizeof(placerId));
        file.read(reinterpret_cast<char*>(&skinId), sizeof(skinId));
        board.flagOwners[static_cast<size_t>(idx)] = { placerId, skinId };
    }

    return file.good();
}

bool SaveManager::saveSlot(int slotIndex, const std::string& slotName, const core::Board& board, float timePlayed, uint64_t scrapCount) {
    std::string tempPath = getSlotPath(slotIndex) + ".tmp";
    std::ofstream file(tempPath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    uint32_t magic = SLOT_MAGIC;
    uint32_t version = SLOT_VERSION;
    file.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));

    uint32_t nameLen = static_cast<uint32_t>(slotName.size());
    file.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
    file.write(slotName.data(), nameLen);

    int32_t dim = board.config.dim;
    int32_t size = board.config.size;
    int32_t bombs = board.config.bombs;
    uint64_t seed = board.config.seed;
    file.write(reinterpret_cast<const char*>(&dim), sizeof(dim));
    file.write(reinterpret_cast<const char*>(&size), sizeof(size));
    file.write(reinterpret_cast<const char*>(&bombs), sizeof(bombs));
    file.write(reinterpret_cast<const char*>(&seed), sizeof(seed));

    uint8_t isGameOver = board.isGameOver ? 1 : 0;
    uint8_t isVictory = board.isVictory ? 1 : 0;
    uint64_t revCount = static_cast<uint64_t>(board.revealedCount);
    uint64_t flgCount = static_cast<uint64_t>(board.flaggedCount);
    int64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    file.write(reinterpret_cast<const char*>(&timePlayed), sizeof(timePlayed));
    file.write(reinterpret_cast<const char*>(&isGameOver), sizeof(isGameOver));
    file.write(reinterpret_cast<const char*>(&isVictory), sizeof(isVictory));
    file.write(reinterpret_cast<const char*>(&revCount), sizeof(revCount));
    file.write(reinterpret_cast<const char*>(&flgCount), sizeof(flgCount));
    file.write(reinterpret_cast<const char*>(&scrapCount), sizeof(scrapCount));
    file.write(reinterpret_cast<const char*>(&timestamp), sizeof(timestamp));

    // Write bitboard state words
    uint32_t wordCount = static_cast<uint32_t>(board.state.data.size());
    file.write(reinterpret_cast<const char*>(&wordCount), sizeof(wordCount));
    file.write(reinterpret_cast<const char*>(board.state.data.data()), wordCount * sizeof(uint64_t));

    // Write flag owners
    uint32_t flagCount = static_cast<uint32_t>(board.flagOwners.size());
    file.write(reinterpret_cast<const char*>(&flagCount), sizeof(flagCount));
    for (const auto& [idx, info] : board.flagOwners) {
        uint64_t fIdx = static_cast<uint64_t>(idx);
        uint32_t pId = info.placerId;
        uint8_t sId = info.skinId;
        file.write(reinterpret_cast<const char*>(&fIdx), sizeof(fIdx));
        file.write(reinterpret_cast<const char*>(&pId), sizeof(pId));
        file.write(reinterpret_cast<const char*>(&sId), sizeof(sId));
    }

    file.close();

    std::error_code ec;
    std::filesystem::rename(tempPath, getSlotPath(slotIndex), ec);
    return !ec;
}

bool SaveManager::createSlot(int slotIndex, const std::string& slotName, const core::BoardConfig& cfg) {
    Board b;
    b.init(cfg.dim, cfg.size, cfg.bombs, cfg.seed);
    return saveSlot(slotIndex, slotName, b, 0.0f, 0);
}

bool SaveManager::deleteSlot(int slotIndex) {
    std::string path = getSlotPath(slotIndex);
    std::error_code ec;
    return std::filesystem::remove(path, ec);
}

} // namespace minesweeper::core
