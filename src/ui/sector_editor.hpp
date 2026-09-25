#pragma once

#include "raylib.h"
#include "../core/campaign.hpp"
#include <string>
#include <vector>

#define CAMPAIGN_EDITOR_ENABLED 1

namespace minesweeper::ui {

#ifdef CAMPAIGN_EDITOR_ENABLED

enum class SectorEditorTab {
    Meta = 0,
    Map = 1,
    Progression = 2,
    Ships = 3,
    Walls = 4
};

enum class EditorTool {
    Select = 0,
    DrawWall = 1,
    AddDock = 2
};

enum class EditorSelectionType {
    None = 0,
    Wall = 1,
    MerchantDock = 2,
    PlayerSpawn = 3,
    StagingDepot = 4
};

enum class DragMode {
    None = 0,
    MoveObject = 1,
    ResizeTopLeft = 2,
    ResizeTopRight = 3,
    ResizeBottomLeft = 4,
    ResizeBottomRight = 5,
    DrawingNewWall = 6
};

class SectorEditor {
public:
    bool isOpen = false;
    SectorEditorTab currentTab = SectorEditorTab::Meta;
    int selectedSectorIndex = 0;

    // In-world visual editing state
    EditorTool currentTool = EditorTool::Select;
    EditorSelectionType selectionType = EditorSelectionType::None;
    int selectedIndex = -1; // Index in customWalls, merchantSpawns, etc.
    DragMode dragMode = DragMode::None;
    float gridSnap = 20.0f; // 0 (OFF), 10, 20, 40
    bool useBombPercentage = false;
    int bombPercentage = 15;
    int getPlayableCellsEstimate() const;
    void updateBombCountFromPercentage();
    int getCalculatedPercentage() const;

    SectorEditor();

    void open(core::CampaignManager& campaignMgr, int defaultSectorIdx = -1);
    void close();
    void toggle(core::CampaignManager& campaignMgr);

    bool isMouseOver(Vector2 mousePos) const;
    bool isDragging() const { return dragMode != DragMode::None; }

    void updateWorldInteraction(core::CampaignManager& campaignMgr, Vector2 worldMouse, float dt);
    void drawWorldGizmos(const core::CampaignManager& campaignMgr, Vector2 worldMouse);

    void drawAndProcess(core::CampaignManager& campaignMgr, int screenW, int screenH);

    const core::SectorConfig& getWorkingConfig() const { return workingConfig; }

    float snapCoord(float val) const;
    Vector2 snapVec(Vector2 v) const;

private:
    Rectangle panelRect{ 0, 0, 0, 0 };
    core::SectorConfig workingConfig;
    bool isLoaded = false;

    // Interaction tracking
    Vector2 dragStartMouse{ 0, 0 };
    Vector2 dragStartPos{ 0, 0 };
    Rectangle dragStartRect{ 0, 0, 0, 0 };
    Rectangle createWallPreview{ 0, 0, 0, 0 };
    Vector2 createWallStart{ 0, 0 };
    int hoveredHandle = -1; // 0: TL, 1: TR, 2: BL, 3: BR

    // Text buffers for active editing
    char nameBuf[128]{};
    char codenameBuf[64]{};
    char subtitleBuf[128]{};
    char descBuf[512]{};
    char depotTitleBuf[128]{};

    bool nameActive = false;
    bool codenameActive = false;
    bool subtitleActive = false;
    bool descActive = false;
    bool depotTitleActive = false;
    char exportFolderBuf[256]{ "assets/campaign/sectors/custom" };
    bool exportFolderActive = false;

    // Toast status feedback
    std::string toastMessage;
    float toastTimer = 0.0f;
    Color toastColor{ 74, 222, 128, 255 }; // Green400

    int selectedWallIndex = 0;
    int selectedDockIndex = 0;

    void loadSector(core::CampaignManager& campaignMgr, int sectorIdx);
    void syncBuffersFromConfig();
    void syncConfigFromBuffers();
    void applyLive(core::CampaignManager& campaignMgr);
    void saveToDisk(core::CampaignManager& campaignMgr);
    void reloadFromDisk(core::CampaignManager& campaignMgr);

};

#else

// Release stub: completely empty inline operations with zero overhead
class SectorEditor {
public:
    bool isOpen = false;
    void open(core::CampaignManager&, int = -1) {}
    void close() {}
    void toggle(core::CampaignManager&) {}
    bool isMouseOver(Vector2) const { return false; }
    bool isDragging() const { return false; }
    void updateWorldInteraction(core::CampaignManager&, Vector2, float) {}
    void drawWorldGizmos(const core::CampaignManager&, Vector2) {}
    void drawAndProcess(core::CampaignManager&, int, int) {}
    float snapCoord(float val) const { return val; }
    Vector2 snapVec(Vector2 v) const { return v; }
};

#endif

} // namespace minesweeper::ui
