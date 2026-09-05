#pragma once

#include <algorithm>

namespace minesweeper::ui {

enum class PreviewLayoutMode : int {
    CameraPerimeter = 0, // 8 perimeter slots matching spatial camera direction
    SideColumns = 1       // Linear stacked columns on left and right edges
};

struct UILayoutConfig {
    float globalScale = 1.0f;       // 0.70f to 1.50f
    float hudScale = 1.0f;          // 0.70f to 1.40f
    float previewScale = 1.0f;      // 0.60f to 1.50f
    
    // Position offsets & margins
    float topBarY = 0.0f;           // Vertical offset of top bar (default 0)
    float bottomBarOffsetY = 0.0f;  // Vertical offset of bottom bar (default 0)
    float previewMarginX = 14.0f;   // Horizontal margin from screen edges
    float previewMarginY = 74.0f;   // Top margin (below top bar)
    
    // Docking & Arrangement
    int topBarDock = 0;             // 0 = Top, 1 = Bottom (swapped)
    int bottomBarDock = 0;          // 0 = Bottom, 1 = Top (swapped)
    PreviewLayoutMode previewMode = PreviewLayoutMode::CameraPerimeter;

    void resetDefaults() {
        globalScale = 1.0f;
        hudScale = 1.0f;
        previewScale = 1.0f;
        topBarY = 0.0f;
        bottomBarOffsetY = 0.0f;
        previewMarginX = 14.0f;
        previewMarginY = 74.0f;
        topBarDock = 0;
        bottomBarDock = 0;
        previewMode = PreviewLayoutMode::CameraPerimeter;
    }

    void applyPresetCompact() {
        globalScale = 0.85f;
        hudScale = 0.85f;
        previewScale = 0.80f;
        topBarY = 0.0f;
        bottomBarOffsetY = 0.0f;
        previewMarginX = 8.0f;
        previewMarginY = 64.0f;
        topBarDock = 0;
        bottomBarDock = 0;
        previewMode = PreviewLayoutMode::CameraPerimeter;
    }

    void applyPresetLarge() {
        globalScale = 1.25f;
        hudScale = 1.20f;
        previewScale = 1.20f;
        topBarY = 0.0f;
        bottomBarOffsetY = 0.0f;
        previewMarginX = 20.0f;
        previewMarginY = 88.0f;
        topBarDock = 0;
        bottomBarDock = 0;
        previewMode = PreviewLayoutMode::CameraPerimeter;
    }

    void clampValues() {
        globalScale = std::clamp(globalScale, 0.70f, 1.50f);
        hudScale = std::clamp(hudScale, 0.70f, 1.40f);
        previewScale = std::clamp(previewScale, 0.60f, 1.50f);
        topBarY = std::clamp(topBarY, 0.0f, 300.0f);
        bottomBarOffsetY = std::clamp(bottomBarOffsetY, -300.0f, 100.0f);
        previewMarginX = std::clamp(previewMarginX, 2.0f, 120.0f);
        previewMarginY = std::clamp(previewMarginY, 40.0f, 200.0f);
        if (static_cast<int>(previewMode) < 0 || static_cast<int>(previewMode) > 1) {
            previewMode = PreviewLayoutMode::CameraPerimeter;
        }
    }
};

} // namespace minesweeper::ui
