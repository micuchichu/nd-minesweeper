#include "campaign.hpp"
#include "json_parser.hpp"
#include "../render/particles.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace minesweeper::core {

namespace {

bool resolveCircleAABB(Ship& ship, const Rectangle& box, render::ParticleSystem* particles) {
    float closestX = std::clamp(ship.position.x, box.x, box.x + box.width);
    float closestY = std::clamp(ship.position.y, box.y, box.y + box.height);

    float dx = ship.position.x - closestX;
    float dy = ship.position.y - closestY;
    float distSq = dx * dx + dy * dy;
    float r = ship.collisionRadius;

    if (distSq < r * r) {
        float dist = std::sqrt(distSq);
        float nx = 0.0f;
        float ny = 0.0f;
        float overlap = 0.0f;

        if (dist > 0.0001f) {
            overlap = r - dist;
            nx = dx / dist;
            ny = dy / dist;
        } else {
            // Circle center inside box: find minimal axis pushout
            float leftPen = ship.position.x - box.x;
            float rightPen = (box.x + box.width) - ship.position.x;
            float topPen = ship.position.y - box.y;
            float botPen = (box.y + box.height) - ship.position.y;

            float minPen = leftPen;
            nx = -1.0f; ny = 0.0f;
            if (rightPen < minPen) { minPen = rightPen; nx = 1.0f; ny = 0.0f; }
            if (topPen < minPen) { minPen = topPen; nx = 0.0f; ny = -1.0f; }
            if (botPen < minPen) { minPen = botPen; nx = 0.0f; ny = 1.0f; }
            overlap = r + minPen;
        }

        ship.position.x += nx * overlap;
        ship.position.y += ny * overlap;

        // Normal velocity reflection & damping
        float vn = ship.velocity.x * nx + ship.velocity.y * ny;
        if (vn < 0.0f) {
            float restitution = 0.40f;
            ship.velocity.x -= (1.0f + restitution) * vn * nx;
            ship.velocity.y -= (1.0f + restitution) * vn * ny;

            if (particles && std::abs(vn) > 60.0f) {
                Vector2 contact = { closestX, closestY };
                particles->emitDebris(contact, 3, Color{ 251, 191, 36, 255 }); // Amber sparks
            }
        }
        return true;
    }
    return false;
}

} // namespace

bool CampaignSector::containsWorldPos(Vector2 pos) const {
    return CheckCollisionPointRec(pos, arenaBounds);
}

bool CampaignSector::containsGridWorldPos(Vector2 pos) const {
    float boardWidth = gridSize * 30.0f;
    Rectangle gridRect = { gridOffset.x, gridOffset.y, boardWidth, boardWidth };
    return CheckCollisionPointRec(pos, gridRect);
}

Vector2 CampaignSector::getCellWorldPosition(size_t cellIndex, float cellSize) const {
    if (gridSize <= 0) return gridOffset;
    size_t x = cellIndex % gridSize;
    size_t y = cellIndex / gridSize;
    return {
        gridOffset.x + (static_cast<float>(x) + 0.5f) * cellSize,
        gridOffset.y + (static_cast<float>(y) + 0.5f) * cellSize
    };
}

int64_t CampaignSector::getCellIndexAtWorldPos(Vector2 worldPos, float cellSize) const {
    float boardWidth = gridSize * cellSize;
    if (worldPos.x < gridOffset.x || worldPos.x >= gridOffset.x + boardWidth ||
        worldPos.y < gridOffset.y || worldPos.y >= gridOffset.y + boardWidth) {
        return -1;
    }
    int x = static_cast<int>((worldPos.x - gridOffset.x) / cellSize);
    int y = static_cast<int>((worldPos.y - gridOffset.y) / cellSize);
    if (x < 0 || x >= gridSize || y < 0 || y >= gridSize) return -1;
    int64_t idx = static_cast<int64_t>(y * gridSize + x);
    if (board.hasTerrainMask && board.isVoid(static_cast<size_t>(idx))) return -1;
    return idx;
}

CampaignManager::CampaignManager() {
    init(12345);
}

std::vector<SectorConfig> CampaignManager::getDefaultSectorConfigs() {
    std::vector<SectorConfig> list;

    SectorConfig s1;
    s1.id = 1;
    s1.name = "Sector 01: Outpost Alpha";
    s1.codename = "OUTPOST-ALPHA";
    s1.subtitle = "Primary Drop Zone & Logistics Depot";
    s1.description = "Initial landing zone. Subterranean sensors detect standard automated mine clusters. Secure this perimeter to power up the orbital launcher to reach the next sector.";
    s1.threatLevel = 1;
    s1.gridSize = 8;
    s1.bombCount = 10;
    s1.isUnlocked = true;
    s1.unlocks = { 2 };
    s1.hasExitLauncher = true;
    s1.launcherTargetSector = 2;
    s1.allowShops = false;
    s1.allowRoulette = false;
    s1.terrainShape = TerrainShape::PerlinIsland;
    s1.terrainShapeName = "PerlinIsland";
    list.push_back(s1);

    SectorConfig s2;
    s2.id = 2;
    s2.name = "Sector 02: Foundry Wastes";
    s2.codename = "FOUNDRY-WASTES";
    s2.subtitle = "Smelting Basin & Scrap Refineries";
    s2.description = "Former mineral refinery. Dense magnetic sub-munitions were seeded during the planetary evacuation. Containment walls prevent hazardous slag from overflowing.";
    s2.threatLevel = 2;
    s2.gridSize = 10;
    s2.bombCount = 18;
    s2.terrainShape = TerrainShape::CellularAutomata;
    s2.terrainShapeName = "CellularAutomata";
    s2.isUnlocked = false;
    s2.unlocks = { 3 };
    s2.hasExitLauncher = true;
    s2.launcherTargetSector = 3;
    s2.allowShops = true;
    s2.allowRoulette = true;
    s2.stagingDepotTitle = "LOGISTICS & REFUELING DOCK // CONTRACTOR HUB";
    s2.merchantSpawns = {
        { "shop3", { 85.0f, 90.0f }, 0.0f },
        { "shop1", { 85.0f, 290.0f }, 0.0f },
        { "roulette", { 85.0f, -70.0f }, 0.0f }
    };
    list.push_back(s2);

    SectorConfig s3;
    s3.id = 3;
    s3.name = "Sector 03: Chemical Basin";
    s3.codename = "CHEMICAL-BASIN";
    s3.subtitle = "Bio-Coolant Reservoirs & Waste Sump";
    s3.description = "Volatile cooling fluid sump. Mine clusters here have corroded into proximity detonators. Orbital launcher gantry clamps remain locked until full chemical stability is secured.";
    s3.threatLevel = 3;
    s3.gridSize = 12;
    s3.bombCount = 28;
    s3.terrainShape = TerrainShape::VoronoiFaultLine;
    s3.terrainShapeName = "VoronoiFaultLine";
    s3.isUnlocked = false;
    s3.unlocks = { 4 };
    s3.hasExitLauncher = true;
    s3.launcherTargetSector = 4;
    s3.allowShops = true;
    s3.allowRoulette = true;
    s3.stagingDepotTitle = "LOGISTICS & REFUELING DOCK // CONTRACTOR HUB";
    s3.merchantSpawns = {
        { "shop2", { 85.0f, 90.0f }, 0.0f },
        { "shop1", { 85.0f, 290.0f }, 0.0f },
        { "roulette", { 85.0f, -70.0f }, 0.0f }
    };
    list.push_back(s3);

    SectorConfig s4;
    s4.id = 4;
    s4.name = "Sector 04: The Core Vault";
    s4.codename = "CORE-VAULT";
    s4.subtitle = "Sub-surface Planetary Reactor Chamber";
    s4.description = "Deepest subterranean facility. Quantum-stabilized high-explosive ordnance encases the primary core generators. Final objective: full planetary clearance.";
    s4.threatLevel = 4;
    s4.gridSize = 14;
    s4.bombCount = 42;
    s4.terrainShape = TerrainShape::PerlinIsland;
    s4.terrainShapeName = "PerlinIsland";
    s4.isUnlocked = false;
    s4.unlocks = {};
    s4.hasExitLauncher = false;
    s4.launcherTargetSector = -1;
    s4.allowShops = true;
    s4.allowRoulette = false;
    s4.stagingDepotTitle = "LOGISTICS & REFUELING DOCK // CONTRACTOR HUB";
    s4.merchantSpawns = {
        { "shop3", { 85.0f, 100.0f }, 0.0f },
        { "shop2", { 85.0f, 320.0f }, 0.0f }
    };
    list.push_back(s4);

    return list;
}

