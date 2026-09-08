#include "widgets.hpp"
#include "render/procedural_textures.hpp"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <algorithm>

namespace minesweeper::ui {

float Widgets::guiScale = 1.0f;
bool Widgets::enableCRT = true;

void Widgets::setScale(float scale) {
    guiScale = (scale > 0.01f) ? scale : 1.0f;
}

void Widgets::setCRT(bool enabled) {
    enableCRT = enabled;
}

void Widgets::beginScissor(float x, float y, float width, float height) {
    int sx = static_cast<int>(std::round(x * guiScale));
    int sy = static_cast<int>(std::round(y * guiScale));
    int sw = static_cast<int>(std::round(width * guiScale));
    int sh = static_cast<int>(std::round(height * guiScale));
    BeginScissorMode(sx, sy, sw, sh);
}

void Widgets::endScissor() {
    EndScissorMode();
}

Vector2 Widgets::getUIMousePos() {
    Vector2 mouse = GetMousePosition();
    if (!enableCRT) {
        return { mouse.x / guiScale, mouse.y / guiScale };
    }

    float screenW = static_cast<float>(GetScreenWidth());
    float screenH = static_cast<float>(GetScreenHeight());
    if (screenW <= 0.0f || screenH <= 0.0f) return { mouse.x / guiScale, mouse.y / guiScale };

    Vector2 centered = { (mouse.x / screenW) - 0.5f, (mouse.y / screenH) - 0.5f };
    float r2 = (centered.x * centered.x) + (centered.y * centered.y);

    Vector2 crtMouse = {
        (mouse.x / screenW + centered.x * (r2 * 0.05f)) * screenW,
        (mouse.y / screenH + centered.y * (r2 * 0.05f)) * screenH
    };

    return { crtMouse.x / guiScale, crtMouse.y / guiScale };
}

bool Widgets::button(const char* label, Rectangle rect, Color baseCol, Color hoverCol, bool locked, int fontSize) {
    Vector2 mouse = getUIMousePos();
    bool hover = !locked && CheckCollisionPointRec(mouse, rect);

    Color bg = locked ? Colors::Zinc800 : (hover ? hoverCol : baseCol);
    Color fg = locked ? Colors::Zinc600 : WHITE;

    DrawRectangleRec(rect, bg);
    DrawRectangleLinesEx(rect, 1.5f, hover ? hoverCol : (locked ? Colors::Zinc700 : Colors::Zinc600));

    int textW = MeasureText(label, fontSize);
    DrawText(label, static_cast<int>(rect.x + (rect.width - textW) * 0.5f), static_cast<int>(rect.y + (rect.height - fontSize) * 0.5f), fontSize, fg);

    return (hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON));
}

bool Widgets::checkbox(const char* label, Vector2 pos, bool& checked, bool locked) {
    float boxSize = 20.0f;
    Rectangle rect = { pos.x, pos.y, boxSize, boxSize };
    Vector2 mouse = getUIMousePos();
    bool hover = !locked && CheckCollisionPointRec(mouse, rect);

    if (hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        checked = !checked;
    }

    Color bg = locked ? Colors::Zinc800 : (hover ? Colors::Zinc700 : BLANK);
    Color border = locked ? Colors::Zinc600 : (checked ? Colors::Green500 : Colors::Zinc600);

    DrawRectangleRec(rect, bg);
    DrawRectangleLinesEx(rect, 2.0f, border);

    if (checked) {
        DrawRectangleRec({ pos.x + 4, pos.y + 4, boxSize - 8, boxSize - 8 }, locked ? Colors::Zinc600 : Colors::Green500);
    }

    DrawText(label, static_cast<int>(pos.x + boxSize + 8), static_cast<int>(pos.y + 2), 16, locked ? Colors::Zinc600 : Colors::Zinc400);
    return hover;
}

