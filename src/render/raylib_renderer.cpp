#include "raylib_renderer.hpp"
#include "asset_manager.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <filesystem>

namespace minesweeper::render {

namespace {
inline float hashTile(int32_t x, int32_t y, int32_t z = 0, int32_t w = 0) {
    uint32_t seed = 0x5D1B4A5D;
    uint32_t mx = 0x85EBCA6B;
    uint32_t my = 0xC2B2AE35;
    uint32_t mz = 0x7A9D1C2F;
    uint32_t mw = 0x9E3779B9;

    uint32_t hash = static_cast<uint32_t>(x) * mx;
    hash = (hash << 13) ^ hash;

    hash ^= static_cast<uint32_t>(y) * my;
    hash = (hash >> 15) ^ hash;

    hash ^= static_cast<uint32_t>(z) * mz;
    hash = (hash << 11) ^ hash;

    hash ^= static_cast<uint32_t>(w) * mw;
    hash = (hash >> 17) ^ hash;

    hash *= seed;
    hash ^= hash >> 16;

    return static_cast<float>(hash) / static_cast<float>(0xFFFFFFFF);
}
}

std::vector<CursorSkinItem> RaylibRenderer::cursorSkins;

int RaylibRenderer::getCursorSkinCount() {
    return static_cast<int>(cursorSkins.size());
}

const char* RaylibRenderer::getCursorSkinName(int skin) {
    if (skin >= 0 && skin < static_cast<int>(cursorSkins.size())) {
        return cursorSkins[skin].name.c_str();
    }
    return "DEFAULT";
}

Texture2D RaylibRenderer::getCursorSkinTexture(int skin) {
    if (skin >= 0 && skin < static_cast<int>(cursorSkins.size())) {
        return cursorSkins[skin].texture;
    }
    return Texture2D{ 0 };
}

std::vector<FlagSkinItem> RaylibRenderer::flagSkins;

int RaylibRenderer::getFlagSkinCount() {
    return static_cast<int>(flagSkins.size());
}

const char* RaylibRenderer::getFlagSkinName(int skin) {
    if (skin >= 0 && skin < static_cast<int>(flagSkins.size())) {
        return flagSkins[skin].name.c_str();
    }
    return "DEFAULT";
}

Texture2D RaylibRenderer::getFlagTexture(int skin) {
    if (skin >= 0 && skin < static_cast<int>(flagSkins.size())) {
        return flagSkins[skin].texture;
    }
    if (!flagSkins.empty()) {
        return flagSkins[0].texture;
    }
    return {0};
}

std::vector<PlayerSkinItem> RaylibRenderer::playerSkins;

int RaylibRenderer::getPlayerSkinCount() {
    return static_cast<int>(playerSkins.size());
}

const char* RaylibRenderer::getPlayerSkinName(int skin) {
    if (skin >= 0 && skin < static_cast<int>(playerSkins.size())) {
        return playerSkins[skin].name.c_str();
    }
    return "DEFAULT";
}

Texture2D RaylibRenderer::getPlayerTexture(int skin) {
    if (skin >= 0 && skin < static_cast<int>(playerSkins.size())) {
        return playerSkins[skin].texture;
    }
    if (!playerSkins.empty()) {
        return playerSkins[0].texture;
    }
    return {0};
}

RaylibRenderer::RaylibRenderer() = default;

RaylibRenderer::~RaylibRenderer() {
    cleanup();
}

void RaylibRenderer::init() {
    initShaders();
    initCellTextures();

    // Load custom flag skins from assets/skins/flags (or fallback to assets/flag.png)
    flagSkins.clear();
    namespace fs = std::filesystem;

    struct SkinCandidate {
        std::string stem;
        std::string path;
    };

    auto skinSortPred = [](const SkinCandidate& a, const SkinCandidate& b) {
        if (a.stem == "DEFAULT" && b.stem != "DEFAULT") return true;
        if (b.stem == "DEFAULT" && a.stem != "DEFAULT") return false;
        return a.stem < b.stem;
    };

    std::vector<std::string> flagSearchDirs = {
        "assets/skins/flags",
        std::string(GetApplicationDirectory()) + "assets/skins/flags",
        std::string(GetApplicationDirectory()) + "../assets/skins/flags",
        std::string(GetApplicationDirectory()) + "../../assets/skins/flags"
    };

    std::vector<SkinCandidate> flagCandidates;
    auto hasFlagCandidate = [&](const std::string& stem) {
        for (const auto& c : flagCandidates) {
            if (c.stem == stem) return true;
        }
        return false;
    };

    for (const auto& fDir : flagSearchDirs) {
        if (!DirectoryExists(fDir.c_str())) continue;
        try {
            for (const auto& entry : fs::directory_iterator(fDir)) {
                if (entry.is_regular_file()) {
                    auto path = entry.path();
                    std::string ext = path.extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
                        return static_cast<char>(std::tolower(c));
                    });
                    if (ext == ".png") {
                        std::string stem = path.stem().string();
                        for (char& c : stem) c = static_cast<char>(::toupper(c));
                        if (!hasFlagCandidate(stem)) {
                            flagCandidates.push_back({ stem, path.string() });
                        }
                    }
                }
            }
        } catch (...) {}
        if (!flagCandidates.empty()) break;
    }

    std::sort(flagCandidates.begin(), flagCandidates.end(), skinSortPred);

    for (const auto& c : flagCandidates) {
        Texture2D tex = LoadTexture(c.path.c_str());
        if (tex.id != 0) {
            SetTextureFilter(tex, TEXTURE_FILTER_POINT);
            flagSkins.push_back({ c.stem, tex });
        }
    }

    if (flagSkins.empty() && FileExists("assets/flag.png")) {
        Texture2D tex = LoadTexture("assets/flag.png");
        if (tex.id != 0) {
            SetTextureFilter(tex, TEXTURE_FILTER_POINT);
            flagSkins.push_back({ "DEFAULT", tex });
        }
    }
    if (flagSkins.empty()) {
        Image img = GenImageColor(48, 16, BLANK);
        for (int frame = 0; frame < 3; ++frame) {
            int ox = frame * 16;
            for (int y = 2; y < 14; ++y) {
                ImageDrawPixel(&img, ox + 2, y, WHITE);
            }
            for (int y = 2; y < 8; ++y) {
                for (int x = 2; x <= 10 - y + 2; ++x) {
                    ImageDrawPixel(&img, ox + x, y, ui::Colors::Red500);
                }
            }
        }
        Texture2D defTex = LoadTextureFromImage(img);
        UnloadImage(img);
        flagSkins.push_back({ "DEFAULT", defTex });
    }

    // Load custom cursor skins from assets/skins/cursors (or fallback to assets/skins)
    cursorSkins.clear();
    std::vector<std::string> cursorSearchDirs = {
        "assets/skins/cursors",
        std::string(GetApplicationDirectory()) + "assets/skins/cursors",
        std::string(GetApplicationDirectory()) + "../assets/skins/cursors",
        std::string(GetApplicationDirectory()) + "../../assets/skins/cursors",
        "assets/skins",
        std::string(GetApplicationDirectory()) + "assets/skins",
        std::string(GetApplicationDirectory()) + "../assets/skins",
        std::string(GetApplicationDirectory()) + "../../assets/skins"
    };

    std::vector<SkinCandidate> cursorCandidates;
    auto hasCursorCandidate = [&](const std::string& stem) {
        for (const auto& c : cursorCandidates) {
            if (c.stem == stem) return true;
        }
        return false;
    };

    for (const auto& cDir : cursorSearchDirs) {
        if (!DirectoryExists(cDir.c_str())) continue;
        try {
            for (const auto& entry : fs::directory_iterator(cDir)) {
                if (entry.is_regular_file()) {
                    auto path = entry.path();
                    std::string ext = path.extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
                        return static_cast<char>(std::tolower(c));
                    });
                    if (ext == ".png") {
                        std::string stem = path.stem().string();
                        for (char& c : stem) c = static_cast<char>(::toupper(c));
                        if (!hasCursorCandidate(stem)) {
                            cursorCandidates.push_back({ stem, path.string() });
                        }
                    }
                }
            }
        } catch (...) {}
        if (!cursorCandidates.empty()) break;
    }

    std::sort(cursorCandidates.begin(), cursorCandidates.end(), skinSortPred);

    for (const auto& c : cursorCandidates) {
        Texture2D tex = LoadTexture(c.path.c_str());
        if (tex.id != 0) {
            SetTextureFilter(tex, TEXTURE_FILTER_POINT);
            cursorSkins.push_back({ c.stem, tex });
        }
    }

    if (cursorSkins.empty()) {
        Image img = GenImageColor(16, 16, BLANK);
        for (int y = 0; y < 16; ++y) {
            for (int x = 0; x <= y && x < 12; ++x) {
                ImageDrawPixel(&img, x, y, (x == 0 || x == y || y == 15) ? BLACK : WHITE);
            }
        }
        Texture2D defTex = LoadTextureFromImage(img);
        UnloadImage(img);
        cursorSkins.push_back({ "DEFAULT", defTex });
    }

    // Load custom player skins from assets/skins/players
    playerSkins.clear();
    std::vector<std::string> playerSearchDirs = {
        "assets/skins/players",
        std::string(GetApplicationDirectory()) + "assets/skins/players",
        std::string(GetApplicationDirectory()) + "../assets/skins/players",
        std::string(GetApplicationDirectory()) + "../../assets/skins/players"
    };

    std::vector<SkinCandidate> playerCandidates;
    auto hasPlayerCandidate = [&](const std::string& stem) {
        for (const auto& c : playerCandidates) {
            if (c.stem == stem) return true;
        }
        return false;
    };

    for (const auto& pDir : playerSearchDirs) {
        if (!DirectoryExists(pDir.c_str())) continue;
        try {
            for (const auto& entry : fs::directory_iterator(pDir)) {
                if (entry.is_regular_file()) {
                    auto path = entry.path();
                    std::string ext = path.extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
                        return static_cast<char>(std::tolower(c));
                    });
                    if (ext == ".png") {
                        std::string stem = path.stem().string();
                        for (char& c : stem) c = static_cast<char>(::toupper(c));
                        if (!hasPlayerCandidate(stem)) {
                            playerCandidates.push_back({ stem, path.string() });
                        }
                    }
                }
            }
        } catch (...) {}
        if (!playerCandidates.empty()) break;
    }

    std::sort(playerCandidates.begin(), playerCandidates.end(), skinSortPred);

    for (const auto& c : playerCandidates) {
        Texture2D tex = LoadTexture(c.path.c_str());
        if (tex.id != 0) {
            SetTextureFilter(tex, TEXTURE_FILTER_POINT);
            playerSkins.push_back({ c.stem, tex });
        }
    }

    if (playerSkins.empty()) {
        Image img = GenImageColor(24, 24, BLANK);
        for (int y = 4; y < 20; ++y) {
            for (int x = 4; x < 20; ++x) {
                int dx = x - 12;
                int dy = y - 12;
                if (dx*dx + dy*dy <= 49) {
                    ImageDrawPixel(&img, x, y, ui::Colors::Green500);
                }
            }
        }
        ImageDrawPixel(&img, 9, 10, ui::Colors::Zinc950);
        ImageDrawPixel(&img, 15, 10, ui::Colors::Zinc950);
        ImageDrawPixel(&img, 9, 14, ui::Colors::Zinc950);
        ImageDrawPixel(&img, 10, 15, ui::Colors::Zinc950);
        ImageDrawPixel(&img, 11, 15, ui::Colors::Zinc950);
        ImageDrawPixel(&img, 12, 15, ui::Colors::Zinc950);
        ImageDrawPixel(&img, 13, 15, ui::Colors::Zinc950);
        ImageDrawPixel(&img, 14, 15, ui::Colors::Zinc950);
        ImageDrawPixel(&img, 15, 14, ui::Colors::Zinc950);

        Texture2D defTex = LoadTextureFromImage(img);
        UnloadImage(img);
        playerSkins.push_back({ "DEFAULT", defTex });
    }

    // Load all shop ships from assets/shops/ using AssetManager
    shopShips.clear();
    auto shopAssets = AssetManager::instance().loadShopShipAssets();
    for (size_t i = 0; i < shopAssets.size(); ++i) {
        Vector2 initAnchor = { 400.0f, 100.0f + static_cast<float>(i) * 240.0f };
        shopShips.emplace_back(shopAssets[i].texture, initAnchor, shopAssets[i].config);
        shopShips.back().setAnchor(initAnchor, 90.0f);
        shopShips.back().angle = 90.0f;
        shopShips.back().isInitialized = false;
    }
    if (!shopShips.empty()) {
        shopShip = shopShips.front();
    }
}

