#include "sector_editor.hpp"
#include "widgets.hpp"
#include "theme.hpp"
#include "../audio/sound_manager.hpp"
#include <cstring>
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <cmath>

#ifdef CAMPAIGN_EDITOR_ENABLED

namespace minesweeper::ui {

static Rectangle normalizeRect(Vector2 a, Vector2 b) {
    float minX = std::min(a.x, b.x);
    float minY = std::min(a.y, b.y);
    float maxX = std::max(a.x, b.x);
    float maxY = std::max(a.y, b.y);
    return { minX, minY, std::max(1.0f, maxX - minX), std::max(1.0f, maxY - minY) };
}

static Rectangle getHandleRect(const Rectangle& r, int cornerIdx, float size = 12.0f) {
    float half = size * 0.5f;
    switch (cornerIdx) {
        case 0: return { r.x - half, r.y - half, size, size };                          // Top-Left
        case 1: return { r.x + r.width - half, r.y - half, size, size };                 // Top-Right
        case 2: return { r.x - half, r.y + r.height - half, size, size };                 // Bottom-Left
        case 3: default: return { r.x + r.width - half, r.y + r.height - half, size, size }; // Bottom-Right
    }
}

SectorEditor::SectorEditor() = default;

float SectorEditor::snapCoord(float val) const {
    if (gridSnap <= 0.0f) return val;
    return std::round(val / gridSnap) * gridSnap;
}

Vector2 SectorEditor::snapVec(Vector2 v) const {
    return { snapCoord(v.x), snapCoord(v.y) };
}

void SectorEditor::open(core::CampaignManager& campaignMgr, int defaultSectorIdx) {
    if (defaultSectorIdx >= 0 && defaultSectorIdx < static_cast<int>(campaignMgr.sectors.size())) {
        selectedSectorIndex = defaultSectorIdx;
    }
    loadSector(campaignMgr, selectedSectorIndex);
    isOpen = true;
    dragMode = DragMode::None;
    selectionType = EditorSelectionType::None;
    selectedIndex = -1;
}

void SectorEditor::close() {
    isOpen = false;
    nameActive = false;
    codenameActive = false;
    subtitleActive = false;
    descActive = false;
    depotTitleActive = false;
    dragMode = DragMode::None;
}

void SectorEditor::toggle(core::CampaignManager& campaignMgr) {
    if (isOpen) {
        close();
    } else {
        open(campaignMgr, campaignMgr.activeSectorIndex);
    }
}

bool SectorEditor::isMouseOver(Vector2 mousePos) const {
    if (!isOpen) return false;
    return CheckCollisionPointRec(mousePos, panelRect);
}

void SectorEditor::loadSector(core::CampaignManager& campaignMgr, int sectorIdx) {
    if (sectorIdx >= 0 && sectorIdx < static_cast<int>(campaignMgr.sectors.size())) {
        selectedSectorIndex = sectorIdx;
        workingConfig = campaignMgr.sectors[sectorIdx].config;
        syncBuffersFromConfig();
        isLoaded = true;
        selectedWallIndex = 0;
        selectedDockIndex = 0;
        selectionType = EditorSelectionType::None;
        selectedIndex = -1;
        useBombPercentage = false;
        bombPercentage = std::clamp(getCalculatedPercentage(), 5, 50);
    }
}

int SectorEditor::getPlayableCellsEstimate() const {
    if (workingConfig.terrainShape == core::TerrainShape::Rectangle) {
        return workingConfig.gridSize * workingConfig.gridSize;
    }
    core::Board tempBoard;
    tempBoard.init(2, workingConfig.gridSize, 1, 12345, workingConfig.terrainShape);
    return static_cast<int>(tempBoard.totalPlayableCells());
}

void SectorEditor::updateBombCountFromPercentage() {
    int totalPlayable = getPlayableCellsEstimate();
    int maxBombs = std::max(1, totalPlayable - 5);
    workingConfig.bombCount = std::clamp(static_cast<int>(std::round(totalPlayable * (bombPercentage / 100.0f))), 1, maxBombs);
}

int SectorEditor::getCalculatedPercentage() const {
    int totalPlayable = getPlayableCellsEstimate();
    if (totalPlayable <= 0) return 0;
    return static_cast<int>(std::round(workingConfig.bombCount * 100.0f / totalPlayable));
}

void SectorEditor::syncBuffersFromConfig() {
    std::snprintf(nameBuf, sizeof(nameBuf), "%s", workingConfig.name.c_str());
    std::snprintf(codenameBuf, sizeof(codenameBuf), "%s", workingConfig.codename.c_str());
    std::snprintf(subtitleBuf, sizeof(subtitleBuf), "%s", workingConfig.subtitle.c_str());
    std::snprintf(descBuf, sizeof(descBuf), "%s", workingConfig.description.c_str());
    std::snprintf(depotTitleBuf, sizeof(depotTitleBuf), "%s", workingConfig.stagingDepotTitle.c_str());
}

void SectorEditor::syncConfigFromBuffers() {
    workingConfig.name = nameBuf;
    workingConfig.codename = codenameBuf;
    workingConfig.subtitle = subtitleBuf;
    workingConfig.description = descBuf;
    workingConfig.stagingDepotTitle = depotTitleBuf;
}

void SectorEditor::applyLive(core::CampaignManager& campaignMgr) {
    syncConfigFromBuffers();
    if (campaignMgr.rebuildSector(selectedSectorIndex, workingConfig)) {
        toastMessage = TextFormat("LIVE UPDATE: SECTOR %02d APPLIED", workingConfig.id);
        toastColor = Colors::Amber400;
        toastTimer = 2.5f;
    } else {
        toastMessage = "ERROR APPLYING LIVE UPDATE";
        toastColor = Colors::Red400;
        toastTimer = 2.5f;
    }
}

void SectorEditor::saveToDisk(core::CampaignManager& campaignMgr) {
    syncConfigFromBuffers();
    campaignMgr.rebuildSector(selectedSectorIndex, workingConfig);
    std::string targetDir = (exportFolderBuf[0] != '\0') ? exportFolderBuf : "assets/campaign/sectors/custom";
    if (core::CampaignManager::saveSectorConfigToJson(workingConfig, targetDir)) {
        toastMessage = TextFormat("EXPORTED (sector_%02d.json) TO %s", workingConfig.id, targetDir.c_str());
        toastColor = Colors::Green400;
        toastTimer = 3.5f;
        audio::SoundManager::playFlag();
    } else {
        toastMessage = "ERROR EXPORTING TO DISK";
        toastColor = Colors::Red400;
        toastTimer = 2.5f;
    }
}

void SectorEditor::reloadFromDisk(core::CampaignManager& campaignMgr) {
    std::string targetDir = (exportFolderBuf[0] != '\0') ? exportFolderBuf : "assets/campaign/sectors/custom";
    auto freshConfigs = core::CampaignManager::loadSectorConfigs(targetDir);
    if (freshConfigs.empty()) {
        const auto* pCfg = campaignMgr.getActivePlanetConfig();
        std::string fallbackDir = pCfg ? pCfg->sectorDataPath : "assets/campaign/sectors/planet1";
        freshConfigs = core::CampaignManager::loadSectorConfigs(fallbackDir);
    }
    for (const auto& cfg : freshConfigs) {
        if (cfg.id == workingConfig.id) {
            workingConfig = cfg;
            syncBuffersFromConfig();
            campaignMgr.rebuildSector(selectedSectorIndex, workingConfig);
            toastMessage = TextFormat("RELOADED SECTOR %02d FROM DISK", workingConfig.id);
            toastColor = Colors::Cyan400;
            toastTimer = 2.5f;
            audio::SoundManager::playIncrement();
            return;
        }
    }
    toastMessage = "SECTOR CONFIG NOT FOUND ON DISK";
    toastColor = Colors::Red400;
    toastTimer = 2.5f;
}

void SectorEditor::updateWorldInteraction(core::CampaignManager& campaignMgr, Vector2 worldMouse, float dt) {
    (void)dt;
    if (!isOpen) return;

    float boardPx = workingConfig.gridSize * 30.0f;
    float arenaH = boardPx + 2.0f * workingConfig.vertMargin;

    // Keyboard Shortcuts
    if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_Q)) {
        currentTool = EditorTool::Select;
    }
    if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_W)) {
        currentTool = EditorTool::DrawWall;
    }
    if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_E)) {
        currentTool = EditorTool::AddDock;
    }
    if (IsKeyPressed(KEY_G)) {
        if (gridSnap == 0.0f) gridSnap = 10.0f;
        else if (gridSnap == 10.0f) gridSnap = 20.0f;
        else if (gridSnap == 20.0f) gridSnap = 40.0f;
        else gridSnap = 0.0f;
    }
    if (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE)) {
        if (selectionType == EditorSelectionType::Wall && selectedIndex >= 0 && selectedIndex < static_cast<int>(workingConfig.customWalls.size())) {
            workingConfig.customWalls.erase(workingConfig.customWalls.begin() + selectedIndex);
            selectionType = EditorSelectionType::None;
            selectedIndex = -1;
            selectedWallIndex = 0;
            applyLive(campaignMgr);
            audio::SoundManager::playFlag();
        } else if (selectionType == EditorSelectionType::MerchantDock && selectedIndex >= 0 && selectedIndex < static_cast<int>(workingConfig.merchantSpawns.size())) {
            workingConfig.merchantSpawns.erase(workingConfig.merchantSpawns.begin() + selectedIndex);
            selectionType = EditorSelectionType::None;
            selectedIndex = -1;
            selectedDockIndex = 0;
            applyLive(campaignMgr);
            audio::SoundManager::playFlag();
        }
    }
    if (IsKeyPressed(KEY_H)) {
        if (selectionType == EditorSelectionType::Wall && selectedIndex >= 0 && selectedIndex < static_cast<int>(workingConfig.customWalls.size())) {
            workingConfig.customWalls[selectedIndex].isHazard = !workingConfig.customWalls[selectedIndex].isHazard;
            applyLive(campaignMgr);
            audio::SoundManager::playIncrement();
        }
    }
    if (IsKeyPressed(KEY_R) || IsKeyPressed(KEY_TAB)) {
        if (selectionType == EditorSelectionType::MerchantDock && selectedIndex >= 0 && selectedIndex < static_cast<int>(workingConfig.merchantSpawns.size())) {
            auto& type = workingConfig.merchantSpawns[selectedIndex].type;
            if (type == "shop1") type = "shop2";
            else if (type == "shop2") type = "shop3";
            else if (type == "shop3") type = "roulette";
            else type = "shop1";
            applyLive(campaignMgr);
            audio::SoundManager::playIncrement();
        }
    }

    // Hover handle detection for currently selected wall
    hoveredHandle = -1;
    if (selectionType == EditorSelectionType::Wall && selectedIndex >= 0 && selectedIndex < static_cast<int>(workingConfig.customWalls.size())) {
        const auto& w = workingConfig.customWalls[selectedIndex];
        for (int c = 0; c < 4; ++c) {
            Rectangle hr = getHandleRect(w.rect, c, 14.0f);
            if (CheckCollisionPointRec(worldMouse, hr)) {
                hoveredHandle = c;
                break;
            }
        }
    }

    // Mouse Button Pressed
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        // 1. Quick Draw (or Shift+Click)
        if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT) || currentTool == EditorTool::DrawWall) {
            dragMode = DragMode::DrawingNewWall;
            createWallStart = snapVec(worldMouse);
            createWallPreview = { createWallStart.x, createWallStart.y, 0, 0 };
            return;
        }

        // 2. Add Merchant Dock Tool
        if (currentTool == EditorTool::AddDock) {
            core::MerchantSpawn ms;
            ms.type = "shop1";
            ms.pos = snapVec(worldMouse);
            ms.angle = 0.0f;
            workingConfig.merchantSpawns.push_back(ms);
            selectionType = EditorSelectionType::MerchantDock;
            selectedIndex = static_cast<int>(workingConfig.merchantSpawns.size()) - 1;
            selectedDockIndex = selectedIndex;
            currentTab = SectorEditorTab::Ships;
            currentTool = EditorTool::Select;
            applyLive(campaignMgr);
            audio::SoundManager::playLaser();
            return;
        }

        // 3. Corner Handles of currently selected wall
        if (hoveredHandle >= 0 && selectionType == EditorSelectionType::Wall && selectedIndex >= 0 && selectedIndex < static_cast<int>(workingConfig.customWalls.size())) {
            dragMode = static_cast<DragMode>(static_cast<int>(DragMode::ResizeTopLeft) + hoveredHandle);
            dragStartMouse = worldMouse;
            dragStartRect = workingConfig.customWalls[selectedIndex].rect;
            return;
        }

        // 4. Merchant Docks
        for (size_t i = 0; i < workingConfig.merchantSpawns.size(); ++i) {
            Vector2 dockPos = workingConfig.merchantSpawns[i].pos;
            if (dockPos.y < 0.0f) dockPos.y = arenaH + dockPos.y;
            if (CheckCollisionPointCircle(worldMouse, dockPos, 45.0f)) {
                selectionType = EditorSelectionType::MerchantDock;
                selectedIndex = static_cast<int>(i);
                selectedDockIndex = static_cast<int>(i);
                dragMode = DragMode::MoveObject;
                dragStartMouse = worldMouse;
                dragStartPos = dockPos;
                currentTab = SectorEditorTab::Ships;
                audio::SoundManager::playIncrement();
                return;
            }
        }

        // 5. Player Spawn Point
        Vector2 spPos = workingConfig.spawnPos;
        if (spPos.y < 0.0f) spPos.y = arenaH * 0.5f;
        if (CheckCollisionPointCircle(worldMouse, spPos, 40.0f)) {
            selectionType = EditorSelectionType::PlayerSpawn;
            selectedIndex = 0;
            dragMode = DragMode::MoveObject;
            dragStartMouse = worldMouse;
            dragStartPos = spPos;
            currentTab = SectorEditorTab::Map;
            audio::SoundManager::playIncrement();
            return;
        }

        // 6. Staging Depot
        Vector2 dpPos = workingConfig.stagingDepotPos;
        if (dpPos.y < 0.0f) dpPos.y = arenaH * 0.5f;
        Rectangle dpRec = { dpPos.x - 75.0f, dpPos.y - 75.0f, 150.0f, 150.0f };
        if (CheckCollisionPointRec(worldMouse, dpRec)) {
            selectionType = EditorSelectionType::StagingDepot;
            selectedIndex = 0;
            dragMode = DragMode::MoveObject;
            dragStartMouse = worldMouse;
            dragStartPos = dpPos;
            currentTab = SectorEditorTab::Map;
            audio::SoundManager::playIncrement();
            return;
        }

        // 7. Custom Walls (hit test in reverse order)
        for (int i = static_cast<int>(workingConfig.customWalls.size()) - 1; i >= 0; --i) {
            if (CheckCollisionPointRec(worldMouse, workingConfig.customWalls[i].rect)) {
                selectionType = EditorSelectionType::Wall;
                selectedIndex = i;
                selectedWallIndex = i;
                dragMode = DragMode::MoveObject;
                dragStartMouse = worldMouse;
                dragStartRect = workingConfig.customWalls[i].rect;
                currentTab = SectorEditorTab::Walls;
                audio::SoundManager::playIncrement();
                return;
            }
        }

        // 8. Click on empty space
        selectionType = EditorSelectionType::None;
        selectedIndex = -1;
    }

    // Right-Click: quick cycle ship type
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        for (size_t i = 0; i < workingConfig.merchantSpawns.size(); ++i) {
            Vector2 dockPos = workingConfig.merchantSpawns[i].pos;
            if (dockPos.y < 0.0f) dockPos.y = arenaH + dockPos.y;
            if (CheckCollisionPointCircle(worldMouse, dockPos, 45.0f)) {
                auto& type = workingConfig.merchantSpawns[i].type;
                if (type == "shop1") type = "shop2";
                else if (type == "shop2") type = "shop3";
                else if (type == "shop3") type = "roulette";
                else type = "shop1";
                selectionType = EditorSelectionType::MerchantDock;
                selectedIndex = static_cast<int>(i);
                selectedDockIndex = static_cast<int>(i);
                currentTab = SectorEditorTab::Ships;
                applyLive(campaignMgr);
                audio::SoundManager::playLaser();
                return;
            }
        }
    }

    // Mouse Dragging
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && dragMode != DragMode::None) {
        Vector2 curSnapped = snapVec(worldMouse);
        Vector2 mouseDelta = { curSnapped.x - snapCoord(dragStartMouse.x), curSnapped.y - snapCoord(dragStartMouse.y) };

        if (dragMode == DragMode::MoveObject) {
            if (selectionType == EditorSelectionType::Wall && selectedIndex >= 0 && selectedIndex < static_cast<int>(workingConfig.customWalls.size())) {
                auto& w = workingConfig.customWalls[selectedIndex];
                float newX = snapCoord(dragStartRect.x + mouseDelta.x);
                float newY = snapCoord(dragStartRect.y + mouseDelta.y);
                if (std::abs(w.rect.x - newX) > 0.1f || std::abs(w.rect.y - newY) > 0.1f) {
                    w.rect.x = newX;
                    w.rect.y = newY;
                    applyLive(campaignMgr);
                }
            } else if (selectionType == EditorSelectionType::MerchantDock && selectedIndex >= 0 && selectedIndex < static_cast<int>(workingConfig.merchantSpawns.size())) {
                auto& ms = workingConfig.merchantSpawns[selectedIndex];
                float newX = snapCoord(dragStartPos.x + mouseDelta.x);
                float newY = snapCoord(dragStartPos.y + mouseDelta.y);
                if (std::abs(ms.pos.x - newX) > 0.1f || std::abs(ms.pos.y - newY) > 0.1f) {
                    ms.pos = { newX, newY };
                    applyLive(campaignMgr);
                }
            } else if (selectionType == EditorSelectionType::PlayerSpawn) {
                float newX = snapCoord(dragStartPos.x + mouseDelta.x);
                float newY = snapCoord(dragStartPos.y + mouseDelta.y);
                if (std::abs(workingConfig.spawnPos.x - newX) > 0.1f || std::abs(workingConfig.spawnPos.y - newY) > 0.1f) {
                    workingConfig.spawnPos = { newX, newY };
                    applyLive(campaignMgr);
                }
            } else if (selectionType == EditorSelectionType::StagingDepot) {
                float newX = snapCoord(dragStartPos.x + mouseDelta.x);
                float newY = snapCoord(dragStartPos.y + mouseDelta.y);
                if (std::abs(workingConfig.stagingDepotPos.x - newX) > 0.1f || std::abs(workingConfig.stagingDepotPos.y - newY) > 0.1f) {
                    workingConfig.stagingDepotPos = { newX, newY };
                    applyLive(campaignMgr);
                }
            }
        }
        else if (dragMode == DragMode::ResizeTopLeft && selectionType == EditorSelectionType::Wall && selectedIndex >= 0 && selectedIndex < static_cast<int>(workingConfig.customWalls.size())) {
            auto& w = workingConfig.customWalls[selectedIndex];
            float right = dragStartRect.x + dragStartRect.width;
            float bottom = dragStartRect.y + dragStartRect.height;
            float newX = std::min(right - 16.0f, snapCoord(worldMouse.x));
            float newY = std::min(bottom - 16.0f, snapCoord(worldMouse.y));
            w.rect.x = newX;
            w.rect.y = newY;
            w.rect.width = right - newX;
            w.rect.height = bottom - newY;
            applyLive(campaignMgr);
        }
        else if (dragMode == DragMode::ResizeTopRight && selectionType == EditorSelectionType::Wall && selectedIndex >= 0 && selectedIndex < static_cast<int>(workingConfig.customWalls.size())) {
            auto& w = workingConfig.customWalls[selectedIndex];
            float bottom = dragStartRect.y + dragStartRect.height;
            float newY = std::min(bottom - 16.0f, snapCoord(worldMouse.y));
            float newW = std::max(16.0f, snapCoord(worldMouse.x) - dragStartRect.x);
            w.rect.y = newY;
            w.rect.width = newW;
            w.rect.height = bottom - newY;
            applyLive(campaignMgr);
        }
        else if (dragMode == DragMode::ResizeBottomLeft && selectionType == EditorSelectionType::Wall && selectedIndex >= 0 && selectedIndex < static_cast<int>(workingConfig.customWalls.size())) {
            auto& w = workingConfig.customWalls[selectedIndex];
            float right = dragStartRect.x + dragStartRect.width;
            float newX = std::min(right - 16.0f, snapCoord(worldMouse.x));
            float newH = std::max(16.0f, snapCoord(worldMouse.y) - dragStartRect.y);
            w.rect.x = newX;
            w.rect.width = right - newX;
            w.rect.height = newH;
            applyLive(campaignMgr);
        }
        else if (dragMode == DragMode::ResizeBottomRight && selectionType == EditorSelectionType::Wall && selectedIndex >= 0 && selectedIndex < static_cast<int>(workingConfig.customWalls.size())) {
            auto& w = workingConfig.customWalls[selectedIndex];
            float newW = std::max(16.0f, snapCoord(worldMouse.x) - dragStartRect.x);
            float newH = std::max(16.0f, snapCoord(worldMouse.y) - dragStartRect.y);
            w.rect.width = newW;
            w.rect.height = newH;
            applyLive(campaignMgr);
        }
        else if (dragMode == DragMode::DrawingNewWall) {
            createWallPreview = normalizeRect(createWallStart, snapVec(worldMouse));
        }
    }

    // Mouse Button Released
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        if (dragMode == DragMode::DrawingNewWall) {
            if (createWallPreview.width >= 16.0f && createWallPreview.height >= 16.0f) {
                core::SectorWall nw;
                nw.rect = createWallPreview;
                nw.isHazard = false;
                workingConfig.customWalls.push_back(nw);
                selectionType = EditorSelectionType::Wall;
                selectedIndex = static_cast<int>(workingConfig.customWalls.size()) - 1;
                selectedWallIndex = selectedIndex;
                currentTab = SectorEditorTab::Walls;
                applyLive(campaignMgr);
                audio::SoundManager::playLaser();
            }
            createWallPreview = { 0, 0, 0, 0 };
            if (currentTool == EditorTool::DrawWall) {
                currentTool = EditorTool::Select;
            }
        }
        dragMode = DragMode::None;
    }
}

