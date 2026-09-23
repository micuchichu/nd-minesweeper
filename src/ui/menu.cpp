#include "menu.hpp"
#include "widgets.hpp"
#include "theme.hpp"
#include "../render/raylib_renderer.hpp"
#include "../net/steam_manager.hpp"
#include "../core/save_manager.hpp"
#include "../core/campaign.hpp"
#include "../audio/sound_manager.hpp"
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <algorithm>

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

    // Scrap Currency Badge in Menu (Top-Right)
    const char* scrapStr = TextFormat("%llu", scrapCount);
    int scrapTextW = MeasureText(scrapStr, 15);
    float scrapBadgeW = static_cast<float>(32 + scrapTextW + 14);
    float scrapBadgeH = 32.0f;
    float scrapBadgeX = screenW - scrapBadgeW - 24.0f;
    float scrapBadgeY = 20.0f;

    Rectangle scrapRect = { scrapBadgeX, scrapBadgeY, scrapBadgeW, scrapBadgeH };
    DrawRectangleRec(scrapRect, Colors::Zinc900);
    DrawRectangleLinesEx(scrapRect, 1.0f, Colors::Zinc700);

    float iconSize = 20.0f;
    float iconCenterX = scrapBadgeX + 16.0f;
    float iconCenterY = scrapBadgeY + scrapBadgeH * 0.5f;
    if (scrapTexture.id != 0) {
        Rectangle sSrc = { 0.0f, 0.0f, static_cast<float>(scrapTexture.width), static_cast<float>(scrapTexture.height) };
        Rectangle sDst = { iconCenterX, iconCenterY, iconSize, iconSize };
        Vector2 sOrigin = { iconSize * 0.5f, iconSize * 0.5f };
        DrawTexturePro(scrapTexture, sSrc, sDst, sOrigin, 0.0f, WHITE);
    }
    DrawText(scrapStr, static_cast<int>(scrapBadgeX + 32), static_cast<int>(scrapBadgeY + (scrapBadgeH - 15) * 0.5f), 15, Colors::Amber400);

    // Hover tooltip
    Vector2 mPos = Widgets::getUIMousePos();
    if (CheckCollisionPointRec(mPos, scrapRect)) {
        const char* tip = TextFormat("TOTAL SCRAP: %llu", scrapCount);
        int tipW = MeasureText(tip, 12);
        DrawRectangle(static_cast<int>(scrapBadgeX - tipW + scrapBadgeW - 12), static_cast<int>(scrapBadgeY + scrapBadgeH + 4), tipW + 12, 20, Colors::Zinc950);
        DrawRectangleLines(static_cast<int>(scrapBadgeX - tipW + scrapBadgeW - 12), static_cast<int>(scrapBadgeY + scrapBadgeH + 4), tipW + 12, 20, Colors::Zinc700);
        DrawText(tip, static_cast<int>(scrapBadgeX - tipW + scrapBadgeW - 6), static_cast<int>(scrapBadgeY + scrapBadgeH + 7), 12, Colors::Zinc300);
    }

    if (currentScreen == MenuScreen::Main) {
        DrawText(title1, static_cast<int>(centerX - t1W * 0.5f + 4), static_cast<int>(centerY - 216), 70, Fade(BLACK, 0.85f));
        DrawText(title1, static_cast<int>(centerX - t1W * 0.5f), static_cast<int>(centerY - 220), 70, Colors::Zinc200);

        DrawText(title2, static_cast<int>(centerX - t2W * 0.5f + 4), static_cast<int>(centerY - 146), 70, Fade(BLACK, 0.85f));
        DrawText(title2, static_cast<int>(centerX - t2W * 0.5f), static_cast<int>(centerY - 150), 70, Colors::Amber500);
    } else if (currentScreen != MenuScreen::Campaign) {
        const char* subTitle = (currentScreen == MenuScreen::Play) ? "// CUSTOM GAME • SAVED WORLDS //"
            : ((currentScreen == MenuScreen::NewSave) ? "// CONFIGURE NEW SAVE //"
            : ((currentScreen == MenuScreen::HostConfirm) ? "// HOST CO-OP LOBBY //"
            : ((currentScreen == MenuScreen::Join) ? "// JOIN MULTIPLAYER //"
            : ((currentScreen == MenuScreen::Settings) ? "// SETTINGS //" : "// CUSTOMIZATION //"))));
        int subW = MeasureText(subTitle, 28);
        float subTitleY = (currentScreen == MenuScreen::Customize)
            ? std::max(56.0f, centerY - 215.0f)
            : (centerY - 230.0f);
        DrawText(subTitle, static_cast<int>(centerX - subW * 0.5f + 2), static_cast<int>(subTitleY + 2), 28, Fade(BLACK, 0.85f));
        DrawText(subTitle, static_cast<int>(centerX - subW * 0.5f), static_cast<int>(subTitleY), 28, Colors::Amber400);
    }

    static bool joinInputActive = false;
    static bool hostPortActive = false;
    static bool nameInputActive = false;

    if (currentScreen == MenuScreen::Main) {
        float panelW = 340.0f;
        float panelH = 340.0f;
        float panelX = centerX - panelW * 0.5f;
        float panelY = centerY - 65.0f;

        Widgets::mindustryPanel({ panelX, panelY, panelW, panelH }, "MAIN DIRECTIVE", Colors::Amber500);

        float mBtnW = panelW - 36.0f;
        float mBtnH = 44.0f;
        float mBtnX = panelX + 18.0f;
        float curBtnY = panelY + 44.0f;
        float btnGap = 8.0f;

        if (Widgets::mindustryButton("CAMPAIGN", "CLEAR THE PLANET // SECTORS 01-04", { mBtnX, curBtnY, mBtnW, mBtnH }, Colors::Amber500, false, 18)) {
            currentScreen = MenuScreen::Campaign;
            statusMessage.clear();
            if (campaignManager) {
                int fIdx = campaignManager->activeSectorIndex;
                int sIdx = planetRenderer.getSectorIdxForFortress(fIdx);
                campaignSelectedSector = (sIdx >= 0) ? sIdx : 0;
            }
            planetRenderer.focusSector(campaignSelectedSector);
        }
        curBtnY += mBtnH + btnGap;

        if (Widgets::mindustryButton("CUSTOM GAME", "CUSTOM GRIDS // 2D - 4D FREE-PLAY", { mBtnX, curBtnY, mBtnW, mBtnH }, Colors::Cyan500, false, 18)) {
            currentScreen = MenuScreen::Play;
            statusMessage.clear();
        }
        curBtnY += mBtnH + btnGap;

        if (Widgets::mindustryButton("CUSTOMIZE", "SKINS & APPEARANCE", { mBtnX, curBtnY, mBtnW, mBtnH }, Colors::Green500, false, 18)) {
            currentScreen = MenuScreen::Customize;
            statusMessage.clear();
        }
        curBtnY += mBtnH + btnGap;

        if (Widgets::mindustryButton("SETTINGS", "SCALING & GRAPHICS", { mBtnX, curBtnY, mBtnW, mBtnH }, Colors::Zinc400, false, 18)) {
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
    else if (currentScreen == MenuScreen::Campaign) {
        float dt = GetFrameTime();
        planetRenderer.update(dt);

        Rectangle planetArea = { 0.0f, 0.0f, static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight()) };
        
        Rectangle topLeftBox = { 24.0f, 24.0f, 220.0f, 168.0f };
        Rectangle diffBox = { 24.0f, 202.0f, 220.0f, 58.0f };
        float rightPanelW = 310.0f;
        Rectangle rightPanel = { static_cast<float>(screenW) - rightPanelW - 24.0f, 24.0f, rightPanelW, static_cast<float>(screenH) - 48.0f };
        Rectangle backBtnRec = { 24.0f, static_cast<float>(screenH) - 64.0f, 110.0f, 40.0f };

        Vector2 uiMouse = Widgets::getUIMousePos();
#if defined(_DEBUG) || !defined(NDEBUG)
        Rectangle editorBtnRec = { 24.0f, 270.0f, 220.0f, 40.0f };
        bool overEditorBtn = CheckCollisionPointRec(uiMouse, editorBtnRec);
#else
        bool overEditorBtn = false;
#endif
        bool overUI = CheckCollisionPointRec(uiMouse, topLeftBox) ||
                      CheckCollisionPointRec(uiMouse, diffBox) ||
                      overEditorBtn ||
                      CheckCollisionPointRec(uiMouse, rightPanel) ||
                      CheckCollisionPointRec(uiMouse, backBtnRec) ||
                      showDifficultyModal;

        if (!overUI || planetRenderer.isDragging) {
            int clicked = planetRenderer.handleInput(planetArea, campaignHoveredSector);
            if (clicked >= 0) {
                campaignSelectedSector = clicked;
                audio::SoundManager::playButton();
            }
        } else {
            campaignHoveredSector = -1;
        }

        // 1. Exit 2D UI camera to render 3D low-poly faceted planet & starfield at 1:1 screen resolution
        EndMode2D();

        planetRenderer.drawGlobe(*campaignManager, campaignSelectedSector, campaignHoveredSector);
        planetRenderer.drawSectorOverlays(*campaignManager, campaignSelectedSector, campaignHoveredSector);

        // 2. Re-enter 2D UI camera so all HUD panels and buttons match Widgets::getUIMousePos() exactly
        Camera2D uiCam = { 0 };
        uiCam.zoom = Widgets::guiScale;
        BeginMode2D(uiCam);

        // -------------------------------------------------------------
        // 1. TOP-LEFT PLANET LIST PANEL (Planetary System Lore)
        // -------------------------------------------------------------
        DrawRectangleRec(topLeftBox, Fade(Colors::Zinc950, 0.90f));
        DrawRectangleLinesEx(topLeftBox, 1.5f, Colors::Zinc700);

        static std::vector<core::PlanetConfig> fallbackPlanets = core::CampaignManager::getDefaultPlanetConfigs();
        const auto& planetList = (campaignManager && !campaignManager->planets.empty()) ? campaignManager->planets : fallbackPlanets;
        int numPlanets = static_cast<int>(planetList.size());

        float entryH = 48.0f;
        for (int p = 0; p < numPlanets; ++p) {
            float entryY = topLeftBox.y + 6.0f + static_cast<float>(p) * (entryH + 4.0f);
            Rectangle eRec = { topLeftBox.x + 8.0f, entryY, topLeftBox.width - 16.0f, entryH };

            bool isSelectedPlanet = (p == currentPlanetIdx);

            if (isSelectedPlanet) {
                DrawRectangleRec(eRec, Fade(planetList[p].visual.iconColor, 0.15f));
                DrawRectangleLinesEx(eRec, 2.0f, planetList[p].visual.iconColor);
            }

            Vector2 iconPos = { eRec.x + 18.0f, eRec.y + eRec.height * 0.5f };
            DrawCircleV(iconPos, 10.0f, planetList[p].visual.iconColor);
            DrawCircleLines(static_cast<int>(iconPos.x), static_cast<int>(iconPos.y), 10, WHITE);
            DrawCircle(static_cast<int>(iconPos.x - 3), static_cast<int>(iconPos.y - 2), 2, Fade(BLACK, 0.45f));
            DrawCircle(static_cast<int>(iconPos.x + 2), static_cast<int>(iconPos.y + 3), 1.5f, Fade(BLACK, 0.45f));

            Color nameCol = planetList[p].isUnlocked ? WHITE : Colors::Zinc500;
            DrawText(planetList[p].name.c_str(), static_cast<int>(eRec.x + 36.0f), static_cast<int>(eRec.y + 8.0f), 15, nameCol);
            DrawText(planetList[p].tagline.c_str(), static_cast<int>(eRec.x + 36.0f), static_cast<int>(eRec.y + 26.0f), 9, Colors::Zinc400);

            if (!planetList[p].isUnlocked) {
                DrawText("[LOCKED]", static_cast<int>(eRec.x + eRec.width - 56.0f), static_cast<int>(eRec.y + 18.0f), 9, Colors::Zinc600);
            } else {
                bool eHover = CheckCollisionPointRec(uiMouse, eRec);
                if (eHover) {
                    if (!isSelectedPlanet) DrawRectangleLinesEx(eRec, 1.0f, Colors::Zinc500);
                    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        currentPlanetIdx = p;
                        if (campaignManager) {
                            campaignManager->selectPlanet(p);
                        }
                        planetRenderer.applyPlanetConfig(planetList[p]);
                        campaignSelectedSector = planetRenderer.getSectorIdxForFortress(0);
                        audio::SoundManager::playButton();
                    }
                }
            }
        }

        // -------------------------------------------------------------
        // 2. PLANET INTEL BOX
        // -------------------------------------------------------------
        DrawRectangleRec(diffBox, Fade(Colors::Zinc950, 0.90f));
        DrawRectangleLinesEx(diffBox, 1.5f, Colors::Zinc700);

        // Tactical sensor radar icon
        Vector2 radarC = { diffBox.x + 22.0f, diffBox.y + diffBox.height * 0.5f };
        DrawCircleLines(static_cast<int>(radarC.x), static_cast<int>(radarC.y), 11, Colors::Amber500);
        DrawCircle(static_cast<int>(radarC.x), static_cast<int>(radarC.y), 2, Colors::Amber400);
        DrawLine(static_cast<int>(radarC.x), static_cast<int>(radarC.y - 11), static_cast<int>(radarC.x), static_cast<int>(radarC.y + 11), Fade(Colors::Amber500, 0.5f));
        DrawLine(static_cast<int>(radarC.x - 11), static_cast<int>(radarC.y), static_cast<int>(radarC.x + 11), static_cast<int>(radarC.y), Fade(Colors::Amber500, 0.5f));

        DrawText("PLANET INTEL", static_cast<int>(diffBox.x + 42.0f), static_cast<int>(diffBox.y + 10.0f), 14, Colors::Amber400);
        DrawText("SECTOR TOPOLOGY & MANDATE", static_cast<int>(diffBox.x + 42.0f), static_cast<int>(diffBox.y + 27.0f), 10, Colors::Zinc400);
        DrawText("VIEW DOSSIER >", static_cast<int>(diffBox.x + 42.0f), static_cast<int>(diffBox.y + 41.0f), 10, Colors::Cyan400);

        bool diffHover = CheckCollisionPointRec(uiMouse, diffBox);
        if (diffHover) {
            DrawRectangleLinesEx(diffBox, 1.5f, Colors::Amber400);
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                showDifficultyModal = !showDifficultyModal;
                audio::SoundManager::playButton();
            }
        }

