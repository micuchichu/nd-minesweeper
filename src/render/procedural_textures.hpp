#pragma once

#include "raylib.h"

namespace minesweeper::render {

class ProceduralTextures {
public:
    static ProceduralTextures& instance();

    void init(float cellSize = 30.0f, float cellMargin = 2.0f);
    void updateCellSize(float cellSize, float cellMargin);
    void cleanup();

    // 1. Soft Radial Shadow (Flags, Pickups, Scrap)
    Texture2D shadowTexture = { 0 };

    // 2. Soft Glowing Particle Ember (Exhaust, Debris, Sparks)
    Texture2D particleTexture = { 0 };

    // 3. Radial Glow Flare (Thruster idle nozzles, Halos, Laser flash/impact)
    Texture2D glowTexture = { 0 };

    // 4. Plasma Laser Beam Capsule
    Texture2D laserBeamTexture = { 0 };

    // 5. Tactical Starting Cell Reticle (Backdrop + Border + 4 Corner Brackets)
    RenderTexture2D safeStartReticleRT = { 0 };

    // 6. Mindustry 9-Patch Panel
    Texture2D panelNPatchTexture = { 0 };
    NPatchInfo panelNPatchInfo = { { 0.0f, 0.0f, 48.0f, 48.0f }, 12, 12, 12, 12, NPATCH_NINE_PATCH };

    // 7. Mindustry 9-Patch Buttons
    Texture2D buttonNPatchNormal = { 0 };
    Texture2D buttonNPatchHover = { 0 };
    Texture2D buttonNPatchLocked = { 0 };
    NPatchInfo buttonNPatchInfo = { { 0.0f, 0.0f, 48.0f, 36.0f }, 8, 8, 8, 8, NPATCH_NINE_PATCH };

    bool isInitialized() const { return initialized; }

private:
    ProceduralTextures() = default;
    ~ProceduralTextures() { cleanup(); }
    ProceduralTextures(const ProceduralTextures&) = delete;
    ProceduralTextures& operator=(const ProceduralTextures&) = delete;

    bool initialized = false;
    float currentCellSize = 30.0f;
    float currentCellMargin = 2.0f;

    void generateShadowTexture();
    void generateParticleTexture();
    void generateGlowTexture();
    void generateLaserBeamTexture();
    void generateSafeStartReticle(float cellSize, float cellMargin);
    void generateUITextures();
};

} // namespace minesweeper::render
