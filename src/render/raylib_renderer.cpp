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
    if (mouseCRT.y < 80.0f || mouseCRT.y > static_cast<float>(GetScreenHeight()) - 95.0f) {
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

                if (isNeighbor) {
                    if (isHovered) {
                        DrawRectangleRec(cellRect, Fade(WHITE, 0.25f));
                        DrawRectangleLinesEx(cellRect, 2.0f, WHITE);
                    } else {
                        DrawRectangleRounded(cellRect, 0.2f, 4, Fade(WHITE, 0.08f));
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

                DrawRectangle(static_cast<int>(posX + 1), static_cast<int>(posY + 1), static_cast<int>(cellSize - 2), static_cast<int>(cellSize - 2), lodCol);

                if (isHovered) {
                    Rectangle lodRect = { posX + 1.0f, posY + 1.0f, cellSize - 2.0f, cellSize - 2.0f };
                    DrawRectangleLinesEx(lodRect, 1.5f, WHITE);
                } else if (isNeighbor) {
                    DrawRectangle(static_cast<int>(posX + 1), static_cast<int>(posY + 1), static_cast<int>(cellSize - 2), static_cast<int>(cellSize - 2), Fade(WHITE, 0.08f));
                }
            }
        }
    }
}

void RaylibRenderer::drawCursorSkin(uint8_t skin, Vector2 pos, Color col, const char* name, float scale) {
    int skinIdx = static_cast<int>(skin);
    bool drewTexture = false;

    if (skinIdx >= 0 && skinIdx < static_cast<int>(cursorSkins.size()) && cursorSkins[skinIdx].texture.id != 0) {
        const auto& tex = cursorSkins[skinIdx].texture;
        Rectangle src = { 0.0f, 0.0f, static_cast<float>(tex.width), static_cast<float>(tex.height) };
        Rectangle dest = { pos.x, pos.y, static_cast<float>(tex.width) * scale, static_cast<float>(tex.height) * scale };
        DrawTexturePro(tex, src, dest, { 0.0f, 0.0f }, 0.0f, WHITE);
        drewTexture = true;
    }

    if (!drewTexture) {
        int px = static_cast<int>(pos.x);
        int py = static_cast<int>(pos.y);
        DrawRectangle(px, py, 10, 10, col);
        DrawRectangleLines(px, py, 10, 10, WHITE);
    }

    if (name && name[0] != '\0') {
        int nameW = MeasureText(name, 12);
        Rectangle badge = { pos.x + 12.0f * scale, pos.y + 12.0f * scale, static_cast<float>(nameW + 8), 16.0f };
        DrawRectangleRec(badge, Fade(BLACK, 0.85f));
        DrawRectangleLinesEx(badge, 1.0f, col);
        DrawText(name, static_cast<int>(badge.x + 4), static_cast<int>(badge.y + 2), 12, WHITE);
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

    // Draw remote multiplayer cursors with custom skins
    for (const auto& [id, cursor] : net.remoteCursors) {
        Color curCol = ui::Colors::Red500;
        if (id != 0) {
            curCol = ColorFromHSV(std::fmod(id * 137.5f, 360.0f), 0.8f, 1.0f);
        }
        const char* tag = cursor.name[0] != '\0' ? cursor.name : (id == 0 ? "HOST" : TextFormat("P%u", id));
        drawCursorSkin(cursor.skin, { cursor.x, cursor.y }, curCol, tag);
    }

    EndMode2D();
}

} // namespace minesweeper::render