#if defined(_DEBUG) || !defined(NDEBUG)
        if (Widgets::mindustryButton("[F6] SECTOR EDITOR", "DEV TACTICAL SUITE", editorBtnRec, Colors::Amber500, false, 12)) {
            actions.toggleSectorEditor = true;
        }
#endif

        // -------------------------------------------------------------
        // 3. TOP-CENTER PLANETARY PROGRESS BAR
        // -------------------------------------------------------------
        float clearPct = campaignManager ? (campaignManager->planetClearPercentage * 100.0f) : 0.0f;
        float leftEdge = topLeftBox.x + topLeftBox.width;
        float centerGap = rightPanel.x - leftEdge;
        float topBarW = std::clamp(centerGap - 32.0f, 160.0f, 230.0f);
        float topBarH = 32.0f;
        float topBarX = leftEdge + (centerGap - topBarW) * 0.5f;
        float topBarY = 24.0f;

        DrawRectangleRec({ topBarX, topBarY, topBarW, topBarH }, Fade(Colors::Zinc950, 0.90f));
        DrawRectangleLinesEx({ topBarX, topBarY, topBarW, topBarH }, 1.5f, (clearPct >= 100.0f) ? Colors::Green400 : Colors::Amber500);

        if (clearPct > 0.0f) {
            float fillW = (topBarW - 4.0f) * std::clamp(clearPct / 100.0f, 0.0f, 1.0f);
            DrawRectangleRec({ topBarX + 2.0f, topBarY + topBarH - 4.0f, fillW, 2.0f }, Colors::Green400);
        }

        std::string pName = (campaignManager && campaignManager->getActivePlanetConfig()) ? campaignManager->getActivePlanetConfig()->name : "TARTARUS-IV";
        std::string pNameUpper = pName;
        for (auto& c : pNameUpper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

        const char* topBarText = TextFormat("%s // %.1f%% SECURED", pNameUpper.c_str(), clearPct);
        int tbTextW = MeasureText(topBarText, 11);
        DrawText(topBarText, static_cast<int>(topBarX + (topBarW - tbTextW) * 0.5f), static_cast<int>(topBarY + 9.0f), 11, (clearPct >= 100.0f) ? Colors::Green400 : Colors::Zinc200);

        // -------------------------------------------------------------
        // 4. RIGHT-SIDE SELECTED SECTOR DOSSIER PANEL
        // -------------------------------------------------------------
        Widgets::mindustryPanel(rightPanel, "SECTOR TACTICAL INTEL", Colors::Amber500);

        if (campaignSelectedSector < 0 || campaignSelectedSector >= static_cast<int>(planetRenderer.sectors.size())) {
            campaignSelectedSector = 0;
        }

        const auto& hexSec = planetRenderer.sectors[campaignSelectedSector];
        bool isFortress = hexSec.isFortress;
        int fortressIdx = hexSec.fortressIdx;

        const core::CampaignSector* sec = (isFortress && campaignManager) ? campaignManager->getSectorByIndex(fortressIdx) : nullptr;
        bool isSecured = sec ? sec->isCleared : false;
        bool isHostile = sec ? (sec->isUnlocked && !sec->isCleared) : (!isFortress ? false : (fortressIdx == 0));
        bool isLocked = sec ? !sec->isUnlocked : false;

        float rContentX = rightPanel.x + 20.0f;
        float curY = rightPanel.y + 48.0f;

        // Sector Codename & Status Badge
        const char* codeStr = hexSec.codename.c_str();
        Color codeCol = isFortress ? (isSecured ? Colors::Green400 : (isHostile ? Colors::Red400 : Colors::Zinc400)) : Colors::Cyan400;
        DrawText(codeStr, static_cast<int>(rContentX), static_cast<int>(curY), 20, codeCol);

        const char* badgeStr = isFortress ? (isSecured ? "[ SECURED ]" : (isHostile ? "[ ACTIVE THREAT ]" : "[ LOCKED ]")) : "[ TERRITORIAL HEX ]";
        Color badgeCol = isFortress ? (isSecured ? Colors::Green400 : (isHostile ? Colors::Red400 : Colors::Zinc600)) : Colors::Cyan400;
        int badgeW = MeasureText(badgeStr, 12);
        DrawText(badgeStr, static_cast<int>(rightPanel.x + rightPanel.width - 20.0f - badgeW), static_cast<int>(curY + 4.0f), 12, badgeCol);
        curY += 28.0f;

        // Sector Name
        const char* sTitle = hexSec.name.c_str();
        DrawText(sTitle, static_cast<int>(rContentX), static_cast<int>(curY), 16, WHITE);
        curY += 24.0f;

        // Subtitle / Environment type
        const char* subStr = hexSec.subtitle.c_str();
        DrawText(subStr, static_cast<int>(rContentX), static_cast<int>(curY), 12, Colors::Zinc400);
        curY += 20.0f;

        // Horizontal divider
        DrawLineEx({ rContentX, curY }, { rightPanel.x + rightPanel.width - 20.0f, curY }, 1.0f, Colors::Zinc800);
        curY += 12.0f;

        // Tactical Briefing lore text (word-wrapped)
        std::string briefingStr;
        if (sec) {
            briefingStr = sec->loreBriefing;
        } else {
            const auto* activePlanet = campaignManager ? campaignManager->getActivePlanetConfig() : nullptr;
            std::string curPName = activePlanet ? activePlanet->name : "Tartarus-IV";
            std::string geoStat = activePlanet ? activePlanet->visual.geologicalStatus : "Volcanic Basalt & Obsidian Crust";
            briefingStr = "Unoccupied territorial sector on " + curPName + ". Geodesic dual facet with " + geoStat + ". Perimeter walls protect adjacent fortress zones.";
        }

        float maxLoreW = rightPanel.width - 40.0f;
        std::vector<std::string> words;
        std::string curWord;
        for (char ch : briefingStr) {
            if (ch == ' ') {
                if (!curWord.empty()) { words.push_back(curWord); curWord.clear(); }
            } else {
                curWord += ch;
            }
        }
        if (!curWord.empty()) words.push_back(curWord);

        std::string curLine;
        for (const auto& w : words) {
            std::string testLine = curLine.empty() ? w : (curLine + " " + w);
            if (MeasureText(testLine.c_str(), 11) > maxLoreW) {
                DrawText(curLine.c_str(), static_cast<int>(rContentX), static_cast<int>(curY), 11, Colors::Zinc300);
                curY += 15.0f;
                curLine = w;
            } else {
                curLine = testLine;
            }
        }
        if (!curLine.empty()) {
            DrawText(curLine.c_str(), static_cast<int>(rContentX), static_cast<int>(curY), 11, Colors::Zinc300);
            curY += 15.0f;
        }
        curY += 8.0f;

        // Specs Grid Box
        float boxW = rightPanel.width - 40.0f;
        float boxH = 82.0f;
        DrawRectangleRec({ rContentX, curY, boxW, boxH }, Fade(Colors::Zinc900, 0.70f));
        DrawRectangleLinesEx({ rContentX, curY, boxW, boxH }, 1.0f, Colors::Zinc800);

        if (sec) {
            int gSize = sec->gridSize;
            int bCount = sec->bombCount;
            DrawText("FORTRESS MINEFIELD SPECIFICATIONS", static_cast<int>(rContentX + 10.0f), static_cast<int>(curY + 8.0f), 10, Colors::Amber400);
            DrawText(TextFormat("Grid Dimension: %dx%d (%d cells)", gSize, gSize, gSize * gSize), static_cast<int>(rContentX + 10.0f), static_cast<int>(curY + 24.0f), 12, Colors::Zinc200);
            DrawText(TextFormat("Threat Density: %d Subterranean Bombs", bCount), static_cast<int>(rContentX + 10.0f), static_cast<int>(curY + 42.0f), 12, Colors::Zinc300);
            DrawText(TextFormat("Clearance Progress: %llu / %llu Safe Cells", sec->board.revealedCount, sec->board.safeCells()), static_cast<int>(rContentX + 10.0f), static_cast<int>(curY + 62.0f), 12, (isSecured ? Colors::Green400 : Colors::Amber300));
        } else {
            DrawText("TERRITORIAL SECTOR SPECIFICATIONS", static_cast<int>(rContentX + 10.0f), static_cast<int>(curY + 8.0f), 10, Colors::Cyan400);
            DrawText(TextFormat("Topology: %zu-Sided Geodesic Facet", hexSec.corners.size()), static_cast<int>(rContentX + 10.0f), static_cast<int>(curY + 24.0f), 12, Colors::Zinc200);
            const auto* activePlanet = campaignManager ? campaignManager->getActivePlanetConfig() : nullptr;
            std::string geoStat = activePlanet ? activePlanet->visual.geologicalStatus : "Volcanic Basalt & Obsidian Crust";
            DrawText(TextFormat("Geological Status: %s", geoStat.c_str()), static_cast<int>(rContentX + 10.0f), static_cast<int>(curY + 42.0f), 12, Colors::Zinc300);
            DrawText("Perimeter Barrier: Heavy Encased Forcefield Wall", static_cast<int>(rContentX + 10.0f), static_cast<int>(curY + 62.0f), 12, Colors::Zinc400);
        }
        curY += boxH + 14.0f;

        // Quick Fortress Selector Steppers (< PREV / NEXT >)
        float navW = (boxW - 10.0f) * 0.5f;
        float navH = 28.0f;
        int activeFIdx = isFortress ? fortressIdx : -1;
        int numF = campaignManager ? static_cast<int>(campaignManager->sectors.size()) : 4;
        if (Widgets::button("< PREV FORTRESS", { rContentX, curY, navW, navH }, Colors::Zinc800, Colors::Zinc700, (activeFIdx == 0), 12)) {
            int prevF = (activeFIdx <= 0) ? (numF - 1) : (activeFIdx - 1);
            int sIdx = planetRenderer.getSectorIdxForFortress(prevF);
            if (sIdx >= 0) {
                campaignSelectedSector = sIdx;
                planetRenderer.focusSector(campaignSelectedSector);
            }
        }
        if (Widgets::button("NEXT FORTRESS >", { rContentX + navW + 10.0f, curY, navW, navH }, Colors::Zinc800, Colors::Zinc700, (activeFIdx == numF - 1), 12)) {
            int nextF = (activeFIdx < 0) ? 0 : (activeFIdx + 1);
            if (nextF >= numF) nextF = 0;
            int sIdx = planetRenderer.getSectorIdxForFortress(nextF);
            if (sIdx >= 0) {
                campaignSelectedSector = sIdx;
                planetRenderer.focusSector(campaignSelectedSector);
            }
        }
        curY += navH + 18.0f;

        // Action Launch Buttons
        float actBtnH = 40.0f;
        float actGap = 8.0f;

        if (isFortress) {
            // DEPLOY SOLO
            bool canLaunch = !isLocked;
            if (Widgets::mindustryButton(isSecured ? "RE-ENTER SECTOR" : "DEPLOY SOLO", canLaunch ? "LAUNCH SHIP INTO SECTOR" : "REQUIRES CLEARANCE", { rContentX, curY, boxW, actBtnH }, Colors::Green500, !canLaunch, 16)) {
                if (canLaunch) {
                    if (campaignManager) {
                        campaignManager->activeSectorIndex = fortressIdx;
                    }
                    actions.playCampaignSolo = true;
                }
            }
            curY += actBtnH + actGap;

            // HOST CO-OP
            if (Widgets::mindustryButton("HOST CO-OP", canLaunch ? "LAUNCH HOST WITH STEAM LOBBY" : "REQUIRES CLEARANCE", { rContentX, curY, boxW, actBtnH }, Colors::Amber500, !canLaunch, 16)) {
                if (canLaunch) {
                    if (campaignManager) {
                        campaignManager->activeSectorIndex = fortressIdx;
                    }
                    actions.playCampaignHost = true;
                }
            }
            curY += actBtnH + actGap;

            // JOIN CO-OP
            if (Widgets::mindustryButton("JOIN CO-OP", "ENTER REMOTE HOST SERVER", { rContentX, curY, boxW, actBtnH }, Colors::Cyan500, false, 16)) {
                currentScreen = MenuScreen::Join;
            }
            curY += actBtnH + actGap;
        } else {
            // Jump to nearest active campaign fortress
            int targetF = campaignManager ? campaignManager->activeSectorIndex : 0;
            int sIdx = planetRenderer.getSectorIdxForFortress(targetF);
            if (Widgets::mindustryButton("TARGET PRIMARY FORTRESS", "FOCUS ACTIVE CAMPAIGN FORTRESS", { rContentX, curY, boxW, actBtnH }, Colors::Amber500, false, 16)) {
                if (sIdx >= 0) {
                    campaignSelectedSector = sIdx;
                    planetRenderer.focusSector(campaignSelectedSector);
                }
            }
            curY += actBtnH + actGap;

            // Seismic Sensor ping
            if (Widgets::mindustryButton("SEISMIC SCAN", "SURVEY PERIMETER TECTONICS", { rContentX, curY, boxW, actBtnH }, Colors::Cyan500, false, 16)) {
                audio::SoundManager::playIncrement();
            }
            curY += actBtnH + actGap;

            // JOIN CO-OP
            if (Widgets::mindustryButton("JOIN CO-OP", "ENTER REMOTE HOST SERVER", { rContentX, curY, boxW, actBtnH }, Colors::Cyan500, false, 16)) {
                currentScreen = MenuScreen::Join;
            }
            curY += actBtnH + actGap;
        }

        // RESET CAMPAIGN
        if (confirmingResetCampaign) {
            if (Widgets::button("CONFIRM RESET ALL PROGRESS?", { rContentX, curY, boxW, 32.0f }, Colors::Red500, Colors::Red600, false, 12)) {
                actions.resetCampaign = true;
                confirmingResetCampaign = 0;
            }
        } else {
            if (Widgets::button("RESET CAMPAIGN", { rContentX, curY, boxW, 32.0f }, Colors::Zinc900, Colors::Zinc700, false, 12)) {
                confirmingResetCampaign = 1;
            }
        }

        // -------------------------------------------------------------
        // 5. BOTTOM-LEFT BACK BUTTON
        // -------------------------------------------------------------
        if (Widgets::button("< BACK", backBtnRec, Colors::Zinc800, Colors::Zinc600, false, 14)) {
            currentScreen = MenuScreen::Main;
            confirmingResetCampaign = 0;
            showDifficultyModal = false;
        }

        // -------------------------------------------------------------
        // 6. PLANET INTEL MODAL OVERLAY (if active)
        // -------------------------------------------------------------
        if (showDifficultyModal) {
            float mWidth = 460.0f;
            float mHeight = 270.0f;
            float mX = (screenW - mWidth) * 0.5f;
            float mY = (screenH - mHeight) * 0.5f;

            DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.65f));
            const auto* activePlanet = campaignManager ? campaignManager->getActivePlanetConfig() : nullptr;
            std::string pTitle = activePlanet ? ("PLANETARY INTEL // " + activePlanet->name) : "PLANETARY INTEL // TARTARUS-IV";
            for (auto& c : pTitle) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            Widgets::mindustryPanel({ mX, mY, mWidth, mHeight }, pTitle.c_str(), Colors::Amber500);

            std::string directiveStr = activePlanet ? ("EXPEDITION MANDATE: " + activePlanet->missionDirective) : "EXPEDITION MANDATE: QUARANTINE CLEANSING";
            DrawText(directiveStr.c_str(), static_cast<int>(mX + 20.0f), static_cast<int>(mY + 44.0f), 12, Colors::Amber400);

            float dossierY = mY + 66.0f;
            if (activePlanet && !activePlanet->intelDossier.empty()) {
                for (const auto& line : activePlanet->intelDossier) {
                    DrawText(TextFormat("• %s", line.c_str()), static_cast<int>(mX + 20.0f), static_cast<int>(dossierY), 11, Colors::Zinc200);
                    dossierY += 18.0f;
                }
            } else {
                DrawText("• Surface Topology: Dual Geodesic Hexagonal Lattice (42 Sectors)", static_cast<int>(mX + 20.0f), static_cast<int>(dossierY), 11, Colors::Zinc200);
                dossierY += 18.0f;
                DrawText("• 4 Strategic Fortress Hubs with Subterranean Minefields", static_cast<int>(mX + 20.0f), static_cast<int>(dossierY), 11, Colors::Zinc200);
                dossierY += 18.0f;
                DrawText("• 38 Territorial Hexes Encased in Heavy Basalt Barrier Walls", static_cast<int>(mX + 20.0f), static_cast<int>(dossierY), 11, Colors::Zinc200);
                dossierY += 18.0f;
                DrawText("• Clear All 4 Primary Fortresses to Secure the Planet", static_cast<int>(mX + 20.0f), static_cast<int>(dossierY), 11, Colors::Green400);
                dossierY += 18.0f;
            }

            if (Widgets::button("CLOSE INTEL", { mX + (mWidth - 120.0f) * 0.5f, mY + mHeight - 40.0f, 120.0f, 30.0f }, Colors::Zinc800, Colors::Zinc700, false, 13)) {
                showDifficultyModal = false;
            }
        }
    }
    else if (currentScreen == MenuScreen::Play) {
        float panelW = 680.0f;
        float panelH = 430.0f;
        float panelX = centerX - panelW * 0.5f;
        float panelY = centerY - 160.0f;

        Widgets::mindustryPanel({ panelX, panelY, panelW, panelH }, "SAVED WORLDS", Colors::Amber500);

        std::vector<core::SaveSlotMetadata> slots;
        if (saveManager) {
            slots = saveManager->getSlots();
        } else {
            slots.resize(core::SaveManager::NUM_SLOTS);
            for (size_t i = 0; i < slots.size(); ++i) {
                slots[i].slotIndex = static_cast<int>(i + 1);
                slots[i].slotName = "Slot " + std::to_string(i + 1);
            }
        }

        float cardW = panelW - 36.0f;
        float cardH = 82.0f;
        float cardX = panelX + 18.0f;
        float startCardY = panelY + 44.0f;

        for (size_t i = 0; i < slots.size(); ++i) {
            const auto& sl = slots[i];
            float cardY = startCardY + static_cast<float>(i) * (cardH + 10.0f);
            Rectangle cardRect = { cardX, cardY, cardW, cardH };

            if (!sl.isEmpty) {
                DrawRectangleRec(cardRect, Colors::Zinc900);
                DrawRectangleLinesEx(cardRect, 1.0f, (selectedSlot == sl.slotIndex) ? Colors::Amber500 : Colors::Zinc700);

                // Slot Title & Name
                const char* titleStr = TextFormat("[SLOT %d]  %s", sl.slotIndex, sl.slotName.c_str());
                DrawText(titleStr, static_cast<int>(cardX + 14), static_cast<int>(cardY + 10), 16, Colors::Zinc200);

                // Dimensions, size, bombs, seed
                const char* dimStr = (sl.dim == 2) ? "2D" : ((sl.dim == 3) ? "3D" : "4D");
                const char* specStr = TextFormat("%s • %dx%d • %d MINES • SEED %llu", dimStr, sl.size, sl.size, sl.bombs, sl.seed);
                DrawText(specStr, static_cast<int>(cardX + 14), static_cast<int>(cardY + 32), 12, Colors::Zinc400);

                // Progression stats & scrap
                float clearPct = (sl.totalCells > static_cast<size_t>(sl.bombs))
                    ? (static_cast<float>(sl.revealedCount) / static_cast<float>(sl.totalCells - static_cast<size_t>(sl.bombs)) * 100.0f)
                    : 0.0f;
                const char* statusStr = sl.isVictory ? "STATUS: VICTORY"
                    : (sl.isGameOver ? "STATUS: DEFEAT"
                    : TextFormat("CLEARED: %.0f%% (%llu/%llu)", clearPct, sl.revealedCount, sl.totalCells - sl.bombs));
                Color statCol = sl.isVictory ? Colors::Green400 : (sl.isGameOver ? Colors::Red500 : Colors::Cyan400);
                DrawText(statusStr, static_cast<int>(cardX + 14), static_cast<int>(cardY + 54), 13, statCol);

                int sw = MeasureText(statusStr, 13);
                const char* scrapTimeStr = TextFormat("SCRAP: %llu   TIME: %02d:%02d", sl.scrapCount, static_cast<int>(sl.timePlayed) / 60, static_cast<int>(sl.timePlayed) % 60);
                DrawText(scrapTimeStr, static_cast<int>(cardX + 26 + sw), static_cast<int>(cardY + 54), 13, Colors::Amber400);

                // Action buttons on right side
                float rightBtnY = cardY + 24.0f;
                float soloW = 72.0f;
                float hostW = 72.0f;
                float delW = 50.0f;
                float curBtnX = cardX + cardW - 12.0f - delW;

                // Delete / Confirm button
                if (confirmingDeleteSlot == sl.slotIndex) {
                    float confirmW = 90.0f;
                    curBtnX = cardX + cardW - 12.0f - confirmW;
                    if (Widgets::button("CONFIRM?", { curBtnX, rightBtnY - 6.0f, confirmW, 34.0f }, Colors::Red500, Colors::Red600, false, 13)) {
                        if (saveManager) {
                            saveManager->deleteSlot(sl.slotIndex);
                        }
                        confirmingDeleteSlot = 0;
                    }
                } else {
                    if (Widgets::button("DEL", { curBtnX, rightBtnY - 6.0f, delW, 32.0f }, Colors::Zinc800, Colors::Zinc600, false, 12)) {
                        confirmingDeleteSlot = sl.slotIndex;
                    }

                    curBtnX -= hostW + 8.0f;
                    if (Widgets::button("HOST", { curBtnX, rightBtnY - 6.0f, hostW, 32.0f }, Colors::Amber500, Colors::Amber400, false, 13)) {
                        selectedSlot = sl.slotIndex;
                        currentScreen = MenuScreen::HostConfirm;
                        confirmingDeleteSlot = 0;
                    }

                    curBtnX -= soloW + 8.0f;
                    if (Widgets::button("SOLO", { curBtnX, rightBtnY - 6.0f, soloW, 32.0f }, Colors::Green500, Colors::Green400, false, 13)) {
                        selectedSlot = sl.slotIndex;
                        actions.selectedSlot = sl.slotIndex;
                        actions.playSolo = true;
                        confirmingDeleteSlot = 0;
                    }
                }
            } else {
                DrawRectangleRec(cardRect, Fade(Colors::Zinc900, 0.45f));
                DrawRectangleLinesEx(cardRect, 1.0f, Fade(Colors::Zinc700, 0.45f));

                const char* emptyTitle = TextFormat("[SLOT %d]  EMPTY SAVE FILE", sl.slotIndex);
                DrawText(emptyTitle, static_cast<int>(cardX + 14), static_cast<int>(cardY + 18), 16, Colors::Zinc500);

                const char* emptySub = "No mission data recorded. Ready for fresh solo or co-op expedition.";
                DrawText(emptySub, static_cast<int>(cardX + 14), static_cast<int>(cardY + 44), 13, Colors::Zinc600);

                float newBtnW = 120.0f;
                float newBtnH = 38.0f;
                float newBtnX = cardX + cardW - newBtnW - 14.0f;
                float newBtnY = cardY + 22.0f;
                if (Widgets::button("+ NEW GAME", { newBtnX, newBtnY, newBtnW, newBtnH }, Colors::Cyan500, Colors::Cyan400, false, 13)) {
                    selectedSlot = sl.slotIndex;
                    std::snprintf(newSaveNameBuf, sizeof(newSaveNameBuf), "World %d", selectedSlot);
                    newSaveDim = 2;
                    newSaveSize = 10;
                    newSaveBombs = 15;
                    newSaveSeed = (static_cast<uint64_t>(GetTime() * 100000.0) ^ 0x9E3779ULL) % 100000000ULL;
                    if (newSaveSeed == 0) newSaveSeed = 12345;
                    std::snprintf(newSaveSeedBuf, sizeof(newSaveSeedBuf), "%llu", newSaveSeed);
                    newSaveNameActive = false;
                    newSaveSeedActive = false;
                    currentScreen = MenuScreen::NewSave;
                    confirmingDeleteSlot = 0;
                }
            }
        }

        // Bottom bar buttons
        float bBtnY = panelY + panelH - 52.0f;
        if (Widgets::mindustryButton("JOIN MULTIPLAYER", "CONNECT TO HOST VIA IP", { cardX, bBtnY, 220.0f, 40.0f }, Colors::Cyan500, false, 14)) {
            currentScreen = MenuScreen::Join;
            statusMessage.clear();
            confirmingDeleteSlot = 0;
        }

        if (Widgets::mindustryButton("BACK", nullptr, { cardX + cardW - 110.0f, bBtnY, 110.0f, 40.0f }, Colors::Zinc600, false, 14) || IsKeyPressed(KEY_ESCAPE)) {
            currentScreen = MenuScreen::Main;
            statusMessage.clear();
            confirmingDeleteSlot = 0;
        }
    }
    else if (currentScreen == MenuScreen::NewSave) {
        float panelW = 480.0f;
        float panelH = 370.0f;
        float panelX = centerX - panelW * 0.5f;
        float panelY = centerY - 150.0f;

        const char* pTitle = TextFormat("NEW SAVE - SLOT %d", selectedSlot);
        Widgets::mindustryPanel({ panelX, panelY, panelW, panelH }, pTitle, Colors::Amber500);

        float curY = panelY + 46.0f;
        float curX = panelX + 24.0f;

        // Slot Name input
        DrawText("SAVE NAME:", static_cast<int>(curX), static_cast<int>(curY + 6), 14, Colors::Zinc400);
        Widgets::textInput({ curX + 100.0f, curY, 320.0f, 32.0f }, newSaveNameBuf, sizeof(newSaveNameBuf), newSaveNameActive, "World 1");
        curY += 46.0f;

        // Spinners: DIM, SIZE, BOMBS
        Widgets::spinner("DIM", { curX, curY }, newSaveDim, 2, 4, false, 36);
        Widgets::spinner("SIZE", { curX + 140.0f, curY }, newSaveSize, 4, 200, false, 40);

        uint64_t maxCells = 1;
        for (int d = 0; d < newSaveDim; ++d) maxCells *= newSaveSize;
        int maxAllowedBombs = static_cast<int>(std::min<uint64_t>(maxCells - 1, 100000));
        newSaveBombs = std::clamp(newSaveBombs, 1, maxAllowedBombs);

        Widgets::spinner("BOMBS", { curX + 280.0f, curY }, newSaveBombs, 1, maxAllowedBombs, false, 56);
        curY += 48.0f;

        // Density stats
        float density = (maxCells > 0) ? (static_cast<float>(newSaveBombs) / static_cast<float>(maxCells) * 100.0f) : 0.0f;
        const char* densityStr = TextFormat("(%llu cells, %.1f%% mines)", maxCells, density);
        DrawText(densityStr, static_cast<int>(curX + 6), static_cast<int>(curY), 13, Colors::Zinc500);
        curY += 26.0f;

        // Seed input & Reroll button
        DrawText("SEED:", static_cast<int>(curX), static_cast<int>(curY + 6), 14, Colors::Zinc400);
        Widgets::textInput({ curX + 54.0f, curY, 260.0f, 32.0f }, newSaveSeedBuf, sizeof(newSaveSeedBuf), newSaveSeedActive, "12345");
        if (Widgets::button("REROLL", { curX + 324.0f, curY, 96.0f, 32.0f }, Colors::Zinc800, Colors::Zinc600, false, 13)) {
            newSaveSeed = (static_cast<uint64_t>(GetTime() * 100000.0) ^ 0x9E3779ULL) % 100000000ULL;
            if (newSaveSeed == 0) newSaveSeed = 12345;
            std::snprintf(newSaveSeedBuf, sizeof(newSaveSeedBuf), "%llu", newSaveSeed);
        }
        newSaveSeed = std::strtoull(newSaveSeedBuf, nullptr, 10);
        if (newSaveSeed == 0) newSaveSeed = 12345;
        curY += 56.0f;

        // Action buttons
        float bW = 202.0f;
        if (Widgets::mindustryButton("CREATE & SOLO", "START LOCAL GAME", { curX, curY, bW, 44.0f }, Colors::Green500, false, 15)) {
            actions.selectedSlot = selectedSlot;
            actions.startNewInSlot = true;
            actions.newSlotName = (newSaveNameBuf[0] != '\0') ? newSaveNameBuf : TextFormat("World %d", selectedSlot);
            actions.newSlotConfig = { newSaveDim, newSaveSize, newSaveBombs, newSaveSeed };
            actions.playSolo = true;
        }

        if (Widgets::mindustryButton("CREATE & HOST", "START CO-OP LOBBY", { curX + bW + 20.0f, curY, bW, 44.0f }, Colors::Amber500, false, 15)) {
            actions.selectedSlot = selectedSlot;
            actions.startNewInSlot = true;
            actions.newSlotName = (newSaveNameBuf[0] != '\0') ? newSaveNameBuf : TextFormat("World %d", selectedSlot);
            actions.newSlotConfig = { newSaveDim, newSaveSize, newSaveBombs, newSaveSeed };
            currentScreen = MenuScreen::HostConfirm;
        }

        if (Widgets::button("CANCEL", { curX + panelW - 48.0f - 80.0f, panelY + panelH - 34.0f, 80.0f, 26.0f }, Colors::Zinc800, Colors::Zinc600, false, 12) || IsKeyPressed(KEY_ESCAPE)) {
            currentScreen = MenuScreen::Play;
            newSaveNameActive = false;
            newSaveSeedActive = false;
        }
    }
    else if (currentScreen == MenuScreen::HostConfirm) {
        float panelW = 420.0f;
        float panelH = 260.0f;
        float panelX = centerX - panelW * 0.5f;
        float panelY = centerY - 85.0f;

        Widgets::mindustryPanel({ panelX, panelY, panelW, panelH }, "HOST CO-OP LOBBY", Colors::Amber500);

        const char* slotPrompt = TextFormat("HOSTING SAVE: SLOT %d", selectedSlot);
        int spW = MeasureText(slotPrompt, 15);
        DrawText(slotPrompt, static_cast<int>(centerX - spW * 0.5f), static_cast<int>(panelY + 48), 15, Colors::Amber400);

        const char* prompt = "HOST PORT:";
        int pW = MeasureText(prompt, 14);
        DrawText(prompt, static_cast<int>(centerX - pW * 0.5f), static_cast<int>(panelY + 76), 14, Colors::Zinc400);

        Widgets::textInput({ centerX - 110, panelY + 98, 220, 38 }, hostPortBuf, sizeof(hostPortBuf), hostPortActive, "7777");

        float btnW = panelW - 48.0f;
        float btnX = panelX + 24.0f;
        if (Widgets::mindustryButton("START SERVER", "BIND PORT & BEGIN HOSTING", { btnX, panelY + 146, btnW, 44 }, Colors::Green500, false, 16)) {
            actions.selectedSlot = selectedSlot;
            actions.hostPort = static_cast<uint16_t>(std::atoi(hostPortBuf));
            if (actions.hostPort == 0) actions.hostPort = 7777;
            actions.hostGame = true;
        }
        if (Widgets::mindustryButton("BACK", nullptr, { btnX, panelY + 198, btnW, 36 }, Colors::Zinc600, false, 14) || IsKeyPressed(KEY_ESCAPE)) {
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
        float subTitleY = std::max(56.0f, centerY - 215.0f);
        float startY = subTitleY + 48.0f;

        // Player Name Bar
        const char* prompt = "PLAYER NAME:";
        int pW = MeasureText(prompt, 15);
        DrawText(prompt, static_cast<int>(centerX - pW * 0.5f), static_cast<int>(startY), 15, Colors::Zinc400);
        Widgets::textInput({ centerX - 140, startY + 22, 280, 34 }, playerName, sizeof(playerName), nameInputActive, "Player");

        // HORIZONTAL REEL CONTAINER (Cursors & Flags)
        float reelContainerW = std::min(580.0f, static_cast<float>(screenW) - 40.0f);
        float reelContainerH = 118.0f;
        float reelContainerX = centerX - reelContainerW * 0.5f;

        // CATEGORY TABS (CURSORS, FLAGS)
        float tabW = std::min(180.0f, (reelContainerW - 16.0f) * 0.5f);
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
            { CustomizeTab::Cursors, "SHIPS", render::RaylibRenderer::getCursorSkinCount() },
            { CustomizeTab::Flags,   "FLAGS", render::RaylibRenderer::getFlagSkinCount() }
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

        Widgets::beginScissor(vpX, vpY, vpW, vpH);

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
                render::RaylibRenderer::drawCursorSkin(static_cast<uint8_t>(i), iconCenter, 0.0f, isEquipped ? Colors::Green500 : WHITE, nullptr, 1.8f);
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

        Widgets::endScissor();

        // LIVE SHOWCASE PREVIEW BOX
        float previewBoxW = std::min(420.0f, reelContainerW);
        float previewBoxH = 64.0f;
        float previewBoxX = centerX - previewBoxW * 0.5f;
        float previewBoxY = reelContainerY + reelContainerH + 14.0f;

        DrawRectangleRec({ previewBoxX, previewBoxY, previewBoxW, previewBoxH }, Colors::Zinc900);
        DrawRectangleLinesEx({ previewBoxX, previewBoxY, previewBoxW, previewBoxH }, 1.5f, Colors::Zinc800);

        DrawText("SHOWCASE PREVIEW", static_cast<int>(previewBoxX + 14), static_cast<int>(previewBoxY + 8), 11, Colors::Zinc500);

        if (activeTab == CustomizeTab::Cursors) {
            Vector2 pCenter = { previewBoxX + previewBoxW * 0.5f - 40.0f, previewBoxY + 28.0f };
            render::RaylibRenderer::drawCursorSkin(static_cast<uint8_t>(cursorSkin), pCenter, 0.0f, Colors::Green500, playerName, 1.8f);
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
        float doneBtnW = std::min(240.0f, reelContainerW);
        float doneBtnH = 44.0f;
        float doneBtnX = centerX - doneBtnW * 0.5f;
        float doneBtnY = previewBoxY + previewBoxH + 18.0f;

        if (Widgets::mindustryButton("DONE", "SAVE & RETURN", { doneBtnX, doneBtnY, doneBtnW, doneBtnH }, Colors::Green500, false, 18) || IsKeyPressed(KEY_ESCAPE)) {
            currentScreen = MenuScreen::Main;
            nameInputActive = false;
        }
    }
    else if (currentScreen == MenuScreen::Settings) {
        float panelW = 620.0f;
        float panelH = 500.0f;
        float panelX = centerX - panelW * 0.5f;
        float panelY = centerY - 250.0f;

        Widgets::mindustryPanel({ panelX, panelY, panelW, panelH }, "SYSTEM SETTINGS", Colors::Cyan500);

        float rowX = panelX + 24.0f;
        float rowW = panelW - 48.0f;

        // Top Tabs: [ GRAPHICS ], [ AUDIO ], and [ CONTROLS ]
        float tabW = (rowW - 16.0f) / 3.0f;
        float tabH = 36.0f;
        float tabY = panelY + 44.0f;

        bool isGfx = (activeSettingsTab == SettingsTab::Graphics);
        bool isAud = (activeSettingsTab == SettingsTab::Audio);
        bool isCtr = (activeSettingsTab == SettingsTab::Controls);

        if (Widgets::button("GRAPHICS", { rowX, tabY, tabW, tabH }, isGfx ? Colors::Cyan500 : Colors::Zinc800, Colors::Cyan400, false, 15)) {
            activeSettingsTab = SettingsTab::Graphics;
        }
        if (Widgets::button("AUDIO", { rowX + tabW + 8.0f, tabY, tabW, tabH }, isAud ? Colors::Cyan500 : Colors::Zinc800, Colors::Cyan400, false, 15)) {
            activeSettingsTab = SettingsTab::Audio;
        }
        if (Widgets::button("CONTROLS", { rowX + (tabW + 8.0f) * 2.0f, tabY, tabW, tabH }, isCtr ? Colors::Cyan500 : Colors::Zinc800, Colors::Cyan400, false, 15)) {
            activeSettingsTab = SettingsTab::Controls;
        }

        float curY = tabY + tabH + 16.0f;

        if (activeSettingsTab == SettingsTab::Graphics) {
            // 1. CRT Shader
            if (Widgets::checkbox("ENABLE RETRO CRT SHADER", { rowX, curY }, crtEnabled, false)) {
                actions.toggleCRT = true;
            }
            curY += 26.0f;
            DrawText("Simulates authentic curved cathode-ray tube phosphor scanlines and vignette.", static_cast<int>(rowX + 28.0f), static_cast<int>(curY), 12, Colors::Zinc500);
            curY += 22.0f;

            // 2. VSync
            if (Widgets::checkbox("VERTICAL SYNC (VSYNC)", { rowX, curY }, vsyncEnabled, false)) {
                actions.vsyncChanged = true;
            }
            curY += 26.0f;
            DrawText("Synchronizes frame presentation to your monitor's refresh rate to eliminate tearing.", static_cast<int>(rowX + 28.0f), static_cast<int>(curY), 12, Colors::Zinc500);
            curY += 22.0f;

            // 3. Show FPS
            Widgets::checkbox("SHOW FPS COUNTER", { rowX, curY }, showFPS, false);
            curY += 26.0f;
            DrawText("Displays a live real-time frames-per-second performance overlay.", static_cast<int>(rowX + 28.0f), static_cast<int>(curY), 12, Colors::Zinc500);
            curY += 24.0f;

            // 4. Frame Rate Limit Slider
            Rectangle fpsRect = { rowX, curY, rowW, 24.0f };
            const char* fpsCustom = (fpsLimit >= 245.0f) ? "NO LIMIT" : TextFormat("%d FPS", static_cast<int>(fpsLimit));
            if (Widgets::slider("FRAME RATE LIMIT", fpsRect, fpsLimit, 30.0f, 250.0f, 170, "%.0f", false, fpsCustom)) {
                actions.fpsLimitChanged = true;
            }
            curY += 32.0f;

            // 5. GUI Scale Slider
            Rectangle scaleRect = { rowX, curY, rowW, 24.0f };
            if (Widgets::slider("GUI SCALE", scaleRect, guiScale, 0.75f, 1.50f, 170, "%.0f%%", true)) {
                actions.guiScaleChanged = true;
            }
            curY += 26.0f;
            DrawText("Adjusts interface scale and menu element size (75% to 150%).", static_cast<int>(rowX + 28.0f), static_cast<int>(curY), 12, Colors::Zinc500);
            curY += 24.0f;

            DrawLineEx({ rowX, curY }, { rowX + rowW, curY }, 1.0f, Colors::PanelBorder);
            curY += 12.0f;

            DrawText("TOGGLE FULLSCREEN", static_cast<int>(rowX), static_cast<int>(curY), 13, Colors::Zinc400);
            int valW = MeasureText("KEYBOARD [ F11 ]", 13);
            DrawText("KEYBOARD [ F11 ]", static_cast<int>(rowX + rowW - valW), static_cast<int>(curY), 13, Colors::Cyan400);
        }
        else if (activeSettingsTab == SettingsTab::Audio) {
            // 1. Background Music / Ambience
            Widgets::checkbox("ENABLE BACKGROUND SOUNDS", { rowX, curY }, bgmEnabled, false);
            curY += 28.0f;

            if (bgmEnabled) {
                Rectangle bgmVolRect = { rowX + 16.0f, curY, rowW - 32.0f, 22.0f };
                Widgets::slider("MUSIC VOLUME", bgmVolRect, bgmVolume, 0.0f, 1.0f, 160, "%.0f%%", true);
                curY += 28.0f;

                if (!bgmStatusText.empty()) {
                    DrawText(bgmStatusText.c_str(), static_cast<int>(rowX + 16.0f), static_cast<int>(curY + 3.0f), 12, Colors::Zinc400);
                    float skipBtnW = 100.0f;
                    float skipBtnH = 22.0f;
                    Rectangle skipRect = { rowX + rowW - skipBtnW - 16.0f, curY, skipBtnW, skipBtnH };
                    if (Widgets::button("SKIP TRACK", skipRect, Colors::Zinc800, Colors::Cyan400, false, 11)) {
                        actions.bgmSkip = true;
                    }
                    curY += 28.0f;
                }
            } else {
                DrawText("Background ambience and music are disabled.", static_cast<int>(rowX + 28.0f), static_cast<int>(curY), 12, Colors::Zinc500);
                curY += 24.0f;
            }

            DrawLineEx({ rowX, curY }, { rowX + rowW, curY }, 1.0f, Colors::PanelBorder);
            curY += 12.0f;

            // 2. Sound Effects (SFX)
            Widgets::checkbox("ENABLE SOUND EFFECTS (SFX)", { rowX, curY }, sfxEnabled, false);
            curY += 28.0f;

            if (sfxEnabled) {
                Rectangle sfxVolRect = { rowX + 16.0f, curY, rowW - 32.0f, 22.0f };
                Widgets::slider("SFX VOLUME", sfxVolRect, sfxVolume, 0.0f, 1.0f, 160, "%.0f%%", true);
                curY += 28.0f;
            } else {
                DrawText("Gameplay and interface sound effects are muted.", static_cast<int>(rowX + 28.0f), static_cast<int>(curY), 12, Colors::Zinc500);
                curY += 24.0f;
            }

            DrawLineEx({ rowX, curY }, { rowX + rowW, curY }, 1.0f, Colors::PanelBorder);
            curY += 12.0f;

            // 3. Section: Proximity Voice Chat
            Widgets::checkbox("ENABLE PROXIMITY VOICE CHAT", { rowX, curY }, voiceSettings.enabled, false);
            curY += 26.0f;

            if (voiceSettings.enabled) {
                Widgets::checkbox("SPATIAL 3D/4D PROXIMITY ATTENUATION", { rowX + 16.0f, curY }, voiceSettings.proximity, false);
                curY += 24.0f;

                Widgets::checkbox("PUSH-TO-TALK (HOLD [V] TO SPEAK)", { rowX + 16.0f, curY }, voiceSettings.pushToTalk, false);
                curY += 26.0f;

                Rectangle volRect = { rowX + 16.0f, curY, rowW - 32.0f, 22.0f };
                Widgets::slider("VOICE VOLUME", volRect, voiceSettings.voiceVolume, 0.0f, 1.5f, 160, "%.0f%%", true);
                curY += 26.0f;

                Rectangle micRect = { rowX + 16.0f, curY, rowW - 32.0f, 22.0f };
                Widgets::slider("MIC INPUT GAIN", micRect, voiceSettings.micGain, 0.5f, 2.5f, 160, "%.0f%%", true);
                curY += 26.0f;

                // Live Microphone Level Meter
                DrawText("MIC TEST LEVEL:", static_cast<int>(rowX + 16.0f), static_cast<int>(curY + 3.0f), 13, Colors::Zinc300);
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
                curY += 22.0f;
            } else {
                DrawText("Voice chat is disabled. No audio capture or playback will occur.", static_cast<int>(rowX + 28.0f), static_cast<int>(curY), 12, Colors::Zinc500);
                curY += 24.0f;
            }

            DrawLineEx({ rowX, curY }, { rowX + rowW, curY }, 1.0f, Colors::PanelBorder);
            curY += 10.0f;

            DrawText("PUSH-TO-TALK KEY", static_cast<int>(rowX), static_cast<int>(curY), 12, Colors::Zinc400);
            int valW = MeasureText("KEYBOARD [ V ]", 12);
            DrawText("KEYBOARD [ V ]", static_cast<int>(rowX + rowW - valW), static_cast<int>(curY), 12, Colors::Cyan400);
        }
        else if (activeSettingsTab == SettingsTab::Controls) {
            // 1. Movement Mode Toggle
            DrawText("SHIP MOVEMENT CONTROL MODE", static_cast<int>(rowX), static_cast<int>(curY), 14, Colors::Zinc300);
            curY += 22.0f;

            float modeBtnW = (rowW - 8.0f) * 0.5f;
            float modeBtnH = 34.0f;
            bool isMouse = (controlMode == 0);
            bool isKeyb = (controlMode == 1);

            if (Widgets::button("MOUSE FOLLOWER", { rowX, curY, modeBtnW, modeBtnH }, isMouse ? Colors::Cyan500 : Colors::Zinc800, Colors::Cyan400, false, 14)) {
                if (controlMode != 0) {
                    controlMode = 0;
                    actions.controlModeChanged = true;
                }
            }
            if (Widgets::button("KEYBOARD / CONTROLLER", { rowX + modeBtnW + 8.0f, curY, modeBtnW, modeBtnH }, isKeyb ? Colors::Cyan500 : Colors::Zinc800, Colors::Cyan400, false, 14)) {
                if (controlMode != 1) {
                    controlMode = 1;
                    actions.controlModeChanged = true;
                }
            }
            curY += modeBtnH + 6.0f;

            if (controlMode == 0) {
                DrawText("Ship smoothly tracks the cursor. Left-Click reveals, Right-Click flags.", static_cast<int>(rowX + 4.0f), static_cast<int>(curY), 12, Colors::Zinc500);
            } else {
                DrawText("Direct ship piloting via WASD / Arrows or Gamepad stick. Press M in-game to toggle.", static_cast<int>(rowX + 4.0f), static_cast<int>(curY), 12, Colors::Green400);
            }
            curY += 20.0f;

            DrawLineEx({ rowX, curY }, { rowX + rowW, curY }, 1.0f, Colors::PanelBorder);
            curY += 10.0f;

            // 2. Controller / Gamepad Support Section
            bool padAvailable = IsGamepadAvailable(0);
            DrawText("GAMEPAD / CONTROLLER SUPPORT", static_cast<int>(rowX), static_cast<int>(curY), 14, Colors::Zinc300);
            if (padAvailable) {
                const char* padName = GetGamepadName(0);
                std::string connectedStr = "CONNECTED: " + std::string(padName ? padName : "Gamepad");
                int connW = MeasureText(connectedStr.c_str(), 12);
                DrawText(connectedStr.c_str(), static_cast<int>(rowX + rowW - connW), static_cast<int>(curY + 1.0f), 12, Colors::Green400);
            } else {
                int noConnW = MeasureText("NO CONTROLLER DETECTED", 12);
                DrawText("NO CONTROLLER DETECTED", static_cast<int>(rowX + rowW - noConnW), static_cast<int>(curY + 1.0f), 12, Colors::Zinc500);
            }
            curY += 20.0f;

            // Box listing gamepad controls
            float padBoxH = 92.0f;
            DrawRectangleRec({ rowX, curY, rowW, padBoxH }, Colors::Zinc900);
            DrawRectangleLinesEx({ rowX, curY, rowW, padBoxH }, 1.0f, Colors::Zinc800);

            float col1X = rowX + 14.0f;
            float col2X = rowX + rowW * 0.5f + 14.0f;
            float lineY = curY + 8.0f;

            DrawText("LEFT STICK / D-PAD", static_cast<int>(col1X), static_cast<int>(lineY), 12, Colors::Cyan400);
            DrawText("Fly Ship (Movement)", static_cast<int>(col1X + 130.0f), static_cast<int>(lineY), 12, Colors::Zinc400);

            DrawText("CROSS [X] / (A)", static_cast<int>(col2X), static_cast<int>(lineY), 12, Colors::Green400);
            DrawText("Uncover Cell", static_cast<int>(col2X + 110.0f), static_cast<int>(lineY), 12, Colors::Zinc400);

            lineY += 20.0f;
            DrawText("RIGHT STICK", static_cast<int>(col1X), static_cast<int>(lineY), 12, Colors::Cyan400);
            DrawText("Select Cell & Aim", static_cast<int>(col1X + 130.0f), static_cast<int>(lineY), 12, Colors::Zinc400);

            DrawText("CIRCLE [O] / (B)", static_cast<int>(col2X), static_cast<int>(lineY), 12, Colors::Amber500);
            DrawText("Flag / Unflag Cell", static_cast<int>(col2X + 110.0f), static_cast<int>(lineY), 12, Colors::Zinc400);

            lineY += 20.0f;
            DrawText("LT / RT TRIGGERS", static_cast<int>(col1X), static_cast<int>(lineY), 12, Colors::Cyan400);
            DrawText("Zoom Camera Out/In", static_cast<int>(col1X + 130.0f), static_cast<int>(lineY), 12, Colors::Zinc400);

            DrawText("SQUARE [] / R1", static_cast<int>(col2X), static_cast<int>(lineY), 12, Colors::Purple400);
            DrawText("Chord Number Cell", static_cast<int>(col2X + 110.0f), static_cast<int>(lineY), 12, Colors::Zinc400);

            lineY += 20.0f;
            DrawText("L3 / SELECT", static_cast<int>(col1X), static_cast<int>(lineY), 12, Colors::Cyan400);
            DrawText("Center Camera", static_cast<int>(col1X + 130.0f), static_cast<int>(lineY), 12, Colors::Zinc400);

            curY += padBoxH + 10.0f;

            // 3. Keyboard Controls Reference
            DrawText("KEYBOARD CONTROLS SUMMARY", static_cast<int>(rowX), static_cast<int>(curY), 14, Colors::Zinc300);
            curY += 18.0f;

            float kbdBoxH = 64.0f;
            DrawRectangleRec({ rowX, curY, rowW, kbdBoxH }, Colors::Zinc900);
            DrawRectangleLinesEx({ rowX, curY, rowW, kbdBoxH }, 1.0f, Colors::Zinc800);

            float kLineY = curY + 8.0f;
            DrawText("W / A / S / D or ARROWS", static_cast<int>(col1X), static_cast<int>(kLineY), 12, Colors::Cyan400);
            DrawText("Fly Ship (Movement Only)", static_cast<int>(col1X + 160.0f), static_cast<int>(kLineY), 12, Colors::Zinc400);

            DrawText("SPACE / ENTER", static_cast<int>(col2X), static_cast<int>(kLineY), 12, Colors::Green400);
            DrawText("Uncover Cell", static_cast<int>(col2X + 110.0f), static_cast<int>(kLineY), 12, Colors::Zinc400);

            kLineY += 18.0f;
            DrawText("KEY [ M ]", static_cast<int>(col1X), static_cast<int>(kLineY), 12, Colors::Cyan400);
            DrawText("Toggle Movement Mode", static_cast<int>(col1X + 160.0f), static_cast<int>(kLineY), 12, Colors::Zinc400);

            DrawText("KEY [ F ]", static_cast<int>(col2X), static_cast<int>(kLineY), 12, Colors::Amber500);
            DrawText("Flag Cell", static_cast<int>(col2X + 110.0f), static_cast<int>(kLineY), 12, Colors::Zinc400);

            kLineY += 18.0f;
            DrawText("KEY [ C ]", static_cast<int>(col1X), static_cast<int>(kLineY), 12, Colors::Cyan400);
            DrawText("Chord Revealed Number", static_cast<int>(col1X + 160.0f), static_cast<int>(kLineY), 12, Colors::Zinc400);
        }

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