bool CampaignManager::parseSectorJson(const std::string& jsonContent, SectorConfig& outConfig) {
    JsonValue root;
    if (!SimpleJsonParser::parse(jsonContent, root) || !root.isObject()) {
        return false;
    }

    if (root.contains("id")) outConfig.id = root["id"].asInt(outConfig.id);
    if (root.contains("name")) outConfig.name = root["name"].asString(outConfig.name);
    if (root.contains("codename")) outConfig.codename = root["codename"].asString(outConfig.codename);
    if (root.contains("subtitle")) outConfig.subtitle = root["subtitle"].asString(outConfig.subtitle);
    if (root.contains("description")) outConfig.description = root["description"].asString(outConfig.description);
    else if (root.contains("lore")) outConfig.description = root["lore"].asString(outConfig.description);
    else if (root.contains("loreBriefing")) outConfig.description = root["loreBriefing"].asString(outConfig.description);

    if (root.contains("threatLevel")) outConfig.threatLevel = root["threatLevel"].asInt(outConfig.threatLevel);
    else if (root.contains("threat")) outConfig.threatLevel = root["threat"].asInt(outConfig.threatLevel);

    if (root.contains("modifier")) {
        std::string modStr = root["modifier"].asString("None");
        outConfig.modifierName = modStr;
        if (modStr == "FoundryWastes" || modStr == "Foundry-Wastes") outConfig.modifier = SectorModifier::FoundryWastes;
        else if (modStr == "IonStorm" || modStr == "Ion-Storm") outConfig.modifier = SectorModifier::IonStorm;
        else if (modStr == "JammedComms" || modStr == "Jammed-Comms") outConfig.modifier = SectorModifier::JammedComms;
        else outConfig.modifier = SectorModifier::None;
    }

    // Map section: check nested object first, else check top-level
    const JsonValue& mapVal = root.contains("map") ? root["map"] : root;
    if (mapVal.isObject()) {
        if (mapVal.contains("gridSize")) outConfig.gridSize = mapVal["gridSize"].asInt(outConfig.gridSize);
        else if (mapVal.contains("size")) outConfig.gridSize = mapVal["size"].asInt(outConfig.gridSize);

        if (mapVal.contains("bombCount")) outConfig.bombCount = mapVal["bombCount"].asInt(outConfig.bombCount);
        else if (mapVal.contains("bombs")) outConfig.bombCount = mapVal["bombs"].asInt(outConfig.bombCount);

        if (mapVal.contains("dimension")) outConfig.dimension = mapVal["dimension"].asInt(outConfig.dimension);
        else if (mapVal.contains("dim")) outConfig.dimension = mapVal["dim"].asInt(outConfig.dimension);

        if (mapVal.contains("wallThickness")) outConfig.wallThickness = mapVal["wallThickness"].asFloat(outConfig.wallThickness);
        if (mapVal.contains("westMargin")) outConfig.westMargin = mapVal["westMargin"].asFloat(outConfig.westMargin);
        if (mapVal.contains("eastMargin")) outConfig.eastMargin = mapVal["eastMargin"].asFloat(outConfig.eastMargin);
        if (mapVal.contains("vertMargin")) outConfig.vertMargin = mapVal["vertMargin"].asFloat(outConfig.vertMargin);

        if (mapVal.contains("spawnPos") && mapVal["spawnPos"].isArray()) {
            const auto& sp = mapVal["spawnPos"];
            if (sp.arrVal.size() >= 2) {
                outConfig.spawnPos = { sp[0].asFloat(outConfig.spawnPos.x), sp[1].asFloat(outConfig.spawnPos.y) };
            }
        }
        if (mapVal.contains("stagingDepotPos") && mapVal["stagingDepotPos"].isArray()) {
            const auto& dp = mapVal["stagingDepotPos"];
            if (dp.arrVal.size() >= 2) {
                outConfig.stagingDepotPos = { dp[0].asFloat(outConfig.stagingDepotPos.x), dp[1].asFloat(outConfig.stagingDepotPos.y) };
            }
        }
        if (mapVal.contains("stagingDepotTitle")) {
            outConfig.stagingDepotTitle = mapVal["stagingDepotTitle"].asString(outConfig.stagingDepotTitle);
        }

        if (mapVal.contains("customWalls") && mapVal["customWalls"].isArray()) {
            outConfig.customWalls.clear();
            for (const auto& wVal : mapVal["customWalls"].arrVal) {
                if (wVal.isObject()) {
                    SectorWall wall;
                    wall.rect.x = wVal["x"].asFloat(0.0f);
                    wall.rect.y = wVal["y"].asFloat(0.0f);
                    wall.rect.width = wVal["w"].asFloat(wVal["width"].asFloat(32.0f));
                    wall.rect.height = wVal["h"].asFloat(wVal["height"].asFloat(32.0f));
                    wall.isHazard = wVal["isHazard"].asBool(false);
                    wall.isCorridor = wVal["isCorridor"].asBool(false);
                    outConfig.customWalls.push_back(wall);
                }
            }
        }

        bool hasTerrainShape = false;
        if (mapVal.contains("terrainShape")) {
            std::string ts = mapVal["terrainShape"].asString("");
            if (!ts.empty()) {
                outConfig.terrainShapeName = ts;
                if (ts == "PerlinIsland" || ts == "perlin" || ts == "island") {
                    outConfig.terrainShape = TerrainShape::PerlinIsland;
                    hasTerrainShape = true;
                } else if (ts == "CellularAutomata" || ts == "cave" || ts == "caves" || ts == "automata") {
                    outConfig.terrainShape = TerrainShape::CellularAutomata;
                    hasTerrainShape = true;
                } else if (ts == "VoronoiFaultLine" || ts == "fault" || ts == "chasm" || ts == "voronoi") {
                    outConfig.terrainShape = TerrainShape::VoronoiFaultLine;
                    hasTerrainShape = true;
                } else if (ts == "Rectangle" || ts == "square") {
                    outConfig.terrainShape = TerrainShape::Rectangle;
                    hasTerrainShape = true;
                }
            }
        }

        if (!hasTerrainShape) {
            if (outConfig.id % 3 == 1) {
                outConfig.terrainShape = TerrainShape::PerlinIsland;
                outConfig.terrainShapeName = "PerlinIsland";
            } else if (outConfig.id % 3 == 2) {
                outConfig.terrainShape = TerrainShape::CellularAutomata;
                outConfig.terrainShapeName = "CellularAutomata";
            } else {
                outConfig.terrainShape = TerrainShape::VoronoiFaultLine;
                outConfig.terrainShapeName = "VoronoiFaultLine";
            }
        }
    }

    // Progression section: check nested object first, else check top-level
    const JsonValue& progVal = root.contains("progression") ? root["progression"] : root;
    if (progVal.isObject()) {
        if (progVal.contains("isUnlocked")) outConfig.isUnlocked = progVal["isUnlocked"].asBool(outConfig.isUnlocked);
        if (progVal.contains("unlocks")) {
            outConfig.unlocks.clear();
            if (progVal["unlocks"].isArray()) {
                for (const auto& u : progVal["unlocks"].arrVal) {
                    outConfig.unlocks.push_back(u.asInt());
                }
            } else if (progVal["unlocks"].isNumber()) {
                outConfig.unlocks.push_back(progVal["unlocks"].asInt());
            }
        }
        if (progVal.contains("hasExitLauncher")) outConfig.hasExitLauncher = progVal["hasExitLauncher"].asBool(outConfig.hasExitLauncher);
        if (progVal.contains("launcherTargetSector")) outConfig.launcherTargetSector = progVal["launcherTargetSector"].asInt(outConfig.launcherTargetSector);
        else if (progVal.contains("targetSector")) outConfig.launcherTargetSector = progVal["targetSector"].asInt(outConfig.launcherTargetSector);

        if (progVal.contains("launcherOpeningHeight")) outConfig.launcherOpeningHeight = progVal["launcherOpeningHeight"].asFloat(outConfig.launcherOpeningHeight);
        else if (progVal.contains("launcherOpeningH")) outConfig.launcherOpeningHeight = progVal["launcherOpeningH"].asFloat(outConfig.launcherOpeningHeight);
    }

    // Ships section: check nested object first, else check top-level
    const JsonValue& shipsVal = root.contains("ships") ? root["ships"] : root;
    if (shipsVal.isObject()) {
        if (shipsVal.contains("allowShops")) outConfig.allowShops = shipsVal["allowShops"].asBool(outConfig.allowShops);
        if (shipsVal.contains("allowRoulette")) outConfig.allowRoulette = shipsVal["allowRoulette"].asBool(outConfig.allowRoulette);

        if (shipsVal.contains("allowedShipTypes") && shipsVal["allowedShipTypes"].isArray()) {
            outConfig.allowedShipTypes.clear();
            for (const auto& st : shipsVal["allowedShipTypes"].arrVal) {
                outConfig.allowedShipTypes.push_back(st.asString());
            }
        }

        if (shipsVal.contains("shopDockPos") && shipsVal["shopDockPos"].isArray()) {
            const auto& sdp = shipsVal["shopDockPos"];
            if (sdp.arrVal.size() >= 2) {
                outConfig.shopDockPos = { sdp[0].asFloat(outConfig.shopDockPos.x), sdp[1].asFloat(outConfig.shopDockPos.y) };
            }
        }
        if (shipsVal.contains("rouletteDockPos") && shipsVal["rouletteDockPos"].isArray()) {
            const auto& rdp = shipsVal["rouletteDockPos"];
            if (rdp.arrVal.size() >= 2) {
                outConfig.rouletteDockPos = { rdp[0].asFloat(outConfig.rouletteDockPos.x), rdp[1].asFloat(outConfig.rouletteDockPos.y) };
            }
        }

        if (shipsVal.contains("merchantSpawns") && shipsVal["merchantSpawns"].isArray()) {
            outConfig.merchantSpawns.clear();
            for (const auto& msVal : shipsVal["merchantSpawns"].arrVal) {
                if (msVal.isObject()) {
                    MerchantSpawn spawn;
                    spawn.type = msVal["type"].asString("shop1");
                    spawn.pos.x = msVal["x"].asFloat(85.0f);
                    spawn.pos.y = msVal["y"].asFloat(70.0f);
                    spawn.angle = msVal["angle"].asFloat(0.0f);
                    outConfig.merchantSpawns.push_back(spawn);
                }
            }
        }

        // Backward compatibility: synthesize merchantSpawns if legacy JSON lacks the field
        if (outConfig.merchantSpawns.empty()) {
            if (outConfig.allowShops) {
                outConfig.merchantSpawns.push_back({ "shop1", outConfig.shopDockPos, 0.0f });
            }
            if (outConfig.allowRoulette) {
                outConfig.merchantSpawns.push_back({ "roulette", outConfig.rouletteDockPos, 0.0f });
            }
        }
    }

    return true;
}