bool Widgets::spinner(const char* label, Vector2 pos, int& value, int minVal, int maxVal, bool locked, int labelWidth) {
    static const char* activeSpinnerLabel = nullptr;
    static char editBuf[32] = "";

    DrawText(label, static_cast<int>(pos.x), static_cast<int>(pos.y + 5), 17, locked ? Colors::Zinc600 : Colors::Zinc400);

    float minusX = pos.x + labelWidth;

    bool isActive = (activeSpinnerLabel == label);
    if (locked && isActive) {
        activeSpinnerLabel = nullptr;
        isActive = false;
    }

    const char* displayStr = isActive ? editBuf : TextFormat("%d", value);
    int textW = MeasureText(displayStr, 17);
    float boxW = std::max(38.0f, static_cast<float>(textW) + 16.0f);
    Rectangle textRect = { minusX + 32, pos.y, boxW, 28 };
    float plusX = textRect.x + boxW + 4;

    int step = (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) ? 10 : 1;

    if (button("-", { minusX, pos.y, 28, 28 }, Colors::Zinc700, Colors::Zinc600, locked || value <= minVal, 18)) {
        value = std::max(minVal, value - step);
    }
    if (button("+", { plusX, pos.y, 28, 28 }, Colors::Zinc700, Colors::Zinc600, locked || value >= maxVal, 18)) {
        value = std::min(maxVal, value + step);
    }

    Vector2 mouse = getUIMousePos();
    bool hover = !locked && CheckCollisionPointRec(mouse, textRect);

    if (!locked && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (hover) {
            activeSpinnerLabel = label;
            snprintf(editBuf, sizeof(editBuf), "%d", value);
        } else if (isActive) {
            if (editBuf[0] != '\0') {
                value = std::clamp(std::atoi(editBuf), minVal, maxVal);
            }
            activeSpinnerLabel = nullptr;
            isActive = false;
        }
    }

    if (isActive) {
        int key = GetCharPressed();
        while (key > 0) {
            if (key >= '0' && key <= '9' && std::strlen(editBuf) < 10) {
                size_t len = std::strlen(editBuf);
                editBuf[len] = static_cast<char>(key);
                editBuf[len + 1] = '\0';
            }
            key = GetCharPressed();
        }

        if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
            size_t len = std::strlen(editBuf);
            if (len > 0) editBuf[len - 1] = '\0';
        }

        if (IsKeyPressed(KEY_ENTER)) {
            if (editBuf[0] != '\0') {
                value = std::clamp(std::atoi(editBuf), minVal, maxVal);
            }
            activeSpinnerLabel = nullptr;
            isActive = false;
        }
    }

    DrawRectangleRec(textRect, isActive ? Colors::Zinc800 : (hover ? Colors::Zinc700 : Colors::Zinc900));
    DrawRectangleLinesEx(textRect, 1.5f, isActive ? Colors::Green500 : Colors::Zinc600);

    DrawText(displayStr, static_cast<int>(textRect.x + (textRect.width - textW) * 0.5f), static_cast<int>(textRect.y + 5), 17, isActive ? Colors::Green500 : (locked ? Colors::Zinc600 : Colors::Zinc200));

    if (isActive && (static_cast<int>(GetTime() * 2.0) % 2 == 0)) {
        DrawRectangle(static_cast<int>(textRect.x + (textRect.width + textW) * 0.5f + 2), static_cast<int>(textRect.y + 4), 2, 20, WHITE);
    }

    return hover;
}