void RaylibRenderer::initShaders() {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    if (screenW <= 0) screenW = 1000;
    if (screenH <= 0) screenH = 900;

    offscreenTarget = LoadRenderTexture(screenW, screenH);

    if (FileExists("assets/shader.glsl")) {
        postProcessShader = LoadShader(nullptr, "assets/shader.glsl");
        ppTimeLoc = GetShaderLocation(postProcessShader, "time");
        ppResLoc = GetShaderLocation(postProcessShader, "resolution");
    }

    if (FileExists("assets/grid.glsl")) {
        gridShader = LoadShader(nullptr, "assets/grid.glsl");
        gridShaderGridSizeLoc = GetShaderLocation(gridShader, "gridSize");
        gridShaderCellSizeLoc = GetShaderLocation(gridShader, "cellSize");
        int marginLoc = GetShaderLocation(gridShader, "margin");
        int bgColorLoc = GetShaderLocation(gridShader, "bgColor");

        if (marginLoc >= 0) SetShaderValue(gridShader, marginLoc, &cellMargin, SHADER_UNIFORM_FLOAT);
        if (bgColorLoc >= 0) {
            Vector4 bgNorm = ColorNormalize(ui::Colors::BgSlice);
            SetShaderValue(gridShader, bgColorLoc, &bgNorm, SHADER_UNIFORM_VEC4);
        }
    }
}

void RaylibRenderer::initCellTextures() {
    float innerSize = cellSize - (cellMargin * 2.0f);
    int texSize = static_cast<int>(std::ceil(innerSize));
    if (texSize < 1) texSize = 1;

    if (cellHiddenRT.id != 0) UnloadRenderTexture(cellHiddenRT);
    cellHiddenRT = LoadRenderTexture(texSize, texSize);
    BeginTextureMode(cellHiddenRT);
    ClearBackground(BLANK);
    DrawRectangleRounded({ 0, 0, static_cast<float>(texSize), static_cast<float>(texSize) }, 0.2f, 4, ui::Colors::CellHidden);
    EndTextureMode();

    if (cellRevealRT.id != 0) UnloadRenderTexture(cellRevealRT);
    cellRevealRT = LoadRenderTexture(texSize, texSize);
    BeginTextureMode(cellRevealRT);
    ClearBackground(BLANK);
    DrawRectangleRounded({ 0, 0, static_cast<float>(texSize), static_cast<float>(texSize) }, 0.2f, 4, ui::Colors::CellRevealed);
    EndTextureMode();

    if (gridShader.id != 0 && gridShaderCellSizeLoc >= 0) {
        SetShaderValue(gridShader, gridShaderCellSizeLoc, &cellSize, SHADER_UNIFORM_FLOAT);
    }
}

void RaylibRenderer::cleanup() {
    unloadAssets();
    particles.clear();
}

void RaylibRenderer::unloadAssets() {
    if (flagTexture.id != 0) { UnloadTexture(flagTexture); flagTexture = {0}; }
    if (cellHiddenRT.id != 0) { UnloadRenderTexture(cellHiddenRT); cellHiddenRT = {0}; }
    if (cellRevealRT.id != 0) { UnloadRenderTexture(cellRevealRT); cellRevealRT = {0}; }
    if (offscreenTarget.id != 0) { UnloadRenderTexture(offscreenTarget); offscreenTarget = {0}; }
    if (gridShader.id != 0) { UnloadShader(gridShader); gridShader = {0}; }
    if (postProcessShader.id != 0) { UnloadShader(postProcessShader); postProcessShader = {0}; }

    for (auto& s : cursorSkins) {
        if (s.texture.id != 0) {
            UnloadTexture(s.texture);
            s.texture = {0};
        }
    }
    cursorSkins.clear();

    for (auto& f : flagSkins) {
        if (f.texture.id != 0) {
            UnloadTexture(f.texture);
            f.texture = {0};
        }
    }
    flagSkins.clear();

    for (auto& p : playerSkins) {
        if (p.texture.id != 0) {
            UnloadTexture(p.texture);
            p.texture = {0};
        }
    }
    playerSkins.clear();

    AssetManager::instance().shutdown();
    shopShips.clear();
}

void RaylibRenderer::update(float dt) {
    if (outOfReachTimer > 0.0f) {
        outOfReachTimer -= dt;
        if (outOfReachTimer <= 0.0f) {
            outOfReachCell = -1;
        }
    }

    for (auto it = flagDropAnims.begin(); it != flagDropAnims.end(); ) {
        it->second.timer -= dt;
        float p = std::clamp(1.0f - (it->second.timer / it->second.duration), 0.0f, 1.0f);
        if (p >= 0.75f && !it->second.landed) {
            it->second.landed = true;
            particles.emitDebris(it->second.groundPos, 4, ui::Colors::Zinc400);
        }
        if (it->second.timer <= 0.0f) {
            it = flagDropAnims.erase(it);
        } else {
            ++it;
        }
    }

    for (size_t i = 0; i < flagPickupAnims.size(); ) {
        flagPickupAnims[i].timer -= dt;
        if (flagPickupAnims[i].timer <= 0.0f) {
            Vector2 shipPos = flagPickupAnims[i].targetPos;
            if (flagPickupAnims[i].isLocal) {
                shipPos = localShip.position;
            } else if (flagPickupAnims[i].pickerId != 0 && remoteShips.count(flagPickupAnims[i].pickerId)) {
                shipPos = remoteShips.at(flagPickupAnims[i].pickerId).position;
            }
            particles.emitDebris(shipPos, 3, ui::Colors::Cyan400);
            flagPickupAnims[i] = flagPickupAnims.back();
            flagPickupAnims.pop_back();
        } else {
            ++i;
        }
    }

    for (auto& [id, rShip] : remoteShips) {
        if (rShip.isMoving) {
            rShip.emitThrusterParticles(dt, 1.0f);
        }
        rShip.updateExhaust(dt);
    }

    if (IsWindowResized()) {
        int w = GetScreenWidth();
        int h = GetScreenHeight();
        if (w > 0 && h > 0) {
            UnloadRenderTexture(offscreenTarget);
            offscreenTarget = LoadRenderTexture(w, h);
        }
    }
}

void RaylibRenderer::beginOffscreen() {
    BeginTextureMode(offscreenTarget);
    ClearBackground(BLACK);
}

void RaylibRenderer::endOffscreen() {
    EndTextureMode();
}

void RaylibRenderer::drawOffscreenToScreen() {
    if (enableCRT && postProcessShader.id != 0) {
        BeginShaderMode(postProcessShader);

        float timeVal = static_cast<float>(GetTime());
        SetShaderValue(postProcessShader, ppTimeLoc, &timeVal, SHADER_UNIFORM_FLOAT);

        float resVal[2] = { static_cast<float>(offscreenTarget.texture.width), static_cast<float>(offscreenTarget.texture.height) };
        SetShaderValue(postProcessShader, ppResLoc, resVal, SHADER_UNIFORM_VEC2);

        Rectangle source = { 0.0f, 0.0f, static_cast<float>(offscreenTarget.texture.width), -static_cast<float>(offscreenTarget.texture.height) };
        Rectangle dest = { 0.0f, 0.0f, static_cast<float>(offscreenTarget.texture.width), static_cast<float>(offscreenTarget.texture.height) };
        DrawTexturePro(offscreenTarget.texture, source, dest, { 0, 0 }, 0.0f, WHITE);

        EndShaderMode();
    } else {
        Rectangle source = { 0.0f, 0.0f, static_cast<float>(offscreenTarget.texture.width), -static_cast<float>(offscreenTarget.texture.height) };
        Rectangle dest = { 0.0f, 0.0f, static_cast<float>(offscreenTarget.texture.width), static_cast<float>(offscreenTarget.texture.height) };
        DrawTexturePro(offscreenTarget.texture, source, dest, { 0, 0 }, 0.0f, WHITE);
    }
}