std::vector<SectorConfig> CampaignManager::loadSectorConfigs(const std::string& directoryPath) {
    namespace fs = std::filesystem;
    std::vector<std::string> candidateDirs = {
        directoryPath,
        "assets/" + directoryPath,
        "../assets/" + directoryPath,
        "../../assets/" + directoryPath,
        "../" + directoryPath,
        "../../" + directoryPath,
        "assets/campaign/sectors/planet1",
        "../assets/campaign/sectors/planet1",
        "../../assets/campaign/sectors/planet1"
    };

    std::vector<SectorConfig> result;

    for (const auto& dirStr : candidateDirs) {
        std::error_code ec;
        fs::path p(dirStr);
        if (fs::exists(p, ec) && fs::is_directory(p, ec)) {
            std::vector<fs::path> files;
            for (const auto& entry : fs::directory_iterator(p, ec)) {
                if (entry.is_regular_file(ec) && entry.path().extension() == ".json") {
                    files.push_back(entry.path());
                }
            }

            // Fallback: If no .json files were found directly in p (e.g. legacy assets/campaign/sectors path),
            // check for planet1 subfolder
            if (files.empty()) {
                fs::path p1 = p / "planet1";
                if (fs::exists(p1, ec) && fs::is_directory(p1, ec)) {
                    for (const auto& entry : fs::directory_iterator(p1, ec)) {
                        if (entry.is_regular_file(ec) && entry.path().extension() == ".json") {
                            files.push_back(entry.path());
                        }
                    }
                }
            }

            std::sort(files.begin(), files.end());

            for (const auto& f : files) {
                std::ifstream ifs(f);
                if (ifs.is_open()) {
                    std::stringstream ss;
                    ss << ifs.rdbuf();
                    SectorConfig cfg;
                    if (parseSectorJson(ss.str(), cfg)) {
                        result.push_back(cfg);
                    }
                }
            }

            if (!result.empty()) {
                std::sort(result.begin(), result.end(), [](const SectorConfig& a, const SectorConfig& b) {
                    return a.id < b.id;
                });
                std::cout << "[CAMPAIGN] Loaded " << result.size() << " data-driven sector(s) from " << p.string() << std::endl;
                return result;
            }
        }
    }

    std::cout << "[CAMPAIGN] No external sector JSONs found. Using default built-in sector definitions." << std::endl;
    return getDefaultSectorConfigs();
}

static std::string escapeJsonString(const std::string& str) {
    std::string out;
    out.reserve(str.size() + 8);
    for (char c : str) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else out += c;
    }
    return out;
}

std::string CampaignManager::exportSectorConfigToJson(const SectorConfig& cfg) {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"id\": " << cfg.id << ",\n";
    ss << "  \"name\": \"" << escapeJsonString(cfg.name) << "\",\n";
    ss << "  \"codename\": \"" << escapeJsonString(cfg.codename) << "\",\n";
    ss << "  \"subtitle\": \"" << escapeJsonString(cfg.subtitle) << "\",\n";
    ss << "  \"description\": \"" << escapeJsonString(cfg.description) << "\",\n";
    ss << "  \"threatLevel\": " << cfg.threatLevel << ",\n";
    const char* modStr = "None";
    if (cfg.modifier == SectorModifier::FoundryWastes) modStr = "Foundry-Wastes";
    else if (cfg.modifier == SectorModifier::IonStorm) modStr = "Ion-Storm";
    else if (cfg.modifier == SectorModifier::JammedComms) modStr = "Jammed-Comms";
    ss << "  \"modifier\": \"" << modStr << "\",\n";
    ss << "  \"map\": {\n";
    ss << "    \"gridSize\": " << cfg.gridSize << ",\n";
    ss << "    \"bombCount\": " << cfg.bombCount << ",\n";
    ss << "    \"dimension\": " << cfg.dimension << ",\n";
    ss << "    \"terrainShape\": \"" << cfg.terrainShapeName << "\",\n";
    ss << "    \"wallThickness\": " << cfg.wallThickness << ",\n";
    ss << "    \"westMargin\": " << cfg.westMargin << ",\n";
    ss << "    \"eastMargin\": " << cfg.eastMargin << ",\n";
    ss << "    \"vertMargin\": " << cfg.vertMargin << ",\n";
    ss << "    \"spawnPos\": [" << cfg.spawnPos.x << ", " << cfg.spawnPos.y << "],\n";
    ss << "    \"stagingDepotPos\": [" << cfg.stagingDepotPos.x << ", " << cfg.stagingDepotPos.y << "],\n";
    ss << "    \"stagingDepotTitle\": \"" << escapeJsonString(cfg.stagingDepotTitle) << "\",\n";
    ss << "    \"customWalls\": [";
    for (size_t i = 0; i < cfg.customWalls.size(); ++i) {
        const auto& w = cfg.customWalls[i];
        if (i > 0) ss << ", ";
        ss << "{\"x\": " << w.rect.x << ", \"y\": " << w.rect.y
           << ", \"w\": " << w.rect.width << ", \"h\": " << w.rect.height
           << ", \"isHazard\": " << (w.isHazard ? "true" : "false")
           << ", \"isCorridor\": " << (w.isCorridor ? "true" : "false") << "}";
    }
    ss << "]\n";
    ss << "  },\n";
    ss << "  \"progression\": {\n";
    ss << "    \"isUnlocked\": " << (cfg.isUnlocked ? "true" : "false") << ",\n";
    ss << "    \"unlocks\": [";
    for (size_t i = 0; i < cfg.unlocks.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << cfg.unlocks[i];
    }
    ss << "],\n";
    ss << "    \"hasExitLauncher\": " << (cfg.hasExitLauncher ? "true" : "false") << ",\n";
    ss << "    \"launcherTargetSector\": " << cfg.launcherTargetSector << ",\n";
    ss << "    \"launcherOpeningHeight\": " << cfg.launcherOpeningHeight << "\n";
    ss << "  },\n";
    ss << "  \"ships\": {\n";
    ss << "    \"allowShops\": " << (cfg.allowShops ? "true" : "false") << ",\n";
    ss << "    \"allowRoulette\": " << (cfg.allowRoulette ? "true" : "false") << ",\n";
    ss << "    \"allowedShipTypes\": [";
    for (size_t i = 0; i < cfg.allowedShipTypes.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << "\"" << escapeJsonString(cfg.allowedShipTypes[i]) << "\"";
    }
    ss << "],\n";
    ss << "    \"shopDockPos\": [" << cfg.shopDockPos.x << ", " << cfg.shopDockPos.y << "],\n";
    ss << "    \"rouletteDockPos\": [" << cfg.rouletteDockPos.x << ", " << cfg.rouletteDockPos.y << "],\n";
    ss << "    \"merchantSpawns\": [";
    for (size_t i = 0; i < cfg.merchantSpawns.size(); ++i) {
        const auto& ms = cfg.merchantSpawns[i];
        if (i > 0) ss << ", ";
        ss << "{\"type\": \"" << escapeJsonString(ms.type) << "\", \"x\": " << ms.pos.x << ", \"y\": " << ms.pos.y << ", \"angle\": " << ms.angle << "}";
    }
    ss << "]\n";
    ss << "  }\n";
    ss << "}\n";
    return ss.str();
}