bool Widgets::slider(const char* label, Rectangle rect, float& value, float minVal, float maxVal, int labelWidth, const char* format, bool asPercent, const char* customDisplay) {
    Vector2 mouse = getUIMousePos();
    bool changed = false;

    float textY = rect.y + (rect.height - 16.0f) * 0.5f;
    if (label && label[0] != '\0' && labelWidth > 0) {
        DrawText(label, static_cast<int>(rect.x), static_cast<int>(textY), 16, Colors::Zinc300);
    }

    float trackX = rect.x + labelWidth;
    float valueWidth = (customDisplay != nullptr) ? 75.0f : 54.0f;
    float trackWidth = std::max(60.0f, rect.width - labelWidth - valueWidth - 10.0f);
    float trackHeight = 6.0f;
    float trackY = rect.y + (rect.height - trackHeight) * 0.5f;

    Rectangle interactionRect = { trackX - 6.0f, rect.y, trackWidth + 12.0f, rect.height };

    static const char* activeSliderLabel = nullptr;
    bool hover = CheckCollisionPointRec(mouse, interactionRect);

    if (hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        activeSliderLabel = label;
    }
    if (activeSliderLabel == label) {
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            float frac = std::clamp((mouse.x - trackX) / trackWidth, 0.0f, 1.0f);
            float newVal = minVal + frac * (maxVal - minVal);
            if (std::abs(newVal - value) > 0.001f) {
                value = newVal;
                changed = true;
            }
        } else {
            activeSliderLabel = nullptr;
        }
    }

    float frac = std::clamp((value - minVal) / (maxVal - minVal), 0.0f, 1.0f);

    // Draw background track
    DrawRectangleRounded({ trackX, trackY, trackWidth, trackHeight }, 0.5f, 2, Colors::Zinc800);
    // Draw filled portion
    if (frac > 0.0f) {
        DrawRectangleRounded({ trackX, trackY, trackWidth * frac, trackHeight }, 0.5f, 2, Colors::Green500);
    }

    // Draw thumb handle
    float thumbRadius = (hover || activeSliderLabel == label) ? 8.0f : 6.5f;
    Vector2 thumbCenter = { trackX + trackWidth * frac, rect.y + rect.height * 0.5f };
    DrawCircleV(thumbCenter, thumbRadius, Colors::Zinc200);
    DrawCircleLines(static_cast<int>(thumbCenter.x), static_cast<int>(thumbCenter.y), thumbRadius, Colors::Green400);

    // Value text
    char valBuf[32];
    if (customDisplay && customDisplay[0] != '\0') {
        snprintf(valBuf, sizeof(valBuf), "%s", customDisplay);
    } else if (asPercent) {
        snprintf(valBuf, sizeof(valBuf), format, value * 100.0f);
    } else {
        snprintf(valBuf, sizeof(valBuf), format, value);
    }
    float valX = trackX + trackWidth + 12.0f;
    DrawText(valBuf, static_cast<int>(valX), static_cast<int>(textY), 15, Colors::Green400);

    return changed;
}

bool Widgets::segmented(const char* label, Rectangle rect, int& selectedIdx, const char* const* options, int optionCount, int labelWidth) {
    if (optionCount <= 0) return false;
    bool changed = false;

    float textY = rect.y + (rect.height - 16.0f) * 0.5f;
    if (label && label[0] != '\0' && labelWidth > 0) {
        DrawText(label, static_cast<int>(rect.x), static_cast<int>(textY), 16, Colors::Zinc300);
    }

    float segStartX = rect.x + labelWidth;
    float segTotalW = rect.width - labelWidth;
    float itemW = segTotalW / static_cast<float>(optionCount);

    Vector2 mouse = getUIMousePos();

    for (int i = 0; i < optionCount; ++i) {
        Rectangle itemRect = { segStartX + i * itemW, rect.y, itemW, rect.height };
        bool isSelected = (selectedIdx == i);
        bool hover = CheckCollisionPointRec(mouse, itemRect);

        if (hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            selectedIdx = i;
            changed = true;
        }

        Color bg = isSelected ? Colors::Zinc800 : (hover ? Colors::Zinc900 : Colors::Zinc950);
        Color border = isSelected ? Colors::Green500 : (hover ? Colors::Zinc600 : Colors::Zinc800);
        DrawRectangleRec(itemRect, bg);
        DrawRectangleLinesEx(itemRect, isSelected ? 2.0f : 1.0f, border);

        int tw = MeasureText(options[i], 14);
        DrawText(options[i], static_cast<int>(itemRect.x + (itemW - tw) * 0.5f), static_cast<int>(rect.y + (rect.height - 14.0f) * 0.5f), 14, isSelected ? Colors::Green400 : (hover ? WHITE : Colors::Zinc400));
    }

    return changed;
}