int64_t RaylibRenderer::getHoveredCellIndex(const core::Board& board) const {
    Vector2 mouseCRT = camera.getCRTMousePosition();
    float topH = 70.0f * guiScale;
    if (mouseCRT.y >= 0.0f && mouseCRT.y < topH) {
        return -1;
    }
    float btmH = 92.0f * guiScale;
    float btmY = static_cast<float>(GetScreenHeight()) - btmH;
    if (mouseCRT.y >= btmY && mouseCRT.y <= btmY + btmH) {
        return -1;
    }

    Vector2 mouseWorld = camera.getScreenToWorld(mouseCRT);
    float boardWidth = board.config.size * cellSize;
    float sliceStride = boardWidth + slicePadding;

    if (board.config.dim == 2) {
        if (mouseWorld.x >= 0.0f && mouseWorld.x < boardWidth &&
            mouseWorld.y >= 0.0f && mouseWorld.y < boardWidth) {
            size_t x = static_cast<size_t>(mouseWorld.x / cellSize);
            size_t y = static_cast<size_t>(mouseWorld.y / cellSize);
            if (x < board.coord.size && y < board.coord.size) {
                return static_cast<int64_t>(board.coord.toIndex2D(x, y));
            }
        }
    }
    else if (board.config.dim == 3) {
        if (mouseWorld.x >= 0.0f && mouseWorld.x < boardWidth && mouseWorld.y >= 0.0f) {
            size_t z = static_cast<size_t>(mouseWorld.y / sliceStride);
            if (z < board.coord.size) {
                float localY = mouseWorld.y - (z * sliceStride);
                if (localY >= 0.0f && localY < boardWidth) {
                    size_t x = static_cast<size_t>(mouseWorld.x / cellSize);
                    size_t y = static_cast<size_t>(localY / cellSize);
                    if (x < board.coord.size && y < board.coord.size) {
                        return static_cast<int64_t>(board.coord.toIndex3D(x, y, z));
                    }
                }
            }
        }
    }
    else if (board.config.dim >= 4) {
        if (mouseWorld.x >= 0.0f && mouseWorld.y >= 0.0f) {
            size_t z = static_cast<size_t>(mouseWorld.x / sliceStride);
            size_t w = static_cast<size_t>(mouseWorld.y / sliceStride);
            if (z < board.coord.size && w < board.coord.size) {
                float localX = mouseWorld.x - (z * sliceStride);
                float localY = mouseWorld.y - (w * sliceStride);
                if (localX >= 0.0f && localX < boardWidth && localY >= 0.0f && localY < boardWidth) {
                    size_t x = static_cast<size_t>(localX / cellSize);
                    size_t y = static_cast<size_t>(localY / cellSize);
                    if (x < board.coord.size && y < board.coord.size) {
                        return static_cast<int64_t>(board.coord.toIndex4D(x, y, z, w));
                    }
                }
            }
        }
    }

    return -1;
}

Vector2 RaylibRenderer::getCellWorldPosition(size_t index, const core::Board& board) const {
    size_t x = 0, y = 0, z = 0, w = 0;
    float boardWidth = board.config.size * cellSize;
    float sliceStride = boardWidth + slicePadding;

    if (board.config.dim == 2) {
        board.coord.toCoord2D(index, x, y);
        return { (x + 0.5f) * cellSize, (y + 0.5f) * cellSize };
    }
    else if (board.config.dim == 3) {
        board.coord.toCoord3D(index, x, y, z);
        return { (x + 0.5f) * cellSize, (z * sliceStride) + (y + 0.5f) * cellSize };
    }
    else {
        board.coord.toCoord4D(index, x, y, z, w);
        return { (z * sliceStride) + (x + 0.5f) * cellSize, (w * sliceStride) + (y + 0.5f) * cellSize };
    }
}

void RaylibRenderer::drawSlice(const core::Board& board, size_t sliceZ, size_t sliceW, float sliceOriginX, float sliceOriginY, int64_t hoveredIndex) {
    float boardWidth = board.config.size * cellSize;

    // Background base slice
    if (gridShader.id != 0 && cellRevealRT.id != 0 && gridShaderGridSizeLoc >= 0) {
        Vector2 gridSizeVec = { static_cast<float>(board.config.size), static_cast<float>(board.config.size) };
        SetShaderValue(gridShader, gridShaderGridSizeLoc, &gridSizeVec, SHADER_UNIFORM_VEC2);

        BeginShaderMode(gridShader);
        Rectangle source = { 0.0f, 0.0f, static_cast<float>(cellRevealRT.texture.width), -static_cast<float>(cellRevealRT.texture.height) };
        Rectangle dest = { sliceOriginX, sliceOriginY, boardWidth, boardWidth };
        DrawTexturePro(cellRevealRT.texture, source, dest, {0, 0}, 0.0f, WHITE);
        EndShaderMode();
    } else {
        DrawRectangleRounded({ sliceOriginX, sliceOriginY, boardWidth, boardWidth }, 0.02f, 4, ui::Colors::BgSlice);
    }

    // Viewport Culling calculation for cells in this slice
    Vector2 viewMin = camera.getScreenToWorld({0, 0});
    Vector2 viewMax = camera.getScreenToWorld({ static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight()) });

    int startX = std::max(0, static_cast<int>((viewMin.x - sliceOriginX) / cellSize));
    int endX = std::min(board.config.size, static_cast<int>((viewMax.x - sliceOriginX) / cellSize + 1));
    int startY = std::max(0, static_cast<int>((viewMin.y - sliceOriginY) / cellSize));
    int endY = std::min(board.config.size, static_cast<int>((viewMax.y - sliceOriginY) / cellSize + 1));

    if (startX >= endX || startY >= endY) return;

    float visualSize = camera.getZoom() * cellSize;
    bool highDetail = visualSize > 14.0f;
    float fade = std::clamp((25.0f - visualSize) / 15.0f, 0.0f, 1.0f);

    float blink = 0.5f + 0.5f * std::sin(static_cast<float>(GetTime()) * 7.0f);
    float neighborAlpha = 0.02f + 0.14f * blink;

    size_t hX = 0, hY = 0, hZ = 0, hW = 0;
    bool hasHover = (hoveredIndex >= 0);
    if (hasHover) {
        if (board.config.dim == 2) board.coord.toCoord2D(static_cast<size_t>(hoveredIndex), hX, hY);
        else if (board.config.dim == 3) board.coord.toCoord3D(static_cast<size_t>(hoveredIndex), hX, hY, hZ);
        else board.coord.toCoord4D(static_cast<size_t>(hoveredIndex), hX, hY, hZ, hW);
    }

    for (int y = startY; y < endY; ++y) {
        for (int x = startX; x < endX; ++x) {
            size_t idx = 0;
            if (board.config.dim == 2) idx = board.coord.toIndex2D(x, y);
            else if (board.config.dim == 3) idx = board.coord.toIndex3D(x, y, sliceZ);
            else idx = board.coord.toIndex4D(x, y, sliceZ, sliceW);

            core::CellState state = board.getState(idx);
            uint8_t count = board.getCount(idx);
            bool isBomb = board.isBomb(idx);

            float posX = sliceOriginX + (x * cellSize);
            float posY = sliceOriginY + (y * cellSize);

            bool isHovered = (hasHover && static_cast<size_t>(hoveredIndex) == idx);
            bool isNeighbor = false;
            if (hasHover) {
                bool zMatch = (board.config.dim < 3) || (std::abs(static_cast<int>(sliceZ) - static_cast<int>(hZ)) <= 1);
                bool wMatch = (board.config.dim < 4) || (std::abs(static_cast<int>(sliceW) - static_cast<int>(hW)) <= 1);
                if (zMatch && wMatch && std::abs(static_cast<int>(x) - static_cast<int>(hX)) <= 1 && std::abs(static_cast<int>(y) - static_cast<int>(hY)) <= 1) {
                    isNeighbor = true;
                }
            }

            // Empty revealed cells (0 count) are fully cleared unless neighbor-highlighted
            if (state == core::CellState::Revealed && count == 0 && !isNeighbor) {
                continue;
            }

            Rectangle cellRect = {
                posX + cellMargin, posY + cellMargin,
                cellSize - (cellMargin * 2.0f),
                cellSize - (cellMargin * 2.0f)
            };

            if (highDetail) {
                if (board.isGameOver && isBomb) {
                    DrawRectangleRounded(cellRect, 0.2f, 4, ui::Colors::Red500);
                    int tw = MeasureText("*", 22);
                    DrawText("*", static_cast<int>(cellRect.x + (cellRect.width - tw) * 0.5f), static_cast<int>(cellRect.y + 2), 22, ui::Colors::Red700);
                }
                else if (state == core::CellState::Revealed) {
                    if (count > 0) {
                        Color tc = ui::getNeighborColor(count);
                        const char* numStr = TextFormat("%d", count);
                        int fontSize = std::clamp(static_cast<int>(cellSize * 0.65f), 12, 28);
                        int tw = MeasureText(numStr, fontSize);
                        DrawText(numStr, static_cast<int>(cellRect.x + (cellRect.width - tw) * 0.5f), static_cast<int>(cellRect.y + (cellRect.height - fontSize) * 0.5f), fontSize, tc);
                    }
                }
                else if (state == core::CellState::Flagged) {
                    if (cellHiddenRT.id != 0) {
                        Rectangle src = { 0.0f, 0.0f, static_cast<float>(cellHiddenRT.texture.width), -static_cast<float>(cellHiddenRT.texture.height) };
                        DrawTexturePro(cellHiddenRT.texture, src, cellRect, {0, 0}, 0.0f, WHITE);
                    } else {
                        DrawRectangleRounded(cellRect, 0.2f, 4, ui::Colors::CellHidden);
                    }

                    auto animIt = flagDropAnims.find(idx);
                    bool isMidFlight = false;
                    float dropY = 0.0f;
                    float squashX = 1.0f;
                    float squashY = 1.0f;

                    if (animIt != flagDropAnims.end()) {
                        float p = std::clamp(1.0f - (animIt->second.timer / animIt->second.duration), 0.0f, 1.0f);
                        if (p < 0.75f) {
                            isMidFlight = true;
                        } else {
                            float b = (p - 0.75f) / 0.25f;
                            dropY = -5.0f * std::sin(b * 3.14159265f) * (1.0f - b);
                            if (b < 0.4f) {
                                float sq = std::sin((b / 0.4f) * 3.14159265f);
                                squashX += 0.20f * sq;
                                squashY -= 0.18f * sq;
                            }
                        }
                    }

                    if (!isMidFlight) {
                        uint8_t cellFlagSkin = board.getFlagSkin(idx, static_cast<uint8_t>(activeFlagSkin));
                        Texture2D curFlag = getFlagTexture(cellFlagSkin);

                        if (curFlag.id != 0) {
                            float time = static_cast<float>(GetTime()) + (hashTile(static_cast<int32_t>(x), static_cast<int32_t>(y), static_cast<int32_t>(sliceZ), static_cast<int32_t>(sliceW)) * 10.0f);
                            int numFrames = (curFlag.width >= curFlag.height * 2) ? (curFlag.width / curFlag.height) : 1;
                            if (numFrames < 1) numFrames = 1;
                            float frameW = static_cast<float>(curFlag.width) / static_cast<float>(numFrames);
                            float frameH = static_cast<float>(curFlag.height);
                            int frame = (numFrames > 1) ? (static_cast<int>(time) % numFrames) : 0;
                            Rectangle flagSrc = { frame * frameW, 0.0f, frameW, frameH };

                            float flagScale = 1.05f;
                            float flagW = cellRect.width * flagScale;
                            float flagH = cellRect.height * flagScale;
                            float baseX = cellRect.x + 4.0f * cellRect.width / 16.0f;
                            float baseY = cellRect.y + 12.0f * cellRect.height / 16.0f;

                            // Soft ground contact shadow
                            float shadowFactor = (cellRect.width / 30.0f);
                            DrawEllipse(static_cast<int>(baseX + 1.0f), static_cast<int>(baseY + 1.0f), 5.0f * shadowFactor, 2.4f * shadowFactor, Fade(BLACK, 0.40f));

                            // Ground silhouette shadow
                            Rectangle shadowDest = { baseX + 2.0f, baseY + 1.5f, flagW, flagH };
                            Vector2 shadowOrigin = { 4.0f * shadowDest.width / 16.0f, 12.0f * shadowDest.height / 16.0f };
                            float tilt = std::sin(time) * 5.0f;
                            DrawTexturePro(curFlag, flagSrc, shadowDest, shadowOrigin, tilt, Fade(BLACK, 0.16f));

                            // Flag sprite (bouncing / settled waving)
                            float finalFlagW = flagW * squashX;
                            float finalFlagH = flagH * squashY;
                            Vector2 origin = { 4.0f * finalFlagW / 16.0f, 12.0f * finalFlagH / 16.0f };
                            Rectangle destRect = { baseX, baseY + dropY, finalFlagW, finalFlagH };

                            DrawTexturePro(curFlag, flagSrc, destRect, origin, tilt, WHITE);
                        } else {
                            DrawRectangleRounded({ cellRect.x + 4.0f, cellRect.y + 6.0f, cellRect.width - 8.0f, cellRect.height - 8.0f }, 0.2f, 4, Fade(BLACK, 0.20f));
                            DrawRectangleRounded({ cellRect.x + 4.0f, cellRect.y + 4.0f + dropY, cellRect.width - 8.0f, cellRect.height - 8.0f }, 0.2f, 4, ui::Colors::Red500);
                        }
                    }
                }
                else {
                    if (cellHiddenRT.id != 0) {
                        Rectangle src = { 0.0f, 0.0f, static_cast<float>(cellHiddenRT.texture.width), -static_cast<float>(cellHiddenRT.texture.height) };
                        DrawTexturePro(cellHiddenRT.texture, src, cellRect, {0, 0}, 0.0f, WHITE);
                    } else {
                        DrawRectangleRounded(cellRect, 0.2f, 4, ui::Colors::CellHidden);
                    }
                }

                bool isOutOfReach = (outOfReachTimer > 0.0f && outOfReachCell >= 0 && static_cast<int64_t>(idx) == outOfReachCell);
                if (isOutOfReach) {
                    float reachAlpha = std::min(1.0f, outOfReachTimer * 2.5f);
                    float t = static_cast<float>(GetTime());
                    float pulse = 0.5f + 0.5f * std::sin(t * 14.0f);
                    DrawRectangleRounded(cellRect, 0.2f, 4, Fade(ui::Colors::Red600, (0.6f + 0.35f * pulse) * reachAlpha));
                    DrawRectangleLinesEx(cellRect, 2.0f, Fade(ui::Colors::Red400, (0.85f + 0.15f * pulse) * reachAlpha));
                }

                bool isStartingCell = (board.revealedCount == 0 && !board.isGameOver && !board.isVictory && board.startingCell >= 0 && static_cast<int64_t>(idx) == board.startingCell);
                if (isStartingCell) {
                    float t = static_cast<float>(GetTime());
                    float pulse = 0.5f + 0.5f * std::sin(t * 6.0f);
                    Color safeCol = ui::Colors::Green400;
                    Color safeBg = ui::Colors::Green600;

                    // Pulsing emerald highlight backdrop
                    DrawRectangleRounded(cellRect, 0.2f, 4, Fade(safeBg, 0.35f + 0.25f * pulse));

                    // Glowing border
                    DrawRectangleLinesEx(cellRect, 2.0f, Fade(safeCol, 0.75f + 0.25f * pulse));

                    // Tactical Corner Brackets (Mindustry high-tech targeting reticle)
                    float bracketLen = std::clamp(cellRect.width * 0.3f, 4.0f, 9.0f);
                    float bx = cellRect.x;
                    float by = cellRect.y;
                    float bw = cellRect.width;
                    float bh = cellRect.height;
                    Color bracketCol = WHITE;

                    // Top-Left
                    DrawLineEx({ bx, by }, { bx + bracketLen, by }, 2.0f, bracketCol);
                    DrawLineEx({ bx, by }, { bx, by + bracketLen }, 2.0f, bracketCol);
                    // Top-Right
                    DrawLineEx({ bx + bw, by }, { bx + bw - bracketLen, by }, 2.0f, bracketCol);
                    DrawLineEx({ bx + bw, by }, { bx + bw, by + bracketLen }, 2.0f, bracketCol);
                    // Bottom-Left
                    DrawLineEx({ bx, by + bh }, { bx + bracketLen, by + bh }, 2.0f, bracketCol);
                    DrawLineEx({ bx, by + bh }, { bx, by + bh - bracketLen }, 2.0f, bracketCol);
                    // Bottom-Right
                    DrawLineEx({ bx + bw, by + bh }, { bx + bw - bracketLen, by + bh }, 2.0f, bracketCol);
                    DrawLineEx({ bx + bw, by + bh }, { bx + bw, by + bh - bracketLen }, 2.0f, bracketCol);
                }

                if (isNeighbor) {
                    if (isHovered) {
                        DrawRectangleRec(cellRect, Fade(WHITE, 0.22f + 0.06f * blink));
                        DrawRectangleLinesEx(cellRect, 2.0f, WHITE);
                    } else {
                        DrawRectangleRounded(cellRect, 0.2f, 4, Fade(WHITE, neighborAlpha));
                    }
                }

                if (state == core::CellState::Revealed && count > 0) {
                    Color overlay = ui::getNeighborColor(count);
                    overlay.a = static_cast<unsigned char>(15 + count * 2 + fade * 150.0f);
                    DrawRectangleRounded(cellRect, 0.2f, 4, overlay);
                }
            }
            else {
                // LOD Mode for distant zoom
                Color lodCol = ui::Colors::CellHidden;
                if (board.isGameOver && isBomb) lodCol = ui::Colors::Red500;
                else if (state == core::CellState::Revealed) {
                    if (count > 0) lodCol = ui::getNeighborColor(count);
                    else if (!isNeighbor) continue;
                    else lodCol = BLANK;
                }
                else if (state == core::CellState::Flagged) lodCol = ui::Colors::Red500;

                bool isStartingCell = (board.revealedCount == 0 && !board.isGameOver && !board.isVictory && board.startingCell >= 0 && static_cast<int64_t>(idx) == board.startingCell);
                if (isStartingCell) {
                    float t = static_cast<float>(GetTime());
                    float pulse = 0.5f + 0.5f * std::sin(t * 6.0f);
                    lodCol = ui::Colors::Green400;
                    DrawRectangle(static_cast<int>(posX + 1), static_cast<int>(posY + 1), static_cast<int>(cellSize - 2), static_cast<int>(cellSize - 2), Fade(lodCol, 0.6f + 0.4f * pulse));
                    DrawRectangleLinesEx({ posX + 1.0f, posY + 1.0f, cellSize - 2.0f, cellSize - 2.0f }, 1.5f, WHITE);
                } else {
                    DrawRectangle(static_cast<int>(posX + 1), static_cast<int>(posY + 1), static_cast<int>(cellSize - 2), static_cast<int>(cellSize - 2), lodCol);
                }

                bool isOutOfReach = (outOfReachTimer > 0.0f && outOfReachCell >= 0 && static_cast<int64_t>(idx) == outOfReachCell);
                if (isOutOfReach) {
                    float reachAlpha = std::min(1.0f, outOfReachTimer * 2.5f);
                    float t = static_cast<float>(GetTime());
                    float pulse = 0.5f + 0.5f * std::sin(t * 14.0f);
                    DrawRectangle(static_cast<int>(posX + 1), static_cast<int>(posY + 1), static_cast<int>(cellSize - 2), static_cast<int>(cellSize - 2), Fade(ui::Colors::Red600, (0.7f + 0.3f * pulse) * reachAlpha));
                    DrawRectangleLinesEx({ posX + 1.0f, posY + 1.0f, cellSize - 2.0f, cellSize - 2.0f }, 2.0f, Fade(ui::Colors::Red400, reachAlpha));
                }

                if (isHovered) {
                    Rectangle lodRect = { posX + 1.0f, posY + 1.0f, cellSize - 2.0f, cellSize - 2.0f };
                    DrawRectangleLinesEx(lodRect, 1.5f, WHITE);
                } else if (isNeighbor) {
                    DrawRectangle(static_cast<int>(posX + 1), static_cast<int>(posY + 1), static_cast<int>(cellSize - 2), static_cast<int>(cellSize - 2), Fade(WHITE, neighborAlpha));
                }
            }
        }
    }
}