bool CampaignManager::saveSectorConfigToJson(const SectorConfig& cfg, const std::string& customPath) {
    std::string jsonStr = exportSectorConfigToJson(cfg);
    if (!customPath.empty() && customPath.size() > 5 && customPath.substr(customPath.size() - 5) == ".json") {
        std::ofstream ofs(customPath);
        if (ofs.is_open()) {
            ofs << jsonStr;
            return true;
        }
        return false;
    }

    char fname[64];
    std::snprintf(fname, sizeof(fname), "sector_%02d.json", cfg.id);

    std::string baseDir = customPath.empty() ? "assets/campaign/sectors/planet1" : customPath;
    const std::vector<std::string> candidateDirs = {
        baseDir,
        "assets/" + baseDir,
        "../assets/" + baseDir,
        "../../assets/" + baseDir,
        "../" + baseDir,
        "../../" + baseDir,
        "assets/campaign/sectors/planet1",
        "../assets/campaign/sectors/planet1",
        "../../assets/campaign/sectors/planet1"
    };

    bool savedAny = false;
    for (const auto& dir : candidateDirs) {
        std::error_code ec;
        if (std::filesystem::exists(dir, ec) && std::filesystem::is_directory(dir, ec)) {
            std::string fullPath = dir + "/" + fname;
            std::ofstream ofs(fullPath);
            if (ofs.is_open()) {
                ofs << jsonStr;
                savedAny = true;
                std::cout << "[CAMPAIGN] Saved sector " << cfg.id << " config to " << fullPath << std::endl;
            }
        }
    }
    return savedAny;
}

static Color parseColorJson(const JsonValue& val, Color defaultColor) {
    if (val.isArray()) {
        const auto& arr = val.arrVal;
        if (arr.size() >= 3) {
            Color c = defaultColor;
            c.r = static_cast<unsigned char>(std::clamp(arr[0].asInt(c.r), 0, 255));
            c.g = static_cast<unsigned char>(std::clamp(arr[1].asInt(c.g), 0, 255));
            c.b = static_cast<unsigned char>(std::clamp(arr[2].asInt(c.b), 0, 255));
            if (arr.size() >= 4) {
                c.a = static_cast<unsigned char>(std::clamp(arr[3].asInt(c.a), 0, 255));
            } else {
                c.a = 255;
            }
            return c;
        }
    }
    return defaultColor;
}

static std::string colorToJson(Color c) {
    return "[" + std::to_string(c.r) + ", " + std::to_string(c.g) + ", " + std::to_string(c.b) + "]";
}

std::vector<PlanetConfig> CampaignManager::getDefaultPlanetConfigs() {
    std::vector<PlanetConfig> list;

    PlanetConfig p1;
    p1.id = 1;
    p1.name = "Tartarus-IV";
    p1.codename = "TARTARUS-04";
    p1.systemName = "Tartarus Sub-Cluster 09";
    p1.tagline = "QUARANTINE MINING ARRAY";
    p1.description = "Orbital scans indicate ancient automated defense clusters and subterranean sub-munitions have infested the planetary crust. All civilian and industrial activity is suspended under quarantine. As deep-space clearance contractors, your team is deployed to systematically neutralize each fortified sector.";
    p1.missionDirective = "Clear the planet. Sector by sector.";
    p1.threatLevel = 1;
    p1.seed = 12345;
    p1.isUnlocked = true;
    p1.unlocksPlanets = { 2 };
    p1.sectorDataPath = "assets/campaign/sectors/planet1";
    p1.sectors = { 1, 2, 3, 4 };
    p1.visual.iconColor = Color{ 239, 68, 68, 255 };
    p1.visual.atmosphereColor = Color{ 244, 63, 94, 255 };
    p1.visual.wireframeColor = Color{ 180, 75, 95, 255 };
    p1.visual.surfaceTones = {
        Color{ 28, 14, 20, 255 },
        Color{ 38, 18, 24, 255 },
        Color{ 48, 22, 30, 255 },
        Color{ 24, 12, 16, 255 },
        Color{ 58, 28, 36, 255 }
    };
    p1.visual.hasMagmaRifts = true;
    p1.visual.magmaRiftColor = Color{ 160, 40, 52, 255 };
    p1.visual.magmaRiftChance = 0.09f;
    p1.visual.geologicalStatus = "Volcanic Basalt & Obsidian Crust";
    p1.intelDossier = {
        "EXPEDITION MANDATE: QUARANTINE CLEANSING",
        "Surface Topology: Dual Geodesic Hexagonal Lattice (42 Sectors)",
        "4 Strategic Fortress Hubs with Subterranean Minefields",
        "38 Territorial Hexes Encased in Heavy Basalt Barrier Walls",
        "Full Orbital Support: Casino Supply Ships & Scrap Collection",
        "Clear All 4 Primary Fortresses to Secure the Planet"
    };
    list.push_back(p1);

    PlanetConfig p2;
    p2.id = 2;
    p2.name = "Acheron-Prime";
    p2.codename = "ACHERON-01";
    p2.systemName = "Acheron Foundry Drift";
    p2.tagline = "SUB-CLUSTER FOUNDRY";
    p2.description = "High-temperature smelting world characterized by massive industrial blast basins, slag runoff trenches, and heavy tectonic vibration. Heavily fortified automated perimeter defense clusters remain active throughout the mantle.";
    p2.missionDirective = "Neutralize automated perimeter defenses across foundry facilities.";
    p2.threatLevel = 2;
    p2.seed = 54321;
    p2.isUnlocked = false;
    p2.unlocksPlanets = { 3 };
    p2.sectorDataPath = "assets/campaign/sectors/planet2";
    p2.sectors = { 1, 2, 3, 4 };
    p2.visual.iconColor = Color{ 251, 146, 60, 255 };
    p2.visual.atmosphereColor = Color{ 251, 146, 60, 255 };
    p2.visual.wireframeColor = Color{ 180, 95, 40, 255 };
    p2.visual.surfaceTones = {
        Color{ 42, 24, 14, 255 },
        Color{ 56, 32, 18, 255 },
        Color{ 68, 38, 22, 255 },
        Color{ 35, 20, 12, 255 },
        Color{ 80, 46, 26, 255 }
    };
    p2.visual.hasMagmaRifts = true;
    p2.visual.magmaRiftColor = Color{ 255, 120, 20, 255 };
    p2.visual.magmaRiftChance = 0.12f;
    p2.visual.geologicalStatus = "Sulfuric Slag & Ferrite Outcrops";
    p2.intelDossier = {
        "EXPEDITION MANDATE: HEAVY INDUSTRIAL EXPEDITION",
        "Surface Topology: Smelting Basins & Tectonic Slag Veins",
        "High Threat Seismic Activity and Proximity Detonators",
        "Reinforced Heat-Shielding Required for Core Sectors",
        "Strategic Scrap Refineries Available for Extraction"
    };
    list.push_back(p2);

    PlanetConfig p3;
    p3.id = 3;
    p3.name = "Caelum-VII";
    p3.codename = "CAELUM-07";
    p3.systemName = "Caelum Glacial Expanse";
    p3.tagline = "CRYO-VAULT ARSENAL";
    p3.description = "Sub-zero glacial world containing deep cryogenic military research vaults locked beneath kilometer-thick permafrost sheets and quantum-stabilized minefields.";
    p3.missionDirective = "Breach cryogenic silos and decrypt primary research vaults.";
    p3.threatLevel = 3;
    p3.seed = 98765;
    p3.isUnlocked = false;
    p3.unlocksPlanets = {};
    p3.sectorDataPath = "assets/campaign/sectors/planet3";
    p3.sectors = { 1, 2, 3, 4 };
    p3.visual.iconColor = Color{ 56, 189, 248, 255 };
    p3.visual.atmosphereColor = Color{ 56, 189, 248, 255 };
    p3.visual.wireframeColor = Color{ 70, 130, 180, 255 };
    p3.visual.surfaceTones = {
        Color{ 20, 32, 48, 255 },
        Color{ 28, 44, 64, 255 },
        Color{ 36, 56, 80, 255 },
        Color{ 16, 26, 40, 255 },
        Color{ 46, 70, 98, 255 }
    };
    p3.visual.hasMagmaRifts = true;
    p3.visual.magmaRiftColor = Color{ 140, 230, 255, 255 };
    p3.visual.magmaRiftChance = 0.08f;
    p3.visual.geologicalStatus = "Glacial Permafrost & Cryo-Crystalline Formations";
    p3.intelDossier = {
        "EXPEDITION MANDATE: CRYO SILO EXTRACTION",
        "Surface Topology: Sub-Zero Glacial Ice Lattice",
        "Quantum-Shielded High Density Ordnance Clusters",
        "Permafrost Thermal Dampening Active Across All Sectors",
        "Full Orbital Supply Corridor Ready Upon Deployment"
    };
    list.push_back(p3);

    return list;
}