bool Widgets::textInput(Rectangle rect, char* buffer, size_t maxLen, bool& isActive, const char* placeholder, bool leftAlign, int fontSize) {

    Vector2 mouse = getUIMousePos();
    bool hover = CheckCollisionPointRec(mouse, rect);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        isActive = hover;
    }

    if (isActive) {
        // Handle Ctrl+V clipboard paste
        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_V)) {
            const char* clip = GetClipboardText();
            if (clip) {
                size_t clipLen = std::strlen(clip);
                size_t curLen = std::strlen(buffer);
                size_t toCopy = std::min(clipLen, maxLen - 1 - curLen);
                std::strncat(buffer, clip, toCopy);
            }
        }

        int key = GetCharPressed();
        while (key > 0) {
            if (key >= 32 && key <= 126) {
                size_t len = std::strlen(buffer);
                if (len + 1 < maxLen) {
                    buffer[len] = static_cast<char>(key);
                    buffer[len + 1] = '\0';
                }
            }
            key = GetCharPressed();
        }

        if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
            size_t len = std::strlen(buffer);
            if (len > 0) buffer[len - 1] = '\0';
        }

        if (IsKeyPressed(KEY_ENTER)) {
            isActive = false;
        }
    }

    DrawRectangleRec(rect, isActive ? Colors::Zinc800 : (hover ? Colors::Zinc700 : Colors::Zinc900));
    DrawRectangleLinesEx(rect, 2.0f, isActive ? Colors::Green500 : Colors::Zinc600);

    const char* toDraw = (buffer[0] != '\0') ? buffer : placeholder;
    Color textColor = (buffer[0] != '\0') ? WHITE : Colors::Zinc500;

    int textW = MeasureText(toDraw, fontSize);
    float textX = leftAlign
        ? ((textW > rect.width - 16) ? (rect.x + rect.width - 8 - textW) : (rect.x + 8))
        : (rect.x + (rect.width - textW) * 0.5f);
    float textY = rect.y + (rect.height - fontSize) * 0.5f;

    beginScissor(rect.x + 2.0f, rect.y + 2.0f, rect.width - 4.0f, rect.height - 4.0f);
    DrawText(toDraw, static_cast<int>(textX), static_cast<int>(textY), fontSize, textColor);

    if (isActive && (static_cast<int>(GetTime() * 2.0) % 2 == 0)) {
        DrawRectangle(static_cast<int>(textX + textW + 2), static_cast<int>(textY), 2, fontSize, WHITE);
    }
    endScissor();

    return hover;
}

