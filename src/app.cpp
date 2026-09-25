#include "app.hpp"
#include "net/steam_manager.hpp"
#include "ui/widgets.hpp"
#include "render/procedural_textures.hpp"
#include "core/item.hpp"
#include <fstream>
#include <iostream>
#include <cmath>
#include <cstring>
#include <algorithm>

namespace minesweeper {

App::App() = default;

App::~App() {
    cleanup();
}

void App::init() {
    if (!DirectoryExists("assets") && DirectoryExists("../assets")) {
        ChangeDirectory("..");
    } else if (!DirectoryExists("assets")) {
        ChangeDirectory(GetApplicationDirectory());
    }

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1000, 900, "Dimension Sweeper");
    SetWindowMinSize(800, 600);
    SetTargetFPS(144);

    net.initialize();

    saveMgr.init();
    menu.saveManager = &saveMgr;
    menu.campaignManager = &campaignMgr;
    loadSettings();

    if (saveMgr.hasCampaignSave()) {
        loadCampaignProgress();
    } else {
        campaignMgr.init(12345);
    }

    // Setup Steam Overlay lobby join callback
    net::SteamManager::instance().setLobbyJoinRequestedCallback([this](uint64_t lobbyId) {
        std::cout << "[STEAM] Joining lobby from Overlay request: " << lobbyId << std::endl;
        if (net.connectToSteamLobby(lobbyId)) {
            board.init(2, 0, 0, 0);
            state = AppState::InGame;
            menu.statusMessage.clear();
        }
    });

    if (std::strcmp(menu.playerName, "Player") == 0 && net::SteamManager::instance().isSteamActive()) {
        const char* persona = net::SteamManager::instance().getPersonaName();
        if (persona && persona[0] != '\0') {
            std::strncpy(menu.playerName, persona, sizeof(menu.playerName) - 1);
            menu.playerName[sizeof(menu.playerName) - 1] = '\0';
        }
    }

    if (initialLobbyId != 0 && net::SteamManager::instance().isSteamActive()) {
        std::cout << "[STEAM] Joining initial lobby from command line: " << initialLobbyId << std::endl;
        if (net.connectToSteamLobby(initialLobbyId)) {
            board.init(2, 0, 0, 0);
            state = AppState::InGame;
        }
    }

    renderer.enableCRT = menu.crtEnabled;
    renderer.camera.enableCRT = menu.crtEnabled;
    renderer.init();
    core::ItemCatalog::instance().init();
    for (auto& s : renderer.shopShips) {
        s.initializeInventory();
    }
    if (render::RaylibRenderer::getCursorSkinCount() > 0) {
        menu.cursorSkin = std::clamp<int>(menu.cursorSkin, 0, render::RaylibRenderer::getCursorSkinCount() - 1);
    }
    if (render::RaylibRenderer::getFlagSkinCount() > 0) {
        menu.flagSkin = std::clamp<int>(menu.flagSkin, 0, render::RaylibRenderer::getFlagSkinCount() - 1);
    }
    if (render::RaylibRenderer::getPlayerSkinCount() > 0) {
        menu.playerSkin = std::clamp<int>(menu.playerSkin, 0, render::RaylibRenderer::getPlayerSkinCount() - 1);
    }
    renderer.activeFlagSkin = menu.flagSkin;
    renderer.activePlayerSkin = menu.playerSkin;

    if (testEditorMode) {
        menu.currentScreen = ui::MenuScreen::Main;
    }

    loadSaveSlot(activeSaveSlot);

    if (menu.vsyncEnabled) {
        SetWindowState(FLAG_VSYNC_HINT);
    } else {
        ClearWindowState(FLAG_VSYNC_HINT);
    }

    if (menu.fpsLimit >= 245.0f) {
        SetTargetFPS(0);
    } else {
        SetTargetFPS(static_cast<int>(menu.fpsLimit));
    }

    ui::Widgets::setScale(menu.guiScale);
    renderer.guiScale = menu.guiScale;

    voiceMgr.init();
    voiceMgr.getSettings() = menu.voiceSettings;
    net.onVoiceReceived = [this](const net::PacketVoice& pkt) {
        voiceMgr.receiveVoicePacket(pkt);
    };

    soundMgr.init();
    soundMgr.setEnabled(menu.bgmEnabled);
    soundMgr.setVolume(menu.bgmVolume);
    soundMgr.setSfxEnabled(menu.sfxEnabled);
    soundMgr.setSfxVolume(menu.sfxVolume);

    scrapSystem.init();
    scrapSystem.setSharedTextures(
        render::ProceduralTextures::instance().shadowTexture,
        render::ProceduralTextures::instance().glowTexture,
        render::ProceduralTextures::instance().particleTexture
    );
    hud.scrapTexture = scrapSystem.texture;
    menu.scrapTexture = scrapSystem.texture;
    hud.scrapCount = scrapCount;
    menu.scrapCount = scrapCount;
}

void App::cleanup() {
    saveCurrentSlot();
    saveSettings();
    soundMgr.cleanup();
    scrapSystem.cleanup();
    voiceMgr.cleanup();
    core::ItemCatalog::instance().shutdown();
    renderer.cleanup();
    net.cleanup();
    CloseWindow();
}

void App::loadSettings() {
    core::GlobalSettings gs;
    if (saveMgr.loadGlobalSettings(gs)) {
        std::strncpy(menu.joinIpBuf, gs.joinIp, sizeof(menu.joinIpBuf));
        std::strncpy(menu.hostPortBuf, gs.hostPort, sizeof(menu.hostPortBuf));
        std::strncpy(menu.playerName, gs.playerName, sizeof(menu.playerName));
        std::strncpy(net.playerName, gs.playerName, sizeof(net.playerName));

        menu.crtEnabled = gs.crtEnabled;
        menu.cursorSkin = gs.cursorSkin;
        menu.flagSkin = gs.flagSkin;
        menu.playerSkin = gs.playerSkin;
        hud.randomizeSeed = gs.randomizeSeed;

        menu.voiceSettings.enabled = gs.voiceEnabled;
        menu.voiceSettings.proximity = gs.voiceProximity;
        menu.voiceSettings.pushToTalk = gs.voicePushToTalk;
        menu.voiceSettings.voiceVolume = gs.voiceVolume;
        menu.voiceSettings.micGain = gs.micGain;

        menu.bgmEnabled = gs.bgmEnabled;
        menu.bgmVolume = gs.bgmVolume;
        menu.sfxEnabled = gs.sfxEnabled;
        menu.sfxVolume = gs.sfxVolume;

        menu.vsyncEnabled = gs.vsyncEnabled;
        menu.showFPS = gs.showFPS;
        menu.fpsLimit = gs.fpsLimit;
        menu.guiScale = std::clamp(gs.guiScale, 0.75f, 1.50f);
        menu.controlMode = std::clamp(gs.controlMode, 0, 1);

        activeSaveSlot = std::clamp(gs.lastActiveSlot, 1, core::SaveManager::NUM_SLOTS);
        menu.selectedSlot = activeSaveSlot;
    }
}

void App::saveSettings() {
    core::GlobalSettings gs;
    std::strncpy(gs.joinIp, menu.joinIpBuf, sizeof(gs.joinIp));
    std::strncpy(gs.hostPort, menu.hostPortBuf, sizeof(gs.hostPort));
    std::strncpy(gs.playerName, menu.playerName, sizeof(menu.playerName));
    gs.crtEnabled = menu.crtEnabled;
    gs.cursorSkin = static_cast<uint8_t>(menu.cursorSkin);
    gs.flagSkin = static_cast<uint8_t>(menu.flagSkin);
    gs.playerSkin = static_cast<uint8_t>(menu.playerSkin);
    gs.randomizeSeed = hud.randomizeSeed;

    gs.voiceEnabled = menu.voiceSettings.enabled;
    gs.voiceProximity = menu.voiceSettings.proximity;
    gs.voicePushToTalk = menu.voiceSettings.pushToTalk;
    gs.voiceVolume = menu.voiceSettings.voiceVolume;
    gs.micGain = menu.voiceSettings.micGain;

    gs.bgmEnabled = menu.bgmEnabled;
    gs.bgmVolume = menu.bgmVolume;
    gs.sfxEnabled = menu.sfxEnabled;
    gs.sfxVolume = menu.sfxVolume;

    gs.vsyncEnabled = menu.vsyncEnabled;
    gs.showFPS = menu.showFPS;
    gs.fpsLimit = menu.fpsLimit;
    gs.guiScale = menu.guiScale;
    gs.lastActiveSlot = activeSaveSlot;
    gs.controlMode = menu.controlMode;

    saveMgr.saveGlobalSettings(gs);
}

void App::saveCurrentSlot() {
    if (net.role != net::NetRole::Client && activeSaveSlot >= 1 && activeSaveSlot <= core::SaveManager::NUM_SLOTS) {
        saveMgr.saveSlot(activeSaveSlot, activeSaveName, board, timePlayed, scrapCount);
    }
}

bool App::loadSaveSlot(int slotIndex) {
    activeSaveSlot = slotIndex;
    renderer.localShip.isInitialized = false;
    pendingUncoverCell = -1;
    renderer.clearOutOfReach();
    bool ok = saveMgr.loadSlot(slotIndex, board, timePlayed, scrapCount, activeSaveName);
    if (!ok) {
        activeSaveName = "World " + std::to_string(slotIndex);
        board.init(2, 10, 15, 12345);
        timePlayed = 0.0f;
        scrapCount = 0;
        saveCurrentSlot();
    }
    hud.init(board.config);
    hud.nextSeed = board.config.seed;
    std::snprintf(hud.seedBuf, sizeof(hud.seedBuf), "%llu", board.config.seed);
    hud.scrapCount = scrapCount;
    menu.scrapCount = scrapCount;

    // Center camera on board (or safe starting cell if fresh game)
    float boardWidth = board.config.size * renderer.cellSize;
    float sliceStride = boardWidth + renderer.slicePadding;
    Vector2 center = { boardWidth * 0.5f, boardWidth * 0.5f };
    if (board.revealedCount == 0 && board.startingCell >= 0) {
        center = renderer.getCellWorldPosition(static_cast<size_t>(board.startingCell), board);
    } else if (board.config.dim == 3) {
        center = { boardWidth * 0.5f, (board.config.size * sliceStride) * 0.5f };
    } else if (board.config.dim >= 4) {
        center = { (board.config.size * sliceStride) * 0.5f, (board.config.size * sliceStride) * 0.5f };
    }
    float initialZoom = 1.0f;
    if (board.config.size > 30) initialZoom = 30.0f / static_cast<float>(board.config.size);
    renderer.camera.reset({ 0.0f, 0.0f }, initialZoom);
    renderer.camera.centerOn(center);
    renderer.updateShopAnchor(board);
    renderer.localShip.position = center;
    renderer.localShip.velocity = { 0.0f, 0.0f };
    renderer.localShip.isMoving = false;
    renderer.localShip.isInitialized = true;
    return ok;
}

void App::startCampaignGame(bool isHost) {
    (void)isHost;
    currentMode = GameMode::Campaign;
    renderer.clearParticles();
    renderer.localShip.isInitialized = false;
    pendingUncoverCell = -1;
    currentHoveredCell = -1;
    renderer.clearOutOfReach();

    if (!loadCampaignProgress()) {
        campaignMgr.init(12345);
        saveCampaignProgress();
    }

    // Deploy directly into selected fortress if valid and unlocked
    int selSec = menu.campaignSelectedSector;
    if (selSec >= 0 && selSec < static_cast<int>(menu.planetRenderer.sectors.size())) {
        const auto& hSec = menu.planetRenderer.sectors[selSec];
        if (hSec.isFortress && hSec.fortressIdx >= 0 && hSec.fortressIdx < static_cast<int>(campaignMgr.sectors.size())) {
            if (campaignMgr.sectors[hSec.fortressIdx].isUnlocked) {
                campaignMgr.activeSectorIndex = hSec.fortressIdx;
            }
        }
    }

    board.isGameOver = false;
    board.isVictory = false;
    renderer.activeCampaignSector = campaignMgr.activeSectorIndex;

    // If active sector was uncleared and in game over state, reset its board so player can retry immediately
    if (campaignMgr.activeSectorIndex >= 0 && campaignMgr.activeSectorIndex < static_cast<int>(campaignMgr.sectors.size())) {
        auto& activeSec = campaignMgr.sectors[campaignMgr.activeSectorIndex];
        if (activeSec.board.isGameOver && !activeSec.isCleared) {
            activeSec.board.init(activeSec.board.config);
        }
    }

    Vector2 spawn = campaignMgr.getSectorSpawnPosition(campaignMgr.activeSectorIndex);
    renderer.localShip.position = spawn;
    renderer.localShip.velocity = { 0.0f, 0.0f };
    renderer.localShip.isInitialized = !sectorEditor.isOpen;
    renderer.camera.reset({ 0.0f, 0.0f }, 1.0f);
    renderer.camera.centerOn(spawn);
    renderer.updateShopAnchorCampaign(campaignMgr);

    hud.scrapCount = scrapCount;
    menu.scrapCount = scrapCount;
}

void App::triggerSectorWarp(int newSectorIdx) {
    if (newSectorIdx < 0 || newSectorIdx >= static_cast<int>(campaignMgr.sectors.size())) return;

    campaignMgr.activeSectorIndex = newSectorIdx;
    campaignMgr.sectors[newSectorIdx].isUnlocked = true;
    renderer.activeCampaignSector = newSectorIdx;

    Vector2 newSpawn = campaignMgr.getSectorSpawnPosition(newSectorIdx);
    renderer.localShip.position = newSpawn;
    renderer.localShip.velocity = { 180.0f, 0.0f };
    renderer.camera.reset({ 0.0f, 0.0f }, 1.0f);
    renderer.camera.centerOn(newSpawn);
    renderer.updateShopAnchorCampaign(campaignMgr);

    renderer.emitWarpSpawn(newSpawn, ui::Colors::Cyan400);
    soundMgr.playUncoverSound();

    pendingUncoverCell = -1;
    currentHoveredCell = -1;
    renderer.clearOutOfReach();

    std::cout << "[CAMPAIGN] Orbital launcher engaged! Transiting to " << campaignMgr.sectors[newSectorIdx].name << "!" << std::endl;
    saveCampaignProgress();
}

void App::saveCampaignProgress() {
    if (net.role != net::NetRole::Client) {
        saveMgr.saveCampaign(campaignMgr, timePlayed, scrapCount);
    }
}

bool App::loadCampaignProgress() {
    bool ok = saveMgr.loadCampaign(campaignMgr, timePlayed, scrapCount);
    hud.scrapCount = scrapCount;
    menu.scrapCount = scrapCount;
    return ok;
}

void App::startNewGame(int dim, int size, int bombs, uint64_t seed) {
    renderer.clearParticles();
    renderer.localShip.isInitialized = false;
    pendingUncoverCell = -1;
    currentHoveredCell = -1;
    renderer.clearOutOfReach();
    int leftover = scrapSystem.collectAll();
    if (leftover > 0) {
        scrapCount += leftover;
        hud.scrapCount = scrapCount;
        menu.scrapCount = scrapCount;
        saveSettings();
    }
    board.init(dim, size, bombs, seed);
    renderer.updateShopAnchor(board);
    hud.nextSeed = seed;
    std::snprintf(hud.seedBuf, sizeof(hud.seedBuf), "%llu", seed);
    timePlayed = 0.0f;
    hud.endModalDismissed = false;
    saveCurrentSlot();
    openedShopIndex = -1;
    nearbyShopIndex = -1;
    hoveredShopIndex = -1;
    isHoveredShopInRange = false;
    shopProximityAlpha = 0.0f;
    isRouletteOpen = false;
    isRouletteHovered = false;
    isRouletteInRange = false;
    rouletteProximityAlpha = 0.0f;

    // Center camera on safe starting cell
    float boardWidth = size * renderer.cellSize;
    float sliceStride = boardWidth + renderer.slicePadding;
    Vector2 center = { boardWidth * 0.5f, boardWidth * 0.5f };
    if (board.startingCell >= 0) {
        center = renderer.getCellWorldPosition(static_cast<size_t>(board.startingCell), board);
    } else if (dim == 3) {
        center = { boardWidth * 0.5f, (size * sliceStride) * 0.5f };
    } else if (dim >= 4) {
        center = { (size * sliceStride) * 0.5f, (size * sliceStride) * 0.5f };
    }

    float initialZoom = 1.0f;
    if (size > 30) initialZoom = 30.0f / static_cast<float>(size);
    renderer.camera.reset({ 0.0f, 0.0f }, initialZoom);
    renderer.camera.centerOn(center);
    renderer.localShip.position = center;
    renderer.localShip.velocity = { 0.0f, 0.0f };
    renderer.localShip.isMoving = false;
    renderer.localShip.isInitialized = true;
}

