#pragma once

#include <raylib.h>
#include <vector>
#include <string>
#include <cstdint>

namespace minesweeper::render {

enum class ScrapState {
    Bouncing,
    Settled,
    Carried,
    FlyingToHUD
};

struct ScrapItem {
    uint32_t id = 0;
    Vector2 basePos = {0, 0};        // Ground/tile center in world space
    Vector2 currentPos = {0, 0};     // World position
    Vector2 size = { 20.0f, 20.0f }; // Token size (strictly <= 65% of tile)
    Vector2 velocity = {0, 0};       // World velocity (px/s)
    float rotation = 0.0f;           // Degrees
    float rotSpeed = 0.0f;           // Degrees/s
    int bounceCount = 0;
    float squashTimer = 0.0f;
    ScrapState state = ScrapState::Bouncing;
    float stateTimer = 0.0f;

    // Carrying state
    bool isCarried = false;
    uint32_t carrierPlayerId = 0;

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
    Texture2D shadowTexture{};
    Texture2D glowTexture{};
    Texture2D sparkTexture{};
    void setSharedTextures(Texture2D shadow, Texture2D glow, Texture2D spark) {
        shadowTexture = shadow;
        glowTexture = glow;
        sparkTexture = spark;
    }
    std::vector<ScrapItem> items;
    std::vector<FloatingText> floatingTexts;
    std::vector<ScrapSpark> sparks;
    int pendingCollected = 0;
    int carriedItemIndex = -1;
    bool hopperDepositTriggered = false;

    ScrapSystem();
    ~ScrapSystem();

    void init();
    void cleanup();
    void clear();

    void spawn(Vector2 cellCenterPos);
    void update(float dt, Vector2 hudScrapScreenPos, const Camera2D& camera, Vector2 mouseWorldPos, bool isMouseDown, bool mouseClicked, Rectangle hopperRect = { -9999.0f, -9999.0f, 0.0f, 0.0f });
    void update(float dt, Vector2 hudScrapScreenPos, const Camera2D& camera, Vector2 mouseWorldPos, bool mouseClicked) {
        update(dt, hudScrapScreenPos, camera, mouseWorldPos, false, mouseClicked);
    }
    void drawWorld(const Camera2D& camera);
    void drawScreen(float guiScale);

    int collectPending();
    int collectAll();

    static bool isScrapCell(uint64_t seed, size_t cellIndex, size_t totalCells, int bombCount);
};

} // namespace minesweeper::render