int Widgets::modal(int screenW, int screenH, const char* title, const char* message, const char* btnConfirm, const char* btnCancel, bool allowClose, bool confirmLocked) {
    DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.75f));

    float cardW = 440.0f;
    float cardH = 220.0f;
    Rectangle card = { (screenW - cardW) * 0.5f, (screenH - cardH) * 0.5f, cardW, cardH };

    DrawRectangleRec(card, Colors::Zinc900);
    DrawRectangleLinesEx(card, 2.0f, Colors::Zinc700);

    int titleW = MeasureText(title, 24);
    DrawText(title, static_cast<int>(card.x + (cardW - titleW) * 0.5f), static_cast<int>(card.y + 25), 24, WHITE);

    int msgW = MeasureText(message, 17);
    DrawText(message, static_cast<int>(card.x + (cardW - msgW) * 0.5f), static_cast<int>(card.y + 75), 17, Colors::Zinc400);

    int result = 0;

    if (allowClose) {
        Rectangle closeBtn = { card.x + cardW - 36, card.y + 12, 24, 24 };
        Vector2 mouse = getUIMousePos();
        bool closeHover = CheckCollisionPointRec(mouse, closeBtn);
        DrawRectangleRec(closeBtn, closeHover ? Colors::Red500 : Colors::Zinc800);
        DrawRectangleLinesEx(closeBtn, 1.0f, closeHover ? Colors::Red700 : Colors::Zinc700);
        int xW = MeasureText("X", 14);
        DrawText("X", static_cast<int>(closeBtn.x + (24 - xW) * 0.5f), static_cast<int>(closeBtn.y + 5), 14, closeHover ? WHITE : Colors::Zinc400);

        if ((closeHover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) || IsKeyPressed(KEY_ESCAPE)) {
            result = 3;
        }
    }

    if (btnCancel) {
        float btnW = 140.0f;
        float btnH = 38.0f;
        if (button(btnConfirm, { card.x + 40, card.y + cardH - 60, btnW, btnH }, Colors::Zinc800, Colors::Green500, confirmLocked, 18)) {
            result = 1;
        }
        if (button(btnCancel, { card.x + cardW - 180, card.y + cardH - 60, btnW, btnH }, Colors::Zinc800, Colors::Red500, false, 18)) {
            result = 2;
        }
    } else {
        float btnW = 220.0f;
        float btnH = 40.0f;
        if (button(btnConfirm, { card.x + (cardW - btnW) * 0.5f, card.y + cardH - 65, btnW, btnH }, Colors::Zinc800, Colors::Green500, confirmLocked, 18)) {
            result = 1;
        }
    }

    return result;
}

bool Widgets::mindustryButton(const char* label, const char* sublabel, Rectangle rect, Color accentCol, bool locked, int fontSize) {
    Vector2 mouse = getUIMousePos();
    bool hover = !locked && CheckCollisionPointRec(mouse, rect);
    bool pressed = hover && IsMouseButtonDown(MOUSE_LEFT_BUTTON);

    Rectangle r = rect;
    if (pressed) {
        r.y += 1.0f;
    }

    // Base background plate & border
    auto& procTex = render::ProceduralTextures::instance();
    if (procTex.buttonNPatchNormal.id != 0 && procTex.buttonNPatchHover.id != 0 && procTex.buttonNPatchLocked.id != 0) {
        if (locked) {
            DrawTextureNPatch(procTex.buttonNPatchLocked, procTex.buttonNPatchInfo, r, { 0.0f, 0.0f }, 0.0f, WHITE);
        } else if (hover) {
            DrawTextureNPatch(procTex.buttonNPatchHover, procTex.buttonNPatchInfo, r, { 0.0f, 0.0f }, 0.0f, accentCol);
        } else {
            DrawTextureNPatch(procTex.buttonNPatchNormal, procTex.buttonNPatchInfo, r, { 0.0f, 0.0f }, 0.0f, WHITE);
        }
    } else {
        Color bg = locked ? Colors::Zinc900 : (hover ? Colors::MetalLight : Colors::MetalDark);
        DrawRectangleRec(r, bg);

        Color borderCol = locked ? Colors::Zinc800 : (hover ? accentCol : Colors::PanelBorder);
        DrawRectangleLinesEx(r, 1.5f, borderCol);

        if (hover && !locked) {
            DrawRectangle(static_cast<int>(r.x), static_cast<int>(r.y), 4, static_cast<int>(r.height), accentCol);
            DrawRectangle(static_cast<int>(r.x + r.width - 6.0f), static_cast<int>(r.y), 6, 2, accentCol);
            DrawRectangle(static_cast<int>(r.x + r.width - 2.0f), static_cast<int>(r.y), 2, 6, accentCol);
            DrawRectangle(static_cast<int>(r.x + r.width - 6.0f), static_cast<int>(r.y + r.height - 2.0f), 6, 2, accentCol);
            DrawRectangle(static_cast<int>(r.x + r.width - 2.0f), static_cast<int>(r.y + r.height - 6.0f), 2, 6, accentCol);
        }
    }

    // Text rendering
    Color textCol = locked ? Colors::Zinc600 : (hover ? WHITE : Colors::Zinc200);
    if (sublabel && sublabel[0] != '\0') {
        int mainW = MeasureText(label, fontSize);
        int subFontSize = std::max(10, fontSize - 8);
        int subW = MeasureText(sublabel, subFontSize);

        float textStartY = r.y + (r.height - (fontSize + subFontSize + 4.0f)) * 0.5f;
        DrawText(label, static_cast<int>(r.x + (r.width - mainW) * 0.5f + (hover ? 2.0f : 0.0f)), static_cast<int>(textStartY), fontSize, textCol);
        DrawText(sublabel, static_cast<int>(r.x + (r.width - subW) * 0.5f + (hover ? 2.0f : 0.0f)), static_cast<int>(textStartY + fontSize + 4.0f), subFontSize, hover ? accentCol : Colors::Zinc500);
    } else {
        int textW = MeasureText(label, fontSize);
        DrawText(label, static_cast<int>(r.x + (r.width - textW) * 0.5f + (hover ? 2.0f : 0.0f)), static_cast<int>(r.y + (r.height - fontSize) * 0.5f), fontSize, textCol);
    }

    return (hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON));
}

