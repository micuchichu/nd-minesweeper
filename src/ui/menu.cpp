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
        DrawText(title2, static_cast<int>(centerX - t2W * 0.5f), static_cast<int>(centerY - 150), 70, Colors::Amber500);
    } else {
        const char* subTitle = (currentScreen == MenuScreen::Play) ? "// SELECT GAME MODE //"
            : ((currentScreen == MenuScreen::Host) ? "// HOST MULTIPLAYER //"
            : ((currentScreen == MenuScreen::Join) ? "// JOIN MULTIPLAYER //"
            : ((currentScreen == MenuScreen::Settings) ? "// SETTINGS //" : "// CUSTOMIZATION //")));
        int subW = MeasureText(subTitle, 28);
        DrawText(subTitle, static_cast<int>(centerX - subW * 0.5f + 2), static_cast<int>(centerY - 228), 28, Fade(BLACK, 0.85f));
        DrawText(subTitle, static_cast<int>(centerX - subW * 0.5f), static_cast<int>(centerY - 230), 28, Colors::Amber400);
    }

    static bool joinInputActive = false;
    static bool hostPortActive = false;
    static bool nameInputActive = false;

    if (currentScreen == MenuScreen::Main) {
        float panelW = 360.0f;
        float panelH = 268.0f;
        float panelX = centerX - panelW * 0.5f;
        float panelY = centerY - 54.0f;

        Widgets::mindustryPanel({ panelX, panelY, panelW, panelH }, "COMMAND CONSOLE", Colors::Amber500);

        float mBtnW = panelW - 36.0f;
        float mBtnH = 46.0f;
        float mBtnX = panelX + 18.0f;
        float curBtnY = panelY + 48.0f;
        float btnGap = 8.0f;

        if (Widgets::mindustryButton("PLAY", "START OR JOIN A GAME", { mBtnX, curBtnY, mBtnW, mBtnH }, Colors::Amber500, false, 18)) {
            currentScreen = MenuScreen::Play;
            statusMessage.clear();
        }
        curBtnY += mBtnH + btnGap;

        if (Widgets::mindustryButton("CUSTOMIZE", "SKINS & APPEARANCE", { mBtnX, curBtnY, mBtnW, mBtnH }, Colors::Green500, false, 18)) {
            currentScreen = MenuScreen::Customize;
            statusMessage.clear();
        }
        curBtnY += mBtnH + btnGap;

        if (Widgets::mindustryButton("SETTINGS", "SCALING & GRAPHICS", { mBtnX, curBtnY, mBtnW, mBtnH }, Colors::Cyan500, false, 18)) {
            currentScreen = MenuScreen::Settings;
            statusMessage.clear();
        }
        curBtnY += mBtnH + btnGap;

        if (Widgets::mindustryButton("QUIT", "EXIT TO DESKTOP", { mBtnX, curBtnY, mBtnW, mBtnH }, Colors::Red500, false, 18)) {
            actions.quit = true;
        }

        bool steamOn = net::SteamManager::instance().isSteamActive();
        const char* steamText = steamOn
            ? TextFormat("STEAM: ONLINE (%s - 480)", net::SteamManager::instance().getPersonaName())
            : "STEAM: OFFLINE";
        Color steamCol = steamOn ? Colors::Green400 : Colors::Zinc600;
        int stW = MeasureText(steamText, 12);
        DrawText(steamText, static_cast<int>(screenW - stW - 16), screenH - 24, 12, steamCol);

        const char* verText = "DIMENSION SWEEPER v1.0.0";
        DrawText(verText, 16, screenH - 24, 12, Colors::Zinc600);
    }
    else if (currentScreen == MenuScreen::Play) {
        float panelW = 420.0f;
        float panelH = 306.0f;
        float panelX = centerX - panelW * 0.5f;
        float panelY = centerY - 100.0f;

        Widgets::mindustryPanel({ panelX, panelY, panelW, panelH }, "SELECT MODE", Colors::Amber500);

        float pBtnW = panelW - 40.0f;
        float pBtnH = 50.0f;
        float pBtnX = panelX + 20.0f;
        float curY = panelY + 50.0f;
        float gap = 9.0f;

        if (Widgets::mindustryButton("PLAY SOLO", "OFFLINE SINGLEPLAYER MATCH", { pBtnX, curY, pBtnW, pBtnH }, Colors::Green500, false, 18)) {
            actions.playSolo = true;
            statusMessage.clear();
        }
        curY += pBtnH + gap;

        if (Widgets::mindustryButton("HOST MULTIPLAYER", "HOST LAN / INTERNET / STEAM LOBBY", { pBtnX, curY, pBtnW, pBtnH }, Colors::Amber500, false, 18)) {
            currentScreen = MenuScreen::Host;
            statusMessage.clear();
        }
        curY += pBtnH + gap;

        if (Widgets::mindustryButton("JOIN MULTIPLAYER", "CONNECT VIA IP OR INVITE", { pBtnX, curY, pBtnW, pBtnH }, Colors::Cyan500, false, 18)) {
            currentScreen = MenuScreen::Join;
            statusMessage.clear();
        }
        curY += pBtnH + gap + 4.0f;

        if (Widgets::mindustryButton("BACK", nullptr, { pBtnX, curY, pBtnW, 40.0f }, Colors::Zinc600, false, 15) || IsKeyPressed(KEY_ESCAPE)) {
            currentScreen = MenuScreen::Main;
            statusMessage.clear();
        }
    }
    else if (currentScreen == MenuScreen::Host) {
        float panelW = 420.0f;
        float panelH = 260.0f;
        float panelX = centerX - panelW * 0.5f;
        float panelY = centerY - 85.0f;

        Widgets::mindustryPanel({ panelX, panelY, panelW, panelH }, "HOST SERVER", Colors::Amber500);

        const char* prompt = "HOST PORT:";
        int pW = MeasureText(prompt, 16);
        DrawText(prompt, static_cast<int>(centerX - pW * 0.5f), static_cast<int>(panelY + 54), 16, Colors::Zinc400);

        Widgets::textInput({ centerX - 110, panelY + 78, 220, 42 }, hostPortBuf, sizeof(hostPortBuf), hostPortActive, "7777");

        float btnW = panelW - 48.0f;
        float btnX = panelX + 24.0f;
        if (Widgets::mindustryButton("START SERVER", "BIND PORT & BEGIN HOSTING", { btnX, panelY + 138, btnW, 46 }, Colors::Green500, false, 18)) {
            actions.hostPort = static_cast<uint16_t>(std::atoi(hostPortBuf));
            if (actions.hostPort == 0) actions.hostPort = 7777;
            actions.hostGame = true;
        }
        if (Widgets::mindustryButton("BACK", nullptr, { btnX, panelY + 196, btnW, 40 }, Colors::Zinc600, false, 15) || IsKeyPressed(KEY_ESCAPE)) {
            currentScreen = MenuScreen::Play;
            statusMessage.clear();
            hostPortActive = false;
        }
    }
    else if (currentScreen == MenuScreen::Join) {
        float panelW = 440.0f;
        float panelH = 260.0f;
        float panelX = centerX - panelW * 0.5f;
        float panelY = centerY - 85.0f;

        Widgets::mindustryPanel({ panelX, panelY, panelW, panelH }, "JOIN SERVER", Colors::Cyan500);

        const char* prompt = "SERVER ADDRESS (IP:PORT):";
        int pW = MeasureText(prompt, 16);
        DrawText(prompt, static_cast<int>(centerX - pW * 0.5f), static_cast<int>(panelY + 54), 16, Colors::Zinc400);

        Widgets::textInput({ centerX - 140, panelY + 78, 280, 42 }, joinIpBuf, sizeof(joinIpBuf), joinInputActive, "127.0.0.1:7777");

        float btnW = panelW - 48.0f;
        float btnX = panelX + 24.0f;
        if (Widgets::mindustryButton("CONNECT", "JOIN MULTIPLAYER MATCH", { btnX, panelY + 138, btnW, 46 }, Colors::Green500, false, 18)) {
            actions.joinAddress = joinIpBuf;
            actions.joinGame = true;
        }
        if (Widgets::mindustryButton("BACK", nullptr, { btnX, panelY + 196, btnW, 40 }, Colors::Zinc600, false, 15) || IsKeyPressed(KEY_ESCAPE)) {
            currentScreen = MenuScreen::Play;
            statusMessage.clear();
            joinInputActive = false;
        }
    }
    else if (currentScreen == MenuScreen::Customize) {
        Vector2 mouse = Widgets::getUIMousePos();
        float startY = centerY - 170.0f;

        // Player Name Bar
        const char* prompt = "PLAYER NAME:";
        int pW = MeasureText(prompt, 15);
        DrawText(prompt, static_cast<int>(centerX - pW * 0.5f), static_cast<int>(startY), 15, Colors::Zinc400);
        Widgets::textInput({ centerX - 140, startY + 22, 280, 34 }, playerName, sizeof(playerName), nameInputActive, "Player");

        // CATEGORY TABS (CURSORS, FLAGS)
        float tabW = 180.0f;
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

            Color tBg = isActive ? Colors::Zinc800 : (isHover ? Colors::Zinc900 : Colors::MetalDark);
            Color tBorder = isActive ? Colors::Green500 : (isHover ? Colors::Zinc600 : Colors::PanelBorder);
            DrawRectangleRec(tRect, tBg);
            DrawRectangleLinesEx(tRect, isActive ? 2.0f : 1.0f, tBorder);
            if (isActive) {
                DrawRectangle(static_cast<int>(tX + 4), static_cast<int>(tabY + tabH - 3), static_cast<int>(tabW - 8), 3, Colors::Green500);
            }

            std::string tTitle = (tabs[i].count >= 0)
                ? (std::string(tabs[i].label) + " (" + std::to_string(tabs[i].count) + ")")
                : std::string(tabs[i].label);
            int tw = MeasureText(tTitle.c_str(), 14);
            DrawText(tTitle.c_str(), static_cast<int>(tX + (tabW - tw) * 0.5f), static_cast<int>(tabY + (tabH - 14) * 0.5f), 14, isActive ? Colors::Green400 : (isHover ? WHITE : Colors::Zinc400));
        }

        if (IsKeyPressed(KEY_TAB)) {
            activeTab = (activeTab == CustomizeTab::Cursors) ? CustomizeTab::Flags : CustomizeTab::Cursors;
            reelScrollX = 0.0f;
            reelTargetScrollX = 0.0f;
        }

        // HORIZONTAL REEL CONTAINER (Cursors & Flags)
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

        // DONE BUTTON
        float doneBtnW = 240.0f;
        float doneBtnH = 44.0f;
        float doneBtnX = centerX - doneBtnW * 0.5f;
        float doneBtnY = previewBoxY + previewBoxH + 20.0f;

        if (Widgets::mindustryButton("DONE", "SAVE & RETURN", { doneBtnX, doneBtnY, doneBtnW, doneBtnH }, Colors::Green500, false, 18) || IsKeyPressed(KEY_ESCAPE)) {
            currentScreen = MenuScreen::Main;
            nameInputActive = false;
        }
    }
    else if (currentScreen == MenuScreen::Settings) {
        float panelW = 600.0f;
        float panelH = 500.0f;
        float panelX = centerX - panelW * 0.5f;
        float panelY = centerY - 250.0f;

        Widgets::mindustryPanel({ panelX, panelY, panelW, panelH }, "SYSTEM & AUDIO CONFIGURATION", Colors::Cyan500);

        float rowX = panelX + 24.0f;
        float rowW = panelW - 48.0f;
        float curY = panelY + 42.0f;

        // Section 1: Display & Graphics
        DrawText("// DISPLAY & SHADERS //", static_cast<int>(rowX), static_cast<int>(curY), 13, Colors::Zinc400);
        curY += 22.0f;

        if (Widgets::checkbox("ENABLE RETRO CRT SHADER", { rowX, curY }, crtEnabled, false)) {
            actions.toggleCRT = true;
        }
        curY += 30.0f;

        const char* crtDesc = "Simulates authentic curved cathode-ray tube phosphor scanlines and vignette.";
        DrawText(crtDesc, static_cast<int>(rowX + 28.0f), static_cast<int>(curY), 12, Colors::Zinc500);
        curY += 22.0f;

        DrawLineEx({ rowX, curY }, { rowX + rowW, curY }, 1.0f, Colors::PanelBorder);
        curY += 14.0f;

        // Section 2: Proximity Voice Chat
        DrawText("// PROXIMITY VOICE CHAT //", static_cast<int>(rowX), static_cast<int>(curY), 13, Colors::Cyan400);
        curY += 22.0f;

        Widgets::checkbox("ENABLE PROXIMITY VOICE CHAT", { rowX, curY }, voiceSettings.enabled, false);
        curY += 30.0f;

        if (voiceSettings.enabled) {
            Widgets::checkbox("SPATIAL 3D/4D PROXIMITY ATTENUATION", { rowX + 16.0f, curY }, voiceSettings.proximity, false);
            curY += 28.0f;

            Widgets::checkbox("PUSH-TO-TALK (HOLD [V] TO SPEAK)", { rowX + 16.0f, curY }, voiceSettings.pushToTalk, false);
            curY += 32.0f;

            Rectangle volRect = { rowX + 16.0f, curY, rowW - 32.0f, 24.0f };
            Widgets::slider("VOICE VOLUME", volRect, voiceSettings.voiceVolume, 0.0f, 1.5f, 160, "%.0f%%", true);
            curY += 30.0f;

            Rectangle micRect = { rowX + 16.0f, curY, rowW - 32.0f, 24.0f };
            Widgets::slider("MIC INPUT GAIN", micRect, voiceSettings.micGain, 0.5f, 2.5f, 160, "%.0f%%", true);
            curY += 32.0f;

            // Live Microphone Level Meter
            DrawText("MIC TEST LEVEL:", static_cast<int>(rowX + 16.0f), static_cast<int>(curY + 3.0f), 15, Colors::Zinc300);
            float meterX = rowX + 180.0f;
            float meterW = rowW - 200.0f;
            float meterH = 14.0f;
            float meterY = curY + 4.0f;

            DrawRectangleRec({ meterX, meterY, meterW, meterH }, Colors::Zinc900);
            DrawRectangleLinesEx({ meterX, meterY, meterW, meterH }, 1.0f, Colors::Zinc700);

            float levelClamped = std::clamp(micInputLevel * voiceSettings.micGain, 0.0f, 1.0f);
            if (levelClamped > 0.01f) {
                Color meterCol = (levelClamped > 0.85f) ? Colors::Red500 : ((levelClamped > 0.6f) ? Colors::Amber500 : Colors::Green500);
                DrawRectangleRec({ meterX + 2.0f, meterY + 2.0f, (meterW - 4.0f) * levelClamped, meterH - 4.0f }, meterCol);
            }
            curY += 26.0f;
        } else {
            DrawText("Voice chat is disabled. No audio capture or playback will occur.", static_cast<int>(rowX + 28.0f), static_cast<int>(curY), 13, Colors::Zinc500);
            curY += 30.0f;
        }

        DrawLineEx({ rowX, curY }, { rowX + rowW, curY }, 1.0f, Colors::PanelBorder);
        curY += 14.0f;

        // Section 3: Hotkeys info
        auto drawInfoRow = [&](const char* title, const char* val) {
            DrawText(title, static_cast<int>(rowX), static_cast<int>(curY), 13, Colors::Zinc400);
            int valW = MeasureText(val, 13);
            DrawText(val, static_cast<int>(rowX + rowW - valW), static_cast<int>(curY), 13, Colors::Cyan400);
            curY += 22.0f;
        };

        drawInfoRow("PUSH-TO-TALK KEY", "KEYBOARD [ V ]");
        drawInfoRow("TOGGLE FULLSCREEN", "KEYBOARD [ F11 ]");

        // DONE BUTTON
        float doneBtnW = 240.0f;
        float doneBtnH = 44.0f;
        float doneBtnX = centerX - doneBtnW * 0.5f;
        float doneBtnY = panelY + panelH + 16.0f;

        if (Widgets::mindustryButton("DONE", "APPLY & RETURN", { doneBtnX, doneBtnY, doneBtnW, doneBtnH }, Colors::Green500, false, 18) || IsKeyPressed(KEY_ESCAPE)) {
            currentScreen = MenuScreen::Main;
        }
    }

    if (!statusMessage.empty()) {
        int smW = MeasureText(statusMessage.c_str(), 16);
        DrawText(statusMessage.c_str(), static_cast<int>(centerX - smW * 0.5f), static_cast<int>(centerY + 275), 16, Colors::Red500);
    }

    return actions;
}

} // namespace minesweeper::ui