void RaylibRenderer::drawCursorSkin(uint8_t skin, Vector2 pos, float angle, Color col, const char* name, float scale, bool isSpeaking, bool isMoving) {
    core::Ship s;
    s.position = pos;
    s.angle = angle;
    s.color = col;
    s.scale = scale;
    s.isSpeaking = isSpeaking;
    s.isMoving = isMoving;
    s.skinId = skin;
    s.texture = getCursorSkinTexture(skin);
    s.draw(name, col, isSpeaking);
}

void RaylibRenderer::syncRemoteShips(const std::map<uint32_t, net::RemoteCursor>& cursors) {
    for (const auto& [id, c] : cursors) {
        auto& s = remoteShips[id];
        s.position = { c.x, c.y };
        s.angle = c.angle;
        s.mass = c.mass;
        s.isMoving = c.isMoving;
        s.skinId = c.skin;
        s.texture = getCursorSkinTexture(c.skin);
        s.name = c.name;
        s.isSpeaking = c.isSpeaking;
        s.isInitialized = true;
    }
    for (auto it = remoteShips.begin(); it != remoteShips.end(); ) {
        if (cursors.find(it->first) == cursors.end()) {
            it = remoteShips.erase(it);
        } else {
            ++it;
        }
    }
}

void RaylibRenderer::updateShopAnchor(const core::Board& board) {
    if (board.config.size <= 0) return;
    float boardWidth = static_cast<float>(board.config.size) * cellSize;
    float sliceStride = boardWidth + slicePadding;
    float totalW = boardWidth;
    float totalH = boardWidth;
    if (board.config.dim == 3) {
        totalH = static_cast<float>(board.config.size - 1) * sliceStride + boardWidth;
    } else if (board.config.dim >= 4) {
        totalW = static_cast<float>(board.config.size - 1) * sliceStride + boardWidth;
        totalH = static_cast<float>(board.config.size - 1) * sliceStride + boardWidth;
    }
    if (shopShips.empty()) return;

    // Calculate dynamic vertical spacing along the right side of the board facing down
    const float gap = 45.0f;
    std::vector<float> relY(shopShips.size(), 0.0f);
    for (size_t i = 1; i < shopShips.size(); ++i) {
        float halfLenPrev = ((shopShips[i - 1].texture.width > 0 ? static_cast<float>(shopShips[i - 1].texture.width) : 64.0f) * shopShips[i - 1].scale) * 0.5f;
        float halfLenCurr = ((shopShips[i].texture.width > 0 ? static_cast<float>(shopShips[i].texture.width) : 64.0f) * shopShips[i].scale) * 0.5f;
        relY[i] = relY[i - 1] + halfLenPrev + 24.0f + gap + halfLenCurr;
    }
    float totalSpan = relY.back() - relY.front();
    float startY = totalH * 0.5f - totalSpan * 0.5f;

    for (size_t i = 0; i < shopShips.size(); ++i) {
        float y = startY + relY[i];
        float xOffset = (shopShips[i].texture.height > 48) ? static_cast<float>(shopShips[i].texture.height - 48) * 0.5f : 0.0f;
        Vector2 newAnchor = { totalW + 90.0f + xOffset, y };
        if (i == 0) shopAnchorPos = newAnchor;

        if (!shopShips[i].isInitialized) {
            shopShips[i].setAnchor(newAnchor, 90.0f);
            shopShips[i].position = newAnchor;
            shopShips[i].angle = 90.0f;
            shopShips[i].velocity = { 0.0f, 0.0f };
            shopShips[i].isInitialized = true;
        } else if (std::abs(shopShips[i].anchorPosition.x - newAnchor.x) > 1.0f ||
                   std::abs(shopShips[i].anchorPosition.y - newAnchor.y) > 1.0f ||
                   std::abs(shopShips[i].restAngle - 90.0f) > 0.1f) {
            Vector2 diff = { shopShips[i].position.x - shopShips[i].anchorPosition.x,
                             shopShips[i].position.y - shopShips[i].anchorPosition.y };
            shopShips[i].setAnchor(newAnchor, 90.0f);
            shopShips[i].position = { newAnchor.x + diff.x, newAnchor.y + diff.y };
            shopShips[i].angle = 90.0f;
        }
    }
    if (!shopShips.empty()) {
        shopShip = shopShips.front();
    }
}

