#pragma once

#include "renderer.hpp"
#include "camera_controller.hpp"
#include "particles.hpp"
#include "../ui/theme.hpp"
#include "../core/ship.hpp"
#include "../core/roulette_ship.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace minesweeper::core {
struct InventorySlot;
class CampaignManager;
}

namespace minesweeper::render {


struct CursorSkinItem {
    std::string name;
    Texture2D texture = {0};
};

struct FlagSkinItem {
    std::string name;
    Texture2D texture = {0};
};

struct FlagDropAnim {
    Vector2 shipPos = { 0.0f, 0.0f };
    Vector2 groundPos = { 0.0f, 0.0f };
    uint8_t flagSkin = 0;
    float timer = 0.0f;
    float duration = 0.11f;
    bool landed = false;
};

struct FlagPickupAnim {
    Vector2 startPos = { 0.0f, 0.0f };
    Vector2 targetPos = { 0.0f, 0.0f };
    uint32_t pickerId = 0;
    bool isLocal = false;
    uint8_t flagSkin = 0;
    float timer = 0.0f;
    float duration = 0.10f;
};

struct PlayerSkinItem {
    std::string name;
    Texture2D texture = {0};
};

struct LaserBeam {
    Vector2 from;
    Vector2 to;
    float life = 0.0f;
    float maxLife = 0.18f;
    Color color = { 0, 229, 255, 255 };
};

class RaylibRenderer : public IRenderer {
public:
    CameraController camera;
    ParticleSystem particles;

    float cellSize = 30.0f;
    float cellMargin = 2.0f;
    float slicePadding = 45.0f;
    bool enableCRT = true;

    static std::vector<CursorSkinItem> cursorSkins;
    static int getCursorSkinCount();
    static const char* getCursorSkinName(int skin);
    static Texture2D getCursorSkinTexture(int skin);
    static void drawCursorSkin(uint8_t skin, Vector2 pos, float angle = 0.0f, Color col = WHITE, const char* name = nullptr, float scale = 1.0f, bool isSpeaking = false, bool isMoving = false);

    int activeCursorSkin = 0;
    core::ScoutShip localShip;
    std::map<uint32_t, core::ScoutShip> remoteShips;

    std::vector<core::ShopShip> shopShips;
    core::ShopShip shopShip;
    core::RouletteShip rouletteShip;
    bool hasRouletteShip = false;
    Vector2 shopAnchorPos = { -95.0f, 150.0f };
    void updateShopAnchor(const core::Board& board);

    int controlMode = 0; // 0 = Mouse Follower, 1 = Keyboard / Controller
    Vector2 moveInput = { 0.0f, 0.0f };
    bool hasAim = false;
    float aimAngle = 0.0f;
    bool isMouseActive = true;

    std::vector<LaserBeam> lasers;
    void fireLaser(Vector2 from, Vector2 to, Color color = { 0, 229, 255, 255 });
    static Color getLaserColorForSkin(int skinId);

    int64_t outOfReachCell = -1;
    Vector2 outOfReachPos = { 0.0f, 0.0f };
    float outOfReachTimer = 0.0f;
    void triggerOutOfReach(int64_t cellIndex, Vector2 cellPos);
    void clearOutOfReach();

    void syncRemoteShips(const std::map<uint32_t, net::RemoteCursor>& cursors);
    void resolveShipCollisions();
    void updateShip(Vector2 targetPos, float dt);

    // Fixed timestep physics (120 Hz) & controlled bumper simulation
    static constexpr float FIXED_PHYSICS_DT = 1.0f / 120.0f;
    float physicsAccumulator = 0.0f;
    void updatePhysics(Vector2 targetPos, float dt);
    void stepPhysics(Vector2 targetPos, float fixedDt);

    bool isLocalSpeaking = false;
    float guiScale = 1.0f;
    int activeCampaignSector = -1;

    static std::vector<FlagSkinItem> flagSkins;
    static int getFlagSkinCount();
    static const char* getFlagSkinName(int skin);
    static Texture2D getFlagTexture(int skin);
    int activeFlagSkin = 0;