void App::restartCurrentGame() {
    uint64_t newSeed = hud.randomizeSeed
        ? (static_cast<uint64_t>(GetTime() * 100000.0) ^ 0x01252D8F21ULL)
        : board.config.seed;
    if (newSeed == 0) newSeed = 12345;

    hud.nextSeed = newSeed;
    std::snprintf(hud.seedBuf, sizeof(hud.seedBuf), "%llu", newSeed);

    if (net.role == net::NetRole::Host) {
        net::PacketInit p;
        p.dim = board.config.dim;
        p.size = board.config.size;
        p.bombs = board.config.bombs;
        p.seed = newSeed;
        net.broadcast(&p, sizeof(p));
    }

    startNewGame(board.config.dim, board.config.size, board.config.bombs, newSeed);
}

void App::handleNetEvents() {
    net.update();

    net::NetEvent ev;
    while (net.pollEvent(ev)) {
        switch (ev.type) {
            case net::NetEventType::ClientConnected: {
                if (net.role == net::NetRole::Host) {
                    uint32_t stateBytes = static_cast<uint32_t>(board.state.data.size() * sizeof(uint64_t));
                    uint32_t flagCount = static_cast<uint32_t>(board.flagOwners.size());
                    uint32_t flagBytes = flagCount * sizeof(net::PacketFlagSync);

                    std::vector<uint8_t> buffer(sizeof(net::PacketSyncHeader) + stateBytes + flagBytes);
                    auto* hdr = reinterpret_cast<net::PacketSyncHeader*>(buffer.data());
                    hdr->type = net::PacketType::Sync;
                    hdr->seed = board.config.seed;
                    hdr->dim = board.config.dim;
                    hdr->size = board.config.size;
                    hdr->bombs = board.config.bombs;
                    hdr->timePlayed = timePlayed;
                    hdr->isGameOver = board.isGameOver ? 1 : 0;
                    hdr->isVictory = board.isVictory ? 1 : 0;
                    hdr->stateWordCount = static_cast<uint32_t>(board.state.data.size());
                    hdr->flagCount = flagCount;

                    std::memcpy(buffer.data() + sizeof(net::PacketSyncHeader), board.state.data.data(), stateBytes);

                    if (flagCount > 0) {
                        auto* flagsPtr = reinterpret_cast<net::PacketFlagSync*>(buffer.data() + sizeof(net::PacketSyncHeader) + stateBytes);
                        size_t fi = 0;
                        for (const auto& [idx, info] : board.flagOwners) {
                            flagsPtr[fi].index = idx;
                            flagsPtr[fi].placerId = info.placerId;
                            flagsPtr[fi].skinId = info.skinId;
                            ++fi;
                        }
                    }

                    std::cout << "[HOST] Client " << ev.peerId << " connected. Sending PacketSync ("
                              << buffer.size() << " bytes, " << board.config.size << "x" << board.config.size 
                              << ", dim=" << board.config.dim << ", bombs=" << board.config.bombs 
                              << ", seed=" << board.config.seed << ")" << std::endl;

                    net.sendToPeer(ev.peerId, buffer.data(), buffer.size(), true);
                }
                break;
            }
            case net::NetEventType::ClientDisconnected: {
                // Keep flags placed by disconnected players on the board
                saveCurrentSlot();
                break;
            }
            case net::NetEventType::SyncBoard: {
                std::cout << "[CLIENT] Synchronized board from host: " << ev.syncData.header.size << "x" << ev.syncData.header.size 
                          << " (dim=" << ev.syncData.header.dim << ", bombs=" << ev.syncData.header.bombs 
                          << ", seed=" << ev.syncData.header.seed << ")" << std::endl;
                startNewGame(ev.syncData.header.dim, ev.syncData.header.size, ev.syncData.header.bombs, ev.syncData.header.seed);
                hud.init(board.config);

                if (board.state.data.size() == ev.syncData.stateWords.size()) {
                    board.state.data = ev.syncData.stateWords;
                    board.revealedCount = board.state.countRevealed();
                    board.flaggedCount = 0;
                    for (size_t i = 0; i < board.coord.totalCells; ++i) {
                        if (board.state.get(i) == core::CellState::Flagged) {
                            ++board.flaggedCount;
                        }
                    }
                }
                board.flagOwners.clear();
                for (const auto& entry : ev.syncData.flagEntries) {
                    board.flagOwners[entry.index] = { entry.placerId, entry.skinId };
                }
                timePlayed = ev.syncData.header.timePlayed;
                board.isGameOver = (ev.syncData.header.isGameOver != 0);
                board.isVictory = (ev.syncData.header.isVictory != 0);

                state = AppState::InGame;
                break;
            }
            case net::NetEventType::InitBoard: {
                std::cout << "[CLIENT] Host started new game: " << ev.initData.size << "x" << ev.initData.size 
                          << " (dim=" << ev.initData.dim << ", bombs=" << ev.initData.bombs 
                          << ", seed=" << ev.initData.seed << ")" << std::endl;
                startNewGame(ev.initData.dim, ev.initData.size, ev.initData.bombs, ev.initData.seed);
                hud.init(board.config);
                state = AppState::InGame;
                break;
            }
            case net::NetEventType::PlayerClick: {
                if (net.role == net::NetRole::Host) {
                    size_t idx = ev.clickData.index;
                    if (idx >= board.totalCells()) break;

                    if (ev.clickData.action == 0) { // Reveal
                        if (board.getState(idx) == core::CellState::Hidden) {
                            Vector2 pos = renderer.getCellWorldPosition(idx, board);
                            std::vector<size_t> newlyRevealed;
                            core::RevealResult res = board.reveal(idx, &newlyRevealed);
                            if (res == core::RevealResult::HitBomb) {
                                renderer.emitExplosion(pos, ui::Colors::CellFlag);
                                soundMgr.playExplosionSound();
                            } else {
                                renderer.emitDebris(pos, ui::Colors::Zinc400);
                                soundMgr.playUncoverSound();
                                for (size_t cIdx : newlyRevealed) {
                                    if (!board.isBomb(cIdx) && render::ScrapSystem::isScrapCell(board.config.seed, cIdx, board.totalCells(), board.config.bombs)) {
                                        Vector2 cPos = renderer.getCellWorldPosition(cIdx, board);
                                        scrapSystem.spawn(cPos);
                                    }
                                }
                            }

                            net::PacketResult pr;
                            pr.index = idx;
                            pr.state = 0;
                            pr.placerId = 0;
                            pr.flagSkin = 0;
                            net.broadcast(&pr, sizeof(pr));
                        }
                    } else if (ev.clickData.action == 1) { // Chord
                        if (board.getState(idx) == core::CellState::Revealed) {
                            std::vector<size_t> newlyRevealed;
                            bool hitBomb = false;
                            if (board.chord(idx, newlyRevealed, hitBomb)) {
                                if (hitBomb) {
                                    soundMgr.playExplosionSound();
                                } else if (!newlyRevealed.empty()) {
                                    soundMgr.playUncoverSound();
                                }
                                for (size_t revIdx : newlyRevealed) {
                                    Vector2 pos = renderer.getCellWorldPosition(revIdx, board);
                                    if (board.isBomb(revIdx)) {
                                        renderer.emitExplosion(pos, ui::Colors::CellFlag);
                                    } else {
                                        renderer.emitDebris(pos, ui::Colors::Zinc400);
                                        if (render::ScrapSystem::isScrapCell(board.config.seed, revIdx, board.totalCells(), board.config.bombs)) {
                                            scrapSystem.spawn(pos);
                                        }
                                    }

                                    net::PacketResult pr;
                                    pr.index = revIdx;
                                    pr.state = 0;
                                    pr.placerId = 0;
                                    pr.flagSkin = 0;
                                    net.broadcast(&pr, sizeof(pr));
                                }
                            }
                        }
                    } else if (ev.clickData.action == 2) { // Flag toggle
                        core::CellState cs = board.getState(idx);
                        Vector2 groundPos = renderer.getFlagBasePosition(idx, board);
                        Vector2 shipPos = (ev.peerId != 0 && renderer.remoteShips.count(ev.peerId)) ? renderer.remoteShips[ev.peerId].position : renderer.localShip.position;
                        if (cs == core::CellState::Flagged) {
                            uint8_t skinId = board.getFlagSkin(idx, ev.clickData.flagSkin);
                            renderer.triggerFlagPickup(groundPos, shipPos, ev.peerId, false, skinId);
                            renderer.removeFlagDrop(idx);
                            board.unflag(idx);
                            net::PacketResult pr;
                            pr.index = idx;
                            pr.state = 1; // Hidden / unflagged
                            pr.placerId = ev.peerId;
                            pr.flagSkin = skinId;
                            net.broadcast(&pr, sizeof(pr));
                        } else if (cs == core::CellState::Hidden) {
                            board.setFlag(idx, ev.peerId, ev.clickData.flagSkin);
                            renderer.triggerFlagDrop(idx, groundPos, shipPos, ev.clickData.flagSkin);
                            soundMgr.playFlagSound();
                            net::PacketResult pr;
                            pr.index = idx;
                            pr.state = 2; // Flagged
                            pr.placerId = ev.peerId;
                            pr.flagSkin = ev.clickData.flagSkin;
                            net.broadcast(&pr, sizeof(pr));
                        }
                    }
                    saveCurrentSlot();
                }
                break;
            }
            case net::NetEventType::BoardResult: {
                size_t idx = ev.resultData.index;
                if (ev.resultData.state == 0) {
                    if (board.getState(idx) == core::CellState::Flagged) {
                        Vector2 groundPos = renderer.getFlagBasePosition(idx, board);
                        Vector2 shipPos = (ev.resultData.placerId != 0 && renderer.remoteShips.count(ev.resultData.placerId)) ? renderer.remoteShips[ev.resultData.placerId].position : renderer.localShip.position;
                        bool isLocal = (ev.resultData.placerId == 0 || (net.role == net::NetRole::Host && ev.resultData.placerId == net::HOST_PLAYER_ID) || (renderer.remoteShips.count(ev.resultData.placerId) == 0));
                        uint8_t skinId = board.getFlagSkin(idx, ev.resultData.flagSkin);
                        renderer.triggerFlagPickup(groundPos, shipPos, ev.resultData.placerId, isLocal, skinId);
                        renderer.removeFlagDrop(idx);
                        board.unflag(idx);
                    }
                    std::vector<size_t> newlyRevealed;
                    core::RevealResult res = board.reveal(idx, &newlyRevealed);
                    for (size_t revIdx : newlyRevealed) {
                        renderer.removeFlagDrop(revIdx);
                    }
                    Vector2 pos = renderer.getCellWorldPosition(idx, board);
                    if (res == core::RevealResult::HitBomb) {
                        renderer.emitExplosion(pos, ui::Colors::CellFlag);
                        soundMgr.playExplosionSound();
                    } else {
                        renderer.emitDebris(pos, ui::Colors::Zinc400);
                        soundMgr.playUncoverSound();
                        for (size_t cIdx : newlyRevealed) {
                            if (!board.isBomb(cIdx) && render::ScrapSystem::isScrapCell(board.config.seed, cIdx, board.totalCells(), board.config.bombs)) {
                                Vector2 cPos = renderer.getCellWorldPosition(cIdx, board);
                                scrapSystem.spawn(cPos);
                            }
                        }
                    }
                    board.flagOwners.erase(idx);
                } else if (ev.resultData.state == 1) { // Unflagged
                    Vector2 groundPos = renderer.getFlagBasePosition(idx, board);
                    Vector2 shipPos = (ev.resultData.placerId != 0 && renderer.remoteShips.count(ev.resultData.placerId)) ? renderer.remoteShips[ev.resultData.placerId].position : renderer.localShip.position;
                    bool isLocal = (ev.resultData.placerId == 0 || (net.role == net::NetRole::Host && ev.resultData.placerId == net::HOST_PLAYER_ID) || (renderer.remoteShips.count(ev.resultData.placerId) == 0));
                    uint8_t skinId = board.getFlagSkin(idx, ev.resultData.flagSkin);
                    renderer.triggerFlagPickup(groundPos, shipPos, ev.resultData.placerId, isLocal, skinId);
                    renderer.removeFlagDrop(idx);
                    board.unflag(idx);
                } else if (ev.resultData.state == 2) { // Flagged
                    board.setFlag(idx, ev.resultData.placerId, ev.resultData.flagSkin);
                    Vector2 groundPos = renderer.getFlagBasePosition(idx, board);
                    Vector2 shipPos = (ev.resultData.placerId != 0 && renderer.remoteShips.count(ev.resultData.placerId)) ? renderer.remoteShips[ev.resultData.placerId].position : renderer.localShip.position;
                    renderer.triggerFlagDrop(idx, groundPos, shipPos, ev.resultData.flagSkin);
                    soundMgr.playFlagSound();
                }
                break;
            }
            case net::NetEventType::LaserFired: {
                uint32_t senderId = ev.laserData.playerID;
                Vector2 from = { ev.laserData.fromX, ev.laserData.fromY };
                Vector2 to = { ev.laserData.toX, ev.laserData.toY };

                if (senderId == 0 || (net.role == net::NetRole::Client && senderId == net::HOST_PLAYER_ID)) {
                    if (renderer.remoteShips.count(0)) {
                        from = renderer.remoteShips[0].getNosePosition();
                    } else if (renderer.remoteShips.count(net::HOST_PLAYER_ID)) {
                        from = renderer.remoteShips[net::HOST_PLAYER_ID].getNosePosition();
                    }
                } else if (renderer.remoteShips.count(senderId)) {
                    from = renderer.remoteShips[senderId].getNosePosition();
                }

                Color laserCol = (ev.laserData.laserType == 1)
                    ? ui::Colors::Red500
                    : renderer.getLaserColorForSkin(ev.laserData.skinId);

                renderer.fireLaser(from, to, laserCol);
                soundMgr.playLaserSound();
                break;
            }
            case net::NetEventType::BubbleTriggered: {
                Vector2 bubblePos = { ev.bubbleData.x, ev.bubbleData.y };
                activateBubbleEffect(bubblePos, false);
                break;
            }
            default:
                break;
        }
    }
}

void App::broadcastLaser(Vector2 from, Vector2 to, uint8_t laserType) {
    Color col = (laserType == 1) ? ui::Colors::Red500 : renderer.getLaserColorForSkin(menu.cursorSkin);
    renderer.fireLaser(from, to, col);
    soundMgr.playLaserSound();

    if (net.role != net::NetRole::Offline) {
        net::PacketLaser pkt;
        pkt.type = net::PacketType::Laser;
        pkt.playerID = (net.role == net::NetRole::Host) ? net::HOST_PLAYER_ID : 0;
        pkt.fromX = from.x;
        pkt.fromY = from.y;
        pkt.toX = to.x;
        pkt.toY = to.y;
        pkt.skinId = static_cast<uint8_t>(menu.cursorSkin);
        pkt.laserType = laserType;

        if (net.role == net::NetRole::Client) {
            net.sendToServer(&pkt, sizeof(pkt), false);
        } else if (net.role == net::NetRole::Host) {
            net.broadcast(&pkt, sizeof(pkt), false);
        }
    }
}

void App::broadcastBubble(Vector2 pos) {
    activateBubbleEffect(pos, true);

    if (net.role != net::NetRole::Offline) {
        net::PacketBubble pkt;
        pkt.type = net::PacketType::Bubble;
        pkt.playerID = (net.role == net::NetRole::Host) ? net::HOST_PLAYER_ID : 0;
        pkt.x = pos.x;
        pkt.y = pos.y;

        if (net.role == net::NetRole::Client) {
            net.sendToServer(&pkt, sizeof(pkt), false);
        } else if (net.role == net::NetRole::Host) {
            net.broadcast(&pkt, sizeof(pkt), false);
        }
    }
}

void App::activateBubbleEffect(Vector2 pos, bool isLocal) {
    if (!isLocal) {
        // Remote player actively bubbling: emit scattered bubbles and apply distance-based blur
        renderer.emitBubbles(pos, 2);
        renderer.applyBubbleBlurSource(pos, 650.0f);
    } else {
        // Local player distance = 0 blur
        renderer.applyBubbleBlurSource(pos, 650.0f);
    }
}