bool CampaignManager::parsePlanetJson(const std::string& jsonContent, PlanetConfig& outConfig) {
    JsonValue root;
    if (!SimpleJsonParser::parse(jsonContent, root) || !root.isObject()) {
        return false;
    }

    if (root.contains("id")) outConfig.id = root["id"].asInt(outConfig.id);
    if (root.contains("name")) outConfig.name = root["name"].asString(outConfig.name);
    if (root.contains("codename")) outConfig.codename = root["codename"].asString(outConfig.codename);
    if (root.contains("systemName")) outConfig.systemName = root["systemName"].asString(outConfig.systemName);
    if (root.contains("tagline")) outConfig.tagline = root["tagline"].asString(outConfig.tagline);
    if (root.contains("description")) outConfig.description = root["description"].asString(outConfig.description);
    if (root.contains("missionDirective")) outConfig.missionDirective = root["missionDirective"].asString(outConfig.missionDirective);
    if (root.contains("threatLevel")) outConfig.threatLevel = root["threatLevel"].asInt(outConfig.threatLevel);
    if (root.contains("seed")) outConfig.seed = static_cast<uint64_t>(root["seed"].asInt(static_cast<int>(outConfig.seed)));
    if (root.contains("isUnlocked")) outConfig.isUnlocked = root["isUnlocked"].asBool(outConfig.isUnlocked);
    if (root.contains("sectorDataPath")) outConfig.sectorDataPath = root["sectorDataPath"].asString(outConfig.sectorDataPath);

    if (root.contains("unlocksPlanets") && root["unlocksPlanets"].isArray()) {
        outConfig.unlocksPlanets.clear();
        for (const auto& item : root["unlocksPlanets"].arrVal) {
            outConfig.unlocksPlanets.push_back(item.asInt());
        }
    }

    if (root.contains("sectors") && root["sectors"].isArray()) {
        outConfig.sectors.clear();
        for (const auto& item : root["sectors"].arrVal) {
            outConfig.sectors.push_back(item.asInt());
        }
    }

    if (root.contains("intelDossier") && root["intelDossier"].isArray()) {
        outConfig.intelDossier.clear();
        for (const auto& item : root["intelDossier"].arrVal) {
            outConfig.intelDossier.push_back(item.asString());
        }
    }

    if (root.contains("visual") && root["visual"].isObject()) {
        const auto& vis = root["visual"];
        if (vis.contains("iconColor")) outConfig.visual.iconColor = parseColorJson(vis["iconColor"], outConfig.visual.iconColor);
        if (vis.contains("atmosphereColor")) outConfig.visual.atmosphereColor = parseColorJson(vis["atmosphereColor"], outConfig.visual.atmosphereColor);
        if (vis.contains("wireframeColor")) outConfig.visual.wireframeColor = parseColorJson(vis["wireframeColor"], outConfig.visual.wireframeColor);
        if (vis.contains("hasMagmaRifts")) outConfig.visual.hasMagmaRifts = vis["hasMagmaRifts"].asBool(outConfig.visual.hasMagmaRifts);
        if (vis.contains("magmaRiftColor")) outConfig.visual.magmaRiftColor = parseColorJson(vis["magmaRiftColor"], outConfig.visual.magmaRiftColor);
        if (vis.contains("magmaRiftChance")) outConfig.visual.magmaRiftChance = vis["magmaRiftChance"].asFloat(outConfig.visual.magmaRiftChance);
        if (vis.contains("geologicalStatus")) outConfig.visual.geologicalStatus = vis["geologicalStatus"].asString(outConfig.visual.geologicalStatus);

        if (vis.contains("surfaceTones") && vis["surfaceTones"].isArray()) {
            outConfig.visual.surfaceTones.clear();
            for (const auto& toneVal : vis["surfaceTones"].arrVal) {
                outConfig.visual.surfaceTones.push_back(parseColorJson(toneVal, Color{ 30, 20, 25, 255 }));
            }
        }
    }

    return true;
}

std::string CampaignManager::exportPlanetConfigToJson(const PlanetConfig& cfg) {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"id\": " << cfg.id << ",\n";
    ss << "  \"name\": \"" << escapeJsonString(cfg.name) << "\",\n";
    ss << "  \"codename\": \"" << escapeJsonString(cfg.codename) << "\",\n";
    ss << "  \"systemName\": \"" << escapeJsonString(cfg.systemName) << "\",\n";
    ss << "  \"tagline\": \"" << escapeJsonString(cfg.tagline) << "\",\n";
    ss << "  \"description\": \"" << escapeJsonString(cfg.description) << "\",\n";
    ss << "  \"missionDirective\": \"" << escapeJsonString(cfg.missionDirective) << "\",\n";
    ss << "  \"threatLevel\": " << cfg.threatLevel << ",\n";
    ss << "  \"seed\": " << cfg.seed << ",\n";
    ss << "  \"isUnlocked\": " << (cfg.isUnlocked ? "true" : "false") << ",\n";
    ss << "  \"unlocksPlanets\": [";
    for (size_t i = 0; i < cfg.unlocksPlanets.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << cfg.unlocksPlanets[i];
    }
    ss << "],\n";
    ss << "  \"sectorDataPath\": \"" << escapeJsonString(cfg.sectorDataPath) << "\",\n";
    ss << "  \"sectors\": [";
    for (size_t i = 0; i < cfg.sectors.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << cfg.sectors[i];
    }
    ss << "],\n";
    ss << "  \"visual\": {\n";
    ss << "    \"iconColor\": " << colorToJson(cfg.visual.iconColor) << ",\n";
    ss << "    \"atmosphereColor\": " << colorToJson(cfg.visual.atmosphereColor) << ",\n";
    ss << "    \"wireframeColor\": " << colorToJson(cfg.visual.wireframeColor) << ",\n";
    ss << "    \"surfaceTones\": [\n";
    for (size_t i = 0; i < cfg.visual.surfaceTones.size(); ++i) {
        ss << "      " << colorToJson(cfg.visual.surfaceTones[i]);
        if (i + 1 < cfg.visual.surfaceTones.size()) ss << ",";
        ss << "\n";
    }
    ss << "    ],\n";
    ss << "    \"hasMagmaRifts\": " << (cfg.visual.hasMagmaRifts ? "true" : "false") << ",\n";
    ss << "    \"magmaRiftColor\": " << colorToJson(cfg.visual.magmaRiftColor) << ",\n";
    ss << "    \"magmaRiftChance\": " << cfg.visual.magmaRiftChance << ",\n";
    ss << "    \"geologicalStatus\": \"" << escapeJsonString(cfg.visual.geologicalStatus) << "\"\n";
    ss << "  },\n";
    ss << "  \"intelDossier\": [\n";
    for (size_t i = 0; i < cfg.intelDossier.size(); ++i) {
        ss << "    \"" << escapeJsonString(cfg.intelDossier[i]) << "\"";
        if (i + 1 < cfg.intelDossier.size()) ss << ",";
        ss << "\n";
    }
    ss << "  ]\n";
    ss << "}\n";
    return ss.str();
}

