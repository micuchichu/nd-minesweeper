#include "procedural_textures.hpp"
#include "ui/theme.hpp"
#include <cmath>
#include <algorithm>
#include <vector>

namespace minesweeper::render {

ProceduralTextures& ProceduralTextures::instance() {
    static ProceduralTextures inst;
    return inst;
}

void ProceduralTextures::init(float cellSize, float cellMargin) {
    if (initialized) return;

    currentCellSize = cellSize;
    currentCellMargin = cellMargin;

    generateShadowTexture();
    generateParticleTexture();
    generateGlowTexture();
    generateLaserBeamTexture();
    generateSafeStartReticle(cellSize, cellMargin);
    generateUITextures();

    initialized = true;
}

void ProceduralTextures::updateCellSize(float cellSize, float cellMargin) {
    if (std::abs(currentCellSize - cellSize) < 0.01f && std::abs(currentCellMargin - cellMargin) < 0.01f) {
        return;
    }
    currentCellSize = cellSize;
    currentCellMargin = cellMargin;
    generateSafeStartReticle(cellSize, cellMargin);
}

void ProceduralTextures::cleanup() {
    if (shadowTexture.id != 0) { UnloadTexture(shadowTexture); shadowTexture = { 0 }; }
    if (particleTexture.id != 0) { UnloadTexture(particleTexture); particleTexture = { 0 }; }
    if (glowTexture.id != 0) { UnloadTexture(glowTexture); glowTexture = { 0 }; }
    if (laserBeamTexture.id != 0) { UnloadTexture(laserBeamTexture); laserBeamTexture = { 0 }; }
    if (safeStartReticleRT.id != 0) { UnloadRenderTexture(safeStartReticleRT); safeStartReticleRT = { 0 }; }
    if (panelNPatchTexture.id != 0) { UnloadTexture(panelNPatchTexture); panelNPatchTexture = { 0 }; }
    if (buttonNPatchNormal.id != 0) { UnloadTexture(buttonNPatchNormal); buttonNPatchNormal = { 0 }; }
    if (buttonNPatchHover.id != 0) { UnloadTexture(buttonNPatchHover); buttonNPatchHover = { 0 }; }
    if (buttonNPatchLocked.id != 0) { UnloadTexture(buttonNPatchLocked); buttonNPatchLocked = { 0 }; }
    initialized = false;
}

void ProceduralTextures::generateShadowTexture() {
    const int w = 64;
    const int h = 32;
    Image img = GenImageColor(w, h, BLANK);
    Color* pixels = static_cast<Color*>(img.data);

    float rx = 31.0f;
    float ry = 15.0f;
    float cx = 31.5f;
    float cy = 15.5f;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float nx = (static_cast<float>(x) - cx) / rx;
            float ny = (static_cast<float>(y) - cy) / ry;
            float d = std::sqrt(nx * nx + ny * ny);
            if (d < 1.0f) {
                float alpha = std::cos(d * 3.14159265f * 0.5f);
                alpha = alpha * alpha; // Smooth cosine falloff
                pixels[y * w + x] = Color{ 255, 255, 255, static_cast<unsigned char>(alpha * 255.0f) };
            } else {
                pixels[y * w + x] = BLANK;
            }
        }
    }

    shadowTexture = LoadTextureFromImage(img);
    SetTextureFilter(shadowTexture, TEXTURE_FILTER_BILINEAR);
    UnloadImage(img);
}

void ProceduralTextures::generateParticleTexture() {
    const int w = 32;
    const int h = 32;
    Image img = GenImageColor(w, h, BLANK);
    Color* pixels = static_cast<Color*>(img.data);

    float cx = 15.5f;
    float cy = 15.5f;
    float r = 15.0f;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float dx = static_cast<float>(x) - cx;
            float dy = static_cast<float>(y) - cy;
            float distCirc = std::sqrt(dx * dx + dy * dy) / r;
            float distDiamond = (std::abs(dx) + std::abs(dy)) / r;
            float d = 0.45f * distCirc + 0.55f * distDiamond;
            if (d < 1.0f) {
                float alpha = std::clamp(1.0f - d, 0.0f, 1.0f);
                alpha = alpha * alpha;
                pixels[y * w + x] = Color{ 255, 255, 255, static_cast<unsigned char>(alpha * 255.0f) };
            } else {
                pixels[y * w + x] = BLANK;
            }
        }
    }

    particleTexture = LoadTextureFromImage(img);
    SetTextureFilter(particleTexture, TEXTURE_FILTER_BILINEAR);
    UnloadImage(img);
}