void SectorEditor::drawWorldGizmos(const core::CampaignManager& campaignMgr, Vector2 worldMouse) {
    (void)campaignMgr;
    if (!isOpen) return;

    float boardPx = workingConfig.gridSize * 30.0f;
    float arenaW = boardPx + workingConfig.westMargin + workingConfig.eastMargin;
    float arenaH = boardPx + 2.0f * workingConfig.vertMargin;

    // 1. Grid Snap Dots in arena
    if (gridSnap >= 10.0f) {
        for (float gx = 0.0f; gx <= arenaW; gx += gridSnap) {
            for (float gy = 0.0f; gy <= arenaH; gy += gridSnap) {
                DrawPixelV({ gx, gy }, Fade(Colors::Zinc700, 0.40f));
            }
        }
    }

    // 2. Player Spawn Point Gizmo
    Vector2 sp = workingConfig.spawnPos;
    if (sp.y < 0.0f) sp.y = arenaH * 0.5f;
    bool spHovered = CheckCollisionPointCircle(worldMouse, sp, 40.0f);
    bool spSelected = (selectionType == EditorSelectionType::PlayerSpawn);
    Color spCol = spSelected ? Colors::Amber400 : (spHovered ? Colors::Green400 : Colors::Green500);

    DrawCircleLines(static_cast<int>(sp.x), static_cast<int>(sp.y), 32.0f, Fade(spCol, 0.70f));
    DrawLine(static_cast<int>(sp.x - 38.0f), static_cast<int>(sp.y), static_cast<int>(sp.x + 38.0f), static_cast<int>(sp.y), Fade(spCol, 0.50f));
    DrawLine(static_cast<int>(sp.x), static_cast<int>(sp.y - 38.0f), static_cast<int>(sp.x), static_cast<int>(sp.y + 38.0f), Fade(spCol, 0.50f));
    DrawCircle(static_cast<int>(sp.x), static_cast<int>(sp.y), 4.0f, spCol);

    const char* spText = "[PLAYER LZ]";
    int spW = MeasureText(spText, 10);
    DrawRectangle(static_cast<int>(sp.x - spW * 0.5f - 4), static_cast<int>(sp.y - 48.0f), spW + 8, 14, Fade(Colors::Zinc950, 0.85f));
    DrawRectangleLines(static_cast<int>(sp.x - spW * 0.5f - 4), static_cast<int>(sp.y - 48.0f), spW + 8, 14, Fade(spCol, 0.70f));
    DrawText(spText, static_cast<int>(sp.x - spW * 0.5f), static_cast<int>(sp.y - 46.0f), 10, spCol);

    // 3. Staging Depot Gizmo
    Vector2 dp = workingConfig.stagingDepotPos;
    if (dp.y < 0.0f) dp.y = arenaH * 0.5f;
    Rectangle dpRec = { dp.x - 75.0f, dp.y - 75.0f, 150.0f, 150.0f };
    bool dpSelected = (selectionType == EditorSelectionType::StagingDepot);
    Color dpCol = dpSelected ? Colors::Amber400 : Colors::Cyan500;
    DrawRectangleLinesEx(dpRec, dpSelected ? 2.0f : 1.0f, Fade(dpCol, dpSelected ? 0.90f : 0.40f));
    const char* dpText = "[STAGING DEPOT]";
    int dpW = MeasureText(dpText, 10);
    DrawText(dpText, static_cast<int>(dp.x - dpW * 0.5f), static_cast<int>(dpRec.y + 6.0f), 10, dpCol);

    // 4. Merchant Docks Gizmos
    for (size_t i = 0; i < workingConfig.merchantSpawns.size(); ++i) {
        const auto& ms = workingConfig.merchantSpawns[i];
        Vector2 mPos = ms.pos;
        if (mPos.y < 0.0f) mPos.y = arenaH + mPos.y;
        bool isDocSelected = (selectionType == EditorSelectionType::MerchantDock && selectedIndex == static_cast<int>(i));
        bool isDocHovered = CheckCollisionPointCircle(worldMouse, mPos, 45.0f);

        Color docCol = Colors::Cyan400;
        const char* typeTag = "MINI 1";
        if (ms.type == "shop2") { docCol = Colors::Green400; typeTag = "CARGO 2"; }
        else if (ms.type == "shop3") { docCol = Colors::Amber400; typeTag = "HEAVY 3 (200px)"; }
        else if (ms.type == "roulette") { docCol = Color{ 168, 85, 247, 255 }; typeTag = "CASINO ROULETTE"; }

        if (isDocSelected) docCol = Colors::Amber300;

        // Docking pad ring and anchor crosshair
        DrawCircleLines(static_cast<int>(mPos.x), static_cast<int>(mPos.y), 45.0f, Fade(docCol, isDocSelected ? 0.95f : (isDocHovered ? 0.80f : 0.45f)));
        if (isDocSelected) {
            DrawCircleLines(static_cast<int>(mPos.x), static_cast<int>(mPos.y), 49.0f, Fade(Colors::Amber400, 0.70f));
        }
        DrawLine(static_cast<int>(mPos.x - 25.0f), static_cast<int>(mPos.y), static_cast<int>(mPos.x + 25.0f), static_cast<int>(mPos.y), Fade(docCol, 0.60f));
        DrawLine(static_cast<int>(mPos.x), static_cast<int>(mPos.y - 25.0f), static_cast<int>(mPos.x), static_cast<int>(mPos.y + 25.0f), Fade(docCol, 0.60f));

        // Ship Hull Preview Outline
        if (ms.type == "shop3") {
            // 200px capsule collision preview outline
            Rectangle capRec = { mPos.x - 100.0f, mPos.y - 35.0f, 200.0f, 70.0f };
            DrawRectangleLinesEx(capRec, 1.0f, Fade(docCol, 0.35f));
        } else if (ms.type == "shop1") {
            Rectangle boxRec = { mPos.x - 35.0f, mPos.y - 24.0f, 70.0f, 48.0f };
            DrawRectangleLinesEx(boxRec, 1.0f, Fade(docCol, 0.35f));
        } else if (ms.type == "shop2") {
            Rectangle boxRec = { mPos.x - 45.0f, mPos.y - 27.0f, 90.0f, 54.0f };
            DrawRectangleLinesEx(boxRec, 1.0f, Fade(docCol, 0.35f));
        }

        // Dock Badge
        const char* badgeStr = TextFormat("DOCK #%d: %s", static_cast<int>(i + 1), typeTag);
        int bW = MeasureText(badgeStr, 10);
        float bX = mPos.x - bW * 0.5f;
        float bY = mPos.y - 62.0f;
        DrawRectangle(static_cast<int>(bX - 6), static_cast<int>(bY - 2), bW + 12, 16, Fade(Colors::Zinc950, 0.88f));
        DrawRectangleLines(static_cast<int>(bX - 6), static_cast<int>(bY - 2), bW + 12, 16, Fade(docCol, 0.80f));
        DrawText(badgeStr, static_cast<int>(bX), static_cast<int>(bY), 10, docCol);

        if (isDocSelected) {
            const char* hintStr = "[R-Click/R]: Change Ship | [Del]: Delete";
            int hW = MeasureText(hintStr, 9);
            DrawRectangle(static_cast<int>(mPos.x - hW * 0.5f - 4), static_cast<int>(mPos.y + 52.0f), hW + 8, 14, Fade(Colors::Zinc950, 0.85f));
            DrawText(hintStr, static_cast<int>(mPos.x - hW * 0.5f), static_cast<int>(mPos.y + 54.0f), 9, Colors::Zinc400);
        }
    }

    // 5. Custom Walls Gizmos
    for (size_t i = 0; i < workingConfig.customWalls.size(); ++i) {
        const auto& w = workingConfig.customWalls[i];
        bool isWSelected = (selectionType == EditorSelectionType::Wall && selectedIndex == static_cast<int>(i));
        bool isWHovered = CheckCollisionPointRec(worldMouse, w.rect);

        if (isWSelected) {
            // Bright amber outline
            DrawRectangleLinesEx(w.rect, 2.0f, Colors::Amber400);

            // 4 Corner Resize Handles
            for (int c = 0; c < 4; ++c) {
                Rectangle hr = getHandleRect(w.rect, c, 12.0f);
                bool hHovered = (hoveredHandle == c);
                DrawRectangleRec(hr, hHovered ? Colors::Amber300 : Colors::Amber500);
                DrawRectangleLinesEx(hr, 1.0f, Colors::Zinc950);
            }

            // Live Dimension Badge: [ W x H ] centered
            const char* dimStr = TextFormat("[ %d x %d ]", static_cast<int>(w.rect.width), static_cast<int>(w.rect.height));
            int dW = MeasureText(dimStr, 11);
            float dX = w.rect.x + (w.rect.width - dW) * 0.5f;
            float dY = w.rect.y + (w.rect.height - 14.0f) * 0.5f;
            DrawRectangle(static_cast<int>(dX - 4), static_cast<int>(dY - 2), dW + 8, 16, Fade(Colors::Zinc950, 0.88f));
            DrawRectangleLines(static_cast<int>(dX - 4), static_cast<int>(dY - 2), dW + 8, 16, Colors::Amber400);
            DrawText(dimStr, static_cast<int>(dX), static_cast<int>(dY), 11, Colors::Amber300);

            // Coordinates Tag at top-left
            const char* coordStr = TextFormat("(%d, %d)", static_cast<int>(w.rect.x), static_cast<int>(w.rect.y));
            DrawText(coordStr, static_cast<int>(w.rect.x), static_cast<int>(w.rect.y - 14.0f), 10, Colors::Zinc400);
        } else if (isWHovered) {
            DrawRectangleLinesEx(w.rect, 1.5f, Colors::Cyan400);
        }
    }

    // 6. Drawing New Wall Preview
    if (dragMode == DragMode::DrawingNewWall) {
        DrawRectangleRec(createWallPreview, Fade(Colors::Cyan500, 0.25f));
        DrawRectangleLinesEx(createWallPreview, 1.5f, Colors::Cyan400);

        const char* dimStr = TextFormat("[ %d x %d ]", static_cast<int>(createWallPreview.width), static_cast<int>(createWallPreview.height));
        int dW = MeasureText(dimStr, 11);
        float dX = createWallPreview.x + (createWallPreview.width - dW) * 0.5f;
        float dY = createWallPreview.y + (createWallPreview.height - 14.0f) * 0.5f;
        DrawRectangle(static_cast<int>(dX - 4), static_cast<int>(dY - 2), dW + 8, 16, Fade(Colors::Zinc950, 0.88f));
        DrawText(dimStr, static_cast<int>(dX), static_cast<int>(dY), 11, Colors::Cyan300);
    }
}