bool CampaignManager::savePlanetConfigToJson(const PlanetConfig& cfg, const std::string& customPath) {
    std::string jsonStr = exportPlanetConfigToJson(cfg);
    if (!customPath.empty()) {
        std::ofstream ofs(customPath);
        if (ofs.is_open()) {
            ofs << jsonStr;
            return true;
        }
        return false;
    }

    char fname[64];
    std::snprintf(fname, sizeof(fname), "planet_%02d.json", cfg.id);

    const std::vector<std::string> candidateDirs = {
        "assets/campaign/planets",
        "../assets/campaign/planets",
        "../../assets/campaign/planets"
    };

    bool savedAny = false;
    for (const auto& dir : candidateDirs) {
        std::error_code ec;
        if (std::filesystem::exists(dir, ec)) {
            std::string fullPath = dir + "/" + fname;
            std::ofstream ofs(fullPath);
            if (ofs.is_open()) {
                ofs << jsonStr;
                savedAny = true;
                std::cout << "[CAMPAIGN] Saved planet " << cfg.id << " config to " << fullPath << std::endl;
            }
        }
    }
    return savedAny;
}

std::vector<PlanetConfig> CampaignManager::loadPlanetConfigs(const std::string& directoryPath) {
    namespace fs = std::filesystem;
    const std::vector<std::string> candidateDirs = {
        directoryPath,
        "assets/campaign/planets",
        "../assets/campaign/planets",
        "../../assets/campaign/planets"
    };

    std::vector<PlanetConfig> result;

    for (const auto& dirStr : candidateDirs) {
        std::error_code ec;
        fs::path p(dirStr);
        if (fs::exists(p, ec) && fs::is_directory(p, ec)) {
            std::vector<fs::path> files;
            for (const auto& entry : fs::directory_iterator(p, ec)) {
                if (entry.is_regular_file(ec) && entry.path().extension() == ".json") {
                    files.push_back(entry.path());
                }
            }
            std::sort(files.begin(), files.end());

            for (const auto& f : files) {
                std::ifstream ifs(f);
                if (ifs.is_open()) {
                    std::stringstream ss;
                    ss << ifs.rdbuf();
                    PlanetConfig cfg;
                    if (parsePlanetJson(ss.str(), cfg)) {
                        result.push_back(cfg);
                    }
                }
            }

            if (!result.empty()) {
                std::sort(result.begin(), result.end(), [](const PlanetConfig& a, const PlanetConfig& b) {
                    return a.id < b.id;
                });
                std::cout << "[CAMPAIGN] Loaded " << result.size() << " data-driven planet(s) from " << p.string() << std::endl;
                return result;
            }
        }
    }

    std::cout << "[CAMPAIGN] No external planet JSONs found. Using default built-in planet definitions." << std::endl;
    return getDefaultPlanetConfigs();
}

bool CampaignManager::rebuildSector(int sectorIdx, const SectorConfig& cfg) {
    if (sectorIdx < 0 || sectorIdx >= static_cast<int>(sectors.size())) {
        return false;
    }

    auto& sec = sectors[sectorIdx];
    sec.config = cfg;
    sec.id = cfg.id;
    sec.name = cfg.name;
    sec.codename = cfg.codename;
    sec.subtitle = cfg.subtitle;
    sec.loreBriefing = cfg.description;
    sec.threatLevel = cfg.threatLevel;

    bool boardParamChanged = (sec.gridSize != cfg.gridSize || sec.bombCount != cfg.bombCount || sec.dimension != cfg.dimension || sec.terrainShape != cfg.terrainShape);
    sec.gridSize = cfg.gridSize;
    sec.bombCount = cfg.bombCount;
    sec.dimension = cfg.dimension;
    sec.terrainShape = cfg.terrainShape;
    sec.terrainShapeName = cfg.terrainShapeName;

    if (boardParamChanged) {
        sec.board.init(sec.dimension, sec.gridSize, sec.bombCount, sec.seed, sec.terrainShape);
        sec.isCleared = false;
        sec.clearAnimTimer = 0.0f;
    }

    sec.isUnlocked = cfg.isUnlocked;
    sec.unlocksSectors = cfg.unlocks;
    sec.hasExitLauncher = cfg.hasExitLauncher;
    sec.launcherTargetSectorId = cfg.launcherTargetSector;
    sec.allowShops = cfg.allowShops;
    sec.allowRoulette = cfg.allowRoulette;
    sec.stagingDepotTitle = cfg.stagingDepotTitle;
    sec.allowedShipTypes = cfg.allowedShipTypes;
    sec.customShopDockPos = cfg.shopDockPos;
    sec.customRouletteDockPos = cfg.rouletteDockPos;
    sec.merchantSpawns = cfg.merchantSpawns;
    sec.modifier = cfg.modifier;
    sec.threatIndex = 1.0f;
    sec.centralDataNodeCleared = false;
    sec.centralDataNodeIdx = sec.board.totalCells() / 2;
    sec.moltenTimers.clear();

    float boardPx = sec.gridSize * 30.0f;
    float arenaW = boardPx + cfg.westMargin + cfg.eastMargin;
    float arenaH = boardPx + 2.0f * cfg.vertMargin;

    sec.worldOffset = { 0.0f, 0.0f };
    sec.gridOffset = { cfg.westMargin, cfg.vertMargin };
    sec.arenaBounds = { 0.0f, 0.0f, arenaW, arenaH };

    float spX = (cfg.spawnPos.x > 0.0f) ? cfg.spawnPos.x : 80.0f;
    float spY = (cfg.spawnPos.y > 0.0f) ? cfg.spawnPos.y : (arenaH * 0.5f);
    sec.spawnPos = { spX, spY };

    float dpX = (cfg.stagingDepotPos.x > 0.0f) ? cfg.stagingDepotPos.x : 90.0f;
    float dpY = (cfg.stagingDepotPos.y > 0.0f) ? cfg.stagingDepotPos.y : (arenaH * 0.5f);
    sec.stagingDepotPos = { dpX, dpY };
    sec.stagingDepotBounds = { dpX - 75.0f, dpY - 75.0f, 150.0f, 150.0f };

    float wallThick = (cfg.wallThickness > 0.0f) ? cfg.wallThickness : 28.0f;
    float launcherOpeningH = (cfg.launcherOpeningHeight > 0.0f) ? cfg.launcherOpeningHeight : 140.0f;

    sec.walls.clear();

    // 1. North Wall (Top)
    sec.walls.push_back({ { -wallThick, -wallThick, arenaW + 2.0f * wallThick, wallThick }, false, false });

    // 2. South Wall (Bottom)
    sec.walls.push_back({ { -wallThick, arenaH, arenaW + 2.0f * wallThick, wallThick }, false, false });

    // 3. West Wall (Left)
    sec.walls.push_back({ { -wallThick, 0.0f, wallThick, arenaH }, false, false });

    // 4. East Wall (Right) & Orbital Launcher
    if (sec.hasExitLauncher) {
        float launcherY = arenaH * 0.5f - launcherOpeningH * 0.5f;
        float topLen = launcherY;
        float botLen = arenaH - (launcherY + launcherOpeningH);

        sec.walls.push_back({ { arenaW, 0.0f, wallThick, topLen }, false, false });
        sec.walls.push_back({ { arenaW, launcherY + launcherOpeningH, wallThick, botLen }, false, false });

        sec.exitLauncher.openingBounds = { arenaW - 30.0f, launcherY, wallThick + 50.0f, launcherOpeningH };
        sec.exitLauncher.barrierBounds = { arenaW, launcherY, wallThick, launcherOpeningH };
        sec.exitLauncher.isLocked = !sec.isCleared;
        sec.exitLauncher.beaconPosA = { arenaW + wallThick * 0.5f, launcherY - 12.0f };
        sec.exitLauncher.beaconPosB = { arenaW + wallThick * 0.5f, launcherY + launcherOpeningH + 12.0f };
        sec.exitLauncher.launchVector = { 1.0f, 0.0f };
    } else {
        sec.walls.push_back({ { arenaW, 0.0f, wallThick, arenaH }, false, false });
    }

    for (const auto& cw : cfg.customWalls) {
        sec.walls.push_back(cw);
    }

    if (sectorIdx == activeSectorIndex) {
        stagingDepotPos = sec.stagingDepotPos;
        stagingDepotBounds = sec.stagingDepotBounds;
    }

    updatePlanetClearance();
    return true;
}

void CampaignManager::handleEmergencyExtraction(int sectorIdx, uint64_t& unbankedScrap) {
    unbankedScrap /= 2; // Lose 50% of unbanked scrap carried on board
    if (auto* sec = getSectorByIndex(sectorIdx)) {
        sec->threatIndex += 0.05f; // Threat index increases by +5%
    }
}

