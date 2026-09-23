#pragma once

#include "board.hpp"
#include "ship_base.hpp"
#include <vector>
#include <string>
#include <cstdint>
#include <memory>

namespace minesweeper::render {
    class ParticleSystem;
}

namespace minesweeper::core {

struct SectorWall {
    Rectangle rect{ 0, 0, 0, 0 };
    bool isHazard = false; // Render hazard caution striping
    bool isCorridor = false;
};

struct OrbitalLauncher {
    Rectangle openingBounds{ 0, 0, 0, 0 }; // Launch rail entry / acceleration cradle
    Rectangle barrierBounds{ 0, 0, 0, 0 }; // Gantry clamp / magnetic restraint field when offline
    bool isLocked = true;                  // Locked until sector is 100% secured
    float openAnim = 0.0f;                 // 0.0f = locked/offline, 1.0f = fully armed/online
    Vector2 beaconPosA{ 0, 0 };            // Upper launcher gantry tower strobe
    Vector2 beaconPosB{ 0, 0 };            // Lower launcher gantry tower strobe
    Vector2 launchVector{ 1.0f, 0.0f };    // Trajectory vector for orbital mass acceleration
};
using SectorGateway = OrbitalLauncher; // Backward compatibility alias

struct MerchantSpawn {
    std::string type = "shop1"; // "shop1", "shop2", "shop3", "roulette"
    Vector2 pos{ 85.0f, 70.0f };
    float angle = 0.0f;
};

struct SectorConfig {
    int id = 1;
    std::string name = "Sector 01: Outpost Alpha";
    std::string codename = "OUTPOST-ALPHA";
    std::string subtitle = "Primary Drop Zone & Logistics Depot";
    std::string description;
    int threatLevel = 1;

    // Map
    int gridSize = 8;
    int bombCount = 10;
    int dimension = 2;
    float wallThickness = 28.0f;
    float westMargin = 180.0f;
    float eastMargin = 180.0f;
    float vertMargin = 120.0f;
    Vector2 spawnPos{ 80.0f, -1.0f };       // -1 means auto-center Y
    Vector2 stagingDepotPos{ 90.0f, -1.0f }; // -1 means auto-center Y
    std::string stagingDepotTitle;
    std::vector<SectorWall> customWalls;

    // Progression
    bool isUnlocked = false;
    std::vector<int> unlocks;
    bool hasExitLauncher = true;
    int launcherTargetSector = -1;
    float launcherOpeningHeight = 140.0f;

    // Ships
    bool allowShops = false;
    bool allowRoulette = false;
    std::vector<std::string> allowedShipTypes;
    Vector2 shopDockPos{ 85.0f, 70.0f };
    Vector2 rouletteDockPos{ 85.0f, -70.0f }; // negative means offset from arena bottom
    std::vector<MerchantSpawn> merchantSpawns;
};

struct PlanetVisualConfig {
    Color iconColor{ 239, 68, 68, 255 };
    Color atmosphereColor{ 244, 63, 94, 255 };
    Color wireframeColor{ 180, 75, 95, 255 };
    std::vector<Color> surfaceTones = {
        Color{ 28, 14, 20, 255 },
        Color{ 38, 18, 24, 255 },
        Color{ 48, 22, 30, 255 },
        Color{ 24, 12, 16, 255 },
        Color{ 58, 28, 36, 255 }
    };
    bool hasMagmaRifts = true;
    Color magmaRiftColor{ 160, 40, 52, 255 };
    float magmaRiftChance = 0.09f;
    std::string geologicalStatus = "Volcanic Basalt & Obsidian Crust";
};

struct PlanetConfig {
    int id = 1;
    std::string name = "Tartarus-IV";
    std::string codename = "TARTARUS-04";
    std::string systemName = "Tartarus Sub-Cluster 09";
    std::string tagline = "QUARANTINE MINING ARRAY";
    std::string description;
    std::string missionDirective = "Clear the planet. Sector by sector.";
    int threatLevel = 1;
    uint64_t seed = 12345;
    bool isUnlocked = true;
    std::vector<int> unlocksPlanets;
    std::vector<int> sectors = { 1, 2, 3, 4 };
    PlanetVisualConfig visual;
    std::vector<std::string> intelDossier;
};

struct CampaignSector {
    int id = 1;                     // Sector 1 to 4
    std::string name;              // e.g. "Sector 01: Outpost Alpha"
    std::string codename;          // e.g. "ALPHA-01"
    std::string subtitle;          // e.g. "Landing Site & Extraction Depot"
    std::string loreBriefing;      // Tactical briefing / lore dossier
    int threatLevel = 1;

    Vector2 worldOffset{ 0, 0 };   // Top-left of sector walled arena
    Vector2 gridOffset{ 0, 0 };    // Top-left of minefield board inside arena
    int gridSize = 8;
    int bombCount = 10;
    int dimension = 2;
    uint64_t seed = 0;

    Board board;                   // Sector minesweeper board
    bool isUnlocked = false;       // Accessible by player
    bool isCleared = false;        // All safe cells revealed
    float clearAnimTimer = 0.0f;

    std::vector<int> unlocksSectors;       // Sectors unlocked upon completion
    int launcherTargetSectorId = -1;       // Destination sector ID when entering launcher

