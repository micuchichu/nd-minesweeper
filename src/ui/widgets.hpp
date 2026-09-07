#pragma once

#include "raylib.h"
#include "theme.hpp"
#include <string>

namespace minesweeper::ui {

class Widgets {
public:
    static float guiScale;
    static bool enableCRT;
    static void setScale(float scale);
    static void setCRT(bool enabled);

    static void beginScissor(float x, float y, float width, float height);
    static void endScissor();

    static Vector2 getUIMousePos();
    static bool button(const char* label, Rectangle rect, Color baseCol = Colors::Zinc800, Color hoverCol = Colors::Green500, bool locked = false, int fontSize = 20);
    static bool checkbox(const char* label, Vector2 pos, bool& checked, bool locked = false);
    static bool spinner(const char* label, Vector2 pos, int& value, int minVal, int maxVal, bool locked = false, int labelWidth = 80);
    static bool slider(const char* label, Rectangle rect, float& value, float minVal, float maxVal, int labelWidth = 140, const char* format = "%.0f%%", bool asPercent = true, const char* customDisplay = nullptr);
    static bool segmented(const char* label, Rectangle rect, int& selectedIdx, const char* const* options, int optionCount, int labelWidth = 140);
    static bool textInput(Rectangle rect, char* buffer, size_t maxLen, bool& isActive, const char* placeholder = "", bool leftAlign = false, int fontSize = 20);
    static int modal(int screenW, int screenH, const char* title, const char* message, const char* btnConfirm, const char* btnCancel = nullptr, bool allowClose = true, bool confirmLocked = false);
    static bool mindustryButton(const char* label, const char* sublabel, Rectangle rect, Color accentCol = Colors::Amber500, bool locked = false, int fontSize = 20);
    static void mindustryPanel(Rectangle rect, const char* headerTitle = nullptr, Color accentCol = Colors::Amber500);
};

} // namespace minesweeper::ui