void RaylibRenderer::resolveShipCollisions() {
    for (auto& [id, rShip] : remoteShips) {
        if (core::Ship::resolveCollision(localShip, rShip)) {
            if (localShip.bumpTimer >= 0.34f) {
                Vector2 contact = {
                    (localShip.position.x + rShip.position.x) * 0.5f,
                    (localShip.position.y + rShip.position.y) * 0.5f
                };
                particles.emitDebris(contact, 1, ui::Colors::Amber400);
            }
        }
    }
    for (auto it1 = remoteShips.begin(); it1 != remoteShips.end(); ++it1) {
        auto it2 = it1;
        ++it2;
        for (; it2 != remoteShips.end(); ++it2) {
            if (core::Ship::resolveCollision(it1->second, it2->second)) {
                if (it1->second.bumpTimer >= 0.34f) {
                    Vector2 contact = {
                        (it1->second.position.x + it2->second.position.x) * 0.5f,
                        (it1->second.position.y + it2->second.position.y) * 0.5f
                    };
                    particles.emitDebris(contact, 1, ui::Colors::Cyan400);
                }
            }
        }
    }
    for (auto& s : shopShips) {
        if (!s.isInitialized) continue;
        core::Ship::resolveCollision(localShip, s);
        for (auto& [id, rShip] : remoteShips) {
            core::Ship::resolveCollision(rShip, s);
        }
    }
    for (size_t i = 0; i < shopShips.size(); ++i) {
        for (size_t j = i + 1; j < shopShips.size(); ++j) {
            core::Ship::resolveCollision(shopShips[i], shopShips[j]);
        }
    }
    if (!shopShips.empty()) {
        shopShip = shopShips.front();
    }
}

void RaylibRenderer::stepPhysics(Vector2 targetPos, float fixedDt) {
    // 1. Update player / local ship physics at fixed timestep
    localShip.skinId = activeCursorSkin;
    localShip.texture = getCursorSkinTexture(activeCursorSkin);
    localShip.update(targetPos, fixedDt);

    // 2. Update shop freighters at fixed timestep
    for (auto& s : shopShips) {
        s.update(fixedDt);
    }
    if (!shopShips.empty()) {
        shopShip = shopShips.front();
    }

    // 3. Resolve all pairwise collisions at fixed timestep
    resolveShipCollisions();
}

void RaylibRenderer::updatePhysics(Vector2 targetPos, float dt) {
    float clampedDt = std::clamp(dt, 0.0f, 0.1f);
    physicsAccumulator += clampedDt;

    int maxSteps = 8;
    while (physicsAccumulator >= FIXED_PHYSICS_DT && maxSteps > 0) {
        stepPhysics(targetPos, FIXED_PHYSICS_DT);
        physicsAccumulator -= FIXED_PHYSICS_DT;
        --maxSteps;
    }
}

void RaylibRenderer::updateShip(Vector2 targetPos, float dt) {
    localShip.skinId = activeCursorSkin;
    localShip.texture = getCursorSkinTexture(activeCursorSkin);
    localShip.update(targetPos, dt);
}

void RaylibRenderer::fireLaser(Vector2 from, Vector2 to, Color color) {
    lasers.push_back({ from, to, 0.18f, 0.18f, color });
    particles.emitDebris(to, 6, color);
}

Color RaylibRenderer::getLaserColorForSkin(int skinId) {
    if (skinId >= 0 && skinId < static_cast<int>(cursorSkins.size())) {
        const std::string& name = cursorSkins[skinId].name;
        if (name == "BLUE")   return Color{ 0, 229, 255, 255 };
        if (name == "BROWN")  return Color{ 255, 170, 0, 255 };
        if (name == "CYAN")   return Color{ 0, 255, 210, 255 };
        if (name == "GREEN")  return Color{ 34, 197, 94, 255 };
        if (name == "ORANGE") return Color{ 255, 120, 0, 255 };
        if (name == "PINK")   return Color{ 255, 105, 180, 255 };
        if (name == "PURPLE") return Color{ 190, 80, 255, 255 };
        if (name == "RED")    return Color{ 255, 50, 70, 255 };
        if (name == "WHITE")  return Color{ 240, 255, 255, 255 };
    }

    switch (skinId) {
        case 0: return Color{ 0, 229, 255, 255 };   // Cyan / Blue
        case 1: return Color{ 255, 170, 0, 255 };   // Amber / Brown
        case 2: return Color{ 0, 255, 210, 255 };   // Bright Cyan
        case 3: return Color{ 34, 197, 94, 255 };   // Emerald Green
        case 4: return Color{ 255, 120, 0, 255 };   // Plasma Orange
        case 5: return Color{ 255, 105, 180, 255 }; // Hot Pink
        case 6: return Color{ 190, 80, 255, 255 };  // Violet / Purple
        case 7: return Color{ 255, 50, 70, 255 };   // Ruby Red
        case 8: return Color{ 240, 255, 255, 255 }; // Pure White / Plasma
        default: return Color{ 0, 229, 255, 255 };
    }
}

void RaylibRenderer::triggerOutOfReach(int64_t cellIndex, Vector2 cellPos) {
    outOfReachCell = cellIndex;
    outOfReachPos = cellPos;
    outOfReachTimer = 1.3f;
}

void RaylibRenderer::clearOutOfReach() {
    outOfReachCell = -1;
    outOfReachTimer = 0.0f;
}

Vector2 RaylibRenderer::getFlagBasePosition(size_t index, const core::Board& board) const {
    Vector2 cellPos = getCellWorldPosition(index, board);
    return { cellPos.x - cellSize * 0.5f + 4.0f * cellSize / 16.0f, cellPos.y - cellSize * 0.5f + 12.0f * cellSize / 16.0f };
}

void RaylibRenderer::triggerFlagDrop(size_t cellIndex, Vector2 groundPos, Vector2 shipPos, uint8_t skinId) {
    FlagDropAnim anim;
    anim.shipPos = shipPos;
    anim.groundPos = groundPos;
    anim.flagSkin = skinId;
    anim.duration = 0.11f;
    anim.timer = anim.duration;
    anim.landed = false;
    flagDropAnims[cellIndex] = anim;
}

void RaylibRenderer::triggerFlagPickup(Vector2 groundPos, Vector2 shipPos, uint32_t pickerId, bool isLocal, uint8_t skinId) {
    FlagPickupAnim anim;
    anim.startPos = groundPos;
    anim.targetPos = shipPos;
    anim.pickerId = pickerId;
    anim.isLocal = isLocal;
    anim.flagSkin = skinId;
    anim.duration = 0.10f;
    anim.timer = anim.duration;
    flagPickupAnims.push_back(anim);

    particles.emitDebris(groundPos, 3, ui::Colors::Zinc400);
}

void RaylibRenderer::removeFlagDrop(size_t cellIndex) {
    flagDropAnims.erase(cellIndex);
}

void RaylibRenderer::clearFlagDrops() {
    flagDropAnims.clear();
    flagPickupAnims.clear();
}

