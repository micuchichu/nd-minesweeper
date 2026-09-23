#pragma once

#include "raylib.h"
#include "raymath.h"
#include "../core/campaign.hpp"
#include <vector>
#include <string>

namespace minesweeper::render {

struct HexSector {
    int id = 0;                     // Global sector ID (0 to 41)
    Vector3 center{ 0, 0, 1 };      // Unit sphere center position
    std::vector<Vector3> corners;   // 5 or 6 corner vertices on unit sphere (ordered CCW)
    Color baseColor{ 32, 16, 22, 255 }; // Volcanic rock tone
    bool isMagmaRift = false;

    // Campaign integration
    bool isFortress = false;        // True if this is one of the 4 Campaign sectors
    int fortressIdx = -1;           // 0 to 3 for Campaign sectors, -1 otherwise
    std::string codename;           // e.g. "ALPHA-01", "SECTOR-18"
    std::string name;               // e.g. "Sector 01: Outpost Alpha"
    std::string subtitle;
    int threatLevel = 1;            // 1 = Low, 2 = Medium, 3 = High, 4 = Extreme
};

struct Star3D {
    Vector3 worldPos{ 0.0f, 0.0f, 0.0f }; // 3D position on celestial sphere
    Vector2 screenPos{ 0.0f, 0.0f };
    float size = 1.0f;
    float brightness = 1.0f;
    float twinkleSpeed = 1.0f;
    float twinkleOffset = 0.0f;
};

class PlanetRenderer {
public:
    static constexpr float PLANET_RADIUS = 1.32f;

    Camera3D camera{};
    float yaw = 0.35f;              // Planet rotation around Y axis (radians)
    float pitch = 0.10f;            // Planet rotation around X axis (radians)
    float targetYaw = 0.35f;
    float targetPitch = 0.10f;
    float cameraDistance = 3.90f;
    float targetCameraDistance = 3.90f;

    bool isDragging = false;
    bool hasDragged = false;
    int potentialClickSector = -1;
    Vector2 dragStartPos{ 0.0f, 0.0f };
    float dragStartYaw = 0.0f;
    float dragStartPitch = 0.0f;
    float idleTimer = 0.0f;
    bool isTransitioning = false;

    // Full-coverage geodesic hexagonal grid (42 sectors)
    std::vector<HexSector> sectors;
    std::vector<Star3D> stars;
    bool initialized = false;

    PlanetRenderer();
    ~PlanetRenderer() = default;

    void init();
    void update(float dt);

    // Coordinate rotation: rotates a unit vector by yaw then pitch
    Vector3 rotateVector(Vector3 v) const;

    // Interaction handling: mouse drag orbit, zoom, hover and click detection
    int handleInput(Rectangle viewport, int& outHoveredSector);

    // Smoothly rotate the planet so sectorIdx faces the camera
    void focusSector(int sectorIdx);

    // 3D low-poly faceted planet globe rendering pass
    void drawGlobe(const core::CampaignManager& campaign, int selectedSectorIdx, int hoveredSectorIdx);

    // 2D screen overlay pass (glowing outlines, icons, tactical badges)
    void drawSectorOverlays(const core::CampaignManager& campaign, int selectedSectorIdx, int hoveredSectorIdx);

    // Helper to get sector by fortress index (0 to 3)
    int getSectorIdxForFortress(int fortressIdx) const;

    // Sync fortress names and metadata from data-driven campaign sectors
    void syncCampaignSectors(const core::CampaignManager& campaign);

    core::PlanetConfig currentPlanetConfig;
    void applyPlanetConfig(const core::PlanetConfig& cfg);

private:
    void generateGeodesicHexGrid(const core::PlanetConfig* cfg = nullptr);
    void generateStars(int screenW, int screenH);
};

} // namespace minesweeper::render
