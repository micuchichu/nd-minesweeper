#pragma once

#include "renderer.hpp"
#include "camera_controller.hpp"
#include "particles.hpp"
#include "../ui/theme.hpp"
#include <string>
#include <vector>

namespace minesweeper::render {


struct CursorSkinItem {
    std::string name;
    Texture2D texture = {0};
};

struct FlagSkinItem {
    std::string name;
    Texture2D texture = {0};
};

struct PlayerSkinItem {
    std::string name;
    Texture2D texture = {0};
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
    static void drawCursorSkin(uint8_t skin, Vector2 pos, Color col, const char* name = nullptr, float scale = 1.0f, bool isSpeaking = false);

    bool isLocalSpeaking = false;
    float guiScale = 1.0f;

    static std::vector<FlagSkinItem> flagSkins;
    static int getFlagSkinCount();
    static const char* getFlagSkinName(int skin);
    static Texture2D getFlagTexture(int skin);
    int activeFlagSkin = 0;

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
    void cleanup() override;

    int64_t getHoveredCellIndex(const core::Board& board) const override;
    Vector2 getCellWorldPosition(size_t index, const core::Board& board) const override;

    void beginOffscreen();
    void endOffscreen();
    void drawOffscreenToScreen();

    void emitExplosion(Vector2 pos, Color col) { particles.emitExplosion(pos, 80, col); }
    void emitDebris(Vector2 pos, Color col) { particles.emitDebris(pos, 8, col); }
    void clearParticles() { particles.clear(); }

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
    RenderTexture2D offscreenTarget = {0};

    void initShaders();
    void initCellTextures();
    void unloadAssets();
    void drawSlice(const core::Board& board, size_t sliceZ, size_t sliceW, float sliceOriginX, float sliceOriginY, int64_t hoveredIndex);
    void drawNeighborPreviews(const core::Board& board, int64_t hoveredIndex);
};


} // namespace minesweeper::render