void App::update(float dt) {
    if (IsKeyPressed(KEY_F11)) {
        ToggleFullscreen();
    }

    if (IsKeyPressed(KEY_F12)) {
        TakeScreenshot("screenshot.png");
    }

#if defined(_DEBUG) || !defined(NDEBUG)
    if (IsKeyPressed(KEY_F6)) {
        sectorEditor.toggle(campaignMgr);
    }
#endif

    bool isEditorOpen = sectorEditor.isOpen;

    if (isEditorOpen) {
        renderer.localShip.isInitialized = false;
        renderer.localShip.velocity = { 0.0f, 0.0f };
        renderer.localShip.isMoving = false;
        pendingUncoverCell = -1;
        renderer.clearOutOfReach();
        currentHoveredCell = -1;
    } else if (wasEditorOpen) {
        // Editor just closed: re-enable and restore player ship or return to main menu
        if (state == AppState::InGame) {
            if (currentMode == GameMode::Editor) {
                state = AppState::Menu;
                menu.currentScreen = ui::MenuScreen::Main;
            } else if (currentMode == GameMode::Campaign) {
                Vector2 spawn = campaignMgr.getSectorSpawnPosition(campaignMgr.activeSectorIndex);
                renderer.localShip.position = spawn;
                renderer.localShip.velocity = { 0.0f, 0.0f };
                renderer.localShip.isMoving = false;
                renderer.localShip.isInitialized = true;
                renderer.camera.centerOn(spawn);
            } else {
                renderer.localShip.isInitialized = true;
                renderer.localShip.velocity = { 0.0f, 0.0f };
                renderer.localShip.isMoving = false;
            }
        }
    }
    wasEditorOpen = isEditorOpen;

    if (testShopMode && state == AppState::InGame) {
        static int testUpdateFrame = 0;
        ++testUpdateFrame;
        if (testUpdateFrame == 8) {
            renderer.fireLaser({ 250.0f, renderer.shopShip.position.y }, { renderer.shopShip.position.x, renderer.shopShip.position.y }, Color{ 0, 229, 255, 255 });
        } else if (testUpdateFrame == 18) {
            renderer.localShip.position = { renderer.shopShip.position.x + 25.0f, renderer.shopShip.position.y };
            renderer.localShip.velocity = { -600.0f, 0.0f };
            renderer.localShip.isInitialized = true;
            renderer.resolveShipCollisions();
            std::cout << "[TEST] Rammed shop ship, shop pos (" 
                      << renderer.shopShip.position.x << ", " << renderer.shopShip.position.y 
                      << "), shop vel (" << renderer.shopShip.velocity.x << ", " << renderer.shopShip.velocity.y << ")" << std::endl;
        } else if (testUpdateFrame >= 19) {
            renderer.localShip.position = { 250.0f, 250.0f };
            renderer.localShip.velocity = { 0.0f, 0.0f };
        }
    }

    if (testShopUIMode && state == AppState::InGame) {
        static int uiFrame = 0;
        ++uiFrame;
        if (uiFrame < 8 && !renderer.shopShips.empty()) {
            openedShopIndex = -1;
            nearbyShopIndex = -1;
            hoveredShopIndex = 0;
            isHoveredShopInRange = true;
            shopProximityAlpha = 0.0f;
            renderer.localShip.position = { renderer.shopShips[0].position.x - 75.0f, renderer.shopShips[0].position.y };
            renderer.localShip.velocity = { 0.0f, 0.0f };
            renderer.localShip.isInitialized = true;
            renderer.camera.centerOn(renderer.shopShips[0].position);
        } else if (uiFrame < 18 && !renderer.shopShips.empty()) {
            openedShopIndex = 0;
            nearbyShopIndex = 0;
            shopProximityAlpha = 1.0f;
            renderer.localShip.position = { renderer.shopShips[0].position.x - 75.0f, renderer.shopShips[0].position.y };
            renderer.localShip.velocity = { 0.0f, 0.0f };
            renderer.localShip.isInitialized = true;
            renderer.camera.centerOn(renderer.shopShips[0].position);
        } else if (uiFrame >= 18 && renderer.shopShips.size() > 1) {
            openedShopIndex = 1;
            nearbyShopIndex = 1;
            shopProximityAlpha = 1.0f;
            renderer.localShip.position = { renderer.shopShips[1].position.x - 85.0f, renderer.shopShips[1].position.y };
            renderer.localShip.velocity = { 0.0f, 0.0f };
            renderer.localShip.isInitialized = true;
            renderer.camera.centerOn(renderer.shopShips[1].position);
        }
        if (uiFrame == 28) {
            auto& cat = core::ItemCatalog::instance();
            const auto* b = cat.getItem(core::ItemId::Banana);
            const auto* r = cat.getItem(core::ItemId::Radar);
            const auto* bub = cat.getItem(core::ItemId::Bubbles);
            if (b) playerInventory.addItem(*b);
            if (r) playerInventory.addItem(*r);
            if (bub) playerInventory.addItem(*bub);
            playerInventory.selectedSlot = 0; // select Banana (emerges from ship)
        }
        if (uiFrame == 33) {
            playerInventory.selectedSlot = 2; // switch to Bubbles
            playerInventory.radarActiveTimer = 3.5f;
            renderer.triggerRadar(3.5f);
        }
        if (uiFrame >= 34 && uiFrame <= 37) {
            auto* held = playerInventory.getSelectedSlot();
            if (held && held->item.id == core::ItemId::Bubbles) {
                isUsingHeldItem = true;
                held->durability = std::max(0.0f, held->durability - 0.15f);
                renderer.emitBubbles(renderer.localShip.position, 3);
                renderer.applyBubbleBlurSource(renderer.localShip.position, 650.0f);
            }
        }
        if (uiFrame == 38) {
            isUsingHeldItem = false;
            openedShopIndex = -1;
            playerInventory.selectedSlot = -1; // unselect item -> triggers retract back into ship
        }
        if (uiFrame >= 41 && renderer.hasRouletteShip) {
            openedShopIndex = -1;
            shopProximityAlpha = 0.0f;
            isRouletteOpen = true;
            rouletteProximityAlpha = 1.0f;
            renderer.localShip.position = { renderer.rouletteShip.position.x - 130.0f, renderer.rouletteShip.position.y };
            renderer.localShip.velocity = { 0.0f, 0.0f };
            renderer.localShip.isInitialized = true;
            renderer.camera.centerOn(renderer.rouletteShip.position);

            if (uiFrame == 42) {
                renderer.rouletteShip.roulette.activeBet.type = core::RouletteBetType::Red;
                renderer.rouletteShip.roulette.activeBet.amount = 50;
            }
            if (uiFrame == 46) {
                renderer.rouletteShip.startSpin(32, 2.0f);
            }
            if (uiFrame >= 47) {
                rouletteUI.animMoveT = 1.0f;
            }
            if (uiFrame == 50) {
                // Advance to win state: landed on 32 Red
                renderer.rouletteShip.roulette.spinTimer = 2.1f;
                renderer.rouletteShip.update(0.016f);
            }
            if (uiFrame == 53) {
                // Start loss state: bet Red, land on Black 15
                renderer.rouletteShip.roulette.activeBet.type = core::RouletteBetType::Red;
                renderer.rouletteShip.roulette.activeBet.amount = 50;
                renderer.rouletteShip.startSpin(15, 0.1f);
                renderer.rouletteShip.roulette.spinTimer = 0.15f;
                renderer.rouletteShip.update(0.016f);
            }
        }
    }

    renderer.enableCRT = menu.crtEnabled;
    renderer.camera.enableCRT = menu.crtEnabled;
    renderer.activeFlagSkin = menu.flagSkin;
    renderer.activeCursorSkin = menu.cursorSkin;
    renderer.activePlayerSkin = menu.playerSkin;
    renderer.update(dt);

    bool isRouletteAnimating = renderer.hasRouletteShip && (
        renderer.rouletteShip.roulette.spinState == core::RouletteSpinState::Spinning ||
        renderer.rouletteShip.roulette.spinState == core::RouletteSpinState::Result ||
        renderer.rouletteShip.isAligning
    );
    rouletteUI.update(dt, isRouletteAnimating, isRouletteOpen);

    // 1. Hotbar slot selection: keys 1-5
    if (state == AppState::InGame) {
        if (shopProximityAlpha < 0.2f) {
            if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1)) playerInventory.selectedSlot = 0;
            if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2)) playerInventory.selectedSlot = 1;
            if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3)) playerInventory.selectedSlot = 2;
            if (IsKeyPressed(KEY_FOUR) || IsKeyPressed(KEY_KP_4)) playerInventory.selectedSlot = 3;
            if (IsKeyPressed(KEY_FIVE) || IsKeyPressed(KEY_KP_5)) playerInventory.selectedSlot = 4;
        }
    }

    // 2. Held Item Usage: hold [E] for bubbles, press [E] for radar / banana
    if (!testShopUIMode) {
        isUsingHeldItem = false;
    }
    if (state == AppState::InGame) {
        auto* held = playerInventory.getSelectedSlot();
        if (held) {
            if (held->item.id == core::ItemId::Bubbles && held->durability > 0.0f) {
                if (IsKeyDown(KEY_E) || IsKeyDown(KEY_B)) {
                    isUsingHeldItem = true;
                    held->durability = std::max(0.0f, held->durability - dt);

                    // Scatter bubbles only while used
                    renderer.emitBubbles(renderer.localShip.position, 2);

                    // Dynamic distance blur at origin
                    renderer.applyBubbleBlurSource(renderer.localShip.position, 650.0f);

                    bubbleNetTimer += dt;
                    if (bubbleNetTimer >= 0.12f) {
                        bubbleNetTimer = 0.0f;
                        broadcastBubble(renderer.localShip.position);
                    }

                    if (held->durability <= 0.0f) {
                        renderer.emitExplosion(renderer.localShip.position, ui::Colors::Cyan400);
                        playerInventory.clearSlot(playerInventory.selectedSlot);
                        isUsingHeldItem = false;
                    }
                }
            } else if (held->item.id == core::ItemId::Radar) {
                if (IsKeyPressed(KEY_E)) {
                    playerInventory.radarActiveTimer = 4.0f;
                    renderer.triggerRadar(4.0f);
                    playerInventory.clearSlot(playerInventory.selectedSlot);
                }
            } else if (held->item.id == core::ItemId::RadarBeacon) {
                if (IsKeyPressed(KEY_E)) {
                    int count5x5 = 0;
                    core::Board* targetBoard = (currentMode == GameMode::Campaign)
                        ? (campaignMgr.getSectorByIndex(campaignMgr.activeSectorIndex) ? &campaignMgr.getSectorByIndex(campaignMgr.activeSectorIndex)->board : nullptr)
                        : &board;
                    if (targetBoard) {
                        for (size_t cIdx = 0; cIdx < targetBoard->totalCells(); ++cIdx) {
                            if (targetBoard->isBomb(cIdx)) {
                                Vector2 cPos = (currentMode == GameMode::Campaign)
                                    ? campaignMgr.getSectorByIndex(campaignMgr.activeSectorIndex)->getCellWorldPosition(cIdx, renderer.cellSize)
                                    : renderer.getCellWorldPosition(cIdx, board);
                                if (std::abs(cPos.x - renderer.localShip.position.x) <= 2.5f * renderer.cellSize &&
                                    std::abs(cPos.y - renderer.localShip.position.y) <= 2.5f * renderer.cellSize) {
                                    count5x5++;
                                }
                            }
                        }
                    }
                    renderer.deployRadarBeacon(renderer.localShip.position, count5x5, 12.0f);
                    soundMgr.playUncoverSound();
                    playerInventory.clearSlot(playerInventory.selectedSlot);
                }
            } else if (held->item.id == core::ItemId::Banana) {
                if (IsKeyPressed(KEY_E)) {
                    playerInventory.bananaBoostTimer = 20.0f;
                    renderer.emitExplosion(renderer.localShip.position, ui::Colors::Amber400);
                    soundMgr.playBananaSound();
                    playerInventory.clearSlot(playerInventory.selectedSlot);
                }
            }
        }
    }

    // 3. Active Buff Timers & Upgrade Stats
    if (playerInventory.bananaBoostTimer > 0.0f) {
        playerInventory.bananaBoostTimer = std::max(0.0f, playerInventory.bananaBoostTimer - dt);
    }
    if (playerInventory.radarActiveTimer > 0.0f) {
        playerInventory.radarActiveTimer = std::max(0.0f, playerInventory.radarActiveTimer - dt);
        renderer.radarTimer = playerInventory.radarActiveTimer;
        renderer.hasRadarActive = (playerInventory.radarActiveTimer > 0.0f);
    } else {
        renderer.hasRadarActive = false;
    }

    // Speed boost upgrade (+30%) while banana buff is active
    float baseSpeed = 600.0f;
    float baseAccel = 2200.0f;
    if (playerInventory.bananaBoostTimer > 0.0f) {
        renderer.localShip.speed = baseSpeed * 1.30f;
        renderer.localShip.maxAccel = baseAccel * 1.30f;
    } else {
        renderer.localShip.speed = baseSpeed;
        renderer.localShip.maxAccel = baseAccel;
    }

    // Provide held slot and active status to renderer for carried item rendering
    renderer.heldSlot = playerInventory.getSelectedSlot();
    renderer.isUsingItem = isUsingHeldItem;

    // Synchronize voice settings and live mic meter with menu
    voiceMgr.getSettings() = menu.voiceSettings;
    menu.micInputLevel = voiceMgr.getMicLevel();

    // Synchronize and update background audio manager
    soundMgr.setEnabled(menu.bgmEnabled);
    soundMgr.setVolume(menu.bgmVolume);
    soundMgr.setSfxEnabled(menu.sfxEnabled);
    soundMgr.setSfxVolume(menu.sfxVolume);
    soundMgr.update(dt);

    if (soundMgr.getState() == audio::BgmPlaybackState::Playing) {
        float played = soundMgr.getCurrentTrackTimePlayed();
        float total = soundMgr.getCurrentTrackTimeLength();
        menu.bgmStatusText = TextFormat("PLAYING: %s (%d:%02d / %d:%02d)",
            soundMgr.getCurrentTrackName().c_str(),
            static_cast<int>(played) / 60, static_cast<int>(played) % 60,
            static_cast<int>(total) / 60, static_cast<int>(total) % 60);
    } else if (soundMgr.getState() == audio::BgmPlaybackState::Waiting) {
        menu.bgmStatusText = TextFormat("STATUS: SILENCE (Next sound in %ds)",
            static_cast<int>(soundMgr.getRemainingWaitTime()));
    } else {
        menu.bgmStatusText = "STATUS: IDLE";
    }

    Vector2 worldMouse = renderer.camera.getScreenToWorld(renderer.camera.getCRTMousePosition());
    if (testShopMode) {
        worldMouse = { 250.0f, 250.0f };
    } else if (testShopUIMode && !renderer.shopShips.empty()) {
        static int simFrame = 0;
        ++simFrame;
        if (simFrame < 8) {
            worldMouse = renderer.shopShips[0].position;
        }
    }

    // Shop Interaction & Proximity Check:
    // The shop menu is displayed if the player clicks the ship and is close enough.
    const float maxShopInteractionDist = 380.0f; // Increased interaction radius
    const float maxShopCloseDist = 450.0f;       // Distance at which an open shop automatically closes

    bool isOverUI = (state == AppState::InGame) && isMouseOverUI();

    hoveredShopIndex = -1;
    isHoveredShopInRange = false;
    int clickedShopIndex = -1;
    isRouletteHovered = false;
    isRouletteInRange = false;
    bool clickedRoulette = false;
    bool mouseHandledByShop = false;
    bool allowShops = (currentMode != GameMode::Campaign || campaignMgr.activeSectorIndex > 0);

    if (sectorEditor.isOpen && (!isOverUI || sectorEditor.isDragging())) {
        sectorEditor.updateWorldInteraction(campaignMgr, worldMouse, dt);
    }

    if (state == AppState::InGame && allowShops && !isEditorOpen) {
        for (size_t i = 0; i < renderer.shopShips.size(); ++i) {
            const auto& s = renderer.shopShips[i];
            Vector2 pA, pB;
            s.getCapsuleSegment(pA, pB);
            Vector2 ab = { pB.x - pA.x, pB.y - pA.y };
            float lenSq = ab.x * ab.x + ab.y * ab.y;

            // Player distance to shop
            float tPlayer = (lenSq > 0.0001f) ? std::clamp(((renderer.localShip.position.x - pA.x) * ab.x + (renderer.localShip.position.y - pA.y) * ab.y) / lenSq, 0.0f, 1.0f) : 0.0f;
            Vector2 closestPlayer = { pA.x + tPlayer * ab.x, pA.y + tPlayer * ab.y };
            float playerDist = Vector2Distance(renderer.localShip.position, closestPlayer);

            // Mouse distance to shop
            float tMouse = (lenSq > 0.0001f) ? std::clamp(((worldMouse.x - pA.x) * ab.x + (worldMouse.y - pA.y) * ab.y) / lenSq, 0.0f, 1.0f) : 0.0f;
            Vector2 closestMouse = { pA.x + tMouse * ab.x, pA.y + tMouse * ab.y };
            float mouseDist = Vector2Distance(worldMouse, closestMouse);

            float clickRadius = std::max({
                s.collisionRadius * s.scale + 16.0f,
                static_cast<float>(s.texture.height) * 0.5f * s.scale + 12.0f,
                36.0f
            });

            if (mouseDist <= clickRadius && !isOverUI) {
                hoveredShopIndex = static_cast<int>(i);
                if (playerDist <= maxShopInteractionDist) {
                    isHoveredShopInRange = true;
                }
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    clickedShopIndex = static_cast<int>(i);
                }
                break;
            }
        }

        // Check roulette ship hover & click
        if (renderer.hasRouletteShip) {
            const auto& r = renderer.rouletteShip;
            float playerDist = Vector2Distance(renderer.localShip.position, r.position);
            float mouseDist = Vector2Distance(worldMouse, r.position);
            float clickRadius = std::max({
                r.collisionRadius * r.scale + 16.0f,
                static_cast<float>(r.texture.height) * 0.5f * r.scale + 12.0f,
                36.0f
            });

            if (mouseDist <= clickRadius && !isOverUI) {
                isRouletteHovered = true;
                if (playerDist <= maxShopInteractionDist) {
                    isRouletteInRange = true;
                }
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    clickedRoulette = true;
                }
            }
        }

        // Handle clicks on shops or shop UI
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (clickedRoulette && isRouletteInRange) {
                isRouletteOpen = true;
                openedShopIndex = -1;
                mouseHandledByShop = true;
            } else if (clickedShopIndex >= 0) {
                // Clicked on a shop ship
                const auto& s = renderer.shopShips[clickedShopIndex];
                Vector2 pA, pB;
                s.getCapsuleSegment(pA, pB);
                Vector2 ab = { pB.x - pA.x, pB.y - pA.y };
                float lenSq = ab.x * ab.x + ab.y * ab.y;
                float tPlayer = (lenSq > 0.0001f) ? std::clamp(((renderer.localShip.position.x - pA.x) * ab.x + (renderer.localShip.position.y - pA.y) * ab.y) / lenSq, 0.0f, 1.0f) : 0.0f;
                Vector2 closestPlayer = { pA.x + tPlayer * ab.x, pA.y + tPlayer * ab.y };
                float playerDist = Vector2Distance(renderer.localShip.position, closestPlayer);

                if (playerDist <= maxShopInteractionDist) {
                    openedShopIndex = clickedShopIndex;
                    nearbyShopIndex = clickedShopIndex;
                    isRouletteOpen = false;
                }
                mouseHandledByShop = true;
            } else if (isRouletteOpen && rouletteProximityAlpha > 0.2f) {
                if (rouletteUI.isMouseOverCard()) {
                    mouseHandledByShop = true;
                } else {
                    isRouletteOpen = false;
                    mouseHandledByShop = true;
                    if (renderer.hasRouletteShip && renderer.rouletteShip.roulette.spinState == core::RouletteSpinState::Result) {
                        renderer.rouletteShip.alignBackToDock();
                    }
                }
            } else if (openedShopIndex >= 0 && shopProximityAlpha > 0.2f) {
                if (shopMenu.isMouseOverCard()) {
                    mouseHandledByShop = true;
                } else {
                    openedShopIndex = -1;
                    mouseHandledByShop = true;
                }
            }
        }

        // Close request from roulette UI [X] button
        if (rouletteUI.requestClose) {
            isRouletteOpen = false;
            rouletteUI.requestClose = false;
            if (renderer.hasRouletteShip && renderer.rouletteShip.roulette.spinState == core::RouletteSpinState::Result) {
                renderer.rouletteShip.alignBackToDock();
            }
        }

        // Pressing ESC closes open shop or roulette
        if (IsKeyPressed(KEY_ESCAPE)) {
            if (isRouletteOpen) {
                isRouletteOpen = false;
                if (renderer.hasRouletteShip && renderer.rouletteShip.roulette.spinState == core::RouletteSpinState::Result) {
                    renderer.rouletteShip.alignBackToDock();
                }
            }
            if (openedShopIndex >= 0) {
                openedShopIndex = -1;
            }
        }

        // Check if player drifted too far from the opened shop
        if (openedShopIndex >= 0 && openedShopIndex < static_cast<int>(renderer.shopShips.size())) {
            const auto& s = renderer.shopShips[openedShopIndex];
            Vector2 pA, pB;
            s.getCapsuleSegment(pA, pB);
            Vector2 ab = { pB.x - pA.x, pB.y - pA.y };
            float lenSq = ab.x * ab.x + ab.y * ab.y;
            float tPlayer = (lenSq > 0.0001f) ? std::clamp(((renderer.localShip.position.x - pA.x) * ab.x + (renderer.localShip.position.y - pA.y) * ab.y) / lenSq, 0.0f, 1.0f) : 0.0f;
            Vector2 closestPlayer = { pA.x + tPlayer * ab.x, pA.y + tPlayer * ab.y };
            float playerDist = Vector2Distance(renderer.localShip.position, closestPlayer);

            if (playerDist > maxShopCloseDist) {
                openedShopIndex = -1;
            }
        } else {
            openedShopIndex = -1;
        }

        // Check if player drifted too far from roulette ship
        if (isRouletteOpen && renderer.hasRouletteShip) {
            float rPlayerDist = Vector2Distance(renderer.localShip.position, renderer.rouletteShip.position);
            if (rPlayerDist > maxShopCloseDist) {
                isRouletteOpen = false;
                if (renderer.rouletteShip.roulette.spinState == core::RouletteSpinState::Result) {
                    renderer.rouletteShip.alignBackToDock();
                }
            }
        }

        // Process roulette payouts upon spin finish:
        // Gold particles on WIN, Red particles on LOSE, and trigger fading popup
        if (renderer.hasRouletteShip) {
            auto& r = renderer.rouletteShip.roulette;
            if (r.spinState == core::RouletteSpinState::Result && !r.payoutAwarded) {
                r.payoutAwarded = true;
                if (r.lastPayout > 0) {
                    scrapCount += r.lastPayout;
                    hud.scrapCount = scrapCount;
                    menu.scrapCount = scrapCount;
                    // WIN: Gold sparkles, plasma motes, shockwave!
                    renderer.particles.emitExplosion(renderer.rouletteShip.position, 25, ui::Colors::Amber400);
                    renderer.particles.emitSparkles(renderer.rouletteShip.position, 20, Color{ 255, 215, 0, 255 }, 40.0f);
                    renderer.particles.emitPlasmaMotes(renderer.rouletteShip.position, 12, ui::Colors::Amber300);
                    renderer.particles.emitShockwave(renderer.rouletteShip.position, 85.0f, ui::Colors::Amber400, 0.40f);
                } else {
                    // LOSE: Red explosion, sparks, dark smoke!
                    renderer.particles.emitExplosion(renderer.rouletteShip.position, 25, ui::Colors::Red500);
                    renderer.particles.emitSparks(renderer.rouletteShip.position, 16, ui::Colors::Red400);
                    renderer.particles.emitSmoke(renderer.rouletteShip.position, 8, Color{ 60, 60, 65, 210 });
                }
                // Trigger fading popup with number landed on and payout
                rouletteUI.triggerPopup(r.winningNumber, r.lastWon, r.lastPayout, r.activeBet.amount);

                saveCurrentSlot();
            }
        }

        float targetAlpha = (openedShopIndex >= 0) ? 1.0f : 0.0f;
        shopProximityAlpha = std::lerp(shopProximityAlpha, targetAlpha, std::clamp(dt * 10.0f, 0.0f, 1.0f));
        if (shopProximityAlpha < 0.01f && openedShopIndex < 0) {
            nearbyShopIndex = -1;
        } else if (openedShopIndex >= 0) {
            nearbyShopIndex = openedShopIndex;
        }

        float targetRouletteAlpha = isRouletteOpen ? 1.0f : 0.0f;
        rouletteProximityAlpha = std::lerp(rouletteProximityAlpha, targetRouletteAlpha, std::clamp(dt * 10.0f, 0.0f, 1.0f));
    } else {
        shopProximityAlpha = 0.0f;
        nearbyShopIndex = -1;
        openedShopIndex = -1;
        hoveredShopIndex = -1;
        isHoveredShopInRange = false;
        rouletteProximityAlpha = 0.0f;
        isRouletteOpen = false;
        isRouletteHovered = false;
        isRouletteInRange = false;
    }

    bool padAvailable = IsGamepadAvailable(0);
    Vector2 moveInput = { 0.0f, 0.0f };

    // 1. Movement: WASD / Arrows (Keyboard) and Left Stick / D-Pad (Gamepad)
    if (!isEditorOpen) {
        if (menu.controlMode == 1) {
            if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) moveInput.y -= 1.0f;
            if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) moveInput.y += 1.0f;
            if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) moveInput.x -= 1.0f;
            if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) moveInput.x += 1.0f;
        }

        if (padAvailable) {
            float stickX = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
            float stickY = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);
            if (std::abs(stickX) < 0.15f) stickX = 0.0f;
            if (std::abs(stickY) < 0.15f) stickY = 0.0f;
            moveInput.x += stickX;
            moveInput.y += stickY;

            if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_UP)) moveInput.y -= 1.0f;
            if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) moveInput.y += 1.0f;
            if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) moveInput.x -= 1.0f;
            if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) moveInput.x += 1.0f;
        }

        float moveInputLen = std::sqrt(moveInput.x * moveInput.x + moveInput.y * moveInput.y);
        if (moveInputLen > 1.0f) {
            moveInput.x /= moveInputLen;
            moveInput.y /= moveInputLen;
        }
    }

    // 2. Mouse tracking state
    Vector2 curMouse = GetMousePosition();
    if (Vector2Distance(curMouse, prevMousePos) > 1.0f ||
        IsMouseButtonDown(MOUSE_LEFT_BUTTON) ||
        IsMouseButtonDown(MOUSE_RIGHT_BUTTON) ||
        IsMouseButtonDown(MOUSE_MIDDLE_BUTTON)) {
        prevMousePos = curMouse;
        renderer.isMouseActive = true;
    }

    // 3. Right Stick: Used for selecting a cell & aiming laser
    bool hasAim = false;
    float aimAngle = 0.0f;
    int64_t rightStickCell = -1;

    if (padAvailable && !isEditorOpen) {
        float rx = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_X);
        float ry = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_Y);
        float rLen = std::sqrt(rx * rx + ry * ry);

        if (rLen > 0.22f) {
            renderer.isMouseActive = false;
            Vector2 rDir = { rx / rLen, ry / rLen };
            hasAim = true;
            aimAngle = std::atan2(rDir.y, rDir.x) * RAD2DEG + 90.0f;

            float maxReach = renderer.cellSize * (1.2f + (rLen - 0.22f) / 0.78f * 5.0f);
            for (float d = maxReach; d >= renderer.cellSize * 0.5f; d -= renderer.cellSize * 0.35f) {
                Vector2 testPt = { renderer.localShip.position.x + rDir.x * d, renderer.localShip.position.y + rDir.y * d };
                int64_t idx = renderer.getCellIndexAtWorldPos(testPt, board);
                if (idx >= 0) {
                    rightStickCell = idx;
                    break;
                }
            }
        }
    }

    // 4. Update cell selection
    if (isEditorOpen) {
        currentHoveredCell = -1;
    } else if (rightStickCell >= 0) {
        currentHoveredCell = rightStickCell;
    } else if (hasAim) {
        currentHoveredCell = -1;
    } else if (renderer.isMouseActive) {
        if (isOverUI) {
            currentHoveredCell = -1;
        } else if (currentMode == GameMode::Campaign) {
            auto* sec = campaignMgr.getSectorAtGridWorldPos(worldMouse);
            if (sec && sec->isUnlocked) {
                int64_t lIdx = sec->getCellIndexAtWorldPos(worldMouse, renderer.cellSize);
                currentHoveredCell = (lIdx >= 0) ? static_cast<int64_t>(core::CampaignManager::toGlobalCellIndex(sec->id - 1, static_cast<size_t>(lIdx))) : -1;
            } else {
                currentHoveredCell = -1;
            }
        } else {
            currentHoveredCell = renderer.getHoveredCellIndex(board);
        }
    }

    renderer.controlMode = menu.controlMode;
    renderer.moveInput = moveInput;
    renderer.hasAim = hasAim;
    renderer.aimAngle = aimAngle;

    renderer.syncRemoteShips(net.remoteCursors);
    renderer.updatePhysics(worldMouse, dt);

    if (state == AppState::InGame && currentMode == GameMode::Campaign) {
        if (renderer.localShip.isInitialized) {
            campaignMgr.resolveShipCollisions(renderer.localShip, &renderer.particles);
        }
        for (auto& [id, rShip] : renderer.remoteShips) {
            campaignMgr.resolveShipCollisions(rShip, &renderer.particles);
        }
        campaignMgr.update(dt);

        // Check if player entered an unlocked Orbital Launcher to inject into orbit / jump to next sector!
        if (renderer.localShip.isInitialized && campaignMgr.checkLauncherTransit(campaignMgr.activeSectorIndex, renderer.localShip.position, renderer.localShip.collisionRadius)) {
            int targetIdx = campaignMgr.getLauncherTargetSectorIndex(campaignMgr.activeSectorIndex);
            if (targetIdx >= 0) {
                triggerSectorWarp(targetIdx);
            }
        }
    }

    if (state == AppState::InGame) {
        bool localInSafeZone = false;
        if (!renderer.shopShips.empty()) {
            localInSafeZone = renderer.shopShips.front().isInsideSafeZone(renderer.localShip.position);
        } else {
            localInSafeZone = renderer.shopShip.isInsideSafeZone(renderer.localShip.position);
        }
        voiceMgr.getSettings().intercomReverb = localInSafeZone;

        if (currentMode == GameMode::Campaign) {
            auto* curSec = campaignMgr.getSectorByIndex(campaignMgr.activeSectorIndex);
            if (curSec && curSec->modifier == core::SectorModifier::JammedComms && !curSec->centralDataNodeCleared) {
                voiceMgr.getSettings().proximityRadiusMultiplier = 0.3f;
            } else {
                voiceMgr.getSettings().proximityRadiusMultiplier = 1.0f;
            }
        } else {
            voiceMgr.getSettings().proximityRadiusMultiplier = 1.0f;
        }
    }

    if (renderer.localShip.isInitialized) {
        voiceMgr.setLocalCursorPos(renderer.localShip.position.x, renderer.localShip.position.y);
    }
    voiceMgr.setPushToTalkActive(IsKeyDown(KEY_V));
    voiceMgr.update(dt);

    renderer.isLocalSpeaking = voiceMgr.isTransmitting();

    if (state == AppState::InGame) {
        // Hotkey M to toggle control mode
        if (IsKeyPressed(KEY_M) && !hud.showLargeGridWarning) {
            menu.controlMode = (menu.controlMode == 0) ? 1 : 0;
            saveSettings();
        }

        // Camera input & smooth edge follow
        renderer.camera.handleInput(!hud.showLargeGridWarning && !isOverUI);

        // Gamepad camera zoom & center
        if (padAvailable && !hud.showLargeGridWarning) {
            float trigL = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_TRIGGER);
            float trigR = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_TRIGGER);
            if (trigL > 0.15f) {
                renderer.camera.setZoom(std::clamp(renderer.camera.getZoom() * (1.0f - 1.2f * dt), 0.02f, 20.0f));
            }
            if (trigR > 0.15f) {
                renderer.camera.setZoom(std::clamp(renderer.camera.getZoom() * (1.0f + 1.2f * dt), 0.02f, 20.0f));
            }
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_THUMB) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_LEFT)) {
                renderer.camera.manualPanActive = false;
                renderer.camera.centerOn(renderer.localShip.position);
            }
        }

        if (!hud.showLargeGridWarning && IsKeyPressed(KEY_SPACE) && renderer.localShip.isInitialized && menu.controlMode == 0) {
            renderer.camera.manualPanActive = false;
            renderer.camera.centerOn(renderer.localShip.position);
        }
        if (!testShopMode && !testCampaignMode && !hud.showLargeGridWarning && renderer.localShip.isInitialized && menu.controlMode == 0) {
            renderer.camera.followShip(renderer.localShip.position, dt);
        }

        // Multiplayer Voice Streaming
        if (net.role != net::NetRole::Offline) {
            net::PacketVoice vPkt;
            while (voiceMgr.getOutgoingPacket(vPkt)) {
                vPkt.playerID = (net.role == net::NetRole::Host) ? net::HOST_PLAYER_ID : 0;
                if (net.role == net::NetRole::Client) {
                    net.sendToServer(&vPkt, sizeof(vPkt), false);
                } else if (net.role == net::NetRole::Host) {
                    net.broadcast(&vPkt, sizeof(vPkt), false);
                }
            }
        }

        // Multiplayer Cursor / Ship Broadcast
        if (net.role != net::NetRole::Offline && renderer.localShip.isInitialized) {
            static Vector2 lastSent = { -9999.0f, -9999.0f };
            static float lastSentAngle = -9999.0f;
            static bool lastSentMoving = false;
            bool isMoving = renderer.localShip.isMoving;

            if (Vector2Distance(renderer.localShip.position, lastSent) > 1.5f ||
                std::abs(renderer.localShip.angle - lastSentAngle) > 2.0f ||
                isMoving != lastSentMoving) {
                lastSent = renderer.localShip.position;
                lastSentAngle = renderer.localShip.angle;
                lastSentMoving = isMoving;

                net::PacketCursor pc;
                pc.playerID = (net.role == net::NetRole::Host) ? net::HOST_PLAYER_ID : 0;
                pc.x = renderer.localShip.position.x;
                pc.y = renderer.localShip.position.y;
                pc.angle = renderer.localShip.angle;
                pc.mass = renderer.localShip.mass;
                pc.isMoving = isMoving;
                pc.skin = static_cast<uint8_t>(menu.cursorSkin);
                std::strncpy(pc.name, menu.playerName, sizeof(pc.name));
                pc.name[sizeof(pc.name) - 1] = '\0';

                if (net.role == net::NetRole::Client) {
                    net.sendToServer(&pc, sizeof(pc), false);
                } else if (net.role == net::NetRole::Host) {
                    net.broadcast(&pc, sizeof(pc), false);
                }
            }
        }

        int64_t hovered = currentHoveredCell;

        bool isCurrentGameOver = false;
        bool isCurrentVictory = false;
        if (currentMode == GameMode::Campaign) {
            auto* curSec = campaignMgr.getSectorByIndex(campaignMgr.activeSectorIndex);
            if (curSec) {
                isCurrentGameOver = curSec->board.isGameOver;
                isCurrentVictory = curSec->isCleared;
            }
        } else {
            isCurrentGameOver = board.isGameOver;
            isCurrentVictory = board.isVictory;
        }

        auto getTargetBoardAndCell = [&](size_t globalIdx, int& outSectorIdx, size_t& outLocalIdx, core::Board*& outBoard, Vector2& outCenter) {
            if (currentMode == GameMode::Campaign) {
                core::CampaignManager::fromGlobalCellIndex(globalIdx, outSectorIdx, outLocalIdx);
                auto* sec = campaignMgr.getSectorByIndex(outSectorIdx);
                if (sec) {
                    outBoard = &sec->board;
                    outCenter = sec->getCellWorldPosition(outLocalIdx, renderer.cellSize);
                } else {
                    outBoard = &board;
                    outCenter = { 0.0f, 0.0f };
                }
            } else {
                outSectorIdx = -1;
                outLocalIdx = globalIdx;
                outBoard = &board;
                outCenter = renderer.getCellWorldPosition(globalIdx, board);
            }
        };

        // In-game Input Handling
        if (!isCurrentGameOver && !isCurrentVictory && !hud.showLargeGridWarning && !isEditorOpen && renderer.localShip.isInitialized) {
            bool triggerUncover = IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !mouseHandledByShop && !isOverUI;
            bool triggerFlag = IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && !mouseHandledByShop && !isOverUI;
            bool triggerChord = IsKeyPressed(KEY_C);
            if (IsMouseButtonReleased(MOUSE_MIDDLE_BUTTON) && !renderer.camera.isMiddleDragging() && !isOverUI) {
                triggerChord = true;
            }

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && isOverUI) {
                pendingUncoverCell = -1;
                renderer.clearOutOfReach();
            }

            if (padAvailable) {
                if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) triggerUncover = true; // Cross / A
                if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) triggerFlag = true;    // Circle / B
                if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_LEFT) ||                       // Square / X
                    IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_1)) triggerChord = true;    // R1 / RB
            }

            if (menu.controlMode == 1) {
                if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) triggerUncover = true;
                if (IsKeyPressed(KEY_F)) triggerFlag = true;
            }

            // Contextual chording: if uncover button is pressed on an already-revealed numbered cell, chord it!
            if (triggerUncover && hovered >= 0) {
                int chkS = 0; size_t chkL = 0; core::Board* chkBoard = nullptr; Vector2 chkCenter;
                getTargetBoardAndCell(static_cast<size_t>(hovered), chkS, chkL, chkBoard, chkCenter);
                if (chkBoard && chkL < chkBoard->totalCells() && chkBoard->getState(chkL) == core::CellState::Revealed) {
                    triggerChord = true;
                    triggerUncover = false;
                }
            }

            bool isMouseAction = (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) ||
                                 IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) ||
                                 (IsMouseButtonReleased(MOUSE_MIDDLE_BUTTON) && !renderer.camera.isMiddleDragging())) && !isOverUI;

            Vector2 actionTarget = worldMouse;
            if (isMouseAction || (renderer.isMouseActive && menu.controlMode == 0)) {
                actionTarget = worldMouse;
            } else if (hovered >= 0) {
                int sI = 0; size_t lI = 0; core::Board* tb = nullptr; Vector2 cCenter;
                getTargetBoardAndCell(static_cast<size_t>(hovered), sI, lI, tb, cCenter);
                actionTarget = cCenter;
            } else if (menu.controlMode == 1) {
                float rad = (renderer.localShip.angle - 90.0f) * DEG2RAD;
                actionTarget = { renderer.localShip.position.x + std::cos(rad) * 60.0f, renderer.localShip.position.y + std::sin(rad) * 60.0f };
            }

            // Uncover Action
            if (triggerUncover) {
                Vector2 toTarget = { actionTarget.x - renderer.localShip.position.x, actionTarget.y - renderer.localShip.position.y };
                float distToTarget = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y);
                if (distToTarget > 0.001f) {
                    renderer.localShip.angle = std::atan2(toTarget.y, toTarget.x) * RAD2DEG + 90.0f;
                }

                broadcastLaser(renderer.localShip.getNosePosition(), actionTarget, 0);

                if (hovered >= 0) {
                    size_t hIdx = static_cast<size_t>(hovered);
                    int sI = 0; size_t lI = 0; core::Board* targetBoard = nullptr; Vector2 cellCenter;
                    getTargetBoardAndCell(hIdx, sI, lI, targetBoard, cellCenter);

                    if (targetBoard && targetBoard->getState(lI) == core::CellState::Hidden) {
                        float dist = Vector2Distance(renderer.localShip.position, cellCenter);

                        if (dist > renderer.localShip.range && menu.controlMode == 0 && !padAvailable) {
                            renderer.triggerOutOfReach(static_cast<int64_t>(hIdx), cellCenter);
                            pendingUncoverCell = static_cast<int64_t>(hIdx);
                        } else {
                            renderer.clearOutOfReach();
                            pendingUncoverCell = -1;

                            if (net.role == net::NetRole::Client) {
                                net::PacketClick pc;
                                pc.index = hIdx;
                                pc.action = 0;
                                pc.flagSkin = 0;
                                net.sendToServer(&pc, sizeof(pc));
                            } else {
                            bool inSafeZone = false;
                            if (!renderer.shopShips.empty()) {
                                inSafeZone = renderer.shopShips.front().isInsideSafeZone(cellCenter) || renderer.shopShips.front().isInsideSafeZone(renderer.localShip.position);
                            } else {
                                inSafeZone = renderer.shopShip.isInsideSafeZone(cellCenter) || renderer.shopShip.isInsideSafeZone(renderer.localShip.position);
                            }
                            auto* held = playerInventory.getSelectedSlot();
                            if (inSafeZone && targetBoard->isBomb(lI)) {
                                targetBoard->setFlag(lI, 0, static_cast<uint8_t>(renderer.activeFlagSkin));
                                soundMgr.playFlagSound();
                                renderer.shopShip.triggerBanter("Safe zone field deflected that detonator. Steady on.");
                                if (!renderer.shopShips.empty()) renderer.shopShips.front().triggerBanter("Safe zone field deflected that detonator. Steady on.");
                            } else if (held && held->occupied && held->item.id == core::ItemId::GroundPenetratingWand) {
                                if (targetBoard->isBomb(lI)) {
                                    targetBoard->setFlag(lI, 0, static_cast<uint8_t>(renderer.activeFlagSkin));
                                    soundMgr.playFlagSound();
                                    renderer.shopShip.triggerBanter("Wand detected explosive charge. Cell auto-flagged.");
                                    if (!renderer.shopShips.empty()) renderer.shopShips.front().triggerBanter("Wand detected explosive charge. Cell auto-flagged.");
                                } else {
                                    std::vector<size_t> wandRev;
                                    targetBoard->reveal(lI, &wandRev);
                                    soundMgr.playUncoverSound();
                                }
                                playerInventory.clearSlot(playerInventory.selectedSlot);
                            } else {
                                std::vector<size_t> newlyRevealed;
                                core::RevealResult res = targetBoard->reveal(lI, &newlyRevealed);
                                for (size_t revIdx : newlyRevealed) {
                                    size_t gIdx = (currentMode == GameMode::Campaign) ? core::CampaignManager::toGlobalCellIndex(sI, revIdx) : revIdx;
                                    renderer.removeFlagDrop(gIdx);
                                }
                                Vector2 pos = cellCenter;
                                if (res == core::RevealResult::HitBomb) {
                                    if (playerInventory.hasItem(core::ItemId::BlastShield)) {
                                        playerInventory.consumeItem(core::ItemId::BlastShield);
                                        targetBoard->isGameOver = false;
                                        renderer.camera.shake(0.65f);
                                        Vector2 knockDir = Vector2Normalize(Vector2Subtract(renderer.localShip.position, cellCenter));
                                        renderer.localShip.velocity = Vector2Scale(knockDir, 460.0f);
                                        renderer.emitExplosion(pos, ui::Colors::Amber500);
                                        soundMgr.playExplosionSound();
                                        renderer.shopShip.triggerBanter("Oof, that sounded expensive! Thank your shields.");
                                        if (!renderer.shopShips.empty()) renderer.shopShips.front().triggerBanter("Oof, that sounded expensive! Thank your shields.");
                                        targetBoard->setFlag(lI, 0, static_cast<uint8_t>(renderer.activeFlagSkin));
                                    } else {
                                        renderer.emitExplosion(pos, ui::Colors::CellFlag);
                                        soundMgr.playExplosionSound();
                                        renderer.shopShip.triggerBanter("Oof, that sounded expensive!");
                                        if (!renderer.shopShips.empty()) renderer.shopShips.front().triggerBanter("Oof, that sounded expensive!");
                                        if (currentMode == GameMode::Campaign) {
                                            campaignMgr.handleEmergencyExtraction(sI, scrapCount);
                                            hud.scrapCount = scrapCount;
                                            menu.scrapCount = scrapCount;
                                            renderer.shopShip.triggerBanter("Engines spooling up! Get to the ship!");
                                            if (!renderer.shopShips.empty()) renderer.shopShips.front().triggerBanter("Engines spooling up! Get to the ship!");
                                        }
                                    }
                                } else {
                                    renderer.emitDebris(pos, ui::Colors::Zinc400);
                                    soundMgr.playUncoverSound();
                                    for (size_t cIdx : newlyRevealed) {
                                        if (!targetBoard->isBomb(cIdx) && render::ScrapSystem::isScrapCell(targetBoard->config.seed, cIdx, targetBoard->totalCells(), targetBoard->config.bombs)) {
                                            Vector2 cPos = (currentMode == GameMode::Campaign)
                                                ? campaignMgr.getSectorByIndex(sI)->getCellWorldPosition(cIdx, renderer.cellSize)
                                                : renderer.getCellWorldPosition(cIdx, board);
                                            scrapSystem.spawn(cPos);
                                        }
                                    }
                                }

                                if (currentMode == GameMode::Campaign) {
                                    auto* curSec = campaignMgr.getSectorByIndex(sI);
                                    if (curSec && curSec->modifier == core::SectorModifier::FoundryWastes) {
                                        for (size_t rev : newlyRevealed) {
                                            if (!targetBoard->isBomb(rev)) {
                                                core::MoltenTileTimer mt;
                                                mt.cellIndex = rev;
                                                mt.timeLeft = 5.0f;
                                                mt.active = true;
                                                curSec->moltenTimers.push_back(mt);
                                            }
                                        }
                                    }
                                    if (curSec && curSec->modifier == core::SectorModifier::JammedComms && !curSec->centralDataNodeCleared) {
                                        for (size_t rev : newlyRevealed) {
                                            if (rev == curSec->centralDataNodeIdx) {
                                                curSec->centralDataNodeCleared = true;
                                                soundMgr.playUncoverSound();
                                                renderer.shopShip.triggerBanter("Central relay decrypted! Communications online.");
                                                if (!renderer.shopShips.empty()) renderer.shopShips.front().triggerBanter("Central relay decrypted! Communications online.");
                                                break;
                                            }
                                        }
                                    }
                                }

                                if (currentMode == GameMode::Campaign) {
                                    bool justSecured = campaignMgr.checkSectorClear(sI);
                                    if (justSecured) {
                                        scrapCount += static_cast<uint64_t>(50 * (sI + 1));
                                        hud.scrapCount = scrapCount;
                                        menu.scrapCount = scrapCount;
                                        hud.triggerScrapPulse();
                                        soundMgr.playUncoverSound();
                                        renderer.triggerSectorClearAnimation(sI, campaignMgr.sectors[sI].board, campaignMgr.sectors[sI].gridOffset);
                                    }
                                    saveCampaignProgress();
                                } else {
                                    if (board.isVictory) {
                                        renderer.triggerSectorClearAnimation(-1, board, { 0.0f, 0.0f });
                                    }
                                    saveCurrentSlot();
                                }

                                if (net.role == net::NetRole::Host) {
                                    for (size_t revIdx : newlyRevealed) {
                                        size_t gIdx = (currentMode == GameMode::Campaign) ? core::CampaignManager::toGlobalCellIndex(sI, revIdx) : revIdx;
                                        net::PacketResult pr;
                                        pr.index = gIdx;
                                        pr.state = 0;
                                        pr.placerId = 0;
                                        pr.flagSkin = 0;
                                        net.broadcast(&pr, sizeof(pr));
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // Pending uncover when ship arrives within reach (mouse follower mode)
            if (pendingUncoverCell >= 0) {
                size_t pIdx = static_cast<size_t>(pendingUncoverCell);
                int sI = 0; size_t lI = 0; core::Board* targetBoard = nullptr; Vector2 cellCenter;
                getTargetBoardAndCell(pIdx, sI, lI, targetBoard, cellCenter);

                if (!targetBoard || targetBoard->isGameOver || targetBoard->isVictory || lI >= targetBoard->totalCells() || targetBoard->getState(lI) != core::CellState::Hidden || IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) || IsKeyPressed(KEY_ESCAPE)) {
                    pendingUncoverCell = -1;
                } else {
                    float dist = Vector2Distance(renderer.localShip.position, cellCenter);
                    if (dist <= renderer.localShip.range) {
                        pendingUncoverCell = -1;
                        renderer.clearOutOfReach();
                        broadcastLaser(renderer.localShip.getNosePosition(), cellCenter, 0);

                        if (net.role == net::NetRole::Client) {
                            net::PacketClick pc;
                            pc.index = pIdx;
                            pc.action = 0;
                            pc.flagSkin = 0;
                            net.sendToServer(&pc, sizeof(pc));
                        } else {
                            bool inSafeZone = false;
                            if (!renderer.shopShips.empty()) {
                                inSafeZone = renderer.shopShips.front().isInsideSafeZone(cellCenter) || renderer.shopShips.front().isInsideSafeZone(renderer.localShip.position);
                            } else {
                                inSafeZone = renderer.shopShip.isInsideSafeZone(cellCenter) || renderer.shopShip.isInsideSafeZone(renderer.localShip.position);
                            }
                            auto* held = playerInventory.getSelectedSlot();
                            if (inSafeZone && targetBoard->isBomb(lI)) {
                                targetBoard->setFlag(lI, 0, static_cast<uint8_t>(renderer.activeFlagSkin));
                                soundMgr.playFlagSound();
                                renderer.shopShip.triggerBanter("Safe zone field deflected that detonator. Steady on.");
                                if (!renderer.shopShips.empty()) renderer.shopShips.front().triggerBanter("Safe zone field deflected that detonator. Steady on.");
                            } else if (held && held->occupied && held->item.id == core::ItemId::GroundPenetratingWand) {
                                if (targetBoard->isBomb(lI)) {
                                    targetBoard->setFlag(lI, 0, static_cast<uint8_t>(renderer.activeFlagSkin));
                                    soundMgr.playFlagSound();
                                    renderer.shopShip.triggerBanter("Wand detected explosive charge. Cell auto-flagged.");
                                    if (!renderer.shopShips.empty()) renderer.shopShips.front().triggerBanter("Wand detected explosive charge. Cell auto-flagged.");
                                } else {
                                    std::vector<size_t> wandRev;
                                    targetBoard->reveal(lI, &wandRev);
                                    soundMgr.playUncoverSound();
                                }
                                playerInventory.clearSlot(playerInventory.selectedSlot);
                            } else {
                                std::vector<size_t> newlyRevealed;
                                core::RevealResult res = targetBoard->reveal(lI, &newlyRevealed);
                                for (size_t revIdx : newlyRevealed) {
                                    size_t gIdx = (currentMode == GameMode::Campaign) ? core::CampaignManager::toGlobalCellIndex(sI, revIdx) : revIdx;
                                    renderer.removeFlagDrop(gIdx);
                                }
                                Vector2 pos = cellCenter;
                                if (res == core::RevealResult::HitBomb) {
                                    if (playerInventory.hasItem(core::ItemId::BlastShield)) {
                                        playerInventory.consumeItem(core::ItemId::BlastShield);
                                        targetBoard->isGameOver = false;
                                        renderer.camera.shake(0.65f);
                                        Vector2 knockDir = Vector2Normalize(Vector2Subtract(renderer.localShip.position, cellCenter));
                                        renderer.localShip.velocity = Vector2Scale(knockDir, 460.0f);
                                        renderer.emitExplosion(pos, ui::Colors::Amber500);
                                        soundMgr.playExplosionSound();
                                        renderer.shopShip.triggerBanter("Oof, that sounded expensive! Thank your shields.");
                                        if (!renderer.shopShips.empty()) renderer.shopShips.front().triggerBanter("Oof, that sounded expensive! Thank your shields.");
                                        targetBoard->setFlag(lI, 0, static_cast<uint8_t>(renderer.activeFlagSkin));
                                    } else {
                                        renderer.emitExplosion(pos, ui::Colors::CellFlag);
                                        soundMgr.playExplosionSound();
                                        renderer.shopShip.triggerBanter("Oof, that sounded expensive!");
                                        if (!renderer.shopShips.empty()) renderer.shopShips.front().triggerBanter("Oof, that sounded expensive!");
                                        if (currentMode == GameMode::Campaign) {
                                            campaignMgr.handleEmergencyExtraction(sI, scrapCount);
                                            hud.scrapCount = scrapCount;
                                            menu.scrapCount = scrapCount;
                                            renderer.shopShip.triggerBanter("Engines spooling up! Get to the ship!");
                                            if (!renderer.shopShips.empty()) renderer.shopShips.front().triggerBanter("Engines spooling up! Get to the ship!");
                                        }
                                    }
                                } else {
                                    renderer.emitDebris(pos, ui::Colors::Zinc400);
                                    soundMgr.playUncoverSound();
                                    for (size_t cIdx : newlyRevealed) {
                                        if (!targetBoard->isBomb(cIdx) && render::ScrapSystem::isScrapCell(targetBoard->config.seed, cIdx, targetBoard->totalCells(), targetBoard->config.bombs)) {
                                            Vector2 cPos = (currentMode == GameMode::Campaign)
                                                ? campaignMgr.getSectorByIndex(sI)->getCellWorldPosition(cIdx, renderer.cellSize)
                                                : renderer.getCellWorldPosition(cIdx, board);
                                            scrapSystem.spawn(cPos);
                                        }
                                    }
                                }

                                if (currentMode == GameMode::Campaign) {
                                    auto* curSec = campaignMgr.getSectorByIndex(sI);
                                    if (curSec && curSec->modifier == core::SectorModifier::FoundryWastes) {
                                        for (size_t rev : newlyRevealed) {
                                            if (!targetBoard->isBomb(rev)) {
                                                core::MoltenTileTimer mt;
                                                mt.cellIndex = rev;
                                                mt.timeLeft = 5.0f;
                                                mt.active = true;
                                                curSec->moltenTimers.push_back(mt);
                                            }
                                        }
                                    }
                                    if (curSec && curSec->modifier == core::SectorModifier::JammedComms && !curSec->centralDataNodeCleared) {
                                        for (size_t rev : newlyRevealed) {
                                            if (rev == curSec->centralDataNodeIdx) {
                                                curSec->centralDataNodeCleared = true;
                                                soundMgr.playUncoverSound();
                                                renderer.shopShip.triggerBanter("Central relay decrypted! Communications online.");
                                                if (!renderer.shopShips.empty()) renderer.shopShips.front().triggerBanter("Central relay decrypted! Communications online.");
                                                break;
                                            }
                                        }
                                    }
                                }

                                if (currentMode == GameMode::Campaign) {
                                    bool justSecured = campaignMgr.checkSectorClear(sI);
                                    if (justSecured) {
                                        scrapCount += static_cast<uint64_t>(50 * (sI + 1));
                                        hud.scrapCount = scrapCount;
                                        menu.scrapCount = scrapCount;
                                        hud.triggerScrapPulse();
                                        soundMgr.playUncoverSound();
                                        renderer.triggerSectorClearAnimation(sI, campaignMgr.sectors[sI].board, campaignMgr.sectors[sI].gridOffset);
                                    }
                                    saveCampaignProgress();
                                } else {
                                    if (board.isVictory) {
                                        renderer.triggerSectorClearAnimation(-1, board, { 0.0f, 0.0f });
                                    }
                                    saveCurrentSlot();
                                }

                                if (net.role == net::NetRole::Host) {
                                    for (size_t revIdx : newlyRevealed) {
                                        size_t gIdx = (currentMode == GameMode::Campaign) ? core::CampaignManager::toGlobalCellIndex(sI, revIdx) : revIdx;
                                        net::PacketResult pr;
                                        pr.index = gIdx;
                                        pr.state = 0;
                                        pr.placerId = 0;
                                        pr.flagSkin = 0;
                                        net.broadcast(&pr, sizeof(pr));
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Flag / Unflag Action
            if (triggerFlag) {
                Vector2 toTarget = { actionTarget.x - renderer.localShip.position.x, actionTarget.y - renderer.localShip.position.y };
                float distToTarget = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y);
                if (distToTarget > 0.001f) {
                    renderer.localShip.angle = std::atan2(toTarget.y, toTarget.x) * RAD2DEG + 90.0f;
                }
                broadcastLaser(renderer.localShip.getNosePosition(), actionTarget, 1);

                pendingUncoverCell = -1;
                renderer.clearOutOfReach();

                if (hovered >= 0) {
                    size_t hIdx = static_cast<size_t>(hovered);
                    int sI = 0; size_t lI = 0; core::Board* targetBoard = nullptr; Vector2 cellCenter;
                    getTargetBoardAndCell(hIdx, sI, lI, targetBoard, cellCenter);

                    if (targetBoard) {
                        if (net.role == net::NetRole::Client) {
                            net::PacketClick pc;
                            pc.index = hIdx;
                            pc.action = 2;
                            pc.flagSkin = static_cast<uint8_t>(menu.flagSkin);
                            net.sendToServer(&pc, sizeof(pc));
                        } else {
                            uint32_t myId = (net.role == net::NetRole::Host) ? net::HOST_PLAYER_ID : 0;
                            core::CellState cs = targetBoard->getState(lI);
                            Vector2 groundPos = (currentMode == GameMode::Campaign)
                                ? campaignMgr.getSectorByIndex(sI)->getCellWorldPosition(lI, renderer.cellSize)
                                : renderer.getFlagBasePosition(hIdx, board);
                            Vector2 shipPos = renderer.localShip.position;
                            if (cs == core::CellState::Flagged) {
                                uint8_t skinId = targetBoard->getFlagSkin(lI, static_cast<uint8_t>(menu.flagSkin));
                                renderer.triggerFlagPickup(groundPos, shipPos, myId, true, skinId);
                                renderer.removeFlagDrop(hIdx);
                                targetBoard->unflag(lI);
                                if (net.role == net::NetRole::Host) {
                                    net::PacketResult pr;
                                    pr.index = hIdx;
                                    pr.state = 1;
                                    pr.placerId = myId;
                                    pr.flagSkin = skinId;
                                    net.broadcast(&pr, sizeof(pr));
                                }
                            } else if (cs == core::CellState::Hidden) {
                                targetBoard->setFlag(lI, myId, static_cast<uint8_t>(menu.flagSkin));
                                renderer.triggerFlagDrop(hIdx, groundPos, shipPos, static_cast<uint8_t>(menu.flagSkin));
                                soundMgr.playFlagSound();
                                if (net.role == net::NetRole::Host) {
                                    net::PacketResult pr;
                                    pr.index = hIdx;
                                    pr.state = 2;
                                    pr.placerId = myId;
                                    pr.flagSkin = static_cast<uint8_t>(menu.flagSkin);
                                    net.broadcast(&pr, sizeof(pr));
                                }
                            }
                            if (currentMode == GameMode::Campaign) saveCampaignProgress();
                            else saveCurrentSlot();
                        }
                    }
                }
            }

            // Chording Action
            if (triggerChord) {
                Vector2 toTarget = { actionTarget.x - renderer.localShip.position.x, actionTarget.y - renderer.localShip.position.y };
                float distToTarget = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y);
                if (distToTarget > 0.001f) {
                    renderer.localShip.angle = std::atan2(toTarget.y, toTarget.x) * RAD2DEG + 90.0f;
                }
                broadcastLaser(renderer.localShip.getNosePosition(), actionTarget, 0);

                if (hovered >= 0) {
                    size_t hIdx = static_cast<size_t>(hovered);
                    int sI = 0; size_t lI = 0; core::Board* targetBoard = nullptr; Vector2 cellCenter;
                    getTargetBoardAndCell(hIdx, sI, lI, targetBoard, cellCenter);

                    if (targetBoard && targetBoard->getState(lI) == core::CellState::Revealed) {
                        float dist = Vector2Distance(renderer.localShip.position, cellCenter);

                        if (dist > renderer.localShip.range && menu.controlMode == 0 && !padAvailable) {
                            renderer.triggerOutOfReach(static_cast<int64_t>(hIdx), cellCenter);
                        } else {
                            renderer.clearOutOfReach();
                            pendingUncoverCell = -1;
                            if (net.role == net::NetRole::Client) {
                                net::PacketClick pc;
                                pc.index = hIdx;
                                pc.action = 1;
                                pc.flagSkin = 0;
                                net.sendToServer(&pc, sizeof(pc));
                            } else {
                                std::vector<size_t> newlyRevealed;
                                bool hitBomb = false;
                                if (targetBoard->chord(lI, newlyRevealed, hitBomb)) {
                                    if (hitBomb) {
                                        if (playerInventory.hasItem(core::ItemId::BlastShield)) {
                                            playerInventory.consumeItem(core::ItemId::BlastShield);
                                            targetBoard->isGameOver = false;
                                            renderer.camera.shake(0.65f);
                                            Vector2 knockDir = Vector2Normalize(Vector2Subtract(renderer.localShip.position, cellCenter));
                                            renderer.localShip.velocity = Vector2Scale(knockDir, 460.0f);
                                            renderer.emitExplosion(cellCenter, ui::Colors::Amber500);
                                            soundMgr.playExplosionSound();
                                            renderer.shopShip.triggerBanter("Oof, that sounded expensive! Thank your shields.");
                                            if (!renderer.shopShips.empty()) renderer.shopShips.front().triggerBanter("Oof, that sounded expensive! Thank your shields.");
                                        } else {
                                            soundMgr.playExplosionSound();
                                            renderer.shopShip.triggerBanter("Oof, that sounded expensive!");
                                            if (!renderer.shopShips.empty()) renderer.shopShips.front().triggerBanter("Oof, that sounded expensive!");
                                            if (currentMode == GameMode::Campaign) {
                                                campaignMgr.handleEmergencyExtraction(sI, scrapCount);
                                                hud.scrapCount = scrapCount;
                                                menu.scrapCount = scrapCount;
                                                renderer.shopShip.triggerBanter("Engines spooling up! Get to the ship!");
                                                if (!renderer.shopShips.empty()) renderer.shopShips.front().triggerBanter("Engines spooling up! Get to the ship!");
                                            }
                                        }
                                    } else if (!newlyRevealed.empty()) {
                                        soundMgr.playUncoverSound();
                                    }
                                    for (size_t revIdx : newlyRevealed) {
                                        size_t gIdx = (currentMode == GameMode::Campaign) ? core::CampaignManager::toGlobalCellIndex(sI, revIdx) : revIdx;
                                        renderer.removeFlagDrop(gIdx);
                                        Vector2 pos = (currentMode == GameMode::Campaign)
                                            ? campaignMgr.getSectorByIndex(sI)->getCellWorldPosition(revIdx, renderer.cellSize)
                                            : renderer.getCellWorldPosition(revIdx, board);
                                        if (targetBoard->isBomb(revIdx)) {
                                            renderer.emitExplosion(pos, ui::Colors::CellFlag);
                                        } else {
                                            renderer.emitDebris(pos, ui::Colors::Zinc400);
                                            if (render::ScrapSystem::isScrapCell(targetBoard->config.seed, revIdx, targetBoard->totalCells(), targetBoard->config.bombs)) {
                                                scrapSystem.spawn(pos);
                                            }
                                        }

                                        if (net.role == net::NetRole::Host) {
                                            net::PacketResult pr;
                                            pr.index = gIdx;
                                            pr.state = 0;
                                            pr.placerId = 0;
                                            pr.flagSkin = 0;
                                            net.broadcast(&pr, sizeof(pr));
                                        }
                                    }
                                    if (currentMode == GameMode::Campaign) {
                                        auto* curSec = campaignMgr.getSectorByIndex(sI);
                                        if (curSec && curSec->modifier == core::SectorModifier::FoundryWastes) {
                                            for (size_t rev : newlyRevealed) {
                                                if (!targetBoard->isBomb(rev)) {
                                                    core::MoltenTileTimer mt;
                                                    mt.cellIndex = rev;
                                                    mt.timeLeft = 5.0f;
                                                    mt.active = true;
                                                    curSec->moltenTimers.push_back(mt);
                                                }
                                            }
                                        }
                                        if (curSec && curSec->modifier == core::SectorModifier::JammedComms && !curSec->centralDataNodeCleared) {
                                            for (size_t rev : newlyRevealed) {
                                                if (rev == curSec->centralDataNodeIdx) {
                                                    curSec->centralDataNodeCleared = true;
                                                    soundMgr.playUncoverSound();
                                                    renderer.shopShip.triggerBanter("Central relay decrypted! Communications online.");
                                                    if (!renderer.shopShips.empty()) renderer.shopShips.front().triggerBanter("Central relay decrypted! Communications online.");
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                    if (currentMode == GameMode::Campaign) {
                                        bool justSecured = campaignMgr.checkSectorClear(sI);
                                        if (justSecured) {
                                            scrapCount += static_cast<uint64_t>(50 * (sI + 1));
                                            hud.scrapCount = scrapCount;
                                            menu.scrapCount = scrapCount;
                                            hud.triggerScrapPulse();
                                            soundMgr.playUncoverSound();
                                            renderer.triggerSectorClearAnimation(sI, campaignMgr.sectors[sI].board, campaignMgr.sectors[sI].gridOffset);
                                        }
                                        saveCampaignProgress();
                                    } else {
                                        if (board.isVictory) {
                                            renderer.triggerSectorClearAnimation(-1, board, { 0.0f, 0.0f });
                                        }
                                        saveCurrentSlot();
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // Periodic Handshake retry if Client is waiting for initial board synchronization
        if (net.role == net::NetRole::Client && board.coord.totalCells == 0) {
            static float syncRetryTimer = 0.0f;
            syncRetryTimer += dt;
            if (syncRetryTimer >= 1.0f) {
                syncRetryTimer = 0.0f;
                net::PacketHandshake hs;
                hs.type = net::PacketType::Handshake;
                std::strncpy(hs.name, menu.playerName, sizeof(hs.name) - 1);
                hs.cursorSkin = static_cast<uint8_t>(menu.cursorSkin);
                hs.flagSkin = static_cast<uint8_t>(menu.flagSkin);
                net.sendToServer(&hs, sizeof(hs), true);
                std::cout << "[CLIENT] Requesting board sync from host..." << std::endl;
            }
        }

        // Cheat / X-Ray Mode ('X')
        if (IsKeyPressed(KEY_X) && net.role != net::NetRole::Client) {
            if (currentMode == GameMode::Campaign) {
                auto* sec = campaignMgr.getSectorByIndex(campaignMgr.activeSectorIndex);
                if (sec) {
                    for (size_t i = 0; i < sec->board.totalCells(); ++i) {
                        if (!sec->board.isBomb(i)) {
                            sec->board.reveal(i);
                        }
                    }
                    campaignMgr.checkSectorClear(campaignMgr.activeSectorIndex);
                    int curAct = campaignMgr.activeSectorIndex;
                    if (curAct >= 0 && curAct < static_cast<int>(campaignMgr.sectors.size())) {
                        renderer.triggerSectorClearAnimation(curAct, campaignMgr.sectors[curAct].board, campaignMgr.sectors[curAct].gridOffset);
                    }
                    saveCampaignProgress();
                }
            } else {
                for (size_t i = 0; i < board.totalCells(); ++i) {
                    if (!board.isBomb(i)) {
                        board.reveal(i);
                    }
                }
                if (board.isVictory) {
                    renderer.triggerSectorClearAnimation(-1, board, { 0.0f, 0.0f });
                }
            }
        }

        // Timer progression
        if (currentMode == GameMode::Campaign) {
            timePlayed += dt;
        } else if (board.revealedCount > 0 && !board.isGameOver && !board.isVictory) {
            timePlayed += dt;
        }

        // Restart hotkey ('R')
        if (currentMode == GameMode::Campaign) {
            auto* curSec = campaignMgr.getSectorByIndex(campaignMgr.activeSectorIndex);
            if (curSec && (curSec->board.isGameOver || campaignMgr.isPlanetCleared) && IsKeyPressed(KEY_R)) {
                if (curSec->board.isGameOver && !curSec->isCleared) {
                    curSec->board.init(curSec->board.config);
                    renderer.localShip.position = curSec->spawnPos;
                    renderer.localShip.velocity = { 0.0f, 0.0f };
                    renderer.camera.centerOn(curSec->spawnPos);
                    renderer.clearOutOfReach();
                    pendingUncoverCell = -1;
                    saveCampaignProgress();
                } else if (campaignMgr.isPlanetCleared) {
                    startCampaignGame(false);
                }
            }
        } else if ((board.isGameOver || board.isVictory) && IsKeyPressed(KEY_R)) {
            if (net.role != net::NetRole::Client) {
                restartCurrentGame();
            }
        }

        // Sync primary shop ship reference
        if (!renderer.shopShips.empty()) {
            renderer.shopShip = renderer.shopShips.front();
        }
        hud.scrapCount = scrapCount;
        menu.scrapCount = scrapCount;

        // Sector Modifiers in Campaign Mode
        if (currentMode == GameMode::Campaign) {
            auto* curSec = campaignMgr.getSectorByIndex(campaignMgr.activeSectorIndex);
            if (curSec) {
                renderer.ionStormActive = (curSec->modifier == core::SectorModifier::IonStorm);

                if (curSec->modifier == core::SectorModifier::FoundryWastes && !curSec->isCleared && !curSec->board.isGameOver) {
                    for (auto& mt : curSec->moltenTimers) {
                        if (mt.active && mt.timeLeft > 0.0f) {
                            mt.timeLeft -= dt;
                            if (mt.timeLeft <= 0.0f) {
                                mt.timeLeft = 0.0f;
                                mt.active = false;
                                Vector2 cellPos = curSec->getCellWorldPosition(mt.cellIndex, renderer.cellSize);
                                renderer.emitExplosion(cellPos, ui::Colors::Amber500);
                                soundMgr.playExplosionSound();
                                renderer.shopShip.triggerBanter("Molten cell collapsed! Watch your hull temperature!");
                                if (!renderer.shopShips.empty()) {
                                    renderer.shopShips.front().triggerBanter("Molten cell collapsed! Watch your hull temperature!");
                                }
                            }
                        }
                    }
                }
            } else {
                renderer.ionStormActive = false;
            }
        } else {
            renderer.ionStormActive = false;
        }
        renderer.updateIonStorm(dt);

        // Update Scrap System with drag-and-drop & hopper deposit
        Vector2 hudScrapPos = hud.getScrapBadgeScreenPos();
        bool isMouseDown = IsMouseButtonDown(MOUSE_LEFT_BUTTON) && !isOverUI && !mouseHandledByShop && !isEditorOpen;
        bool mouseClicked = IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !isOverUI && !mouseHandledByShop && !isEditorOpen;
        Rectangle hopperRect = !renderer.shopShips.empty() ? renderer.shopShips.front().getHopperWorldRect() : renderer.shopShip.getHopperWorldRect();
        scrapSystem.update(dt, hudScrapPos, renderer.camera.camera, worldMouse, isMouseDown, mouseClicked, hopperRect);
        if (scrapSystem.hopperDepositTriggered) {
            renderer.shopShip.triggerBanter("Scrap received! Banking to team account.");
            if (!renderer.shopShips.empty()) {
                renderer.shopShips.front().triggerBanter("Scrap received! Banking to team account.");
            }
        }

        int collected = scrapSystem.collectPending();
        if (collected > 0) {
            scrapCount += collected;
            hud.scrapCount = scrapCount;
            menu.scrapCount = scrapCount;
            hud.triggerScrapPulse();
            saveSettings();
            if (net.role != net::NetRole::Client) {
                if (currentMode == GameMode::Campaign) saveCampaignProgress();
                else saveCurrentSlot();
            }
        }

        // Periodic auto-save while in-game (every 3 seconds)
        static float autoSaveTimer = 0.0f;
        autoSaveTimer += dt;
        if (autoSaveTimer >= 3.0f) {
            autoSaveTimer = 0.0f;
            if (net.role != net::NetRole::Client) {
                if (currentMode == GameMode::Campaign) saveCampaignProgress();
                else saveCurrentSlot();
            }
        }
    }
}

bool App::isMouseOverUI() const {
    return isMouseOverUI(GetMousePosition());
}

bool App::isMouseOverUI(Vector2 mousePos) const {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    float scale = std::clamp(menu.guiScale, 0.75f, 1.50f);
    bool isCampaign = (currentMode == GameMode::Campaign);

    Vector2 uiMouse = { mousePos.x / scale, mousePos.y / scale };
    if (sectorEditor.isOpen && sectorEditor.isMouseOver(uiMouse)) {
        return true;
    }

    // 1. Game HUD (top bar, modals, banners; footer only in non-campaign)
    if (hud.isMouseOver(screenW, screenH, scale, mousePos, isCampaign)) {
        return true;
    }

    // 2. Shop, Roulette, and Inventory Hotbar Dock
    bool allowShops = (!isCampaign || campaignMgr.activeSectorIndex > 0);
    if (allowShops) {
        if (rouletteProximityAlpha > 0.01f && isRouletteOpen) {
            if (rouletteUI.isMouseOverCard(mousePos)) {
                return true;
            }
        }
        if (shopMenu.isMouseOverUI(screenW, screenH, playerInventory, shopProximityAlpha, mousePos)) {
            return true;
        }
    } else {
        // Sector 1: shops and roulette do not exist. Only check hotbar dock if hovering items
        if (shopMenu.isMouseOverHotbar(screenW, screenH, playerInventory, mousePos)) {
            return true;
        }
    }

    // 3. FPS counter overlay badge if active
    if (menu.showFPS) {
        float fpsW = 120.0f;
        float fpsH = 26.0f;
        float fpsX = static_cast<float>(screenW) - fpsW - 10.0f;
        float fpsY = 74.0f * scale + 4.0f;
        Rectangle fpsRect = { fpsX, fpsY, fpsW, fpsH };
        if (CheckCollisionPointRec(mousePos, fpsRect)) {
            return true;
        }
    }

    return false;
}

void App::draw() {
    float scale = std::clamp(menu.guiScale, 0.75f, 1.50f);
    ui::Widgets::setScale(scale);
    ui::Widgets::setCRT(menu.crtEnabled);
    renderer.guiScale = scale;

    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    int uiW = static_cast<int>(screenW / scale);
    int uiH = static_cast<int>(screenH / scale);

    Camera2D uiCam = { 0 };
    uiCam.zoom = scale;

    renderer.beginOffscreen();

    if (state == AppState::Menu) {
        ClearBackground(ui::Colors::Zinc950);

        BeginMode2D(uiCam);
        ui::MenuActions menuAct = menu.drawAndProcess(uiW, uiH);
        EndMode2D();

        if (menuAct.toggleCRT || menuAct.vsyncChanged || menuAct.fpsLimitChanged || menuAct.guiScaleChanged || menuAct.controlModeChanged) {
            renderer.enableCRT = menu.crtEnabled;
            renderer.camera.enableCRT = menu.crtEnabled;
            renderer.guiScale = menu.guiScale;
            ui::Widgets::setScale(menu.guiScale);

            if (menu.vsyncEnabled) {
                SetWindowState(FLAG_VSYNC_HINT);
            } else {
                ClearWindowState(FLAG_VSYNC_HINT);
            }

            if (menu.fpsLimit >= 245.0f) {
                SetTargetFPS(0);
            } else {
                SetTargetFPS(static_cast<int>(menu.fpsLimit));
            }

            saveSettings();
        }

        if (menuAct.bgmSkip) {
            soundMgr.skip();
        }

        if (menuAct.openSectorEditor) {
            currentMode = GameMode::Editor;
            state = AppState::InGame;
            if (campaignMgr.sectors.empty()) {
                campaignMgr.init(12345);
            }
            int sIdx = std::clamp(campaignMgr.activeSectorIndex, 0, static_cast<int>(campaignMgr.sectors.size()) - 1);
            sectorEditor.open(campaignMgr, sIdx);
            renderer.localShip.isInitialized = false;
            renderer.localShip.velocity = { 0.0f, 0.0f };
            renderer.localShip.isMoving = false;
            renderer.camera.reset({ 0.0f, 0.0f }, 0.90f);
            renderer.camera.centerOn({ campaignMgr.sectors[sIdx].arenaBounds.x + campaignMgr.sectors[sIdx].arenaBounds.width * 0.5f,
                                      campaignMgr.sectors[sIdx].arenaBounds.y + campaignMgr.sectors[sIdx].arenaBounds.height * 0.5f });
        }
        if (menuAct.toggleSectorEditor) {
            sectorEditor.toggle(campaignMgr);
        }

        if (menuAct.playCampaignSolo) {
            currentMode = GameMode::Campaign;
            startCampaignGame(false);
            state = AppState::InGame;
            saveSettings();
        }
        else if (menuAct.playCampaignHost) {
            currentMode = GameMode::Campaign;
            startCampaignGame(true);
            if (net.startHost(menuAct.hostPort)) {
                state = AppState::InGame;
                saveSettings();
            } else {
                menu.statusMessage = "FAILED TO BIND PORT";
            }
        }
        else if (menuAct.resetCampaign) {
            saveMgr.deleteCampaignSave();
            campaignMgr.init(static_cast<uint64_t>(GetTime() * 100000.0) ^ 0x51A8C9ULL);
            saveCampaignProgress();
        }
        else if (menuAct.playSolo) {
            currentMode = GameMode::Custom;
            activeSaveSlot = menuAct.selectedSlot;
            if (menuAct.startNewInSlot) {
                activeSaveName = menuAct.newSlotName;
                saveMgr.createSlot(activeSaveSlot, activeSaveName, menuAct.newSlotConfig);
            }
            loadSaveSlot(activeSaveSlot);
            state = AppState::InGame;
            saveSettings();
        }
        else if (menuAct.hostGame) {
            currentMode = GameMode::Custom;
            activeSaveSlot = menuAct.selectedSlot;
            if (menuAct.startNewInSlot) {
                activeSaveName = menuAct.newSlotName;
                saveMgr.createSlot(activeSaveSlot, activeSaveName, menuAct.newSlotConfig);
            }
            loadSaveSlot(activeSaveSlot);
            if (net.startHost(menuAct.hostPort)) {
                state = AppState::InGame;
                saveSettings();
            } else {
                menu.statusMessage = "FAILED TO BIND PORT";
            }
        }
        else if (menuAct.joinGame) {
            currentMode = GameMode::Custom;
            activeSaveSlot = 0;
            if (net.connectToHost(menuAct.joinAddress)) {
                board.init(2, 0, 0, 0);
                timePlayed = 0.0f;
                scrapCount = 0;
                hud.scrapCount = 0;
                state = AppState::InGame;
                menu.statusMessage.clear();
            } else {
                menu.statusMessage = "FAILED TO INITIALIZE NETWORK";
            }
        }
        else if (menuAct.quit) {
            shouldQuit = true;
        }
    }
    else if (state == AppState::InGame) {
        ClearBackground(BLACK);

        if (board.coord.totalCells == 0 && net.role == net::NetRole::Client) {
            static float dotTimer = 0.0f;
            dotTimer += GetFrameTime();
            int dots = static_cast<int>(dotTimer * 2.5f) % 4;
            std::string msg = "SYNCHRONIZING BOARD WITH HOST";
            for (int d = 0; d < dots; ++d) msg += ".";

            BeginMode2D(uiCam);
            int msgW = MeasureText(msg.c_str(), 20);
            DrawText(msg.c_str(), uiW / 2 - msgW / 2, uiH / 2 - 25, 20, ui::Colors::Zinc400);

            if (ui::Widgets::button("CANCEL", { static_cast<float>(uiW / 2 - 60), static_cast<float>(uiH / 2 + 25), 120.0f, 38.0f }, ui::Colors::Zinc800, ui::Colors::Zinc600, false, 16)) {
                net.disconnect();
                state = AppState::Menu;
            }
            EndMode2D();
        }
        else {
            int64_t hovered = currentHoveredCell;
            if (currentMode == GameMode::Campaign || currentMode == GameMode::Editor) {
                renderer.renderCampaign(campaignMgr, hovered, net);
            } else {
                renderer.render(board, hovered, net);
            }

            BeginMode2D(renderer.camera.camera);
            scrapSystem.drawWorld(renderer.camera.camera);

            // Draw interaction reticle and prompt when hovering over a shop ship
            if (hoveredShopIndex >= 0 && hoveredShopIndex < static_cast<int>(renderer.shopShips.size()) && openedShopIndex != hoveredShopIndex) {
                const auto& s = renderer.shopShips[hoveredShopIndex];
                float t = static_cast<float>(GetTime());
                float pulse = (std::sin(t * 8.0f) + 1.0f) * 0.5f;

                Vector2 pA, pB;
                s.getCapsuleSegment(pA, pB);

                Color ringCol = isHoveredShopInRange ? ui::Colors::Amber400 : ui::Colors::Zinc500;
                float r = (s.collisionRadius * s.scale + 6.0f) + pulse * 3.0f;

                // Draw interaction bracket / rings around shop
                DrawCircleLines(static_cast<int>(pA.x), static_cast<int>(pA.y), r, Fade(ringCol, 0.70f));
                if (s.capsuleLength > 0.0f) {
                    DrawCircleLines(static_cast<int>(pB.x), static_cast<int>(pB.y), r, Fade(ringCol, 0.70f));
                }

                // Floating prompt above the ship
                const char* prompt = isHoveredShopInRange ? "[L-CLICK] OPEN SHOP" : "[TOO FAR]";
                int pW = MeasureText(prompt, 11);
                float topY = std::min({ pA.y, pB.y, s.position.y });
                float promptX = s.position.x - pW * 0.5f;
                float promptY = topY - (s.collisionRadius * s.scale + 16.0f);
                Rectangle pBg = { promptX - 6.0f, promptY - 2.0f, static_cast<float>(pW + 12), 16.0f };
                DrawRectangleRec(pBg, Fade(ui::Colors::Zinc950, 0.85f));
                DrawRectangleLinesEx(pBg, 1.0f, Fade(ringCol, 0.80f));
                DrawText(prompt, static_cast<int>(promptX), static_cast<int>(promptY), 11, ringCol);
            }

            // Draw interaction reticle and prompt when hovering over roulette ship
            if (isRouletteHovered && !isRouletteOpen && renderer.hasRouletteShip) {
                const auto& r = renderer.rouletteShip;
                float t = static_cast<float>(GetTime());
                float pulse = (std::sin(t * 8.0f) + 1.0f) * 0.5f;
                Color ringCol = isRouletteInRange ? Color{ 168, 85, 247, 255 } : ui::Colors::Zinc500;
                float rad = (r.collisionRadius * r.scale + 6.0f) + pulse * 3.0f;

                DrawCircleLines(static_cast<int>(r.position.x), static_cast<int>(r.position.y), rad, Fade(ringCol, 0.70f));

                const char* prompt = isRouletteInRange ? "[L-CLICK] PLAY ROULETTE" : "[TOO FAR]";
                int pW = MeasureText(prompt, 11);
                float promptX = r.position.x - pW * 0.5f;
                float promptY = r.position.y - (r.collisionRadius * r.scale + 20.0f);
                Rectangle pBg = { promptX - 6.0f, promptY - 2.0f, static_cast<float>(pW + 12), 16.0f };
                DrawRectangleRec(pBg, Fade(ui::Colors::Zinc950, 0.85f));
                DrawRectangleLinesEx(pBg, 1.0f, Fade(ringCol, 0.80f));
                DrawText(prompt, static_cast<int>(promptX), static_cast<int>(promptY), 11, ringCol);
            }

            if (sectorEditor.isOpen) {
                Vector2 worldMouse = renderer.camera.getScreenToWorld(renderer.camera.getCRTMousePosition());
                sectorEditor.drawWorldGizmos(campaignMgr, worldMouse);
            }

            EndMode2D();

            BeginMode2D(uiCam);
            ui::HUDActions hudAct;
            if (currentMode == GameMode::Campaign) {
                hudAct = hud.drawAndProcessCampaign(uiW, uiH, campaignMgr, timePlayed, net, voiceMgr.isTransmitting(), voiceMgr.getSettings().enabled, voiceMgr.getSettings().pushToTalk);
            } else if (currentMode == GameMode::Custom) {
                hudAct = hud.drawAndProcess(uiW, uiH, board, timePlayed, net, voiceMgr.isTransmitting(), voiceMgr.getSettings().enabled, voiceMgr.getSettings().pushToTalk);
            }
            scrapSystem.drawScreen(scale);
            EndMode2D();

            // Draw Hovering Shop Menu if player is in proximity
            if (shopProximityAlpha > 0.01f && nearbyShopIndex >= 0 && nearbyShopIndex < static_cast<int>(renderer.shopShips.size())) {
                auto& targetShip = renderer.shopShips[nearbyShopIndex];
                bool bought = shopMenu.drawHoverMenu(
                    targetShip,
                    scrapCount,
                    playerInventory,
                    shopProximityAlpha,
                    renderer.camera,
                    screenW,
                    screenH
                );
                if (bought) {
                    hud.scrapCount = scrapCount;
                    menu.scrapCount = scrapCount;
                    renderer.particles.emitSparkles(targetShip.position, 6, ui::Colors::Amber400, 16.0f);
                    renderer.particles.emitPlasmaMotes(targetShip.position, 4, ui::Colors::Amber300, 20.0f, 70.0f);
                    if (currentMode == GameMode::Campaign) saveCampaignProgress();
                    else saveCurrentSlot();
                }
            }

            // Draw Roulette Casino Terminal if open
            if (rouletteProximityAlpha > 0.01f && renderer.hasRouletteShip) {
                bool action = rouletteUI.draw(
                    renderer.rouletteShip,
                    scrapCount,
                    rouletteProximityAlpha,
                    renderer.camera,
                    screenW,
                    screenH
                );
                if (action) {
                    hud.scrapCount = scrapCount;
                    menu.scrapCount = scrapCount;
                    if (currentMode == GameMode::Campaign) saveCampaignProgress();
                    else saveCurrentSlot();
                }
            }

            // Draw Roulette Result Popup if active (draws on top of terminal and game world)
            if (rouletteUI.isPopupActive() && renderer.hasRouletteShip) {
                rouletteUI.drawPopup(
                    renderer.camera,
                    screenW,
                    screenH,
                    renderer.rouletteShip.position,
                    rouletteProximityAlpha
                );
            }

            // Draw HUD 5-Slot Hotbar Inventory Dock
            shopMenu.drawInventoryDock(screenW, screenH, playerInventory, 1.0f);

            if (hudAct.returnToMenu) {
                int leftover = scrapSystem.collectAll();
                if (leftover > 0) {
                    scrapCount += leftover;
                    hud.scrapCount = scrapCount;
                    menu.scrapCount = scrapCount;
                    saveSettings();
                }
                if (currentMode == GameMode::Campaign) {
                    saveCampaignProgress();
                    menu.currentScreen = ui::MenuScreen::Campaign;
                } else {
                    saveCurrentSlot();
                    menu.currentScreen = ui::MenuScreen::Play;
                }
                net.disconnect();
                state = AppState::Menu;
            }
            if (hudAct.requestNewGame) {
                uint64_t nextSeed = hud.nextSeed;

                if (net.role == net::NetRole::Host) {
                    net::PacketInit p;
                    p.dim = hud.nextDim;
                    p.size = hud.nextSize;
                    p.bombs = hud.nextBombs;
                    p.seed = nextSeed;
                    net.broadcast(&p, sizeof(p));
                }
                startNewGame(hud.nextDim, hud.nextSize, hud.nextBombs, nextSeed);
            }
            if (hudAct.restartGame) {
                if (net.role != net::NetRole::Client) {
                    if (currentMode == GameMode::Campaign) {
                        auto* sec = campaignMgr.getSectorByIndex(campaignMgr.activeSectorIndex);
                        if (sec) {
                            sec->board.init(sec->board.config);
                            sec->isCleared = false;
                            renderer.localShip.position = sec->spawnPos;
                            renderer.localShip.velocity = { 0.0f, 0.0f };
                            renderer.camera.centerOn(sec->spawnPos);
                            renderer.clearOutOfReach();
                            pendingUncoverCell = -1;
                            saveCampaignProgress();
                        }
                    } else {
                        restartCurrentGame();
                    }
                }
            }
            if (hudAct.toggleHost) {
                net.startHost(7777);
            }
            if (hudAct.disconnect) {
                net.disconnect();
            }
        }
    }

    if (sectorEditor.isOpen) {
        BeginMode2D(uiCam);
        sectorEditor.drawAndProcess(campaignMgr, uiW, uiH);
        EndMode2D();
    }

    // On-Screen FPS Counter Overlay
    if (menu.showFPS) {
        int fps = GetFPS();
        Color fpsCol = (fps >= 55) ? ui::Colors::Green400 : ((fps >= 30) ? ui::Colors::Amber400 : ui::Colors::Red500);
        const char* fpsText = TextFormat("%d FPS", fps);
        int tw = MeasureText(fpsText, 14);
        float badgeW = static_cast<float>(tw + 14);
        float badgeH = 22.0f;
        float badgeX = static_cast<float>(screenW - badgeW - 10.0f);
        float badgeY = (state == AppState::InGame ? (74.0f * scale + 4.0f) : 8.0f);

        Rectangle badgeRect = { badgeX, badgeY, badgeW, badgeH };
        DrawRectangleRec(badgeRect, Fade(BLACK, 0.8f));
        DrawRectangleLinesEx(badgeRect, 1.0f, ui::Colors::Zinc700);
        DrawText(fpsText, static_cast<int>(badgeX + 7), static_cast<int>(badgeY + 4), 14, fpsCol);
    }

    renderer.endOffscreen();

    BeginDrawing();
    ClearBackground(BLACK);
    renderer.drawOffscreenToScreen();
    EndDrawing();

    if (testShopMode && state == AppState::InGame) {
        static int drawFrame = 0;
        ++drawFrame;
        if (drawFrame == 10) {
            TakeScreenshot("screenshot_docked.png");
            std::cout << "[TEST] Frame 10: Saved screenshot_docked.png at shop pos (" 
                      << renderer.shopShip.position.x << ", " << renderer.shopShip.position.y << ")" << std::endl;
        } else if (drawFrame == 19) {
            TakeScreenshot("screenshot_collide.png");
            std::cout << "[TEST] Frame 19: Saved screenshot_collide.png" << std::endl;
        } else if (drawFrame == 120) {
            TakeScreenshot("screenshot_restored.png");
            float distToAnchor = std::sqrt(
                (renderer.shopShip.position.x - renderer.shopShip.anchorPosition.x) * (renderer.shopShip.position.x - renderer.shopShip.anchorPosition.x) +
                (renderer.shopShip.position.y - renderer.shopShip.anchorPosition.y) * (renderer.shopShip.position.y - renderer.shopShip.anchorPosition.y));
            std::cout << "[TEST] Frame 120: Saved screenshot_restored.png, dist to anchor: " << distToAnchor << std::endl;
            shouldQuit = true;
        }
    }

    if (testCustomizeMode) {
        static int custFrame = 0;
        ++custFrame;
        if (custFrame == 5) {
            menu.guiScale = 1.0f;
        } else if (custFrame == 8) {
            TakeScreenshot("screenshot_cust_100.png");
            std::cout << "[TEST] Saved screenshot_cust_100.png" << std::endl;
            menu.guiScale = 1.5f;
        } else if (custFrame == 14) {
            TakeScreenshot("screenshot_cust_150.png");
            std::cout << "[TEST] Saved screenshot_cust_150.png" << std::endl;
            menu.guiScale = 0.75f;
        } else if (custFrame == 20) {
            TakeScreenshot("screenshot_cust_75.png");
            std::cout << "[TEST] Saved screenshot_cust_75.png" << std::endl;
            shouldQuit = true;
        }
    }

    if (testShopUIMode && state == AppState::InGame) {
        static int drawUIFrame = 0;
        ++drawUIFrame;
        if (drawUIFrame == 5) {
            TakeScreenshot("screenshot_shop_hover.png");
            std::cout << "[TEST-UI] Saved screenshot_shop_hover.png" << std::endl;
        } else if (drawUIFrame == 13) {
            TakeScreenshot("screenshot_shop_menu.png");
            std::cout << "[TEST-UI] Saved screenshot_shop_menu.png" << std::endl;
        } else if (drawUIFrame == 23) {
            TakeScreenshot("screenshot_big_shop.png");
            std::cout << "[TEST-UI] Saved screenshot_big_shop.png" << std::endl;
        } else if (drawUIFrame == 30) {
            TakeScreenshot("screenshot_held_item_deploy.png");
            std::cout << "[TEST-UI] Saved screenshot_held_item_deploy.png" << std::endl;
        } else if (drawUIFrame == 35) {
            TakeScreenshot("screenshot_bubble_blur.png");
            std::cout << "[TEST-UI] Saved screenshot_bubble_blur.png" << std::endl;
        } else if (drawUIFrame == 40) {
            TakeScreenshot("screenshot_held_item_retract.png");
            std::cout << "[TEST-UI] Saved screenshot_held_item_retract.png" << std::endl;
        } else if (drawUIFrame == 44) {
            TakeScreenshot("screenshot_roulette_ui.png");
            std::cout << "[TEST-UI] Saved screenshot_roulette_ui.png" << std::endl;
        } else if (drawUIFrame == 48) {
            TakeScreenshot("screenshot_roulette_spin.png");
            std::cout << "[TEST-UI] Saved screenshot_roulette_spin.png" << std::endl;
        } else if (drawUIFrame == 51) {
            TakeScreenshot("screenshot_roulette_win.png");
            std::cout << "[TEST-UI] Saved screenshot_roulette_win.png" << std::endl;
        } else if (drawUIFrame == 54) {
            TakeScreenshot("screenshot_roulette_loss.png");
            std::cout << "[TEST-UI] Saved screenshot_roulette_loss.png" << std::endl;
            shouldQuit = true;
        }
    }

    if (testCampaignMode) {
        static int campFrame = 0;
        ++campFrame;
        if (campFrame == 12) {
            renderer.saveScreenshot("screenshot_campaign_camera_orbit.png");
            std::cout << "[TEST-CAMPAIGN-UI] Saved screenshot_campaign_camera_orbit.png" << std::endl;
        } else if (campFrame == 14) {
            startCampaignGame(false);
            campaignMgr.init(12345);
            state = AppState::InGame;
            float arenaMidX = campaignMgr.sectors[0].arenaBounds.x + campaignMgr.sectors[0].arenaBounds.width * 0.5f;
            float arenaMidY = campaignMgr.sectors[0].arenaBounds.y + campaignMgr.sectors[0].arenaBounds.height * 0.5f;
            renderer.camera.reset({ 0.0f, 0.0f }, 0.90f);
            renderer.camera.centerOn({ arenaMidX, arenaMidY });
        } else if (campFrame == 16) {
            campaignMgr.sectors[0].board.setFlag(27, 0, 0);
        } else if (campFrame == 22) {
            renderer.saveScreenshot("screenshot_sector_world_locked.png");
            std::cout << "[TEST-CAMPAIGN-UI] Saved screenshot_sector_world_locked.png" << std::endl;
        } else if (campFrame == 24) {
            for (size_t i = 0; i < campaignMgr.sectors[0].board.totalCells(); ++i) {
                if (!campaignMgr.sectors[0].board.isBomb(i)) {
                    campaignMgr.sectors[0].board.reveal(i);
                }
            }
            campaignMgr.checkSectorClear(0);
            renderer.triggerSectorClearAnimation(0, campaignMgr.sectors[0].board, campaignMgr.sectors[0].gridOffset);
            campaignMgr.sectors[0].exitLauncher.openAnim = 1.0f;
        } else if (campFrame == 28) {
            renderer.saveScreenshot("screenshot_sector_clear_animation.png");
            std::cout << "[TEST-CAMPAIGN-UI] Saved screenshot_sector_clear_animation.png" << std::endl;
        } else if (campFrame == 40) {
            renderer.saveScreenshot("screenshot_sector_world_unlocked.png");
            renderer.saveScreenshot("screenshot_sector_cleared_defused_bombs.png");
            std::cout << "[TEST-CAMPAIGN-UI] Saved screenshot_sector_world_unlocked.png & screenshot_sector_cleared_defused_bombs.png" << std::endl;
        } else if (campFrame == 42) {
            triggerSectorWarp(1);
            float s2MidX = campaignMgr.sectors[1].arenaBounds.x + campaignMgr.sectors[1].arenaBounds.width * 0.5f;
            float s2MidY = campaignMgr.sectors[1].arenaBounds.y + campaignMgr.sectors[1].arenaBounds.height * 0.5f;
            renderer.camera.reset({ 0.0f, 0.0f }, 0.82f);
            renderer.camera.centerOn({ s2MidX, s2MidY });
        } else if (campFrame == 48) {
            renderer.saveScreenshot("screenshot_sector_02_world.png");
            std::cout << "[TEST-CAMPAIGN-UI] Saved screenshot_sector_02_world.png" << std::endl;
            shouldQuit = true;
        }
    }

    if (testEditorMode) {
        static int edFrame = 0;
        ++edFrame;
        if (edFrame == 3) {
            renderer.saveScreenshot("screenshot_main_menu_editor.png");
            renderer.saveScreenshot("screenshot_campaign_editor_button.png");
            std::cout << "[TEST-EDITOR] Saved screenshot_main_menu_editor.png" << std::endl;
            currentMode = GameMode::Editor;
            state = AppState::InGame;
            sectorEditor.open(campaignMgr, 0);
            sectorEditor.currentTab = ui::SectorEditorTab::Meta;
        } else if (edFrame == 9) {
            renderer.saveScreenshot("screenshot_editor_meta.png");
            std::cout << "[TEST-EDITOR] Saved screenshot_editor_meta.png" << std::endl;
            sectorEditor.currentTab = ui::SectorEditorTab::Map;
        } else if (edFrame == 15) {
            renderer.saveScreenshot("screenshot_editor_map.png");
            std::cout << "[TEST-EDITOR] Saved screenshot_editor_map.png" << std::endl;
            sectorEditor.currentTab = ui::SectorEditorTab::Progression;
        } else if (edFrame == 21) {
            renderer.saveScreenshot("screenshot_editor_progression.png");
            std::cout << "[TEST-EDITOR] Saved screenshot_editor_progression.png" << std::endl;
            sectorEditor.currentTab = ui::SectorEditorTab::Ships;
        } else if (edFrame == 27) {
            renderer.saveScreenshot("screenshot_editor_ships.png");
            std::cout << "[TEST-EDITOR] Saved screenshot_editor_ships.png" << std::endl;
            sectorEditor.currentTab = ui::SectorEditorTab::Walls;
        } else if (edFrame == 33) {
            renderer.saveScreenshot("screenshot_editor_walls.png");
            std::cout << "[TEST-EDITOR] Saved screenshot_editor_walls.png" << std::endl;
            currentMode = GameMode::Editor;
            campaignMgr.activeSectorIndex = 1;
            sectorEditor.open(campaignMgr, 1);
            sectorEditor.currentTab = ui::SectorEditorTab::Ships;
            sectorEditor.selectionType = ui::EditorSelectionType::MerchantDock;
            sectorEditor.selectedIndex = 0;
            renderer.camera.centerOn({ 150.0f, 200.0f });
        } else if (edFrame == 42) {
            renderer.saveScreenshot("screenshot_editor_ships_docks.png");
            std::cout << "[TEST-EDITOR] Saved screenshot_editor_ships_docks.png" << std::endl;
            sectorEditor.currentTab = ui::SectorEditorTab::Walls;
            core::SectorWall demoWall;
            demoWall.rect = { 180.0f, 160.0f, 140.0f, 60.0f };
            demoWall.isHazard = true;
            auto cfg = sectorEditor.getWorkingConfig();
            cfg.customWalls.push_back(demoWall);
            campaignMgr.rebuildSector(1, cfg);
            sectorEditor.open(campaignMgr, 1);
            sectorEditor.selectionType = ui::EditorSelectionType::Wall;
            sectorEditor.selectedIndex = static_cast<int>(cfg.customWalls.size()) - 1;
        } else if (edFrame == 50) {
            renderer.saveScreenshot("screenshot_editor_walls_gizmos.png");
            std::cout << "[TEST-EDITOR] Saved screenshot_editor_walls_gizmos.png" << std::endl;
            renderer.saveScreenshot("screenshot_editor_ingame.png");
            shouldQuit = true;
        }
    }
}

void App::run() {
    init();

    if (testShopMode) {
        startNewGame(2, 10, 10, 12345);
        state = AppState::InGame;
        float boardWidth = 10 * renderer.cellSize;
        renderer.camera.reset({ boardWidth - 100.0f, -40.0f }, 1.3f);
    }
    if (testShopUIMode) {
        startNewGame(2, 10, 10, 12345);
        state = AppState::InGame;
        scrapCount = 500;
        hud.scrapCount = 500;
        menu.scrapCount = 500;
        if (!renderer.shopShips.empty()) {
            renderer.localShip.position = { renderer.shopShips[0].position.x - 75.0f, renderer.shopShips[0].position.y };
            renderer.localShip.isInitialized = true;
            renderer.camera.centerOn(renderer.shopShips[0].position);
        }
    }
    if (testCustomizeMode) {
        state = AppState::Menu;
        menu.currentScreen = ui::MenuScreen::Customize;
    }
    if (testCampaignMode) {
        state = AppState::Menu;
        menu.currentScreen = ui::MenuScreen::Campaign;
        int sIdx = menu.planetRenderer.getSectorIdxForFortress(0);
        menu.campaignSelectedSector = (sIdx >= 0) ? sIdx : 0;
        menu.planetRenderer.focusSector(menu.campaignSelectedSector);
    }

    while (!WindowShouldClose() && !shouldQuit) {
        handleNetEvents();
        update(GetFrameTime());
        draw();
    }
}

} // namespace minesweeper