void ProceduralTextures::generateGlowTexture() {
    const int w = 64;
    const int h = 64;
    Image img = GenImageColor(w, h, BLANK);
    Color* pixels = static_cast<Color*>(img.data);

    float cx = 31.5f;
    float cy = 31.5f;
    float r = 31.0f;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float dx = static_cast<float>(x) - cx;
            float dy = static_cast<float>(y) - cy;
            float d = std::sqrt(dx * dx + dy * dy) / r;
            if (d < 1.0f) {
                float alpha = std::pow(1.0f - d, 1.85f);
                pixels[y * w + x] = Color{ 255, 255, 255, static_cast<unsigned char>(alpha * 255.0f) };
            } else {
                pixels[y * w + x] = BLANK;
            }
        }
    }

    glowTexture = LoadTextureFromImage(img);
    SetTextureFilter(glowTexture, TEXTURE_FILTER_BILINEAR);
    UnloadImage(img);
}

void ProceduralTextures::generateLaserBeamTexture() {
    const int w = 64;
    const int h = 16;
    Image img = GenImageColor(w, h, BLANK);
    Color* pixels = static_cast<Color*>(img.data);

    float cy = 7.5f;

    for (int y = 0; y < h; ++y) {
        float distY = std::abs(static_cast<float>(y) - cy) / cy;
        float glow = std::clamp(1.0f - distY, 0.0f, 1.0f);
        float core = std::pow(glow, 3.5f);

        for (int x = 0; x < w; ++x) {
            float endTaper = 1.0f;
            if (x < 5) endTaper = static_cast<float>(x + 1) / 6.0f;
            else if (x > 58) endTaper = static_cast<float>(63 - x + 1) / 6.0f;

            float a = (0.5f * glow + 0.5f * core) * endTaper;
            pixels[y * w + x] = Color{ 255, 255, 255, static_cast<unsigned char>(a * 255.0f) };
        }
    }

    laserBeamTexture = LoadTextureFromImage(img);
    SetTextureFilter(laserBeamTexture, TEXTURE_FILTER_BILINEAR);
    UnloadImage(img);
}

void ProceduralTextures::generateSafeStartReticle(float cellSize, float cellMargin) {
    float innerSize = cellSize - (cellMargin * 2.0f);
    int texSize = static_cast<int>(std::ceil(innerSize));
    if (texSize < 1) texSize = 1;

    if (safeStartReticleRT.id != 0) {
        UnloadRenderTexture(safeStartReticleRT);
        safeStartReticleRT = { 0 };
    }

    safeStartReticleRT = LoadRenderTexture(texSize, texSize);
    BeginTextureMode(safeStartReticleRT);
    ClearBackground(BLANK);

    float s = static_cast<float>(texSize);

    // 1. Rounded backdrop (subtle white opacity for tinting)
    DrawRectangleRounded({ 0.0f, 0.0f, s, s }, 0.2f, 4, Fade(WHITE, 0.45f));

    // 2. Rounded border
    DrawRectangleLinesEx({ 0.0f, 0.0f, s, s }, 2.0f, WHITE);

    // 3. Tactical corner brackets (Mindustry targeting reticle)
    float bracketLen = std::clamp(s * 0.3f, 4.0f, 9.0f);
    // Top-Left
    DrawLineEx({ 0.0f, 0.0f }, { bracketLen, 0.0f }, 2.0f, WHITE);
    DrawLineEx({ 0.0f, 0.0f }, { 0.0f, bracketLen }, 2.0f, WHITE);
    // Top-Right
    DrawLineEx({ s, 0.0f }, { s - bracketLen, 0.0f }, 2.0f, WHITE);
    DrawLineEx({ s, 0.0f }, { s, bracketLen }, 2.0f, WHITE);
    // Bottom-Left
    DrawLineEx({ 0.0f, s }, { bracketLen, s }, 2.0f, WHITE);
    DrawLineEx({ 0.0f, s }, { 0.0f, s - bracketLen }, 2.0f, WHITE);
    // Bottom-Right
    DrawLineEx({ s, s }, { s - bracketLen, s }, 2.0f, WHITE);
    DrawLineEx({ s, s }, { s, s - bracketLen }, 2.0f, WHITE);

    EndTextureMode();
}