void RaylibRenderer::drawFlyingFlags(const core::Board& /*board*/) {
    float curTime = static_cast<float>(GetTime());

    // 1. Flying Drops (Flight phase: p < 0.75f)
    for (const auto& pair : flagDropAnims) {
        const auto& anim = pair.second;
        float p = std::clamp(1.0f - (anim.timer / anim.duration), 0.0f, 1.0f);
        if (p >= 0.75f) continue; // Handled on cell surface by drawSlice

        float t = p / 0.75f;
        float easeT = t * (2.0f - t);
        Vector2 curGround = { anim.shipPos.x + (anim.groundPos.x - anim.shipPos.x) * easeT,
                              anim.shipPos.y + (anim.groundPos.y - anim.shipPos.y) * easeT };
        float alt = 36.0f * (1.0f - t) + 20.0f * std::sin(t * 3.14159265f);
        Vector2 flagPos = { curGround.x, curGround.y - alt };

        // Scale up from small (0.25x emerging from ship) to full size (1.0x landing on cell)
        float growScale = 0.25f + 0.75f * std::sin(t * (3.14159265f * 0.5f));

        float shadowScale = (cellSize / 30.0f) * (0.25f + 0.75f * t);
        float shadowAlpha = 0.10f + 0.30f * t;
        DrawEllipse(static_cast<int>(curGround.x + 1.0f), static_cast<int>(curGround.y + 1.0f), 5.0f * shadowScale, 2.4f * shadowScale, Fade(BLACK, shadowAlpha));

        Texture2D curFlag = getFlagTexture(anim.flagSkin);
        if (curFlag.id != 0) {
            int numFrames = (curFlag.width >= curFlag.height * 2) ? (curFlag.width / curFlag.height) : 1;
            if (numFrames < 1) numFrames = 1;
            float frameW = static_cast<float>(curFlag.width) / static_cast<float>(numFrames);
            float frameH = static_cast<float>(curFlag.height);
            int frame = (numFrames > 1) ? (static_cast<int>(curTime * 10.0f) % numFrames) : 0;
            Rectangle flagSrc = { frame * frameW, 0.0f, frameW, frameH };

            float flagScale = (cellSize / 30.0f) * 1.05f * growScale;
            float squashY = 1.0f + 0.15f * (1.0f - t);
            float squashX = 1.0f - 0.08f * (1.0f - t);
            float flagW = 30.0f * flagScale * squashX;
            float flagH = 30.0f * flagScale * squashY;

            Rectangle shadowDest = { curGround.x + 2.0f * shadowScale, curGround.y + 1.5f * shadowScale, flagW, flagH };
            Vector2 shadowOrigin = { 4.0f * shadowDest.width / 16.0f, 12.0f * shadowDest.height / 16.0f };
            float tilt = std::sin(curTime * 8.0f) * 4.0f;
            DrawTexturePro(curFlag, flagSrc, shadowDest, shadowOrigin, tilt, Fade(BLACK, shadowAlpha * 0.35f));

            Vector2 origin = { 4.0f * flagW / 16.0f, 12.0f * flagH / 16.0f };
            Rectangle destRect = { flagPos.x, flagPos.y, flagW, flagH };
            DrawTexturePro(curFlag, flagSrc, destRect, origin, tilt, WHITE);
        } else {
            float boxSize = 20.0f * growScale;
            DrawRectangleRounded({ flagPos.x - boxSize * 0.5f, flagPos.y - boxSize * 0.5f, boxSize, boxSize }, 0.2f, 4, ui::Colors::Red500);
        }
    }

    // 2. Flying Pickups (Ground -> Ship)
    for (const auto& anim : flagPickupAnims) {
        float p = std::clamp(1.0f - (anim.timer / anim.duration), 0.0f, 1.0f);
        Vector2 targetShip = anim.targetPos;
        if (anim.isLocal) {
            targetShip = localShip.position;
        } else if (anim.pickerId != 0 && remoteShips.count(anim.pickerId)) {
            targetShip = remoteShips.at(anim.pickerId).position;
        }

        float t = p * p;
        Vector2 curGround = { anim.startPos.x + (targetShip.x - anim.startPos.x) * t,
                              anim.startPos.y + (targetShip.y - anim.startPos.y) * t };
        float alt = 26.0f * std::sin(p * 3.14159265f);
        Vector2 flagPos = { curGround.x, curGround.y - alt };

        Color beamColor = ui::Colors::Cyan400;
        float beamAlpha = 0.45f * (1.0f - p);
        DrawLineEx(flagPos, targetShip, 2.2f * (1.0f - p), Fade(beamColor, beamAlpha));
        DrawLineEx(flagPos, targetShip, 1.0f, Fade(WHITE, 0.7f * (1.0f - p)));
        DrawCircleV(flagPos, 3.5f * (1.0f - p), Fade(beamColor, 0.6f * (1.0f - p)));

        float shadowAlpha = std::max(0.0f, 0.35f * (1.0f - p) * (1.0f - std::min(1.0f, alt / 28.0f)));
        if (shadowAlpha > 0.01f) {
            float shadowScale = (cellSize / 30.0f) * (1.0f - 0.5f * p);
            DrawEllipse(static_cast<int>(curGround.x + 1.0f), static_cast<int>(curGround.y + 1.0f), 5.0f * shadowScale, 2.4f * shadowScale, Fade(BLACK, shadowAlpha));
        }

        float flagScale = (cellSize / 30.0f) * 1.05f * (1.0f - 0.70f * p);
        Texture2D curFlag = getFlagTexture(anim.flagSkin);
        if (curFlag.id != 0) {
            int numFrames = (curFlag.width >= curFlag.height * 2) ? (curFlag.width / curFlag.height) : 1;
            if (numFrames < 1) numFrames = 1;
            float frameW = static_cast<float>(curFlag.width) / static_cast<float>(numFrames);
            float frameH = static_cast<float>(curFlag.height);
            Rectangle flagSrc = { 0.0f, 0.0f, frameW, frameH };

            float flagW = 30.0f * flagScale;
            float flagH = 30.0f * flagScale;
            Vector2 origin = { 4.0f * flagW / 16.0f, 12.0f * flagH / 16.0f };
            Rectangle destRect = { flagPos.x, flagPos.y, flagW, flagH };
            float tilt = std::sin(curTime * 14.0f) * 8.0f;
            DrawTexturePro(curFlag, flagSrc, destRect, origin, tilt, Fade(WHITE, 1.0f - 0.2f * p));
        } else {
            DrawRectangleRounded({ flagPos.x - 8.0f * (1.0f - p), flagPos.y - 8.0f * (1.0f - p), 16.0f * (1.0f - p), 16.0f * (1.0f - p) }, 0.2f, 4, ui::Colors::Red500);
        }
    }
}


void RaylibRenderer::render(const core::Board& board, int64_t hoveredIndex, const net::NetworkManager& net) {
    BeginMode2D(camera.camera);

    float boardWidth = board.config.size * cellSize;
    float sliceStride = boardWidth + slicePadding;

    Vector2 viewMin = camera.getScreenToWorld({0, 0});
    Vector2 viewMax = camera.getScreenToWorld({ static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight()) });

    if (board.config.dim == 2) {
        drawSlice(board, 0, 0, 0.0f, 0.0f, hoveredIndex);
    }
    else if (board.config.dim == 3) {
        int startZ = std::max(0, static_cast<int>(viewMin.y / sliceStride));
        int endZ = std::min(board.config.size, static_cast<int>(viewMax.y / sliceStride + 1));

        for (int z = startZ; z < endZ; ++z) {
            float sliceY = z * sliceStride;
            drawSlice(board, z, 0, 0.0f, sliceY, hoveredIndex);
        }
    }
    else if (board.config.dim >= 4) {
        int startZ = std::max(0, static_cast<int>(viewMin.x / sliceStride));
        int endZ = std::min(board.config.size, static_cast<int>(viewMax.x / sliceStride + 1));
        int startW = std::max(0, static_cast<int>(viewMin.y / sliceStride));
        int endW = std::min(board.config.size, static_cast<int>(viewMax.y / sliceStride + 1));

        for (int w = startW; w < endW; ++w) {
            float sliceY = w * sliceStride;
            for (int z = startZ; z < endZ; ++z) {
                float sliceX = z * sliceStride;
                drawSlice(board, z, w, sliceX, sliceY, hoveredIndex);
            }
        }
    }

    drawFlyingFlags(board);

    particles.updateAndDraw(GetFrameTime());

    // Ensure shop anchors are aligned with board dimensions
    updateShopAnchor(board);

    // Sync remote players
    syncRemoteShips(net.remoteCursors);

    // 1. Draw exhaust particles beneath all ships
    for (const auto& [id, rShip] : remoteShips) {
        rShip.drawExhaust();
    }
    for (const auto& s : shopShips) {
        s.drawExhaust();
    }
    localShip.drawExhaust();

    // 2. Draw shop ships on the side of the board
    for (const auto& s : shopShips) {
        if (s.isInitialized) {
            s.draw(s.name.c_str(), ui::Colors::Amber400);
        }
    }

    // 3. Draw remote ships
    for (const auto& [id, rShip] : remoteShips) {
        Color curCol = ui::Colors::Red500;
        if (id != 0) {
            curCol = ColorFromHSV(std::fmod(id * 137.5f, 360.0f), 0.8f, 1.0f);
        }
        const char* tag = !rShip.name.empty() ? rShip.name.c_str() : (id == 0 ? "HOST" : TextFormat("P%u", id));
        rShip.draw(tag, curCol, rShip.isSpeaking);
    }

    // 4. Draw local player ship
    localShip.draw(nullptr, ui::Colors::Green500, isLocalSpeaking);

    // 5. Draw active laser beams (rendered on top of ships)
    float frameDt = GetFrameTime();
    for (size_t i = 0; i < lasers.size(); ) {
        lasers[i].life -= frameDt;
        if (lasers[i].life <= 0.0f) {
            lasers[i] = lasers.back();
            lasers.pop_back();
        } else {
            float alpha = lasers[i].life / lasers[i].maxLife;
            // Outer glowing plasma beam
            DrawLineEx(lasers[i].from, lasers[i].to, 4.0f, Fade(lasers[i].color, 0.55f * alpha));
            // Inner core laser beam
            DrawLineEx(lasers[i].from, lasers[i].to, 1.5f, Fade(WHITE, 0.95f * alpha));
            // Muzzle flash at ship's nose
            DrawCircleV(lasers[i].from, 3.8f, Fade(lasers[i].color, 0.8f * alpha));
            DrawCircleV(lasers[i].from, 1.8f, Fade(WHITE, alpha));
            // Impact flare at target cell
            DrawCircleV(lasers[i].to, 5.5f, Fade(lasers[i].color, 0.85f * alpha));
            DrawCircleV(lasers[i].to, 2.5f, Fade(WHITE, alpha));
            ++i;
        }
    }

    // Subtle tactical aim crosshair at the cursor
    Vector2 worldMouse = camera.getScreenToWorld(camera.getCRTMousePosition());
    float chSize = 3.5f;
    DrawLineEx({ worldMouse.x - chSize, worldMouse.y }, { worldMouse.x + chSize, worldMouse.y }, 1.0f, Fade(WHITE, 0.45f));
    DrawLineEx({ worldMouse.x, worldMouse.y - chSize }, { worldMouse.x, worldMouse.y + chSize }, 1.0f, Fade(WHITE, 0.45f));

    // Draw Safe Starting Cell Beacon & Floating Badge (World Space)
    if (board.revealedCount == 0 && !board.isGameOver && !board.isVictory && board.startingCell >= 0 && board.coord.totalCells > 0) {
        Vector2 startPos = getCellWorldPosition(static_cast<size_t>(board.startingCell), board);
        float t = static_cast<float>(GetTime());
        float pulse = 0.5f + 0.5f * std::sin(t * 6.0f);

        // Floating "SAFE START" pill badge above cell
        const char* label = "SAFE START";
        int fontSize = 11;
        int textW = MeasureText(label, fontSize);
        float badgeW = static_cast<float>(textW + 18);
        float badgeH = 19.0f;
        float badgeX = startPos.x - badgeW * 0.5f;
        float badgeY = startPos.y - (cellSize * 0.5f) - badgeH - 6.0f;

        // Badge background & border
        Rectangle badgeRect = { badgeX, badgeY, badgeW, badgeH };
        DrawRectangleRounded(badgeRect, 0.4f, 4, Fade(ui::Colors::Zinc950, 0.92f));
        DrawRectangleLinesEx(badgeRect, 1.0f, Fade(ui::Colors::Green400, 0.85f + 0.15f * pulse));

        // Green dot indicator
        DrawCircle(static_cast<int>(badgeX + 8.0f), static_cast<int>(badgeY + badgeH * 0.5f), 2.5f, ui::Colors::Green400);

        // Label text
        DrawText(label, static_cast<int>(badgeX + 15.0f), static_cast<int>(badgeY + 4.0f), fontSize, ui::Colors::Green400);

        // Downward pointer triangle pointing to the cell
        Vector2 arrowP1 = { startPos.x - 4.5f, badgeY + badgeH };
        Vector2 arrowP2 = { startPos.x + 4.5f, badgeY + badgeH };
        Vector2 arrowP3 = { startPos.x, badgeY + badgeH + 5.0f };
        DrawTriangle(arrowP1, arrowP3, arrowP2, ui::Colors::Green400);
    }

    // Draw Out of Reach Warning Beacon & Floating Badge (World Space)
    if (outOfReachTimer > 0.0f && outOfReachCell >= 0) {
        float alpha = std::min(1.0f, outOfReachTimer * 2.5f);
        float t = static_cast<float>(GetTime());
        float pulse = 0.5f + 0.5f * std::sin(t * 12.0f);

        const char* label = "Cell out of reach, js wait a bit";
        int fontSize = 11;
        int textW = MeasureText(label, fontSize);
        float badgeW = static_cast<float>(textW + 20);
        float badgeH = 20.0f;
        float badgeX = outOfReachPos.x + (cellSize * 0.5f) - badgeW * 0.5f;
        float badgeY = outOfReachPos.y - badgeH - 8.0f;

        // Badge shadow & background
        Rectangle shadowRect = { badgeX + 1.5f, badgeY + 2.0f, badgeW, badgeH };
        DrawRectangleRounded(shadowRect, 0.4f, 4, Fade(BLACK, 0.45f * alpha));
        Rectangle badgeRect = { badgeX, badgeY, badgeW, badgeH };
        DrawRectangleRounded(badgeRect, 0.4f, 4, Fade(ui::Colors::Zinc950, 0.95f * alpha));
        DrawRectangleLinesEx(badgeRect, 1.0f, Fade(ui::Colors::Red500, (0.8f + 0.2f * pulse) * alpha));

        // Red warning dot indicator
        DrawCircle(static_cast<int>(badgeX + 8.0f), static_cast<int>(badgeY + badgeH * 0.5f), 2.5f, Fade(ui::Colors::Red500, alpha));

        // Label text in clear warning red
        DrawText(label, static_cast<int>(badgeX + 16.0f), static_cast<int>(badgeY + 4.0f), fontSize, Fade(ui::Colors::Red300, alpha));

        // Downward pointer triangle pointing to the cell
        Vector2 arrowP1 = { outOfReachPos.x + (cellSize * 0.5f) - 4.5f, badgeY + badgeH };
        Vector2 arrowP2 = { outOfReachPos.x + (cellSize * 0.5f) + 4.5f, badgeY + badgeH };
        Vector2 arrowP3 = { outOfReachPos.x + (cellSize * 0.5f), badgeY + badgeH + 5.0f };
        DrawTriangle(arrowP1, arrowP3, arrowP2, Fade(ui::Colors::Red500, alpha));
    }

    EndMode2D();

    // Draw off-screen neighbor previews on the screen edges (in screen space)
    drawNeighborPreviews(board, hoveredIndex);

    // If the safe starting cell is completely off-screen, show an edge locator arrow (in screen space)
    if (board.revealedCount == 0 && !board.isGameOver && !board.isVictory && board.startingCell >= 0 && board.coord.totalCells > 0) {
        Vector2 startWorld = getCellWorldPosition(static_cast<size_t>(board.startingCell), board);
        Vector2 screenPos = camera.getWorldToScreen(startWorld);
        float screenW = static_cast<float>(GetScreenWidth());
        float screenH = static_cast<float>(GetScreenHeight());

        float marginL = 60.0f;
        float marginR = screenW - 60.0f;
        float marginT = 85.0f * guiScale;
        float marginB = screenH - (95.0f * guiScale);

        bool offScreen = (screenPos.x < marginL || screenPos.x > marginR || screenPos.y < marginT || screenPos.y > marginB);
        if (offScreen) {
            float clampedX = std::clamp(screenPos.x, marginL, marginR);
            float clampedY = std::clamp(screenPos.y, marginT, marginB);

            // Compute direction vector towards target
            float dx = screenPos.x - clampedX;
            float dy = screenPos.y - clampedY;
            float len = std::sqrt(dx * dx + dy * dy);
            if (len > 0.001f) {
                dx /= len;
                dy /= len;
            } else {
                dx = 0.0f;
                dy = -1.0f;
            }

            float t = static_cast<float>(GetTime());
            float pulse = 0.5f + 0.5f * std::sin(t * 6.0f);

            // Locator pill
            const char* locText = "SAFE START";
            int fSize = 12;
            int tWidth = MeasureText(locText, fSize);
            float locW = static_cast<float>(tWidth + 24);
            float locH = 24.0f;
            Rectangle locRect = { clampedX - locW * 0.5f, clampedY - locH * 0.5f, locW, locH };

            DrawRectangleRounded(locRect, 0.4f, 4, Fade(ui::Colors::Zinc950, 0.90f));
            DrawRectangleLinesEx(locRect, 1.5f, Fade(ui::Colors::Green400, 0.8f + 0.2f * pulse));
            DrawText(locText, static_cast<int>(locRect.x + 12.0f), static_cast<int>(locRect.y + 6.0f), fSize, ui::Colors::Green400);

            // Pointer arrow at edge
            Vector2 tip = { clampedX + dx * 16.0f, clampedY + dy * 16.0f };
            Vector2 side1 = { clampedX - dy * 6.0f, clampedY + dx * 6.0f };
            Vector2 side2 = { clampedX + dy * 6.0f, clampedY - dx * 6.0f };
            DrawTriangle(tip, side1, side2, ui::Colors::Green400);
        }
    }
}

