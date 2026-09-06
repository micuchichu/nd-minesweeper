#include "raylib_renderer.hpp"
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

    std::vector<std::string> flagSearchDirs = {
        "assets/skins/flags",
        std::string(GetApplicationDirectory()) + "assets/skins/flags",
        std::string(GetApplicationDirectory()) + "../assets/skins/flags",
        std::string(GetApplicationDirectory()) + "../../assets/skins/flags"
    };

    auto hasFlagSkin = [](const std::string& name) {
        for (const auto& s : flagSkins) {
            if (s.name == name) return true;
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
                        if (!hasFlagSkin(stem)) {
                            Texture2D tex = LoadTexture(path.string().c_str());
                            if (tex.id != 0) {
                                SetTextureFilter(tex, TEXTURE_FILTER_POINT);
                                if (stem == "DEFAULT") {
                                    flagSkins.insert(flagSkins.begin(), { stem, tex });
                                } else {
                                    flagSkins.push_back({ stem, tex });
                                }
                            }
                        }
                    }
                }
            }
        } catch (...) {}
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

    auto hasCursorSkin = [](const std::string& name) {
        for (const auto& s : cursorSkins) {
            if (s.name == name) return true;
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
                        if (!hasCursorSkin(stem)) {
                            Texture2D tex = LoadTexture(path.string().c_str());
                            if (tex.id != 0) {
                                SetTextureFilter(tex, TEXTURE_FILTER_POINT);
                                cursorSkins.push_back({ stem, tex });
                            }
                        }
                    }
                }
            }
        } catch (...) {}
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
    std::string playersDir = "assets/skins/players";
    if (DirectoryExists(playersDir.c_str())) {
        namespace fs = std::filesystem;
        try {
            for (const auto& entry : fs::directory_iterator(playersDir)) {
                if (entry.is_regular_file()) {
                    auto path = entry.path();
                    std::string ext = path.extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
                        return static_cast<char>(std::tolower(c));
                    });
                    if (ext == ".png") {
                        Texture2D tex = LoadTexture(path.string().c_str());
                        if (tex.id != 0) {
                            SetTextureFilter(tex, TEXTURE_FILTER_POINT);
                            std::string stem = path.stem().string();
                            for (char& c : stem) c = static_cast<char>(::toupper(c));
                            if (stem == "DEFAULT") {
                                playerSkins.insert(playerSkins.begin(), { stem, tex });
                            } else {
                                playerSkins.push_back({ stem, tex });
                            }
                        }
                    }
                }
            }
        } catch (...) {}
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
}