void ProceduralTextures::generateUITextures() {
    // 1. Mindustry 9-Patch Panel (48x48)
    {
        RenderTexture2D rt = LoadRenderTexture(48, 48);
        BeginTextureMode(rt);
        ClearBackground(BLANK);

        DrawRectangleRec({ 0.0f, 0.0f, 48.0f, 48.0f }, ui::Colors::PanelBg);
        DrawRectangleLinesEx({ 0.0f, 0.0f, 48.0f, 48.0f }, 1.5f, ui::Colors::PanelBorder);

        float markLen = 8.0f;
        Color markCol = ui::Colors::PanelBorder;
        DrawLineEx({ 0.0f, 0.0f }, { markLen, 0.0f }, 2.0f, markCol);
        DrawLineEx({ 0.0f, 0.0f }, { 0.0f, markLen }, 2.0f, markCol);
        DrawLineEx({ 48.0f, 0.0f }, { 48.0f - markLen, 0.0f }, 2.0f, markCol);
        DrawLineEx({ 48.0f, 0.0f }, { 48.0f, markLen }, 2.0f, markCol);
        DrawLineEx({ 0.0f, 48.0f }, { 0.0f, 48.0f - markLen }, 2.0f, markCol);
        DrawLineEx({ 0.0f, 48.0f }, { markLen, 48.0f }, 2.0f, markCol);
        DrawLineEx({ 48.0f, 48.0f }, { 48.0f - markLen, 48.0f }, 2.0f, markCol);
        DrawLineEx({ 48.0f, 48.0f }, { 48.0f, 48.0f - markLen }, 2.0f, markCol);

        EndTextureMode();

        Image img = LoadImageFromTexture(rt.texture);
        ImageFlipVertical(&img);
        panelNPatchTexture = LoadTextureFromImage(img);
        UnloadImage(img);
        UnloadRenderTexture(rt);
        panelNPatchInfo = { { 0.0f, 0.0f, 48.0f, 48.0f }, 12, 12, 12, 12, NPATCH_NINE_PATCH };
    }

    // 2. Mindustry Buttons (48x36)
    // Normal Button
    {
        RenderTexture2D rt = LoadRenderTexture(48, 36);
        BeginTextureMode(rt);
        ClearBackground(BLANK);

        DrawRectangleRec({ 0.0f, 0.0f, 48.0f, 36.0f }, ui::Colors::MetalDark);
        DrawRectangleLinesEx({ 0.0f, 0.0f, 48.0f, 36.0f }, 1.5f, ui::Colors::PanelBorder);

        EndTextureMode();

        Image img = LoadImageFromTexture(rt.texture);
        ImageFlipVertical(&img);
        buttonNPatchNormal = LoadTextureFromImage(img);
        UnloadImage(img);
        UnloadRenderTexture(rt);
    }

    // Hover Button (with left indicator stripe & 4 corner accents)
    {
        RenderTexture2D rt = LoadRenderTexture(48, 36);
        BeginTextureMode(rt);
        ClearBackground(BLANK);

        DrawRectangleRec({ 0.0f, 0.0f, 48.0f, 36.0f }, ui::Colors::MetalLight);
        DrawRectangleLinesEx({ 0.0f, 0.0f, 48.0f, 36.0f }, 1.5f, WHITE); // Tinted with accentCol at draw time

        // Left indicator stripe
        DrawRectangle(0, 0, 4, 36, WHITE);

        // Corner accents
        DrawRectangle(48 - 6, 0, 6, 2, WHITE);
        DrawRectangle(48 - 2, 0, 2, 6, WHITE);
        DrawRectangle(48 - 6, 36 - 2, 6, 2, WHITE);
        DrawRectangle(48 - 2, 36 - 6, 2, 6, WHITE);

        EndTextureMode();

        Image img = LoadImageFromTexture(rt.texture);
        ImageFlipVertical(&img);
        buttonNPatchHover = LoadTextureFromImage(img);
        UnloadImage(img);
        UnloadRenderTexture(rt);
    }

    // Locked Button
    {
        RenderTexture2D rt = LoadRenderTexture(48, 36);
        BeginTextureMode(rt);
        ClearBackground(BLANK);

        DrawRectangleRec({ 0.0f, 0.0f, 48.0f, 36.0f }, ui::Colors::Zinc900);
        DrawRectangleLinesEx({ 0.0f, 0.0f, 48.0f, 36.0f }, 1.5f, ui::Colors::Zinc800);

        EndTextureMode();

        Image img = LoadImageFromTexture(rt.texture);
        ImageFlipVertical(&img);
        buttonNPatchLocked = LoadTextureFromImage(img);
        UnloadImage(img);
        UnloadRenderTexture(rt);
    }

    buttonNPatchInfo = { { 0.0f, 0.0f, 48.0f, 36.0f }, 8, 8, 8, 8, NPATCH_NINE_PATCH };
}

} // namespace minesweeper::render