void RaylibRenderer::drawNeighborPreviews(const core::Board& board, int64_t hoveredIndex) {
    if (hoveredIndex < 0 || board.config.dim < 3 || board.coord.totalCells == 0) return;

    size_t hX = 0, hY = 0, hZ = 0, hW = 0;
    if (board.config.dim == 3) {
        board.coord.toCoord3D(static_cast<size_t>(hoveredIndex), hX, hY, hZ);
    } else {
        board.coord.toCoord4D(static_cast<size_t>(hoveredIndex), hX, hY, hZ, hW);
    }

    float screenW = static_cast<float>(GetScreenWidth());
    float screenH = static_cast<float>(GetScreenHeight());

    float viewLeft = 20.0f;
    float viewRight = screenW - 20.0f;
    float viewTop = 75.0f;
    float viewBottom = screenH - 96.0f;

    float boardWidth = board.config.size * cellSize;
    float sliceStride = boardWidth + slicePadding;
    float boxWorldSize = 3.0f * cellSize;
    float boxScreenSize = boxWorldSize * camera.getZoom();

    Vector2 mouseCRT = camera.getCRTMousePosition();

    struct MiniCell {
        bool valid = false;
        core::CellState state = core::CellState::Hidden;
        uint8_t count = 0;
        bool isBomb = false;
        uint8_t flagSkin = 0;
        bool isCenter = false;
    };

    enum class CardSlot {
        TopLeft,
        Top,
        TopRight,
        Left,
        Right,
        BottomLeft,
        Bottom,
        BottomRight
    };

    struct PreviewCard {
        std::string title;
        CardSlot slot = CardSlot::Top;
        int flags = 0;
        int hidden = 0;
        MiniCell cells[9];
    };

    std::vector<PreviewCard> activeCards;

    auto processSlice = [&](int dz, int dw, const char* title, CardSlot slot) {
        int nz = static_cast<int>(hZ) + dz;
        int nw = static_cast<int>(hW) + dw;
        if (nz < 0 || nz >= board.config.size || nw < 0 || nw >= board.config.size) return;

        Vector2 worldCenter;
        if (board.config.dim == 3) {
            worldCenter = { (static_cast<float>(hX) + 0.5f) * cellSize, (static_cast<float>(nz) * sliceStride) + (static_cast<float>(hY) + 0.5f) * cellSize };
        } else {
            worldCenter = { (static_cast<float>(nz) * sliceStride) + (static_cast<float>(hX) + 0.5f) * cellSize, (static_cast<float>(nw) * sliceStride) + (static_cast<float>(hY) + 0.5f) * cellSize };
        }

        Vector2 screenCenter = camera.getWorldToScreen(worldCenter);

        float minX = screenCenter.x - boxScreenSize * 0.5f;
        float maxX = screenCenter.x + boxScreenSize * 0.5f;
        float minY = screenCenter.y - boxScreenSize * 0.5f;
        float maxY = screenCenter.y + boxScreenSize * 0.5f;

        // If the 3x3 neighborhood of this slice is fully visible on screen, skip preview
        bool fullyVisible = (minX >= viewLeft && maxX <= viewRight && minY >= viewTop && maxY <= viewBottom);
        if (fullyVisible) return;

        PreviewCard card;
        card.title = title;
        card.slot = slot;

        for (int r = 0; r < 3; ++r) {
            int dy = r - 1;
            int ny = static_cast<int>(hY) + dy;
            for (int c = 0; c < 3; ++c) {
                int dx = c - 1;
                int nx = static_cast<int>(hX) + dx;
                int cellIdx = r * 3 + c;

                if (nx >= 0 && nx < board.config.size && ny >= 0 && ny < board.config.size) {
                    size_t idx = (board.config.dim == 3)
                        ? board.coord.toIndex3D(static_cast<size_t>(nx), static_cast<size_t>(ny), static_cast<size_t>(nz))
                        : board.coord.toIndex4D(static_cast<size_t>(nx), static_cast<size_t>(ny), static_cast<size_t>(nz), static_cast<size_t>(nw));

                    card.cells[cellIdx].valid = true;
                    card.cells[cellIdx].state = board.getState(idx);
                    card.cells[cellIdx].count = board.getCount(idx);
                    card.cells[cellIdx].isBomb = board.isBomb(idx);
                    card.cells[cellIdx].flagSkin = board.getFlagSkin(idx, static_cast<uint8_t>(activeFlagSkin));
                    card.cells[cellIdx].isCenter = (dx == 0 && dy == 0);

                    if (card.cells[cellIdx].state == core::CellState::Flagged) card.flags++;
                    else if (card.cells[cellIdx].state == core::CellState::Hidden) card.hidden++;
                } else {
                    card.cells[cellIdx].valid = false;
                }
            }
        }

        activeCards.push_back(card);
    };

    if (board.config.dim == 3) {
        if (hZ > 0) {
            processSlice(-1, 0, TextFormat("SLICE Z - 1 (Z=%zu)", hZ - 1), CardSlot::Top);
        }
        if (hZ + 1 < static_cast<size_t>(board.config.size)) {
            processSlice(1, 0, TextFormat("SLICE Z + 1 (Z=%zu)", hZ + 1), CardSlot::Bottom);
        }
    } else {
        // 4D Neighbor Slices positioned around the screen perimeter
        struct OffsetDef { int dz; int dw; const char* prefix; CardSlot slot; };
        const OffsetDef defs[] = {
            {-1, -1, "Z-1, W-1 (%d,%d)", CardSlot::TopLeft},
            { 0, -1, "SLICE W - 1 (W=%d)", CardSlot::Top},
            { 1, -1, "Z+1, W-1 (%d,%d)", CardSlot::TopRight},
            {-1,  0, "SLICE Z - 1 (Z=%d)", CardSlot::Left},
            { 1,  0, "SLICE Z + 1 (Z=%d)", CardSlot::Right},
            {-1,  1, "Z-1, W+1 (%d,%d)", CardSlot::BottomLeft},
            { 0,  1, "SLICE W + 1 (W=%d)", CardSlot::Bottom},
            { 1,  1, "Z+1, W+1 (%d,%d)", CardSlot::BottomRight}
        };

        for (const auto& d : defs) {
            int nz = static_cast<int>(hZ) + d.dz;
            int nw = static_cast<int>(hW) + d.dw;
            if (nz >= 0 && nz < board.config.size && nw >= 0 && nw < board.config.size) {
                std::string title;
                if (d.dw == 0) title = TextFormat(d.prefix, nz);
                else if (d.dz == 0) title = TextFormat(d.prefix, nw);
                else title = TextFormat(d.prefix, nz, nw);
                processSlice(d.dz, d.dw, title.c_str(), d.slot);
            }
        }
    }

    if (activeCards.empty()) return;

    float cardW = 124.0f;
    float cardH = 116.0f;

    float marginX = 14.0f;
    float marginY = 74.0f;
    float footerH = 92.0f;
    float btmY = (screenH - footerH) - cardH - 4.0f;
    float leftX = marginX;
    float rightX = screenW - cardW - marginX;

    // Hovered cell's screen position
    Vector2 hWorld = getCellWorldPosition(static_cast<size_t>(hoveredIndex), board);
    Vector2 hScreen = camera.getWorldToScreen(hWorld);

    // Track active corner cards to prevent overlapping
    bool hasTopLeft = false, hasTopRight = false, hasBottomLeft = false, hasBottomRight = false;
    for (const auto& c : activeCards) {
        if (c.slot == CardSlot::TopLeft) hasTopLeft = true;
        if (c.slot == CardSlot::TopRight) hasTopRight = true;
        if (c.slot == CardSlot::BottomLeft) hasBottomLeft = true;
        if (c.slot == CardSlot::BottomRight) hasBottomRight = true;
    }

    // Dynamic horizontal tracking for Top and Bottom cards (slide horizontally with hovered cell)
    float minTopX = hasTopLeft ? (leftX + cardW + 6.0f) : leftX;
    float maxTopX = hasTopRight ? (rightX - cardW - 6.0f) : rightX;
    if (minTopX > maxTopX) minTopX = maxTopX = (screenW - cardW) * 0.5f;
    float topX = std::clamp(hScreen.x - cardW * 0.5f, minTopX, maxTopX);

    float minBtmX = hasBottomLeft ? (leftX + cardW + 6.0f) : leftX;
    float maxBtmX = hasBottomRight ? (rightX - cardW - 6.0f) : rightX;
    if (minBtmX > maxBtmX) minBtmX = maxBtmX = (screenW - cardW) * 0.5f;
    float btmX = std::clamp(hScreen.x - cardW * 0.5f, minBtmX, maxBtmX);

    // Dynamic vertical tracking for Left and Right cards (slide vertically with hovered cell)
    float minLeftY = hasTopLeft ? (marginY + cardH + 6.0f) : marginY;
    float maxLeftY = hasBottomLeft ? (btmY - cardH - 6.0f) : btmY;
    if (minLeftY > maxLeftY) minLeftY = maxLeftY = marginY + (btmY - marginY) * 0.5f;
    float leftY = std::clamp(hScreen.y - cardH * 0.5f, minLeftY, maxLeftY);

    float minRightY = hasTopRight ? (marginY + cardH + 6.0f) : marginY;
    float maxRightY = hasBottomRight ? (btmY - cardH - 6.0f) : btmY;
    if (minRightY > maxRightY) minRightY = maxRightY = marginY + (btmY - marginY) * 0.5f;
    float rightY = std::clamp(hScreen.y - cardH * 0.5f, minRightY, maxRightY);

    auto getSlotRect = [&](CardSlot slot) -> Rectangle {
        switch (slot) {
            case CardSlot::TopLeft:     return { leftX,  marginY, cardW, cardH };
            case CardSlot::Top:         return { topX,   marginY, cardW, cardH };
            case CardSlot::TopRight:    return { rightX, marginY, cardW, cardH };
            case CardSlot::Left:        return { leftX,  leftY,   cardW, cardH };
            case CardSlot::Right:       return { rightX, rightY,  cardW, cardH };
            case CardSlot::BottomLeft:  return { leftX,  btmY,    cardW, cardH };
            case CardSlot::Bottom:      return { btmX,   btmY,    cardW, cardH };
            case CardSlot::BottomRight: return { rightX, btmY,    cardW, cardH };
        }
        return { (screenW - cardW) * 0.5f, marginY, cardW, cardH };
    };

    auto drawSlotArrow = [&](CardSlot slot, float cx, float cy, Color color) {
        float s = 4.0f;
        switch (slot) {
            case CardSlot::Top:
                DrawTriangle({ cx, cy - s }, { cx - s, cy + s }, { cx + s, cy + s }, color);
                break;
            case CardSlot::Bottom:
                DrawTriangle({ cx, cy + s }, { cx + s, cy - s }, { cx - s, cy - s }, color);
                break;
            case CardSlot::Left:
                DrawTriangle({ cx - s, cy }, { cx + s, cy - s }, { cx + s, cy + s }, color);
                break;
            case CardSlot::Right:
                DrawTriangle({ cx + s, cy }, { cx - s, cy + s }, { cx - s, cy - s }, color);
                break;
            case CardSlot::TopLeft:
                DrawTriangle({ cx - s, cy - s }, { cx - s, cy + s }, { cx + s, cy - s }, color);
                break;
            case CardSlot::TopRight:
                DrawTriangle({ cx + s, cy - s }, { cx - s, cy - s }, { cx + s, cy + s }, color);
                break;
            case CardSlot::BottomLeft:
                DrawTriangle({ cx - s, cy + s }, { cx + s, cy + s }, { cx - s, cy - s }, color);
                break;
            case CardSlot::BottomRight:
                DrawTriangle({ cx + s, cy + s }, { cx + s, cy - s }, { cx - s, cy + s }, color);
                break;
        }
    };

    for (size_t i = 0; i < activeCards.size(); ++i) {
        const auto& card = activeCards[i];
        Rectangle cardRect = getSlotRect(card.slot);

        Rectangle hoverBox = { cardRect.x - 8.0f, cardRect.y - 8.0f, cardRect.width + 16.0f, cardRect.height + 16.0f };
        bool mouseNear = CheckCollisionPointRec(mouseCRT, hoverBox);
        float alpha = mouseNear ? 0.22f : 0.94f;

        // Drop shadow
        DrawRectangleRounded({ cardRect.x + 2.0f, cardRect.y + 2.0f, cardW, cardH }, 0.12f, 4, Fade(BLACK, 0.40f * alpha));
        // Card background
        DrawRectangleRounded(cardRect, 0.12f, 4, Fade(ui::Colors::Zinc950, alpha));
        // Border
        DrawRectangleLinesEx(cardRect, 1.5f, Fade(ui::Colors::Zinc700, alpha));

        // Header Title with Directional Chevron Indicator
        int titleFontSize = 10;
        int tw = MeasureText(card.title.c_str(), titleFontSize);
        float textStartX = cardRect.x + (cardW - tw) * 0.5f;
        float headerY = cardRect.y + 5.0f;
        drawSlotArrow(card.slot, textStartX - 7.0f, headerY + 5.0f, Fade(ui::Colors::Green400, alpha));
        DrawText(card.title.c_str(), static_cast<int>(textStartX), static_cast<int>(headerY), titleFontSize, Fade(ui::Colors::Green400, alpha));

        // 3x3 Grid
        float miniCellSize = 23.0f;
        float miniCellMargin = 2.0f;
        float gridStartX = cardRect.x + (cardW - 3.0f * miniCellSize) * 0.5f;
        float gridStartY = cardRect.y + 19.0f;

        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) {
                const auto& cell = card.cells[r * 3 + c];
                Rectangle mRect = {
                    gridStartX + static_cast<float>(c) * miniCellSize + miniCellMargin,
                    gridStartY + static_cast<float>(r) * miniCellSize + miniCellMargin,
                    miniCellSize - miniCellMargin * 2.0f,
                    miniCellSize - miniCellMargin * 2.0f
                };

                if (!cell.valid) {
                    DrawRectangleRounded(mRect, 0.2f, 2, Fade(ui::Colors::Zinc900, 0.6f * alpha));
                    continue;
                }

                if (cell.state == core::CellState::Revealed) {
                    DrawRectangleRounded(mRect, 0.2f, 2, Fade(ui::Colors::CellRevealed, alpha));
                    if (cell.count > 0) {
                        Color tc = ui::getNeighborColor(cell.count);
                        tc.a = static_cast<unsigned char>(255 * alpha);
                        int fs = 12;
                        const char* num = TextFormat("%d", cell.count);
                        int nw = MeasureText(num, fs);
                        DrawText(num, static_cast<int>(mRect.x + (mRect.width - nw) * 0.5f), static_cast<int>(mRect.y + (mRect.height - fs) * 0.5f), fs, tc);
                    }
                }
                else if (cell.state == core::CellState::Flagged) {
                    DrawRectangleRounded(mRect, 0.2f, 2, Fade(ui::Colors::CellHidden, alpha));
                    DrawRectangleRounded({ mRect.x + 2, mRect.y + 2, mRect.width - 4, mRect.height - 4 }, 0.2f, 2, Fade(ui::Colors::Red500, alpha));
                    int fs = 11;
                    int fw = MeasureText("F", fs);
                    DrawText("F", static_cast<int>(mRect.x + (mRect.width - fw) * 0.5f), static_cast<int>(mRect.y + 2), fs, Fade(WHITE, alpha));
                }
                else {
                    DrawRectangleRounded(mRect, 0.2f, 2, Fade(ui::Colors::CellHidden, alpha));
                    if (board.isGameOver && cell.isBomb) {
                        DrawRectangleRounded(mRect, 0.2f, 2, Fade(ui::Colors::Red500, alpha));
                        int fs = 12;
                        int bw = MeasureText("*", fs);
                        DrawText("*", static_cast<int>(mRect.x + (mRect.width - bw) * 0.5f), static_cast<int>(mRect.y + 2), fs, Fade(ui::Colors::Red700, alpha));
                    }
                }

                // Direct projected center neighbor receives a distinct white border
                if (cell.isCenter) {
                    DrawRectangleLinesEx(mRect, 1.5f, Fade(WHITE, 0.95f * alpha));
                }
            }
        }

        // Footer showing flag and hidden totals in that slice's 3x3
        std::string footer = TextFormat("FLAGS: %d  HIDDEN: %d", card.flags, card.hidden);
        int footerFs = 9;
        int ftw = MeasureText(footer.c_str(), footerFs);
        DrawText(footer.c_str(), static_cast<int>(cardRect.x + (cardW - ftw) * 0.5f), static_cast<int>(cardRect.y + cardH - 18.0f), footerFs, Fade(ui::Colors::Zinc400, alpha));
    }
}

} // namespace minesweeper::render
