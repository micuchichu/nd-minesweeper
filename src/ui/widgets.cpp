#include "widgets.hpp"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <algorithm>

namespace minesweeper::ui {

Vector2 Widgets::getUIMousePos() {
    Vector2 mouse = GetMousePosition();
    float screenW = static_cast<float>(GetScreenWidth());
    float screenH = static_cast<float>(GetScreenHeight());
    if (screenW <= 0.0f || screenH <= 0.0f) return mouse;

    Vector2 centered = { (mouse.x / screenW) - 0.5f, (mouse.y / screenH) - 0.5f };
    float r2 = (centered.x * centered.x) + (centered.y * centered.y);

    return {
        (mouse.x / screenW + centered.x * (r2 * 0.05f)) * screenW,
        (mouse.y / screenH + centered.y * (r2 * 0.05f)) * screenH
    };
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

    BeginScissorMode(static_cast<int>(rect.x + 2), static_cast<int>(rect.y + 2), static_cast<int>(rect.width - 4), static_cast<int>(rect.height - 4));
    DrawText(toDraw, static_cast<int>(textX), static_cast<int>(textY), fontSize, textColor);

    if (isActive && (static_cast<int>(GetTime() * 2.0) % 2 == 0)) {
        DrawRectangle(static_cast<int>(textX + textW + 2), static_cast<int>(textY), 2, fontSize, WHITE);
    }
    EndScissorMode();

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

} // namespace minesweeper::ui