    std::unordered_map<size_t, FlagDropAnim> flagDropAnims;
    std::vector<FlagPickupAnim> flagPickupAnims;
    Vector2 getFlagBasePosition(size_t index, const core::Board& board) const;
    void triggerFlagDrop(size_t cellIndex, Vector2 groundPos, Vector2 shipPos, uint8_t skinId);
    void triggerFlagPickup(Vector2 groundPos, Vector2 shipPos, uint32_t pickerId, bool isLocal, uint8_t skinId);
    void removeFlagDrop(size_t cellIndex);
    void clearFlagDrops();
    void drawFlyingFlags(const core::Board& board);

    static std::vector<PlayerSkinItem> playerSkins;
    static int getPlayerSkinCount();
    static const char* getPlayerSkinName(int skin);
    static Texture2D getPlayerTexture(int skin);
    int activePlayerSkin = 0;

    RaylibRenderer();
    ~RaylibRenderer() override;

    void init() override;
    void update(float dt) override;
    void render(const core::Board& board, int64_t hoveredIndex, const net::NetworkManager& net) override;
    void renderCampaign(const core::CampaignManager& campaign, int64_t hoveredGlobalCell, const net::NetworkManager& net);
    void updateShopAnchorCampaign(const core::CampaignManager& campaign);
    void cleanup() override;

    int64_t getHoveredCellIndex(const core::Board& board) const override;
    int64_t getCellIndexAtWorldPos(Vector2 worldPos, const core::Board& board) const;
    Vector2 getCellWorldPosition(size_t index, const core::Board& board) const override;

    void beginOffscreen();
    void endOffscreen();
    void drawOffscreenToScreen();
    void saveScreenshot(const char* filename);

    struct BubbleParticle {
        Vector2 position;
        Vector2 velocity;
        float life;
        float maxLife;
        float size;
        float wobblePhase;
        float wobbleSpeed;
        Color tint;
    };
    std::vector<BubbleParticle> bubbleParticles;
    void emitBubbleBurst(Vector2 pos, int count = 45);
    void emitBubbles(Vector2 pos, int count = 2);
    void updateAndDrawBubbles(float dt);

    float bubbleBlurTimer = 0.0f;
    float currentBubbleBlur = 0.0f;
    float targetBubbleBlur = 0.0f;
    static constexpr float BUBBLE_BLUR_DURATION = 3.0f;
    void triggerBubbleBlur(float duration = BUBBLE_BLUR_DURATION) { bubbleBlurTimer = duration; }
    void applyBubbleBlurSource(Vector2 bubbleSourcePos, float maxRadius = 650.0f);

    bool hasRadarActive = false;
    float radarTimer = 0.0f;
    static constexpr float RADAR_DURATION = 4.0f;
    void triggerRadar(float duration = RADAR_DURATION) {
        hasRadarActive = true;
        radarTimer = duration;
    }
    void drawRadarSweep(Vector2 shipPos, const core::Board& board, float dt);

    enum class HeldItemState {
        Hidden,
        Deploying,
        Deployed,
        Retracting
    };

    const core::InventorySlot* heldSlot = nullptr;
    bool isUsingItem = false;
    HeldItemState heldState = HeldItemState::Hidden;
    float heldAnimProgress = 0.0f; // 0.0f (inside ship) to 1.0f (fully deployed)
    core::InventorySlot activeHeldSlot;
    Vector2 heldItemPos = { 0.0f, 0.0f };
    Vector2 heldItemVel = { 0.0f, 0.0f };
    bool heldItemInit = false;
    void updateHeldItemPhysics(Vector2 shipPos, Vector2 shipVel, float dt);
    void drawHeldItem(Vector2 shipPos, float shipAngle, const core::InventorySlot* slot, bool isUsing);

