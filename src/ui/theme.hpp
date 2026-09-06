#pragma once

#include "raylib.h"
#include "raymath.h"
#include <algorithm>

namespace minesweeper::ui {

namespace Colors {
    constexpr Color Zinc950 = { 12, 12, 14, 255 };
    constexpr Color Zinc900 = { 24, 24, 27, 255 };
    constexpr Color Zinc900Translucent = { 24, 24, 27, 240 };
    constexpr Color Zinc850 = { 31, 31, 35, 255 };
    constexpr Color Zinc800 = { 39, 39, 42, 255 };
    constexpr Color Zinc700 = { 63, 63, 70, 255 };
    constexpr Color Zinc600 = { 82, 82, 91, 255 };
    constexpr Color Zinc500 = { 113, 113, 122, 255 };
    constexpr Color Zinc400 = { 161, 161, 170, 255 };
    constexpr Color Zinc300 = { 212, 212, 216, 255 };
    constexpr Color Zinc200 = { 228, 228, 231, 255 };

    constexpr Color Red500 = { 239, 68, 68, 255 };
    constexpr Color Red600 = { 220, 38, 38, 255 };
    constexpr Color Red700 = { 127, 29, 29, 255 };

    constexpr Color Green400 = { 74, 222, 128, 255 };
    constexpr Color Green500 = { 34, 197, 94, 255 };
    constexpr Color Green600 = { 22, 163, 74, 255 };
    constexpr Color Green700 = { 21, 128, 61, 255 };

    constexpr Color Blue500 = { 59, 130, 246, 255 };

    constexpr Color Amber300 = { 252, 211, 77, 255 };
    constexpr Color Amber400 = { 251, 191, 36, 255 };
    constexpr Color Amber500 = { 245, 158, 11, 255 };
    constexpr Color Amber600 = { 217, 119, 6, 255 };

    constexpr Color Orange500 = { 249, 115, 22, 255 };
    constexpr Color Yellow400 = { 250, 204, 21, 255 };

    constexpr Color Cyan400 = { 56, 189, 248, 255 };
    constexpr Color Cyan500 = { 6, 182, 212, 255 };

    constexpr Color PanelBg = { 16, 16, 20, 248 };
    constexpr Color PanelBorder = { 48, 48, 58, 255 };
    constexpr Color MetalDark = { 22, 22, 26, 255 };
    constexpr Color MetalLight = { 34, 34, 40, 255 };

    constexpr Color CellHidden = Zinc600;
    constexpr Color CellRevealed = Zinc900;
    constexpr Color CellFlag = Red500;
    constexpr Color BgSlice = Zinc800;
}

inline Color getNeighborColor(int count) {
    if (count <= 0) return BLANK;
    if (count == 1) return Colors::Blue500;
    if (count == 2) return Colors::Green500;
    if (count == 3) return Colors::Red500;
    if (count == 4) return Color{ 147, 51, 234, 255 };  // Purple
    if (count == 5) return Color{ 234, 88, 12, 255 };   // Orange
    if (count == 6) return Color{ 13, 148, 136, 255 };  // Teal
    if (count == 7) return Colors::Zinc200;
    if (count == 8) return Color{ 244, 63, 94, 255 };   // Rose

    float t = std::clamp(static_cast<float>(count - 1) / 79.0f, 0.0f, 1.0f);
    float hue = 360.0f * (1.0f - t);
    float saturation = 1.0f - (t * 0.4f);
    return ColorFromHSV(hue, saturation, 1.0f);
}

} // namespace minesweeper::ui