void RaylibRenderer::update(float dt) {
    (void)dt;
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
                        Vector2 origin = { 4.0f * flagW / 16.0f, 12.0f * flagH / 16.0f };
                        Rectangle destRect = { cellRect.x + 4.0f * cellRect.width / 16.0f, cellRect.y + 12.0f * cellRect.height / 16.0f, flagW, flagH };
                        float tilt = std::sin(time) * 5.0f;

                        DrawTexturePro(curFlag, flagSrc, destRect, origin, tilt, WHITE);
                    } else {
                        DrawRectangleRounded({ cellRect.x + 4, cellRect.y + 4, cellRect.width - 8, cellRect.height - 8 }, 0.2f, 4, ui::Colors::Red500);
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
    int skinIdx = static_cast<int>(skin);
    bool drewTexture = false;

    if (skinIdx >= 0 && skinIdx < static_cast<int>(cursorSkins.size()) && cursorSkins[skinIdx].texture.id != 0) {
        const auto& tex = cursorSkins[skinIdx].texture;
        Rectangle src = { 0.0f, 0.0f, static_cast<float>(tex.width), static_cast<float>(tex.height) };
        float w = static_cast<float>(tex.width) * scale;
        float h = static_cast<float>(tex.height) * scale;
        Rectangle dest = { pos.x, pos.y, w, h };
        Vector2 origin = { w * 0.5f, h * 0.5f };

        // Hovering Drop Shadow
        // Offset slightly down-right in world space to create 3D altitude above the board
        Vector2 shadowOffset = { 2.5f * scale, 3.5f * scale };
        Rectangle shadowOuter = { pos.x + shadowOffset.x, pos.y + shadowOffset.y, w * 1.06f, h * 1.06f };
        Vector2 originOuter = { shadowOuter.width * 0.5f, shadowOuter.height * 0.5f };
        DrawTexturePro(tex, src, shadowOuter, originOuter, angle, Fade(BLACK, 0.18f));
        Rectangle shadowDest = { pos.x + shadowOffset.x, pos.y + shadowOffset.y, w, h };
        DrawTexturePro(tex, src, shadowDest, origin, angle, Fade(BLACK, 0.38f));

        float theta = angle * DEG2RAD;
        float cosA = std::cos(theta);
        float sinA = std::sin(theta);

        // Center particles and thrusters at (4, 12) and (11, 12) relative to top-left of 16x16 sprite
        // Relative to texture center (8.0, 8.0): pixel 4 center is -3.5, pixel 11 center is +3.5, row 12 center is +4.5
        float lxLeft = -3.5f * scale;
        float lxRight = 3.5f * scale;
        float ly = 4.5f * scale;

        Vector2 leftThrust = {
            pos.x + (lxLeft * cosA - ly * sinA),
            pos.y + (lxLeft * sinA + ly * cosA)
        };
        Vector2 rightThrust = {
            pos.x + (lxRight * cosA - ly * sinA),
            pos.y + (lxRight * sinA + ly * cosA)
        };

        // Direction out the rear of the ship (downwards in sprite coordinates)
        Vector2 rear = { -sinA, cosA };
        // Direction perpendicular to rear along flame base (horizontal in sprite coordinates)
        Vector2 flameDir = { cosA, sinA };

        float t = static_cast<float>(GetTime());
        if (isMoving) {
            float flicker1 = 4.0f + 3.0f * std::sin(t * 40.0f);
            float flicker2 = 4.0f + 3.0f * std::cos(t * 46.0f);

            Vector2 leftTip = { leftThrust.x + rear.x * (flicker1 * scale), leftThrust.y + rear.y * (flicker1 * scale) };
            Vector2 rightTip = { rightThrust.x + rear.x * (flicker2 * scale), rightThrust.y + rear.y * (flicker2 * scale) };

            float flameW = 1.6f * scale;
            // Outer plasma plume (Orange)
            DrawTriangle(leftTip, { leftThrust.x + flameDir.x * flameW, leftThrust.y + flameDir.y * flameW }, { leftThrust.x - flameDir.x * flameW, leftThrust.y - flameDir.y * flameW }, ui::Colors::Orange500);
            DrawTriangle(rightTip, { rightThrust.x + flameDir.x * flameW, rightThrust.y + flameDir.y * flameW }, { rightThrust.x - flameDir.x * flameW, rightThrust.y - flameDir.y * flameW }, ui::Colors::Orange500);

            // Inner hot plasma core (Amber/Yellow)
            Vector2 leftCoreTip = { leftThrust.x + rear.x * (flicker1 * 0.55f * scale), leftThrust.y + rear.y * (flicker1 * 0.55f * scale) };
            Vector2 rightCoreTip = { rightThrust.x + rear.x * (flicker2 * 0.55f * scale), rightThrust.y + rear.y * (flicker2 * 0.55f * scale) };
            float coreW = 1.0f * scale;
            DrawTriangle(leftCoreTip, { leftThrust.x + flameDir.x * coreW, leftThrust.y + flameDir.y * coreW }, { leftThrust.x - flameDir.x * coreW, leftThrust.y - flameDir.y * coreW }, ui::Colors::Amber300);
            DrawTriangle(rightCoreTip, { rightThrust.x + flameDir.x * coreW, rightThrust.y + flameDir.y * coreW }, { rightThrust.x - flameDir.x * coreW, rightThrust.y - flameDir.y * coreW }, ui::Colors::Amber300);
        } else {
            // Subtle idle thruster glow beneath the ship
            float idlePulse = 0.4f + 0.4f * std::sin(t * 6.0f);
            DrawCircleV(leftThrust, 1.2f * scale + idlePulse, Fade(ui::Colors::Amber500, 0.75f));
            DrawCircleV(rightThrust, 1.2f * scale + idlePulse, Fade(ui::Colors::Amber500, 0.75f));
        }

        // Draw Ship Sprite rotated around center
        DrawTexturePro(tex, src, dest, origin, angle, WHITE);
        drewTexture = true;
    }

    if (!drewTexture) {
        int px = static_cast<int>(pos.x - 5.0f * scale);
        int py = static_cast<int>(pos.y - 5.0f * scale);
        DrawRectangle(px, py, static_cast<int>(10 * scale), static_cast<int>(10 * scale), col);
        DrawRectangleLines(px, py, static_cast<int>(10 * scale), static_cast<int>(10 * scale), WHITE);
    }

    if (isSpeaking) {
        float t = static_cast<float>(GetTime());
        float pulse = (std::sin(t * 12.0f) + 1.0f) * 0.5f;
        Color speakCol = ui::Colors::Green500;
        DrawCircleLines(static_cast<int>(pos.x), static_cast<int>(pos.y), 15.0f * scale + pulse * 6.0f, Fade(speakCol, 0.85f));
        DrawCircleLines(static_cast<int>(pos.x), static_cast<int>(pos.y), 22.0f * scale + pulse * 9.0f, Fade(speakCol, 0.45f));
    }

    if (name && name[0] != '\0') {
        int nameW = MeasureText(name, 12);
        Rectangle badge = { pos.x - static_cast<float>(nameW + (isSpeaking ? 20 : 8)) * 0.5f, pos.y + 16.0f * scale, static_cast<float>(nameW + (isSpeaking ? 20 : 8)), 16.0f };
        DrawRectangleRec(badge, Fade(BLACK, 0.85f));
        DrawRectangleLinesEx(badge, 1.0f, isSpeaking ? ui::Colors::Green500 : col);
        if (isSpeaking) {
            DrawCircle(static_cast<int>(badge.x + 6), static_cast<int>(badge.y + 8), 3.0f, ui::Colors::Green500);
            DrawText(name, static_cast<int>(badge.x + 14), static_cast<int>(badge.y + 2), 12, WHITE);
        } else {
            DrawText(name, static_cast<int>(badge.x + 4), static_cast<int>(badge.y + 2), 12, WHITE);
        }
    }
}