bool CampaignManager::selectPlanet(int planetIdx) {
    if (planetIdx < 0 || planetIdx >= static_cast<int>(planets.size())) {
        return false;
    }

    activePlanetIndex = planetIdx;
    const auto& p = planets[planetIdx];
    planetSeed = p.seed;
    planetName = p.name;
    systemName = p.systemName;
    missionDirective = p.missionDirective;
    loreBackground = p.description;

    std::string sPath = p.sectorDataPath.empty() ? "assets/campaign/sectors/planet1" : p.sectorDataPath;
    auto allSectorConfigs = loadSectorConfigs(sPath);
    if (allSectorConfigs.empty()) {
        allSectorConfigs = getDefaultSectorConfigs();
    }

    std::vector<SectorConfig> planetSectors;
    if (!p.sectors.empty()) {
        for (int secId : p.sectors) {
            for (const auto& sc : allSectorConfigs) {
                if (sc.id == secId) {
                    planetSectors.push_back(sc);
                    break;
                }
            }
        }
    }
    if (planetSectors.empty()) {
        planetSectors = allSectorConfigs;
    }

    sectors.clear();
    isPlanetCleared = false;
    planetClearPercentage = 0.0f;
    activeSectorIndex = 0;

    for (size_t i = 0; i < planetSectors.size(); ++i) {
        const auto& cfg = planetSectors[i];
        CampaignSector sec;
        sec.config = cfg;
        sec.id = cfg.id;
        sec.name = cfg.name;
        sec.codename = cfg.codename;
        sec.subtitle = cfg.subtitle;
        sec.loreBriefing = cfg.description;
        sec.threatLevel = cfg.threatLevel;
        sec.gridSize = cfg.gridSize;
        sec.bombCount = cfg.bombCount;
        sec.dimension = cfg.dimension;
        sec.seed = planetSeed + static_cast<uint64_t>(i * 1013);
        sec.isUnlocked = cfg.isUnlocked;
        sec.isCleared = false;
        sec.clearAnimTimer = 0.0f;
        sec.unlocksSectors = cfg.unlocks;
        sec.hasExitLauncher = cfg.hasExitLauncher;
        sec.launcherTargetSectorId = cfg.launcherTargetSector;
        sec.allowShops = cfg.allowShops;
        sec.allowRoulette = cfg.allowRoulette;
        sec.stagingDepotTitle = cfg.stagingDepotTitle;
        sec.allowedShipTypes = cfg.allowedShipTypes;
        sec.customShopDockPos = cfg.shopDockPos;
        sec.customRouletteDockPos = cfg.rouletteDockPos;
        sec.merchantSpawns = cfg.merchantSpawns;

        float boardPx = sec.gridSize * 30.0f;
        float arenaW = boardPx + cfg.westMargin + cfg.eastMargin;
        float arenaH = boardPx + 2.0f * cfg.vertMargin;

        sec.worldOffset = { 0.0f, 0.0f };
        sec.gridOffset = { cfg.westMargin, cfg.vertMargin };
        sec.arenaBounds = { 0.0f, 0.0f, arenaW, arenaH };

        float spX = (cfg.spawnPos.x > 0.0f) ? cfg.spawnPos.x : 80.0f;
        float spY = (cfg.spawnPos.y > 0.0f) ? cfg.spawnPos.y : (arenaH * 0.5f);
        sec.spawnPos = { spX, spY };

        float dpX = (cfg.stagingDepotPos.x > 0.0f) ? cfg.stagingDepotPos.x : 90.0f;
        float dpY = (cfg.stagingDepotPos.y > 0.0f) ? cfg.stagingDepotPos.y : (arenaH * 0.5f);
        sec.stagingDepotPos = { dpX, dpY };
        sec.stagingDepotBounds = { dpX - 75.0f, dpY - 75.0f, 150.0f, 150.0f };

        sec.terrainShape = cfg.terrainShape;
        sec.terrainShapeName = cfg.terrainShapeName;

        // Initialize minesweeper board with procedural terrain mask!
        sec.board.init(sec.dimension, sec.gridSize, sec.bombCount, sec.seed, sec.terrainShape);

        // Build Walls for standalone enclosed sector world
        float wallThick = (cfg.wallThickness > 0.0f) ? cfg.wallThickness : 28.0f;
        float launcherOpeningH = (cfg.launcherOpeningHeight > 0.0f) ? cfg.launcherOpeningHeight : 140.0f;

        // 1. North Wall (Top)
        sec.walls.push_back({ { -wallThick, -wallThick, arenaW + 2.0f * wallThick, wallThick }, false, false });

        // 2. South Wall (Bottom)
        sec.walls.push_back({ { -wallThick, arenaH, arenaW + 2.0f * wallThick, wallThick }, false, false });

        // 3. West Wall (Left) - solid entrance perimeter
        sec.walls.push_back({ { -wallThick, 0.0f, wallThick, arenaH }, false, false });

        // 4. East Wall (Right) & Orbital Launcher Facility
        if (sec.hasExitLauncher) {
            float launcherY = arenaH * 0.5f - launcherOpeningH * 0.5f;
            float topLen = launcherY;
            float botLen = arenaH - (launcherY + launcherOpeningH);

            sec.walls.push_back({ { arenaW, 0.0f, wallThick, topLen }, false, false });
            sec.walls.push_back({ { arenaW, launcherY + launcherOpeningH, wallThick, botLen }, false, false });

            // Orbital Launcher structures (mass driver accelerator cradle)
            sec.exitLauncher.openingBounds = { arenaW - 30.0f, launcherY, wallThick + 50.0f, launcherOpeningH };
            sec.exitLauncher.barrierBounds = { arenaW, launcherY, wallThick, launcherOpeningH };
            sec.exitLauncher.isLocked = true;
            sec.exitLauncher.openAnim = 0.0f;
            sec.exitLauncher.beaconPosA = { arenaW + wallThick * 0.5f, launcherY - 12.0f };
            sec.exitLauncher.beaconPosB = { arenaW + wallThick * 0.5f, launcherY + launcherOpeningH + 12.0f };
            sec.exitLauncher.launchVector = { 1.0f, 0.0f };
        } else {
            // Final Sector: solid east wall enclosing the core chamber
            sec.walls.push_back({ { arenaW, 0.0f, wallThick, arenaH }, false, false });
        }

        // Custom extra walls from config
        for (const auto& cw : cfg.customWalls) {
            sec.walls.push_back(cw);
        }

        sec.config = cfg;
        sectors.push_back(sec);
    }

    if (!sectors.empty()) {
        stagingDepotPos = sectors[0].stagingDepotPos;
        stagingDepotBounds = sectors[0].stagingDepotBounds;
    }

    updatePlanetClearance();
    return true;
}

const PlanetConfig* CampaignManager::getActivePlanetConfig() const {
    if (activePlanetIndex >= 0 && activePlanetIndex < static_cast<int>(planets.size())) {
        return &planets[activePlanetIndex];
    }
    return nullptr;
}

PlanetConfig* CampaignManager::getActivePlanetConfig() {
    if (activePlanetIndex >= 0 && activePlanetIndex < static_cast<int>(planets.size())) {
        return &planets[activePlanetIndex];
    }
    return nullptr;
}

const PlanetConfig* CampaignManager::getPlanetById(int planetId) const {
    for (const auto& p : planets) {
        if (p.id == planetId) return &p;
    }
    return nullptr;
}

PlanetConfig* CampaignManager::getPlanetById(int planetId) {
    for (auto& p : planets) {
        if (p.id == planetId) return &p;
    }
    return nullptr;
}

void CampaignManager::init(uint64_t seed) {
    planets = loadPlanetConfigs();
    if (planets.empty()) {
        planets = getDefaultPlanetConfigs();
    }
    activePlanetIndex = 0;
    if (seed != 0 && !planets.empty()) {
        planets[0].seed = seed;
    }
    selectPlanet(activePlanetIndex);
}