void Widgets::mindustryPanel(Rectangle rect, const char* headerTitle, Color accentCol) {
    auto& procTex = render::ProceduralTextures::instance();
    if (procTex.panelNPatchTexture.id != 0) {
        DrawTextureNPatch(procTex.panelNPatchTexture, procTex.panelNPatchInfo, rect, { 0.0f, 0.0f }, 0.0f, WHITE);
    } else {
        // Backdrop
        DrawRectangleRec(rect, Colors::PanelBg);
        DrawRectangleLinesEx(rect, 1.5f, Colors::PanelBorder);

        // Tech corner markers
        float markLen = 8.0f;
        Color markCol = Fade(accentCol, 0.70f);
        DrawLineEx({ rect.x, rect.y }, { rect.x + markLen, rect.y }, 2.0f, markCol);
        DrawLineEx({ rect.x, rect.y }, { rect.x, rect.y + markLen }, 2.0f, markCol);
        DrawLineEx({ rect.x + rect.width - markLen, rect.y }, { rect.x + rect.width, rect.y }, 2.0f, markCol);
        DrawLineEx({ rect.x + rect.width, rect.y }, { rect.x + rect.width, rect.y + markLen }, 2.0f, markCol);
        DrawLineEx({ rect.x, rect.y + rect.height - markLen }, { rect.x, rect.y + rect.height }, 2.0f, markCol);
        DrawLineEx({ rect.x, rect.y + rect.height }, { rect.x + markLen, rect.y + rect.height }, 2.0f, markCol);
        DrawLineEx({ rect.x + rect.width - markLen, rect.y + rect.height }, { rect.x + rect.width, rect.y + rect.height }, 2.0f, markCol);
        DrawLineEx({ rect.x + rect.width, rect.y + rect.height - markLen }, { rect.x + rect.width, rect.y + rect.height }, 2.0f, markCol);
    }

    if (headerTitle && headerTitle[0] != '\0') {
        float hH = 36.0f;
        Rectangle headerRect = { rect.x, rect.y, rect.width, hH };
        DrawRectangleRec(headerRect, Colors::Zinc900);
        DrawRectangle(static_cast<int>(rect.x + 12.0f), static_cast<int>(rect.y + 10.0f), 4, static_cast<int>(hH - 20.0f), accentCol);
        DrawLineEx({ rect.x, rect.y + hH }, { rect.x + rect.width, rect.y + hH }, 1.0f, Colors::PanelBorder);

        DrawText(headerTitle, static_cast<int>(rect.x + 24.0f), static_cast<int>(rect.y + (hH - 16.0f) * 0.5f), 16, WHITE);
    }
}

} // namespace minesweeper::ui