void RaylibRenderer::updateShip(Vector2 targetPos, float dt) {
    if (!localShipInit) {
        localShipPos = { targetPos.x - 50.0f, targetPos.y };
        localShipVel = { 0.0f, 0.0f };
        localShipAngle = 0.0f;
        localShipInit = true;
        shipExhaust.clear();
    }

    // 1. Calculate direction to cursor
    Vector2 toMouse = { targetPos.x - localShipPos.x, targetPos.y - localShipPos.y };
    float distToMouse = std::sqrt(toMouse.x * toMouse.x + toMouse.y * toMouse.y);

    const float targetDist = 50.0f;  // Keep 50px distance requested by user
    const float maxSpeed = 600.0f;   // 6x faster (at least 5 times faster requested by user)
    const float slowRadius = 140.0f; // Deceleration braking radius to prevent abrupt stopping
    const float maxAccel = 2200.0f;  // Responsive acceleration force

    // Desired resting position maintains 50px distance from cursor
    Vector2 idealPos;
    if (distToMouse > 0.001f) {
        Vector2 dirFromMouse = { -toMouse.x / distToMouse, -toMouse.y / distToMouse };
        idealPos = { targetPos.x + dirFromMouse.x * targetDist, targetPos.y + dirFromMouse.y * targetDist };
    } else {
        idealPos = { targetPos.x - targetDist, targetPos.y };
    }

    Vector2 toIdeal = { idealPos.x - localShipPos.x, idealPos.y - localShipPos.y };
    float distToIdeal = std::sqrt(toIdeal.x * toIdeal.x + toIdeal.y * toIdeal.y);

    // Smooth arrival deceleration curve
    float desiredSpeed = 0.0f;
    if (distToIdeal > slowRadius) {
        desiredSpeed = maxSpeed;
    } else if (distToIdeal > 0.5f) {
        float t = distToIdeal / slowRadius;
        desiredSpeed = maxSpeed * (t * (2.0f - t)); // Smooth quadratic ease-out brake
    }

    Vector2 desiredVel = { 0.0f, 0.0f };
    if (distToIdeal > 0.001f && desiredSpeed > 0.0f) {
        desiredVel = { (toIdeal.x / distToIdeal) * desiredSpeed, (toIdeal.y / distToIdeal) * desiredSpeed };
    }

    // Acceleration-driven physics
    Vector2 accel = { (desiredVel.x - localShipVel.x) * 10.0f, (desiredVel.y - localShipVel.y) * 10.0f };
    float accelMag = std::sqrt(accel.x * accel.x + accel.y * accel.y);
    if (accelMag > maxAccel) {
        accel.x = (accel.x / accelMag) * maxAccel;
        accel.y = (accel.y / accelMag) * maxAccel;
    }

    localShipVel.x += accel.x * dt;
    localShipVel.y += accel.y * dt;

    // Settle smoothly when very close to rest position
    float curSpeed = std::sqrt(localShipVel.x * localShipVel.x + localShipVel.y * localShipVel.y);
    if (distToIdeal < 1.0f && curSpeed < 8.0f) {
        localShipVel = { 0.0f, 0.0f };
        localShipPos = idealPos;
    } else {
        localShipPos.x += localShipVel.x * dt;
        localShipPos.y += localShipVel.y * dt;
    }

    // 2. Rotate so eyes (two yellow lines at top of sprite) face the cursor
    if (distToMouse > 0.001f) {
        float dirX = toMouse.x / distToMouse;
        float dirY = toMouse.y / distToMouse;
        float desiredAngle = std::atan2(dirY, dirX) * RAD2DEG + 90.0f;
        float diffAngle = desiredAngle - localShipAngle;
        while (diffAngle < -180.0f) diffAngle += 360.0f;
        while (diffAngle > 180.0f) diffAngle -= 360.0f;
        localShipAngle += diffAngle * std::min(1.0f, 18.0f * dt);
    }

    // 3. Update & spawn trailing exhaust particles beneath the ship
    float theta = localShipAngle * DEG2RAD;
    float cosA = std::cos(theta);
    float sinA = std::sin(theta);

    // Center particles and thrusters at (4, 12) and (11, 12) relative to top-left of 16x16 sprite
    // Relative to texture center (8.0, 8.0): pixel 4 center is -3.5, pixel 11 center is +3.5, row 12 center is +4.5
    float scale = 1.6f;
    float lxLeft = -3.5f * scale;
    float lxRight = 3.5f * scale;
    float ly = 4.5f * scale;

    Vector2 leftThrust = {
        localShipPos.x + (lxLeft * cosA - ly * sinA),
        localShipPos.y + (lxLeft * sinA + ly * cosA)
    };
    Vector2 rightThrust = {
        localShipPos.x + (lxRight * cosA - ly * sinA),
        localShipPos.y + (lxRight * sinA + ly * cosA)
    };
    Vector2 rear = { -sinA, cosA };
    Vector2 right = { cosA, sinA };

    if (curSpeed > 25.0f) {
        static float emitAccum = 0.0f;
        emitAccum += dt * (curSpeed / maxSpeed);
        while (emitAccum >= 0.02f) {
            emitAccum -= 0.02f;
            if (shipExhaust.size() < 128) {
                float pSpeed = 40.0f + static_cast<float>(rand() % 40);
                float spread = ((rand() % 100) - 50) * 0.004f;
                Vector2 pVel = { (rear.x + right.x * spread) * pSpeed, (rear.y + right.y * spread) * pSpeed };
                Color c1 = (rand() % 2 == 0) ? ui::Colors::Amber400 : ui::Colors::Orange500;
                Color c2 = (rand() % 2 == 0) ? ui::Colors::Yellow400 : ui::Colors::Amber400;
                shipExhaust.push_back({ leftThrust, pVel, 0.28f, 0.28f, 2.4f, c1 });
                shipExhaust.push_back({ rightThrust, pVel, 0.28f, 0.28f, 2.4f, c2 });
            }
        }
    }

    for (size_t i = 0; i < shipExhaust.size(); ) {
        shipExhaust[i].life -= dt;
        if (shipExhaust[i].life <= 0.0f) {
            shipExhaust[i] = shipExhaust.back();
            shipExhaust.pop_back();
        } else {
            shipExhaust[i].pos.x += shipExhaust[i].vel.x * dt;
            shipExhaust[i].pos.y += shipExhaust[i].vel.y * dt;
            shipExhaust[i].vel.x *= 0.94f;
            shipExhaust[i].vel.y *= 0.94f;
            ++i;
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

    particles.updateAndDraw(GetFrameTime());

    // Draw remote multiplayer cursors with custom skins and speaking effects
    for (const auto& [id, cursor] : net.remoteCursors) {
        Color curCol = ui::Colors::Red500;
        if (id != 0) {
            curCol = ColorFromHSV(std::fmod(id * 137.5f, 360.0f), 0.8f, 1.0f);
        }
        const char* tag = cursor.name[0] != '\0' ? cursor.name : (id == 0 ? "HOST" : TextFormat("P%u", id));
        drawCursorSkin(cursor.skin, { cursor.x, cursor.y }, cursor.angle, curCol, tag, 1.8f, cursor.isSpeaking, cursor.isMoving);
    }

    // Draw trailing ship exhaust particles beneath the ship
    for (const auto& p : shipExhaust) {
        float alpha = p.life / p.maxLife;
        Color c = p.color;
        c.a = static_cast<unsigned char>(alpha * 200.0f);
        float sz = p.size * (0.4f + 0.6f * alpha);
        DrawRectanglePro({ p.pos.x, p.pos.y, sz, sz }, { sz * 0.5f, sz * 0.5f }, 45.0f, c);
    }

    // Draw local player ship following cursor
    Vector2 worldMouse = camera.getScreenToWorld(camera.getCRTMousePosition());
    float currentSpeed = std::sqrt(localShipVel.x * localShipVel.x + localShipVel.y * localShipVel.y);
    bool isMoving = (currentSpeed > 20.0f);
    drawCursorSkin(static_cast<uint8_t>(activeCursorSkin), localShipPos, localShipAngle, ui::Colors::Green500, nullptr, 1.8f, isLocalSpeaking, isMoving);

    // Subtle tactical aim crosshair at the cursor
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