void CampaignManager::update(float dt) {
    totalCampaignTime += dt;

    for (size_t i = 0; i < sectors.size(); ++i) {
        auto& sec = sectors[i];

        // Check if sector safe cells are fully revealed
        if (!sec.isCleared && sec.board.revealedCount >= sec.board.safeCells() && !sec.board.isGameOver) {
            sec.isCleared = true;
            sec.clearAnimTimer = 5.0f; // Celebration timer

            // Arm orbital launcher to next sector
            if (sec.hasExitLauncher) {
                sec.exitLauncher.isLocked = false;
            }
            if (!sec.unlocksSectors.empty()) {
                for (int uId : sec.unlocksSectors) {
                    CampaignSector* nextSec = getSector(uId);
                    if (nextSec) nextSec->isUnlocked = true;
                }
            } else if (i + 1 < sectors.size()) {
                sectors[i + 1].isUnlocked = true;
            }
            std::cout << "[CAMPAIGN] " << sec.name << " SECURED! ORBITAL LAUNCHER ARMED." << std::endl;
        }

        // Animate orbital launcher gantry opening / arming
        if (sec.hasExitLauncher) {
            float targetOpen = sec.exitLauncher.isLocked ? 0.0f : 1.0f;
            float animSpeed = 2.5f;
            if (sec.exitLauncher.openAnim < targetOpen) {
                sec.exitLauncher.openAnim = std::min(targetOpen, sec.exitLauncher.openAnim + dt * animSpeed);
            } else if (sec.exitLauncher.openAnim > targetOpen) {
                sec.exitLauncher.openAnim = std::max(targetOpen, sec.exitLauncher.openAnim - dt * animSpeed);
            }
        }

        if (sec.clearAnimTimer > 0.0f) {
            sec.clearAnimTimer = std::max(0.0f, sec.clearAnimTimer - dt);
        }
    }

    if (activeSectorIndex >= 0 && activeSectorIndex < static_cast<int>(sectors.size())) {
        stagingDepotPos = sectors[activeSectorIndex].stagingDepotPos;
        stagingDepotBounds = sectors[activeSectorIndex].stagingDepotBounds;
    }

    updatePlanetClearance();
}

bool CampaignManager::checkSectorClear(int sectorIdx) {
    if (sectorIdx < 0 || sectorIdx >= static_cast<int>(sectors.size())) return false;
    auto& sec = sectors[sectorIdx];
    if (sec.isCleared) return true;
    if (sec.board.revealedCount >= sec.board.safeCells() && !sec.board.isGameOver) {
        sec.isCleared = true;
        if (sec.hasExitLauncher) {
            sec.exitLauncher.isLocked = false;
        }
        if (!sec.unlocksSectors.empty()) {
            for (int uId : sec.unlocksSectors) {
                CampaignSector* nextSec = getSector(uId);
                if (nextSec) nextSec->isUnlocked = true;
            }
        } else if (sectorIdx + 1 < static_cast<int>(sectors.size())) {
            sectors[sectorIdx + 1].isUnlocked = true;
        }
        updatePlanetClearance();
        return true;
    }
    return false;
}

void CampaignManager::updatePlanetClearance() {
    if (sectors.empty()) {
        planetClearPercentage = 0.0f;
        isPlanetCleared = false;
        return;
    }

    size_t totalSafe = 0;
    size_t totalRev = 0;
    int clearedSectors = 0;

    for (const auto& sec : sectors) {
        totalSafe += sec.board.safeCells();
        totalRev += std::min(sec.board.revealedCount, sec.board.safeCells());
        if (sec.isCleared) ++clearedSectors;
    }

    planetClearPercentage = (totalSafe > 0) ? (static_cast<float>(totalRev) / static_cast<float>(totalSafe)) : 0.0f;
    isPlanetCleared = (clearedSectors == static_cast<int>(sectors.size()));
    if (isPlanetCleared && activePlanetIndex >= 0 && activePlanetIndex < static_cast<int>(planets.size())) {
        for (int pId : planets[activePlanetIndex].unlocksPlanets) {
            auto* pNext = getPlanetById(pId);
            if (pNext) pNext->isUnlocked = true;
        }
    }
}

CampaignSector* CampaignManager::getSector(int id) {
    for (auto& s : sectors) {
        if (s.id == id) return &s;
    }
    return nullptr;
}

const CampaignSector* CampaignManager::getSector(int id) const {
    for (const auto& s : sectors) {
        if (s.id == id) return &s;
    }
    return nullptr;
}

CampaignSector* CampaignManager::getSectorByIndex(int idx) {
    if (idx >= 0 && idx < static_cast<int>(sectors.size())) {
        return &sectors[idx];
    }
    return nullptr;
}

const CampaignSector* CampaignManager::getSectorByIndex(int idx) const {
    if (idx >= 0 && idx < static_cast<int>(sectors.size())) {
        return &sectors[idx];
    }
    return nullptr;
}

int CampaignManager::getSectorIndexById(int sectorId) const {
    for (size_t i = 0; i < sectors.size(); ++i) {
        if (sectors[i].id == sectorId) return static_cast<int>(i);
    }
    return -1;
}

int CampaignManager::getLauncherTargetSectorIndex(int fromSectorIndex) const {
    if (fromSectorIndex < 0 || fromSectorIndex >= static_cast<int>(sectors.size())) return -1;
    const auto& sec = sectors[fromSectorIndex];
    if (sec.launcherTargetSectorId > 0) {
        int targetIdx = getSectorIndexById(sec.launcherTargetSectorId);
        if (targetIdx >= 0) return targetIdx;
    }
    if (fromSectorIndex + 1 < static_cast<int>(sectors.size())) {
        return fromSectorIndex + 1;
    }
    return -1;
}

CampaignSector* CampaignManager::getSectorAtWorldPos(Vector2 pos) {
    auto* active = getSectorByIndex(activeSectorIndex);
    if (active && active->containsWorldPos(pos)) return active;
    for (auto& s : sectors) {
        if (s.containsWorldPos(pos)) return &s;
    }
    return nullptr;
}

const CampaignSector* CampaignManager::getSectorAtWorldPos(Vector2 pos) const {
    const auto* active = getSectorByIndex(activeSectorIndex);
    if (active && active->containsWorldPos(pos)) return active;
    for (const auto& s : sectors) {
        if (s.containsWorldPos(pos)) return &s;
    }
    return nullptr;
}

CampaignSector* CampaignManager::getSectorAtGridWorldPos(Vector2 pos) {
    auto* active = getSectorByIndex(activeSectorIndex);
    if (active && active->containsGridWorldPos(pos)) return active;
    for (auto& s : sectors) {
        if (s.containsGridWorldPos(pos)) return &s;
    }
    return nullptr;
}

const CampaignSector* CampaignManager::getSectorAtGridWorldPos(Vector2 pos) const {
    const auto* active = getSectorByIndex(activeSectorIndex);
    if (active && active->containsGridWorldPos(pos)) return active;
    for (const auto& s : sectors) {
        if (s.containsGridWorldPos(pos)) return &s;
    }
    return nullptr;
}

bool CampaignManager::resolveShipCollisions(Ship& ship, render::ParticleSystem* particles) {
    bool collided = false;
    const auto* sec = getSectorByIndex(activeSectorIndex);
    if (!sec && !sectors.empty()) sec = &sectors[0];
    if (sec) {
        // Test solid walls of the active sector world
        for (const auto& wall : sec->walls) {
            if (resolveCircleAABB(ship, wall.rect, particles)) {
                collided = true;
            }
        }

        // Test locked orbital launcher gantry clamps / barrier
        if (sec->hasExitLauncher && sec->exitLauncher.isLocked) {
            if (resolveCircleAABB(ship, sec->exitLauncher.barrierBounds, particles)) {
                collided = true;
            }
        }
    }
    return collided;
}

size_t CampaignManager::toGlobalCellIndex(int sectorIdx, size_t localIdx) {
    return static_cast<size_t>(sectorIdx) * SECTOR_CELL_STRIDE + localIdx;
}

void CampaignManager::fromGlobalCellIndex(size_t globalIdx, int& outSectorIdx, size_t& outLocalIdx) {
    outSectorIdx = static_cast<int>(globalIdx / SECTOR_CELL_STRIDE);
    outLocalIdx = globalIdx % SECTOR_CELL_STRIDE;
}

Vector2 CampaignManager::getSectorSpawnPosition(int sectorIdx) const {
    if (sectorIdx >= 0 && sectorIdx < static_cast<int>(sectors.size())) {
        return sectors[sectorIdx].spawnPos;
    }
    return { 80.0f, 280.0f };
}

Vector2 CampaignManager::getSpawnPosition() const {
    return getSectorSpawnPosition(activeSectorIndex);
}

bool CampaignManager::checkLauncherTransit(int sectorIdx, Vector2 shipPos, float shipRadius) const {
    if (sectorIdx < 0 || sectorIdx >= static_cast<int>(sectors.size())) return false;
    const auto& sec = sectors[sectorIdx];
    if (!sec.hasExitLauncher || sec.exitLauncher.isLocked) return false;

    // Check if ship circle overlaps orbital launcher acceleration cradle
    Rectangle tr = sec.exitLauncher.openingBounds;
    float closestX = std::clamp(shipPos.x, tr.x, tr.x + tr.width);
    float closestY = std::clamp(shipPos.y, tr.y, tr.y + tr.height);
    float dx = shipPos.x - closestX;
    float dy = shipPos.y - closestY;
    return (dx * dx + dy * dy) <= (shipRadius * shipRadius);
}

} // namespace minesweeper::core