void SectorEditor::drawAndProcess(core::CampaignManager& campaignMgr, int screenW, int screenH) {
    if (!isOpen) return;

    if (toastTimer > 0.0f) {
        toastTimer -= GetFrameTime();
        if (toastTimer < 0.0f) toastTimer = 0.0f;
    }

    float panelW = 420.0f;
    float panelH = std::min(static_cast<float>(screenH) - 40.0f, 840.0f);
    float panelX = static_cast<float>(screenW) - panelW - 20.0f;
    float panelY = 20.0f;
    panelRect = { panelX, panelY, panelW, panelH };

    // 1. Background Panel
    DrawRectangleRec(panelRect, Fade(Colors::Zinc950, 0.94f));
    DrawRectangleLinesEx(panelRect, 1.5f, Colors::Amber500);

    // 2. Header Bar
    float headerH = 40.0f;
    Rectangle headerRect = { panelX, panelY, panelW, headerH };
    DrawRectangleRec(headerRect, Fade(Colors::Zinc900, 0.90f));
    DrawLine(static_cast<int>(panelX), static_cast<int>(panelY + headerH), static_cast<int>(panelX + panelW), static_cast<int>(panelY + headerH), Colors::Zinc700);

    const char* title = TextFormat("CAMPAIGN SECTOR VISUAL BUILDER [SECTOR %02d]", workingConfig.id);
    DrawText(title, static_cast<int>(panelX + 14.0f), static_cast<int>(panelY + 13.0f), 13, Colors::Amber400);

    // Close button (X)
    Rectangle closeBtnRect = { panelX + panelW - 32.0f, panelY + 8.0f, 24.0f, 24.0f };
    if (Widgets::button("X", closeBtnRect, Colors::Zinc800, Colors::Red500, false, 13)) {
        close();
        return;
    }

    float curX = panelX + 16.0f;
    float curY = panelY + headerH + 10.0f;
    float boxW = panelW - 32.0f;

    // 3. In-World Editing Toolbar Row
    float tbX = curX;
    float tbW = (boxW - 12.0f) / 4.0f;
    if (Widgets::button(currentTool == EditorTool::Select ? "[x] SELECT" : "SELECT (1)", { tbX, curY, tbW, 24.0f }, currentTool == EditorTool::Select ? Colors::Zinc700 : Colors::Zinc800, currentTool == EditorTool::Select ? Colors::Amber400 : Colors::Zinc300, false, 11)) {
        currentTool = EditorTool::Select;
    }
    tbX += tbW + 4.0f;
    if (Widgets::button(currentTool == EditorTool::DrawWall ? "[x] DRAW" : "+ WALL (2)", { tbX, curY, tbW, 24.0f }, currentTool == EditorTool::DrawWall ? Colors::Zinc700 : Colors::Zinc800, currentTool == EditorTool::DrawWall ? Colors::Cyan400 : Colors::Zinc300, false, 11)) {
        currentTool = EditorTool::DrawWall;
    }
    tbX += tbW + 4.0f;
    if (Widgets::button(currentTool == EditorTool::AddDock ? "[x] DOCK" : "+ DOCK (3)", { tbX, curY, tbW, 24.0f }, currentTool == EditorTool::AddDock ? Colors::Zinc700 : Colors::Zinc800, currentTool == EditorTool::AddDock ? Colors::Green400 : Colors::Zinc300, false, 11)) {
        currentTool = EditorTool::AddDock;
    }
    tbX += tbW + 4.0f;
    const char* snapLabel = (gridSnap == 0.0f) ? "SNAP: OFF" : TextFormat("SNAP: %d", static_cast<int>(gridSnap));
    if (Widgets::button(snapLabel, { tbX, curY, tbW, 24.0f }, Colors::Zinc800, (gridSnap > 0.0f) ? Colors::Amber400 : Colors::Zinc400, false, 11)) {
        if (gridSnap == 0.0f) gridSnap = 10.0f;
        else if (gridSnap == 10.0f) gridSnap = 20.0f;
        else if (gridSnap == 20.0f) gridSnap = 40.0f;
        else gridSnap = 0.0f;
    }
    curY += 30.0f;

    // Sector Selector Spinner
    int secIdx = selectedSectorIndex;
    int maxSec = static_cast<int>(campaignMgr.sectors.size()) - 1;
    Widgets::spinner("Target Sector:", { curX, curY }, secIdx, 0, maxSec, false, 185);
    if (secIdx != selectedSectorIndex) {
        loadSector(campaignMgr, secIdx);
    }
    curY += 30.0f;

    // 4. Tab Navigation Header
    const char* tabNames[] = { "META", "MAP", "PROG", "SHIPS", "WALLS" };
    float tabBtnW = (boxW - 16.0f) / 5.0f;
    for (int t = 0; t < 5; ++t) {
        SectorEditorTab tabEnum = static_cast<SectorEditorTab>(t);
        bool isCurrent = (currentTab == tabEnum);
        Rectangle tabRect = { curX + static_cast<float>(t) * (tabBtnW + 4.0f), curY, tabBtnW, 26.0f };
        Color bg = isCurrent ? Colors::Amber500 : Colors::Zinc800;
        Color fg = isCurrent ? Colors::Zinc950 : Colors::Zinc300;

        if (Widgets::button(tabNames[t], tabRect, bg, fg, false, 12)) {
            currentTab = tabEnum;
            nameActive = false;
            codenameActive = false;
            subtitleActive = false;
            descActive = false;
            depotTitleActive = false;
        }
    }
    curY += 34.0f;
    DrawLine(static_cast<int>(panelX + 12.0f), static_cast<int>(curY), static_cast<int>(panelX + panelW - 12.0f), static_cast<int>(curY), Colors::Zinc800);
    curY += 12.0f;

    // Content Body based on Active Tab
    if (currentTab == SectorEditorTab::Meta) {
        DrawText("SECTOR IDENTITY & DOSSIER", static_cast<int>(curX), static_cast<int>(curY), 13, Colors::Amber500);
        curY += 22.0f;

        DrawText("Display Name:", static_cast<int>(curX), static_cast<int>(curY), 13, Colors::Zinc400);
        curY += 16.0f;
        Widgets::textInput({ curX, curY, boxW, 26.0f }, nameBuf, sizeof(nameBuf), nameActive, "e.g. Sector 01: Outpost Alpha", true, 13);
        curY += 32.0f;

        DrawText("Tactical Codename:", static_cast<int>(curX), static_cast<int>(curY), 13, Colors::Zinc400);
        curY += 16.0f;
        Widgets::textInput({ curX, curY, boxW, 26.0f }, codenameBuf, sizeof(codenameBuf), codenameActive, "e.g. OUTPOST-ALPHA", true, 13);
        curY += 32.0f;

        DrawText("Subtitle & Role:", static_cast<int>(curX), static_cast<int>(curY), 13, Colors::Zinc400);
        curY += 16.0f;
        Widgets::textInput({ curX, curY, boxW, 26.0f }, subtitleBuf, sizeof(subtitleBuf), subtitleActive, "e.g. Logistics Depot", true, 13);
        curY += 32.0f;

        Widgets::spinner("Threat Level (1-5):", { curX, curY }, workingConfig.threatLevel, 1, 5, false, 185);
        curY += 34.0f;

        DrawText("Tactical Briefing / Lore Description:", static_cast<int>(curX), static_cast<int>(curY), 13, Colors::Zinc400);
        curY += 16.0f;
        Widgets::textInput({ curX, curY, boxW, 56.0f }, descBuf, sizeof(descBuf), descActive, "Operational details...", true, 12);
        curY += 66.0f;
    }
    else if (currentTab == SectorEditorTab::Map) {
        DrawText("MINEFIELD ARENA & GEOMETRY", static_cast<int>(curX), static_cast<int>(curY), 13, Colors::Amber500);
        curY += 22.0f;

        int prevGrid = workingConfig.gridSize;
        Widgets::spinner("Grid Size (NxN):", { curX, curY }, workingConfig.gridSize, 6, 24, false, 185);
        if (workingConfig.gridSize != prevGrid && useBombPercentage) {
            updateBombCountFromPercentage();
        }
        curY += 30.0f;

        Widgets::checkbox("Use Constant Bomb %:", { curX, curY }, useBombPercentage);
        curY += 28.0f;

        int totalPlayable = getPlayableCellsEstimate();
        int maxBombs = std::max(1, totalPlayable - 5);

        if (useBombPercentage) {
            int prevPct = bombPercentage;
            Widgets::spinner("Bomb Ratio (%):", { curX, curY }, bombPercentage, 5, 50, false, 185);
            if (bombPercentage != prevPct) {
                updateBombCountFromPercentage();
            }
            curY += 24.0f;
            DrawText(TextFormat("= %d bombs (out of %d playable tiles)", workingConfig.bombCount, totalPlayable), static_cast<int>(curX + 6.0f), static_cast<int>(curY), 11, Colors::Zinc400);
            curY += 18.0f;
        } else {
            int prevBombs = workingConfig.bombCount;
            Widgets::spinner("Bomb Count:", { curX, curY }, workingConfig.bombCount, 1, maxBombs, false, 185);
            if (workingConfig.bombCount != prevBombs) {
                bombPercentage = std::clamp(getCalculatedPercentage(), 5, 50);
            }
            curY += 24.0f;
            DrawText(TextFormat("= ~%d%% density (%d of %d playable tiles)", getCalculatedPercentage(), workingConfig.bombCount, totalPlayable), static_cast<int>(curX + 6.0f), static_cast<int>(curY), 11, Colors::Zinc400);
            curY += 18.0f;
        }

        int shapeIdx = 0;
        if (workingConfig.terrainShape == core::TerrainShape::CellularAutomata) shapeIdx = 1;
        else if (workingConfig.terrainShape == core::TerrainShape::VoronoiFaultLine) shapeIdx = 2;
        else if (workingConfig.terrainShape == core::TerrainShape::Rectangle) shapeIdx = 3;

        int prevShape = shapeIdx;
        Widgets::spinner(TextFormat("Shape [%s]:", workingConfig.terrainShapeName.c_str()), { curX, curY }, shapeIdx, 0, 3, false, 185);
        if (shapeIdx != prevShape) {
            if (shapeIdx == 0) { workingConfig.terrainShape = core::TerrainShape::PerlinIsland; workingConfig.terrainShapeName = "PerlinIsland"; }
            else if (shapeIdx == 1) { workingConfig.terrainShape = core::TerrainShape::CellularAutomata; workingConfig.terrainShapeName = "CellularAutomata"; }
            else if (shapeIdx == 2) { workingConfig.terrainShape = core::TerrainShape::VoronoiFaultLine; workingConfig.terrainShapeName = "VoronoiFaultLine"; }
            else { workingConfig.terrainShape = core::TerrainShape::Rectangle; workingConfig.terrainShapeName = "Rectangle"; }
            if (useBombPercentage) {
                updateBombCountFromPercentage();
            }
        }
        curY += 32.0f;

        int wt = static_cast<int>(workingConfig.wallThickness);
        Widgets::spinner("Perimeter Wall Thick:", { curX, curY }, wt, 12, 64, false, 185);
        workingConfig.wallThickness = static_cast<float>(wt);
        curY += 32.0f;

        int wm = static_cast<int>(workingConfig.westMargin);
        Widgets::spinner("West Margin (px):", { curX, curY }, wm, 60, 400, false, 185);
        workingConfig.westMargin = static_cast<float>(wm);
        curY += 32.0f;

        int em = static_cast<int>(workingConfig.eastMargin);
        Widgets::spinner("East Margin (px):", { curX, curY }, em, 60, 400, false, 185);
        workingConfig.eastMargin = static_cast<float>(em);
        curY += 32.0f;

        int vm = static_cast<int>(workingConfig.vertMargin);
        Widgets::spinner("Vertical Margin (px):", { curX, curY }, vm, 60, 400, false, 185);
        workingConfig.vertMargin = static_cast<float>(vm);
        curY += 32.0f;

        int spX = static_cast<int>(workingConfig.spawnPos.x);
        Widgets::spinner("Spawn X Pos (px):", { curX, curY }, spX, 20, 800, false, 185);
        workingConfig.spawnPos.x = static_cast<float>(spX);
        curY += 32.0f;

        int dpX = static_cast<int>(workingConfig.stagingDepotPos.x);
        Widgets::spinner("Depot X Pos (px):", { curX, curY }, dpX, 20, 800, false, 185);
        workingConfig.stagingDepotPos.x = static_cast<float>(dpX);
        curY += 32.0f;

        DrawText("Staging Depot Floor Title:", static_cast<int>(curX), static_cast<int>(curY), 13, Colors::Zinc400);
        curY += 16.0f;
        Widgets::textInput({ curX, curY, boxW, 26.0f }, depotTitleBuf, sizeof(depotTitleBuf), depotTitleActive, "e.g. LOGISTICS DOCK", true, 13);
        curY += 32.0f;
    }
    else if (currentTab == SectorEditorTab::Progression) {
        DrawText("PROGRESSION & ORBITAL TRANSIT", static_cast<int>(curX), static_cast<int>(curY), 13, Colors::Amber500);
        curY += 22.0f;

        Widgets::checkbox("Initially Unlocked (Accessible at start)", { curX, curY }, workingConfig.isUnlocked);
        curY += 32.0f;

        Widgets::checkbox("Has Orbital Launcher Mass Driver Exit", { curX, curY }, workingConfig.hasExitLauncher);
        curY += 32.0f;

        if (workingConfig.hasExitLauncher) {
            Widgets::spinner("Target Sector ID:", { curX, curY }, workingConfig.launcherTargetSector, -1, 10, false, 185);
            curY += 32.0f;

            int oh = static_cast<int>(workingConfig.launcherOpeningHeight);
            Widgets::spinner("Opening Height (px):", { curX, curY }, oh, 60, 300, false, 185);
            workingConfig.launcherOpeningHeight = static_cast<float>(oh);
            curY += 34.0f;
        }

        DrawText("Unlocks Sectors On Completion:", static_cast<int>(curX), static_cast<int>(curY), 13, Colors::Zinc400);
        curY += 22.0f;

        for (int sId = 1; sId <= 4; ++sId) {
            bool hasUnlock = std::find(workingConfig.unlocks.begin(), workingConfig.unlocks.end(), sId) != workingConfig.unlocks.end();
            bool toggleUnlock = hasUnlock;
            const char* unlLabel = TextFormat("Sector %02d", sId);
            int col = (sId - 1) % 2;
            int row = (sId - 1) / 2;
            Widgets::checkbox(unlLabel, { curX + static_cast<float>(col) * 180.0f, curY + static_cast<float>(row) * 28.0f }, toggleUnlock);
            if (toggleUnlock != hasUnlock) {
                if (toggleUnlock) {
                    workingConfig.unlocks.push_back(sId);
                    std::sort(workingConfig.unlocks.begin(), workingConfig.unlocks.end());
                } else {
                    workingConfig.unlocks.erase(std::remove(workingConfig.unlocks.begin(), workingConfig.unlocks.end(), sId), workingConfig.unlocks.end());
                }
            }
        }
        curY += 60.0f;
    }
    else if (currentTab == SectorEditorTab::Ships) {
        DrawText("HETEROGENEOUS MERCHANT DOCKS", static_cast<int>(curX), static_cast<int>(curY), 13, Colors::Amber500);
        curY += 20.0f;

        if (Widgets::button("+ ADD MERCHANT DOCK", { curX, curY, 180.0f, 26.0f }, Colors::Zinc800, Colors::Green500, false, 12)) {
            core::MerchantSpawn ms;
            ms.type = "shop1";
            ms.pos = { 85.0f, 150.0f + static_cast<float>(workingConfig.merchantSpawns.size()) * 120.0f };
            ms.angle = 0.0f;
            workingConfig.merchantSpawns.push_back(ms);
            selectedDockIndex = static_cast<int>(workingConfig.merchantSpawns.size()) - 1;
            selectionType = EditorSelectionType::MerchantDock;
            selectedIndex = selectedDockIndex;
            applyLive(campaignMgr);
        }
        curY += 32.0f;

        int numDocks = static_cast<int>(workingConfig.merchantSpawns.size());
        if (numDocks == 0) {
            DrawText("No merchant docks defined in this sector.", static_cast<int>(curX), static_cast<int>(curY + 6.0f), 13, Colors::Zinc500);
            curY += 24.0f;
        } else {
            selectedDockIndex = std::clamp(selectedDockIndex, 0, numDocks - 1);
            Widgets::spinner("Select Dock #:", { curX, curY }, selectedDockIndex, 0, numDocks - 1, false, 185);
            curY += 32.0f;

            auto& ms = workingConfig.merchantSpawns[selectedDockIndex];
            const char* typeHdr = TextFormat("Dock #%d Ship Type: [%s]", selectedDockIndex + 1, ms.type.c_str());
            DrawText(typeHdr, static_cast<int>(curX), static_cast<int>(curY), 12, Colors::Amber400);
            curY += 18.0f;

            // Type Buttons Row: [MINI 1] [CARGO 2] [HEAVY 3] [ROULETTE]
            float btnW = (boxW - 12.0f) / 4.0f;
            if (Widgets::button("MINI 1", { curX, curY, btnW, 24.0f }, (ms.type == "shop1") ? Colors::Cyan500 : Colors::Zinc800, (ms.type == "shop1") ? Colors::Zinc950 : Colors::Cyan400, false, 10)) {
                ms.type = "shop1";
                applyLive(campaignMgr);
            }
            if (Widgets::button("CARGO 2", { curX + btnW + 4.0f, curY, btnW, 24.0f }, (ms.type == "shop2") ? Colors::Green600 : Colors::Zinc800, (ms.type == "shop2") ? Colors::Zinc950 : Colors::Green400, false, 10)) {
                ms.type = "shop2";
                applyLive(campaignMgr);
            }
            if (Widgets::button("HEAVY 3", { curX + (btnW + 4.0f) * 2.0f, curY, btnW, 24.0f }, (ms.type == "shop3") ? Colors::Amber500 : Colors::Zinc800, (ms.type == "shop3") ? Colors::Zinc950 : Colors::Amber300, false, 10)) {
                ms.type = "shop3";
                applyLive(campaignMgr);
            }
            if (Widgets::button("ROULETTE", { curX + (btnW + 4.0f) * 3.0f, curY, btnW, 24.0f }, (ms.type == "roulette") ? Color{ 168, 85, 247, 255 } : Colors::Zinc800, (ms.type == "roulette") ? Colors::Zinc950 : Color{ 192, 132, 252, 255 }, false, 10)) {
                ms.type = "roulette";
                applyLive(campaignMgr);
            }
            curY += 30.0f;

            int dx = static_cast<int>(ms.pos.x);
            int dy = static_cast<int>(ms.pos.y);
            Widgets::spinner("Dock X Pos:", { curX, curY }, dx, -500, 2000, false, 185);
            ms.pos.x = static_cast<float>(dx);
            curY += 28.0f;

            Widgets::spinner("Dock Y Pos:", { curX, curY }, dy, -500, 2000, false, 185);
            ms.pos.y = static_cast<float>(dy);
            curY += 28.0f;

            int da = static_cast<int>(ms.angle);
            Widgets::spinner("Dock Angle:", { curX, curY }, da, 0, 360, false, 185);
            ms.angle = static_cast<float>(da);
            curY += 32.0f;

            if (Widgets::button("DELETE THIS DOCK", { curX, curY, 150.0f, 24.0f }, Colors::Zinc900, Colors::Red500, false, 11)) {
                workingConfig.merchantSpawns.erase(workingConfig.merchantSpawns.begin() + selectedDockIndex);
                if (selectedDockIndex > 0) --selectedDockIndex;
                applyLive(campaignMgr);
            }
            curY += 32.0f;
        }

        // Global Shop / Roulette Toggles for legacy compatibility
        Widgets::checkbox("Allow Contractor Merchant Ships", { curX, curY }, workingConfig.allowShops);
        curY += 26.0f;
        Widgets::checkbox("Allow Roulette Casino Ship", { curX, curY }, workingConfig.allowRoulette);
        curY += 28.0f;
    }
    else if (currentTab == SectorEditorTab::Walls) {
        DrawText("CUSTOM HAZARDS & INTERIOR WALLS", static_cast<int>(curX), static_cast<int>(curY), 13, Colors::Amber500);
        curY += 20.0f;

        if (Widgets::button("+ ADD INTERIOR WALL", { curX, curY, 180.0f, 26.0f }, Colors::Zinc800, Colors::Green500, false, 12)) {
            core::SectorWall newWall;
            newWall.rect = { 100.0f, 100.0f, 32.0f, 120.0f };
            newWall.isHazard = false;
            workingConfig.customWalls.push_back(newWall);
            selectedWallIndex = static_cast<int>(workingConfig.customWalls.size()) - 1;
            selectionType = EditorSelectionType::Wall;
            selectedIndex = selectedWallIndex;
            applyLive(campaignMgr);
        }
        curY += 32.0f;

        int numWalls = static_cast<int>(workingConfig.customWalls.size());
        if (numWalls == 0) {
            DrawText("No custom walls defined in this sector.", static_cast<int>(curX), static_cast<int>(curY + 10.0f), 13, Colors::Zinc500);
        } else {
            selectedWallIndex = std::clamp(selectedWallIndex, 0, numWalls - 1);
            Widgets::spinner("Select Wall #:", { curX, curY }, selectedWallIndex, 0, numWalls - 1, false, 185);
            curY += 32.0f;

            auto& w = workingConfig.customWalls[selectedWallIndex];
            int wx = static_cast<int>(w.rect.x);
            int wy = static_cast<int>(w.rect.y);
            int ww = static_cast<int>(w.rect.width);
            int wh = static_cast<int>(w.rect.height);

            Widgets::spinner("Wall X Pos:", { curX, curY }, wx, -500, 2000, false, 185);
            w.rect.x = static_cast<float>(wx);
            curY += 28.0f;

            Widgets::spinner("Wall Y Pos:", { curX, curY }, wy, -500, 2000, false, 185);
            w.rect.y = static_cast<float>(wy);
            curY += 28.0f;

            Widgets::spinner("Width (px):", { curX, curY }, ww, 8, 800, false, 185);
            w.rect.width = static_cast<float>(ww);
            curY += 28.0f;

            Widgets::spinner("Height (px):", { curX, curY }, wh, 8, 800, false, 185);
            w.rect.height = static_cast<float>(wh);
            curY += 30.0f;

            Widgets::checkbox("Hazard Caution Stripes (H)", { curX, curY }, w.isHazard);
            curY += 30.0f;

            if (Widgets::button("DELETE THIS WALL (Del)", { curX, curY, 170.0f, 26.0f }, Colors::Zinc900, Colors::Red500, false, 11)) {
                workingConfig.customWalls.erase(workingConfig.customWalls.begin() + selectedWallIndex);
                if (selectedWallIndex > 0) --selectedWallIndex;
                selectionType = EditorSelectionType::None;
                selectedIndex = -1;
                applyLive(campaignMgr);
            }
        }
    }

    // 5. Actions Footer Bar
    float expY = panelY + panelH - 98.0f;
    DrawText("Export Folder:", static_cast<int>(curX), static_cast<int>(expY + 5.0f), 11, Colors::Zinc400);
    Rectangle expRec = { curX + 90.0f, expY, boxW - 90.0f, 24.0f };
    Widgets::textInput(expRec, exportFolderBuf, sizeof(exportFolderBuf), exportFolderActive, "assets/campaign/sectors/custom", true, 11);

    float actY = panelY + panelH - 66.0f;
    float actBtnW = (boxW - 16.0f) / 3.0f;

    // APPLY LIVE (Amber)
    if (Widgets::button("APPLY LIVE", { curX, actY, actBtnW, 32.0f }, Colors::Zinc800, Colors::Amber500, false, 13)) {
        applyLive(campaignMgr);
    }

    // EXPORT JSON (Green)
    if (Widgets::button("EXPORT JSON", { curX + actBtnW + 8.0f, actY, actBtnW, 32.0f }, Colors::Zinc800, Colors::Green500, false, 13)) {
        saveToDisk(campaignMgr);
    }

    // RELOAD (Cyan)
    if (Widgets::button("RELOAD", { curX + (actBtnW + 8.0f) * 2.0f, actY, actBtnW, 32.0f }, Colors::Zinc800, Colors::Cyan500, false, 13)) {
        reloadFromDisk(campaignMgr);
    }

    // Toast status message line
    if (toastTimer > 0.0f && !toastMessage.empty()) {
        float toastY = panelY + panelH - 24.0f;
        int msgW = MeasureText(toastMessage.c_str(), 12);
        float msgX = panelX + (panelW - msgW) * 0.5f;
        float fadeAlpha = std::clamp(toastTimer * 2.0f, 0.0f, 1.0f);
        DrawText(toastMessage.c_str(), static_cast<int>(msgX), static_cast<int>(toastY), 12, Fade(toastColor, fadeAlpha));
    }
}

} // namespace minesweeper::ui

#else

namespace minesweeper::ui {
    void dummySectorEditorSymbol() {}
}

#endif