    void emitExplosion(Vector2 pos, Color col) { particles.emitExplosion(pos, 50, col); }
    void emitDebris(Vector2 pos, Color col) { particles.emitDebris(pos, 8, col); }
    void emitSparks(Vector2 pos, int count, Color col, float sMin = 150.0f, float sMax = 650.0f) {
        particles.emitSparks(pos, count, col, sMin, sMax);
    }
    void emitPlasmaMotes(Vector2 pos, int count, Color col, float sMin = 25.0f, float sMax = 140.0f) {
        particles.emitPlasmaMotes(pos, count, col, sMin, sMax);
    }
    void emitSparkles(Vector2 pos, int count, Color col, float spread = 24.0f) {
        particles.emitSparkles(pos, count, col, spread);
    }
    void emitSmoke(Vector2 pos, int count, Color col, float sMin = 10.0f, float sMax = 50.0f) {
        particles.emitSmoke(pos, count, col, sMin, sMax);
    }
    void emitShockwave(Vector2 pos, float maxRadius, Color col, float dur = 0.35f) {
        particles.emitShockwave(pos, maxRadius, col, dur);
    }
    void emitWarpSpawn(Vector2 pos, Color themeCol) {
        particles.emitWarpSpawnFX(pos, themeCol);
    }
    void emitSectorClearFX(Vector2 pos) {
        particles.emitDefusalFX(pos, cellSize);
    }

    std::vector<core::RadarBeaconEntity> radarBeacons;
    void deployRadarBeacon(Vector2 worldPos, int mineCount, float duration = 12.0f);
    void updateRadarBeacons(float dt);
    void drawRadarBeacons();

    bool ionStormActive = false;
    float ionGlitchTimer = 0.0f;
    void updateIonStorm(float dt);
    void drawIonStormOverlay();

    struct BombClearStep {
        size_t cellIndex = 0;
        Vector2 worldPos = { 0.0f, 0.0f };
        float triggerTime = 0.0f;
        bool triggered = false;
        float highlightProgress = 0.0f; // 1.0 down to 0.0
    };

    struct SectorClearAnim {
        bool active = false;
        int sectorIdx = -1;
        float timer = 0.0f;
        float totalDuration = 0.0f;
        std::vector<BombClearStep> steps;
    };

    SectorClearAnim sectorClearAnim;
    void triggerSectorClearAnimation(int sectorIdx, const core::Board& board, Vector2 gridOrigin, float cellSize = 30.0f);
    void updateSectorClearAnimation(float dt);
    const BombClearStep* getActiveClearStep(int sectorIdx, size_t cellIdx) const;

    void clearParticles() {
        particles.clear();
        bubbleParticles.clear();
        radarBeacons.clear();
        localShip.exhaust.clear();
        shopShip.exhaust.clear();
        for (auto& s : shopShips) s.exhaust.clear();
        if (hasRouletteShip) rouletteShip.exhaust.clear();
        lasers.clear();
        clearOutOfReach();
        clearFlagDrops();
        sectorClearAnim.active = false;
        sectorClearAnim.steps.clear();
        heldItemInit = false;
        heldState = HeldItemState::Hidden;
        heldAnimProgress = 0.0f;
    }

private:
    Texture2D flagTexture = {0};
    RenderTexture2D cellHiddenRT = {0};
    RenderTexture2D cellRevealRT = {0};

    Shader gridShader = {0};
    int gridShaderGridSizeLoc = -1;
    int gridShaderCellSizeLoc = -1;

    Shader postProcessShader = {0};
    int ppTimeLoc = -1;
    int ppResLoc = -1;
    int ppBubbleBlurLoc = -1;
    int ppCrtEnabledLoc = -1;
    float radarSweepAngle = 0.0f;
    RenderTexture2D offscreenTarget = {0};

    void initShaders();
    void initCellTextures();
    void unloadAssets();
    void drawSlice(const core::Board& board, size_t sliceZ, size_t sliceW, float sliceOriginX, float sliceOriginY, int64_t hoveredIndex, size_t globalOffset = 0, bool isSectorCleared = false, int sectorIdx = -1);
    void drawCampaignWalls(const core::CampaignManager& campaign);
    void drawCampaignLaunchers(const core::CampaignManager& campaign);
    void drawCampaignGateways(const core::CampaignManager& campaign) { drawCampaignLaunchers(campaign); }
    void drawNeighborPreviews(const core::Board& board, int64_t hoveredIndex);
};


} // namespace minesweeper::render
