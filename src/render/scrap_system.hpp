#pragma once

#include <raylib.h>
#include <vector>
#include <string>
#include <cstdint>

namespace minesweeper::render {

enum class ScrapState {
    Bouncing,
    Settled,
    FlyingToHUD
};

struct ScrapItem {
    Vector2 basePos = {0, 0};        // Ground/tile center in world space
    Vector2 currentPos = {0, 0};     // World position
    Vector2 velocity = {0, 0};       // World velocity (px/s)
    float rotation = 0.0f;           // Degrees
    float rotSpeed = 0.0f;           // Degrees/s
    int bounceCount = 0;
    float squashTimer = 0.0f;
    ScrapState state = ScrapState::Bouncing;
    float stateTimer = 0.0f;

    // Flight towards HUD
    Vector2 flyStartScreen = {0, 0};
    Vector2 targetScreen = {0, 0};
    float flyProgress = 0.0f;
    bool reachedHUD = false;
};

struct FloatingText {
    Vector2 worldPos = {0, 0};
    std::string text;
    Color color = WHITE;
    float life = 0.0f;
    float maxLife = 1.0f;
};

struct ScrapSpark {
    Vector2 pos = {0, 0};
    Vector2 vel = {0, 0};
    Color color = WHITE;
    float life = 0.0f;
    float maxLife = 0.5f;
    float size = 3.0f;
};

class ScrapSystem {
public:
    Texture2D texture{};
    std::vector<ScrapItem> items;
    std::vector<FloatingText> floatingTexts;
    std::vector<ScrapSpark> sparks;
    int pendingCollected = 0;

    ScrapSystem();
    ~ScrapSystem();

    void init();
    void cleanup();
    void clear();

    void spawn(Vector2 cellCenterPos);
    void update(float dt, Vector2 hudScrapScreenPos, const Camera2D& camera, Vector2 mouseWorldPos, bool mouseClicked);
    void drawWorld(const Camera2D& camera);
    void drawScreen(float guiScale);

    int collectPending();
    int collectAll();

    static bool isScrapCell(uint64_t seed, size_t cellIndex, size_t totalCells, int bombCount);
};

} // namespace minesweeper::render
