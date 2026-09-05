#include "menu.hpp"
#include "widgets.hpp"
#include "theme.hpp"
#include "../render/raylib_renderer.hpp"
#include "../net/steam_manager.hpp"
#include <cstring>
#include <cstdlib>
#include <cmath>

namespace minesweeper::ui {

MainMenu::MainMenu() = default;

MenuActions MainMenu::drawAndProcess(int screenW, int screenH) {
    MenuActions actions;

    // Title text: "DIMENSION" and "SWEEPER"
    const char* title1 = "DIMENSION";
    const char* title2 = "SWEEPER";
    int t1W = MeasureText(title1, 70);
    int t2W = MeasureText(title2, 70);

    float centerY = screenH * 0.5f;
    float centerX = screenW * 0.5f;

    if (currentScreen == MenuScreen::Main) {
        DrawText(title1, static_cast<int>(centerX - t1W * 0.5f + 4), static_cast<int>(centerY - 216), 70, Fade(BLACK, 0.85f));
        DrawText(title1, static_cast<int>(centerX - t1W * 0.5f), static_cast<int>(centerY - 220), 70, Colors::Zinc200);

        DrawText(title2, static_cast<int>(centerX - t2W * 0.5f + 4), static_cast<int>(centerY - 146), 70, Fade(BLACK, 0.85f));
        DrawText(title2, static_cast<int>(centerX - t2W * 0.5f), static_cast<int>(centerY - 150), 70, Colors::Red500);
    } else {
        const char* subTitle = (currentScreen == MenuScreen::Host) ? "HOST MULTIPLAYER"
            : ((currentScreen == MenuScreen::Join) ? "JOIN MULTIPLAYER" : "CUSTOMIZE SETTINGS");
        int subW = MeasureText(subTitle, 36);
        DrawText(subTitle, static_cast<int>(centerX - subW * 0.5f + 2), static_cast<int>(centerY - 218), 36, Fade(BLACK, 0.85f));
        DrawText(subTitle, static_cast<int>(centerX - subW * 0.5f), static_cast<int>(centerY - 220), 36, Colors::Zinc200);
    }

    float btnW = 260.0f;
    float btnH = 46.0f;
    float btnX = centerX - btnW * 0.5f;

    static bool joinInputActive = false;
    static bool hostPortActive = false;
    static bool nameInputActive = false;

    if (currentScreen == MenuScreen::Main) {
        if (Widgets::button("PLAY SOLO", { btnX, centerY - 30, btnW, btnH }, Colors::Zinc800, Colors::Green500, false, 20)) {
            actions.playSolo = true;
            statusMessage.clear();
        }
        if (Widgets::button("HOST MULTIPLAYER", { btnX, centerY + 30, btnW, btnH }, Colors::Zinc800, Colors::Green500, false, 20)) {
            currentScreen = MenuScreen::Host;
            statusMessage.clear();
        }
        if (Widgets::button("JOIN MULTIPLAYER", { btnX, centerY + 90, btnW, btnH }, Colors::Zinc800, Colors::Green500, false, 20)) {
            currentScreen = MenuScreen::Join;
            statusMessage.clear();
        }
        if (Widgets::button("CUSTOMIZE / SETTINGS", { btnX, centerY + 150, btnW, btnH }, Colors::Zinc800, Colors::Zinc600, false, 20)) {
            currentScreen = MenuScreen::Customize;
            statusMessage.clear();
        }
        if (Widgets::button("QUIT", { btnX, centerY + 210, btnW, btnH }, Colors::Zinc800, Colors::Red700, false, 20)) {
            actions.quit = true;
        }

        bool steamOn = net::SteamManager::instance().isSteamActive();
        const char* steamText = steamOn
            ? TextFormat("STEAM: ONLINE (%s - 480)", net::SteamManager::instance().getPersonaName())
            : "STEAM: OFFLINE";
        Color steamCol = steamOn ? Colors::Green400 : Colors::Zinc600;
        int stW = MeasureText(steamText, 12);
        DrawText(steamText, static_cast<int>(screenW - stW - 16), screenH - 24, 12, steamCol);
    }
    else if (currentScreen == MenuScreen::Host) {
        const char* prompt = "HOST PORT:";
        int pW = MeasureText(prompt, 18);
        DrawText(prompt, static_cast<int>(centerX - pW * 0.5f), static_cast<int>(centerY - 50), 18, Colors::Zinc400);

        Widgets::textInput({ centerX - 100, centerY - 20, 200, 42 }, hostPortBuf, sizeof(hostPortBuf), hostPortActive, "7777");

        if (Widgets::button("START SERVER", { btnX, centerY + 45, btnW, btnH }, Colors::Zinc800, Colors::Green500, false, 20)) {
            actions.hostPort = static_cast<uint16_t>(std::atoi(hostPortBuf));
            if (actions.hostPort == 0) actions.hostPort = 7777;
            actions.hostGame = true;
        }
        if (Widgets::button("BACK", { btnX, centerY + 105, btnW, btnH }, Colors::Zinc800, Colors::Zinc600, false, 20)) {
            currentScreen = MenuScreen::Main;
            hostPortActive = false;
        }
    }
    else if (currentScreen == MenuScreen::Join) {
        bool steamOn = net::SteamManager::instance().isSteamActive();
        float curY = centerY - 70.0f;

        if (steamOn) {
            if (Widgets::button("JOIN VIA STEAM OVERLAY", { btnX, curY, btnW, btnH }, Colors::Zinc800, Colors::Green500, false, 16)) {
                net::SteamManager::instance().openFriendsOverlay();
            }
            curY += 56.0f;

            const char* orTxt = "- OR DIRECT IP -";
            int orW = MeasureText(orTxt, 13);
            DrawText(orTxt, static_cast<int>(centerX - orW * 0.5f), static_cast<int>(curY), 13, Colors::Zinc500);
            curY += 26.0f;
        }

        const char* prompt = "SERVER ADDRESS (IP:PORT):";
        int pW = MeasureText(prompt, 15);
        DrawText(prompt, static_cast<int>(centerX - pW * 0.5f), static_cast<int>(curY), 15, Colors::Zinc400);
        curY += 24.0f;

        Widgets::textInput({ centerX - 140, curY, 280, 42 }, joinIpBuf, sizeof(joinIpBuf), joinInputActive, "127.0.0.1:7777");
        curY += 54.0f;

        if (Widgets::button("CONNECT IP", { btnX, curY, btnW, btnH }, Colors::Zinc800, Colors::Green500, false, 20)) {
            actions.joinAddress = joinIpBuf;
            actions.joinGame = true;
        }
        curY += 56.0f;

        if (Widgets::button("BACK", { btnX, curY, btnW, btnH }, Colors::Zinc800, Colors::Zinc600, false, 20)) {
            currentScreen = MenuScreen::Main;
            joinInputActive = false;
        }
    }
    else if (currentScreen == MenuScreen::Customize) {
        Vector2 mouse = Widgets::getUIMousePos();
        float startY = centerY - 216.0f;

        // PLAYER DISPLAY NAME INPUT
        const char* prompt = "PLAYER DISPLAY NAME:";
        int pW = MeasureText(prompt, 16);
        DrawText(prompt, static_cast<int>(centerX - pW * 0.5f), static_cast<int>(startY), 16, Colors::Zinc400);
        Widgets::textInput({ centerX - 140, startY + 22, 280, 34 }, playerName, sizeof(playerName), nameInputActive, "Player");

        // CATEGORY TABS
        float tabW = 160.0f;
        float tabH = 34.0f;
        float tabGap = 16.0f;
        float tabsTotalW = 2 * tabW + tabGap;
        float tabStartX = centerX - tabsTotalW * 0.5f;
        float tabY = startY + 68.0f;

        struct TabDef {
            CustomizeTab id;
            const char* label;
            int count;
        };
        TabDef tabs[2] = {
            { CustomizeTab::Cursors, "CURSORS", render::RaylibRenderer::getCursorSkinCount() },
            { CustomizeTab::Flags,   "FLAGS",   render::RaylibRenderer::getFlagSkinCount() }
        };

        for (int i = 0; i < 2; ++i) {
            float tX = tabStartX + i * (tabW + tabGap);
            Rectangle tRect = { tX, tabY, tabW, tabH };
            bool isActive = (activeTab == tabs[i].id);
            bool isHover = CheckCollisionPointRec(mouse, tRect);

            if (isHover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                activeTab = tabs[i].id;
                reelScrollX = 0.0f;
                reelTargetScrollX = 0.0f;
            }

            Color tBg = isActive ? Colors::Zinc800 : (isHover ? Colors::Zinc900 : Colors::Zinc950);
            Color tBorder = isActive ? Colors::Green500 : (isHover ? Colors::Zinc600 : Colors::Zinc800);
            DrawRectangleRec(tRect, tBg);
            DrawRectangleLinesEx(tRect, isActive ? 2.0f : 1.0f, tBorder);

            std::string tTitle = std::string(tabs[i].label) + " (" + std::to_string(tabs[i].count) + ")";
            int tw = MeasureText(tTitle.c_str(), 14);
            DrawText(tTitle.c_str(), static_cast<int>(tX + (tabW - tw) * 0.5f), static_cast<int>(tabY + (tabH - 14) * 0.5f), 14, isActive ? Colors::Green400 : (isHover ? WHITE : Colors::Zinc400));
        }

        // TAB SHORTCUT (Tab key cycles through Cursors and Flags)
        if (IsKeyPressed(KEY_TAB)) {
            activeTab = (activeTab == CustomizeTab::Cursors) ? CustomizeTab::Flags : CustomizeTab::Cursors;
            reelScrollX = 0.0f;
            reelTargetScrollX = 0.0f;
        }

        // HORIZONTAL REEL CONTAINER
        float reelContainerW = 580.0f;
        float reelContainerH = 118.0f;
        float reelContainerX = centerX - reelContainerW * 0.5f;
        float reelContainerY = tabY + 44.0f;

        DrawRectangleRec({ reelContainerX, reelContainerY, reelContainerW, reelContainerH }, Colors::Zinc950);
        DrawRectangleLinesEx({ reelContainerX, reelContainerY, reelContainerW, reelContainerH }, 1.5f, Colors::Zinc800);

        // Arrows & Viewport Dimensions
        float arrowBtnW = 32.0f;
        Rectangle leftBtnRect = { reelContainerX + 6.0f, reelContainerY + 6.0f, arrowBtnW, reelContainerH - 12.0f };
        Rectangle rightBtnRect = { reelContainerX + reelContainerW - arrowBtnW - 6.0f, reelContainerY + 6.0f, arrowBtnW, reelContainerH - 12.0f };

        float vpX = reelContainerX + 44.0f;
        float vpY = reelContainerY + 6.0f;
        float vpW = reelContainerW - 88.0f;
        float vpH = reelContainerH - 12.0f;

        float cardW = 88.0f;
        float cardH = 104.0f;
        float cardSpacing = 10.0f;

        int itemCount = 0;
        int currentEquipped = 0;
        if (activeTab == CustomizeTab::Cursors) {
            itemCount = render::RaylibRenderer::getCursorSkinCount();
            if (cursorSkin >= itemCount) cursorSkin = 0;
            currentEquipped = cursorSkin;
        } else {
            itemCount = render::RaylibRenderer::getFlagSkinCount();
            if (flagSkin >= itemCount) flagSkin = 0;
            currentEquipped = flagSkin;
        }

        float totalCardsW = itemCount > 0 ? (itemCount * cardW + (itemCount - 1) * cardSpacing) : 0.0f;
        float maxScroll = std::max(0.0f, totalCardsW - vpW);

        // Left & Right Arrow Buttons
        if (Widgets::button("<", leftBtnRect, Colors::Zinc900, Colors::Green500, maxScroll <= 0.0f, 18)) {
            reelTargetScrollX -= (cardW + cardSpacing);
        }
        if (Widgets::button(">", rightBtnRect, Colors::Zinc900, Colors::Green500, maxScroll <= 0.0f, 18)) {
            reelTargetScrollX += (cardW + cardSpacing);
        }

        // Mouse Wheel & Drag handling
        bool isOverReel = CheckCollisionPointRec(mouse, { reelContainerX, reelContainerY, reelContainerW, reelContainerH });
        if (isOverReel) {
            float wheel = GetMouseWheelMove();
            if (wheel != 0.0f) {
                reelTargetScrollX -= wheel * (cardW + cardSpacing);
            }
        }

        if (IsKeyPressed(KEY_LEFT)) reelTargetScrollX -= (cardW + cardSpacing);
        if (IsKeyPressed(KEY_RIGHT)) reelTargetScrollX += (cardW + cardSpacing);

        bool mouseOverViewport = CheckCollisionPointRec(mouse, { vpX, vpY, vpW, vpH });
        if (mouseOverViewport && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            reelIsDragging = true;
            reelDragStartX = mouse.x;
            reelDragStartScroll = reelTargetScrollX;
        }

        if (reelIsDragging && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            float deltaX = mouse.x - reelDragStartX;
            reelTargetScrollX = reelDragStartScroll - deltaX;
        }

        bool wasDragged = false;
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            if (reelIsDragging && std::abs(mouse.x - reelDragStartX) > 6.0f) {
                wasDragged = true;
            }
            reelIsDragging = false;
        }

        reelTargetScrollX = std::clamp(reelTargetScrollX, 0.0f, maxScroll);
        float dt = GetFrameTime();
        reelScrollX += (reelTargetScrollX - reelScrollX) * std::min(1.0f, dt * 18.0f);

        // Cards layout
        float contentStartX = (totalCardsW < vpW)
            ? (vpX + (vpW - totalCardsW) * 0.5f)
            : (vpX - reelScrollX);

        BeginScissorMode(static_cast<int>(vpX), static_cast<int>(vpY), static_cast<int>(vpW), static_cast<int>(vpH));

        for (int i = 0; i < itemCount; ++i) {
            float cX = contentStartX + i * (cardW + cardSpacing);
            Rectangle cardRect = { cX, vpY + 1.0f, cardW, cardH };

            if (cX + cardW < vpX || cX > vpX + vpW) continue; // Culling

            bool isEquipped = (i == currentEquipped);
            bool isHovered = mouseOverViewport && CheckCollisionPointRec(mouse, cardRect);

            if (isHovered && IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && !wasDragged) {
                if (activeTab == CustomizeTab::Cursors) cursorSkin = i;
                else flagSkin = i;
                currentEquipped = i;
            }

            Color bgCol = isEquipped ? Fade(Colors::Green500, 0.12f) : (isHovered ? Colors::Zinc850 : Colors::Zinc900);
            Color borderCol = isEquipped ? Colors::Green500 : (isHovered ? Colors::Zinc500 : Colors::Zinc800);
            DrawRectangleRec(cardRect, bgCol);
            DrawRectangleLinesEx(cardRect, isEquipped ? 2.0f : 1.0f, borderCol);

            if (isEquipped) {
                DrawRectangleRec({ cX + 8.0f, cardRect.y + 4.0f, cardW - 16.0f, 13.0f }, Fade(Colors::Green500, 0.35f));
                int eqW = MeasureText("EQUIPPED", 9);
                DrawText("EQUIPPED", static_cast<int>(cX + (cardW - eqW) * 0.5f), static_cast<int>(cardRect.y + 6.0f), 9, Colors::Green400);
            }

            Vector2 iconCenter = { cX + cardW * 0.5f, cardRect.y + (isEquipped ? 52.0f : 46.0f) };
            const char* itemName = "DEFAULT";

            if (activeTab == CustomizeTab::Cursors) {
                itemName = render::RaylibRenderer::getCursorSkinName(i);
                render::RaylibRenderer::drawCursorSkin(static_cast<uint8_t>(i), { iconCenter.x - 14.0f, iconCenter.y - 14.0f }, isEquipped ? Colors::Green500 : WHITE, nullptr, 1.25f);
            } else {
                itemName = render::RaylibRenderer::getFlagSkinName(i);
                Texture2D fTex = render::RaylibRenderer::getFlagTexture(i);
                if (fTex.id != 0) {
                    float t = static_cast<float>(GetTime()) + i * 0.2f;
                    int numFrames = (fTex.width >= fTex.height * 2) ? 3 : 1;
                    float frameW = static_cast<float>(fTex.width) / static_cast<float>(numFrames);
                    int fFrame = (numFrames > 1) ? (static_cast<int>(t * 4.0f) % numFrames) : 0;
                    Rectangle src = { fFrame * frameW, 0.0f, frameW, static_cast<float>(fTex.height) };
                    Rectangle dst = { iconCenter.x - 16.0f, iconCenter.y - 16.0f, 32.0f, 32.0f };
                    DrawTexturePro(fTex, src, dst, {0, 0}, 0.0f, WHITE);
                }
            }

            int fontS = 11;
            int nw = MeasureText(itemName, fontS);
            if (nw > static_cast<int>(cardW - 8.0f)) {
                fontS = 9;
                nw = MeasureText(itemName, fontS);
            }
            DrawText(itemName, static_cast<int>(cX + (cardW - nw) * 0.5f), static_cast<int>(cardRect.y + cardH - 18.0f), fontS, isEquipped ? WHITE : Colors::Zinc400);
        }

        // Soft Edge Fade Gradient
        DrawRectangleGradientH(static_cast<int>(vpX), static_cast<int>(vpY), 24, static_cast<int>(vpH), Fade(Colors::Zinc950, 0.95f), Fade(Colors::Zinc950, 0.0f));
        DrawRectangleGradientH(static_cast<int>(vpX + vpW - 24), static_cast<int>(vpY), 24, static_cast<int>(vpH), Fade(Colors::Zinc950, 0.0f), Fade(Colors::Zinc950, 0.95f));

        EndScissorMode();

        // LIVE SHOWCASE PREVIEW BOX
        float previewBoxW = 420.0f;
        float previewBoxH = 64.0f;
        float previewBoxX = centerX - previewBoxW * 0.5f;
        float previewBoxY = reelContainerY + reelContainerH + 14.0f;

        DrawRectangleRec({ previewBoxX, previewBoxY, previewBoxW, previewBoxH }, Colors::Zinc900);
        DrawRectangleLinesEx({ previewBoxX, previewBoxY, previewBoxW, previewBoxH }, 1.5f, Colors::Zinc800);

        DrawText("SHOWCASE PREVIEW", static_cast<int>(previewBoxX + 14), static_cast<int>(previewBoxY + 8), 11, Colors::Zinc500);

        if (activeTab == CustomizeTab::Cursors) {
            Vector2 pCenter = { previewBoxX + previewBoxW * 0.5f - 40.0f, previewBoxY + 16.0f };
            render::RaylibRenderer::drawCursorSkin(static_cast<uint8_t>(cursorSkin), pCenter, Colors::Green500, playerName, 1.4f);
        } else {
            Texture2D curFlagTex = render::RaylibRenderer::getFlagTexture(flagSkin);
            if (curFlagTex.id != 0) {
                DrawRectangleRounded({ previewBoxX + previewBoxW * 0.5f - 48.0f, previewBoxY + 14.0f, 36.0f, 36.0f }, 0.2f, 4, Colors::CellHidden);
                float t = static_cast<float>(GetTime());
                int numFrames = (curFlagTex.width >= curFlagTex.height * 2) ? 3 : 1;
                float frameW = static_cast<float>(curFlagTex.width) / static_cast<float>(numFrames);
                int fFrame = (numFrames > 1) ? (static_cast<int>(t) % numFrames) : 0;
                Rectangle src = { fFrame * frameW, 0.0f, frameW, static_cast<float>(curFlagTex.height) };
                float cellX = previewBoxX + previewBoxW * 0.5f - 48.0f + 2.0f;
                float cellY = previewBoxY + 14.0f + 2.0f;
                float cellSize = 32.0f;
                float flagW = cellSize * 1.05f;
                float flagH = cellSize * 1.05f;
                Vector2 origin = { 4.0f * flagW / 16.0f, 12.0f * flagH / 16.0f };
                Rectangle dst = { cellX + 4.0f * cellSize / 16.0f, cellY + 12.0f * cellSize / 16.0f, flagW, flagH };
                float tilt = std::sin(t) * 5.0f;
                DrawTexturePro(curFlagTex, src, dst, origin, tilt, WHITE);
            }
            const char* curFlagName = render::RaylibRenderer::getFlagSkinName(flagSkin);
            DrawText(curFlagName, static_cast<int>(previewBoxX + previewBoxW * 0.5f), static_cast<int>(previewBoxY + 24.0f), 16, Colors::Green400);
        }

        // CRT Shader Checkbox
        Widgets::checkbox("ENABLE RETRO CRT SHADER", { centerX - 130, previewBoxY + previewBoxH + 16.0f }, crtEnabled, false);

        // DONE BUTTON
        if (Widgets::button("DONE", { btnX, previewBoxY + previewBoxH + 54.0f, btnW, btnH }, Colors::Zinc800, Colors::Green500, false, 20)) {
            currentScreen = MenuScreen::Main;
            nameInputActive = false;
        }
    }

    if (!statusMessage.empty()) {
        int smW = MeasureText(statusMessage.c_str(), 16);
        DrawText(statusMessage.c_str(), static_cast<int>(centerX - smW * 0.5f), static_cast<int>(centerY + 275), 16, Colors::Red500);
    }

    return actions;
}

} // namespace minesweeper::ui