    bool allowShops = false;               // Allow contractor shop ships
    bool allowRoulette = false;            // Allow roulette casino ship
    std::string stagingDepotTitle;         // Custom staging depot title
    std::vector<std::string> allowedShipTypes;
    Vector2 customShopDockPos{ 85.0f, 70.0f };
    Vector2 customRouletteDockPos{ 85.0f, -70.0f };
    std::vector<MerchantSpawn> merchantSpawns;

    Rectangle arenaBounds{ 0, 0, 0, 0 };
    Vector2 spawnPos{ 0, 0 };      // Entrance spawn point when entering this sector world
    Vector2 stagingDepotPos{ 0, 0 };
    Rectangle stagingDepotBounds{ 0, 0, 0, 0 };
    std::vector<SectorWall> walls;
    OrbitalLauncher exitLauncher;
    bool hasExitLauncher = true;
    SectorConfig config;

    bool containsWorldPos(Vector2 pos) const;
    bool containsGridWorldPos(Vector2 pos) const;
    Vector2 getCellWorldPosition(size_t cellIndex, float cellSize = 40.0f) const;
    int64_t getCellIndexAtWorldPos(Vector2 worldPos, float cellSize = 40.0f) const;
};

class CampaignManager {
public:
    static constexpr int NUM_SECTORS = 4;
    static constexpr uint64_t SECTOR_CELL_STRIDE = 1000000ULL;

    uint64_t planetSeed = 12345;
    std::string planetName = "Tartarus-IV";
    std::string systemName = "Tartarus Sub-Cluster 09";
    std::string missionDirective = "Clear the planet. Sector by sector.";
    std::string loreBackground = 
        "Orbital scans indicate ancient automated defense clusters and subterranean "
        "sub-munitions have infested the planetary crust. All civilian and industrial "
        "activity is suspended under quarantine. As deep-space clearance contractors, "
        "your team is deployed to systematically neutralize each fortified sector.";

    std::vector<CampaignSector> sectors;
    int activeSectorIndex = 0;
    bool isPlanetCleared = false;
    float planetClearPercentage = 0.0f;
    float totalCampaignTime = 0.0f;
    uint64_t totalScrapEarned = 0;

    // Data-driven planets
    std::vector<PlanetConfig> planets;
    int activePlanetIndex = 0;

    // Staging / Transit depot where merchant and roulette ships anchor
    Rectangle stagingDepotBounds{ 0, 0, 0, 0 };
    Vector2 stagingDepotPos{ 0, 0 };

    CampaignManager();
    ~CampaignManager() = default;

    void init(uint64_t seed);
    void update(float dt);

    static std::vector<PlanetConfig> loadPlanetConfigs(const std::string& directoryPath = "assets/campaign/planets");
    static bool parsePlanetJson(const std::string& jsonContent, PlanetConfig& outConfig);
    static std::string exportPlanetConfigToJson(const PlanetConfig& cfg);
    static bool savePlanetConfigToJson(const PlanetConfig& cfg, const std::string& filePath = "");
    static std::vector<PlanetConfig> getDefaultPlanetConfigs();

    bool selectPlanet(int planetIdx);
    const PlanetConfig* getActivePlanetConfig() const;
    PlanetConfig* getActivePlanetConfig();
    const PlanetConfig* getPlanetById(int planetId) const;
    PlanetConfig* getPlanetById(int planetId);

    static std::vector<SectorConfig> loadSectorConfigs(const std::string& directoryPath = "assets/campaign/sectors");
    static bool parseSectorJson(const std::string& jsonContent, SectorConfig& outConfig);
    static std::string exportSectorConfigToJson(const SectorConfig& cfg);
    static bool saveSectorConfigToJson(const SectorConfig& cfg, const std::string& filePath = "");
    static std::vector<SectorConfig> getDefaultSectorConfigs();

    bool rebuildSector(int sectorIdx, const SectorConfig& cfg);

    bool checkSectorClear(int sectorIdx);
    void updatePlanetClearance();

    CampaignSector* getSector(int id);
    const CampaignSector* getSector(int id) const;

    CampaignSector* getSectorByIndex(int idx);
    const CampaignSector* getSectorByIndex(int idx) const;

    int getSectorIndexById(int sectorId) const;
    int getLauncherTargetSectorIndex(int fromSectorIndex) const;

    CampaignSector* getSectorAtWorldPos(Vector2 pos);
    const CampaignSector* getSectorAtWorldPos(Vector2 pos) const;

    CampaignSector* getSectorAtGridWorldPos(Vector2 pos);
    const CampaignSector* getSectorAtGridWorldPos(Vector2 pos) const;

    bool resolveShipCollisions(Ship& ship, render::ParticleSystem* particles = nullptr);

    static size_t toGlobalCellIndex(int sectorIdx, size_t localIdx);
    static void fromGlobalCellIndex(size_t globalIdx, int& outSectorIdx, size_t& outLocalIdx);

    Vector2 getSpawnPosition() const;
    Vector2 getSectorSpawnPosition(int sectorIdx) const;
    bool checkLauncherTransit(int sectorIdx, Vector2 shipPos, float shipRadius) const;
    bool checkGateTransit(int sectorIdx, Vector2 shipPos, float shipRadius) const {
        return checkLauncherTransit(sectorIdx, shipPos, shipRadius);
    }
};

} // namespace minesweeper::core
