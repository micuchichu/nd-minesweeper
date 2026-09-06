#include "app.hpp"
#include "net/steam_manager.hpp"
#include "ui/widgets.hpp"
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
    loadSettings();

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

    scrapSystem.init();
    hud.scrapTexture = scrapSystem.texture;
    menu.scrapTexture = scrapSystem.texture;
    hud.scrapCount = scrapCount;
    menu.scrapCount = scrapCount;
}

void App::cleanup() {
    saveCurrentSlot();
    saveSettings();
    scrapSystem.cleanup();
    voiceMgr.cleanup();
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

        menu.vsyncEnabled = gs.vsyncEnabled;
        menu.showFPS = gs.showFPS;
        menu.fpsLimit = gs.fpsLimit;
        menu.guiScale = std::clamp(gs.guiScale, 0.75f, 1.50f);

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

    gs.vsyncEnabled = menu.vsyncEnabled;
    gs.showFPS = menu.showFPS;
    gs.fpsLimit = menu.fpsLimit;
    gs.guiScale = menu.guiScale;
    gs.lastActiveSlot = activeSaveSlot;

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
    renderer.camera.reset(center, initialZoom);
    return ok;
}

void App::startNewGame(int dim, int size, int bombs, uint64_t seed) {
    renderer.clearParticles();
    renderer.localShip.isInitialized = false;
    pendingUncoverCell = -1;
    renderer.clearOutOfReach();
    int leftover = scrapSystem.collectAll();
    if (leftover > 0) {
        scrapCount += leftover;
        hud.scrapCount = scrapCount;
        menu.scrapCount = scrapCount;
        saveSettings();
    }
    board.init(dim, size, bombs, seed);
    hud.nextSeed = seed;
    std::snprintf(hud.seedBuf, sizeof(hud.seedBuf), "%llu", seed);
    timePlayed = 0.0f;
    hud.endModalDismissed = false;
    saveCurrentSlot();

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
    renderer.camera.reset(center, initialZoom);
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
                board.removeFlagsByPlacer(ev.peerId);
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
                            Vector2 cellCenter = { pos.x + renderer.cellSize * 0.5f, pos.y + renderer.cellSize * 0.5f };
                            if (ev.peerId != 0 && renderer.remoteShips.count(ev.peerId)) {
                                const auto& rShip = renderer.remoteShips[ev.peerId];
                                renderer.fireLaser(rShip.getNosePosition(), cellCenter, renderer.getLaserColorForSkin(rShip.skinId));
                            }

                            std::vector<size_t> newlyRevealed;
                            core::RevealResult res = board.reveal(idx, &newlyRevealed);
                            if (res == core::RevealResult::HitBomb) {
                                renderer.emitExplosion(pos, ui::Colors::CellFlag);
                            } else {
                                renderer.emitDebris(pos, ui::Colors::Zinc400);
                                for (size_t cIdx : newlyRevealed) {
                                    if (!board.isBomb(cIdx) && render::ScrapSystem::isScrapCell(board.config.seed, cIdx, board.totalCells(), board.config.bombs)) {
                                        Vector2 cPos = renderer.getCellWorldPosition(cIdx, board);
                                        scrapSystem.spawn({ cPos.x + renderer.cellSize * 0.5f, cPos.y + renderer.cellSize * 0.5f });
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
                                for (size_t revIdx : newlyRevealed) {
                                    Vector2 pos = renderer.getCellWorldPosition(revIdx, board);
                                    if (board.isBomb(revIdx)) {
                                        renderer.emitExplosion(pos, ui::Colors::CellFlag);
                                    } else {
                                        renderer.emitDebris(pos, ui::Colors::Zinc400);
                                        if (render::ScrapSystem::isScrapCell(board.config.seed, revIdx, board.totalCells(), board.config.bombs)) {
                                            scrapSystem.spawn({ pos.x + renderer.cellSize * 0.5f, pos.y + renderer.cellSize * 0.5f });
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
                    Vector2 cellCenter = { pos.x + renderer.cellSize * 0.5f, pos.y + renderer.cellSize * 0.5f };
                    if (ev.resultData.placerId != 0 && renderer.remoteShips.count(ev.resultData.placerId)) {
                        const auto& rShip = renderer.remoteShips[ev.resultData.placerId];
                        renderer.fireLaser(rShip.getNosePosition(), cellCenter, renderer.getLaserColorForSkin(rShip.skinId));
                    }
                    if (res == core::RevealResult::HitBomb) {
                        renderer.emitExplosion(pos, ui::Colors::CellFlag);
                    } else {
                        renderer.emitDebris(pos, ui::Colors::Zinc400);
                        for (size_t cIdx : newlyRevealed) {
                            if (!board.isBomb(cIdx) && render::ScrapSystem::isScrapCell(board.config.seed, cIdx, board.totalCells(), board.config.bombs)) {
                                Vector2 cPos = renderer.getCellWorldPosition(cIdx, board);
                                scrapSystem.spawn({ cPos.x + renderer.cellSize * 0.5f, cPos.y + renderer.cellSize * 0.5f });
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
                }
                break;
            }
            default:
                break;
        }
    }
}

void App::update(float dt) {
    if (IsKeyPressed(KEY_F11)) {
        ToggleFullscreen();
    }

    renderer.enableCRT = menu.crtEnabled;
    renderer.camera.enableCRT = menu.crtEnabled;
    renderer.activeFlagSkin = menu.flagSkin;
    renderer.activeCursorSkin = menu.cursorSkin;
    renderer.activePlayerSkin = menu.playerSkin;
    renderer.update(dt);

    // Synchronize voice settings and live mic meter with menu
    voiceMgr.getSettings() = menu.voiceSettings;
    menu.micInputLevel = voiceMgr.getMicLevel();

    Vector2 worldMouse = renderer.camera.getScreenToWorld(renderer.camera.getCRTMousePosition());
    renderer.updateShip(worldMouse, dt);

    voiceMgr.setLocalCursorPos(renderer.localShip.position.x, renderer.localShip.position.y);
    voiceMgr.setPushToTalkActive(IsKeyDown(KEY_V));
    voiceMgr.update(dt);

    renderer.isLocalSpeaking = voiceMgr.isTransmitting();

    if (state == AppState::InGame) {
        // Camera input
        renderer.camera.handleInput(!hud.showLargeGridWarning);

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
        if (net.role != net::NetRole::Offline) {
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

        int64_t hovered = renderer.getHoveredCellIndex(board);

        // In-game Input Handling
        if (!board.isGameOver && !board.isVictory && !hud.showLargeGridWarning) {
            // Left Mouse Button: Always point laser towards mouse & activate on every click!
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                // Instantly orient ship toward mouse click location
                Vector2 toMouse = { worldMouse.x - renderer.localShip.position.x, worldMouse.y - renderer.localShip.position.y };
                float distToMouse = std::sqrt(toMouse.x * toMouse.x + toMouse.y * toMouse.y);
                if (distToMouse > 0.001f) {
                    renderer.localShip.angle = std::atan2(toMouse.y, toMouse.x) * RAD2DEG + 90.0f;
                }

                // Fire laser directly towards the mouse!
                renderer.fireLaser(renderer.localShip.getNosePosition(), worldMouse, renderer.getLaserColorForSkin(menu.cursorSkin));

                if (hovered >= 0) {
                    size_t hIdx = static_cast<size_t>(hovered);
                    if (board.getState(hIdx) == core::CellState::Hidden) {
                        Vector2 cellPos = renderer.getCellWorldPosition(hIdx, board);
                        Vector2 cellCenter = { cellPos.x + renderer.cellSize * 0.5f, cellPos.y + renderer.cellSize * 0.5f };
                        float dist = Vector2Distance(renderer.localShip.position, cellCenter);

                        if (dist > renderer.localShip.range) {
                            renderer.triggerOutOfReach(static_cast<int64_t>(hIdx), cellPos);
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
                                std::vector<size_t> newlyRevealed;
                                core::RevealResult res = board.reveal(hIdx, &newlyRevealed);
                                for (size_t revIdx : newlyRevealed) {
                                    renderer.removeFlagDrop(revIdx);
                                }
                                Vector2 pos = renderer.getCellWorldPosition(hIdx, board);
                                if (res == core::RevealResult::HitBomb) {
                                    renderer.emitExplosion(pos, ui::Colors::CellFlag);
                                } else {
                                    renderer.emitDebris(pos, ui::Colors::Zinc400);
                                    for (size_t cIdx : newlyRevealed) {
                                        if (!board.isBomb(cIdx) && render::ScrapSystem::isScrapCell(board.config.seed, cIdx, board.totalCells(), board.config.bombs)) {
                                            Vector2 cPos = renderer.getCellWorldPosition(cIdx, board);
                                            scrapSystem.spawn({ cPos.x + renderer.cellSize * 0.5f, cPos.y + renderer.cellSize * 0.5f });
                                        }
                                    }
                                }

                                if (net.role == net::NetRole::Host) {
                                    for (size_t revIdx : newlyRevealed) {
                                        net::PacketResult pr;
                                        pr.index = revIdx;
                                        pr.state = 0;
                                        pr.placerId = 0;
                                        pr.flagSkin = 0;
                                        net.broadcast(&pr, sizeof(pr));
                                    }
                                }
                                saveCurrentSlot();
                            }
                        }
                    }
                }
            }

            // Pending uncover when ship arrives within reach
            if (pendingUncoverCell >= 0) {
                size_t pIdx = static_cast<size_t>(pendingUncoverCell);
                if (board.isGameOver || board.isVictory || pIdx >= board.totalCells() || board.getState(pIdx) != core::CellState::Hidden || IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) || IsKeyPressed(KEY_ESCAPE)) {
                    pendingUncoverCell = -1;
                } else {
                    Vector2 cellPos = renderer.getCellWorldPosition(pIdx, board);
                    Vector2 cellCenter = { cellPos.x + renderer.cellSize * 0.5f, cellPos.y + renderer.cellSize * 0.5f };
                    float dist = Vector2Distance(renderer.localShip.position, cellCenter);
                    if (dist <= renderer.localShip.range) {
                        pendingUncoverCell = -1;
                        renderer.clearOutOfReach();
                        renderer.fireLaser(renderer.localShip.getNosePosition(), cellCenter, renderer.getLaserColorForSkin(menu.cursorSkin));

                        if (net.role == net::NetRole::Client) {
                            net::PacketClick pc;
                            pc.index = pIdx;
                            pc.action = 0;
                            pc.flagSkin = 0;
                            net.sendToServer(&pc, sizeof(pc));
                        } else {
                            std::vector<size_t> newlyRevealed;
                            core::RevealResult res = board.reveal(pIdx, &newlyRevealed);
                            for (size_t revIdx : newlyRevealed) {
                                renderer.removeFlagDrop(revIdx);
                            }
                            Vector2 pos = renderer.getCellWorldPosition(pIdx, board);
                            if (res == core::RevealResult::HitBomb) {
                                renderer.emitExplosion(pos, ui::Colors::CellFlag);
                            } else {
                                renderer.emitDebris(pos, ui::Colors::Zinc400);
                                for (size_t cIdx : newlyRevealed) {
                                    if (!board.isBomb(cIdx) && render::ScrapSystem::isScrapCell(board.config.seed, cIdx, board.totalCells(), board.config.bombs)) {
                                        Vector2 cPos = renderer.getCellWorldPosition(cIdx, board);
                                        scrapSystem.spawn({ cPos.x + renderer.cellSize * 0.5f, cPos.y + renderer.cellSize * 0.5f });
                                    }
                                }
                            }

                            if (net.role == net::NetRole::Host) {
                                for (size_t revIdx : newlyRevealed) {
                                    net::PacketResult pr;
                                    pr.index = revIdx;
                                    pr.state = 0;
                                    pr.placerId = 0;
                                    pr.flagSkin = 0;
                                    net.broadcast(&pr, sizeof(pr));
                                }
                            }
                            saveCurrentSlot();
                        }
                    }
                }
            }

            // Right Mouse Button: Flag / Unflag
            if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
                Vector2 toMouse = { worldMouse.x - renderer.localShip.position.x, worldMouse.y - renderer.localShip.position.y };
                float distToMouse = std::sqrt(toMouse.x * toMouse.x + toMouse.y * toMouse.y);
                if (distToMouse > 0.001f) {
                    renderer.localShip.angle = std::atan2(toMouse.y, toMouse.x) * RAD2DEG + 90.0f;
                }
                renderer.fireLaser(renderer.localShip.getNosePosition(), worldMouse, ui::Colors::Red500);

                pendingUncoverCell = -1;
                renderer.clearOutOfReach();

                if (hovered >= 0) {
                    size_t hIdx = static_cast<size_t>(hovered);
                    if (net.role == net::NetRole::Client) {
                        net::PacketClick pc;
                        pc.index = hIdx;
                        pc.action = 2;
                        pc.flagSkin = static_cast<uint8_t>(menu.flagSkin);
                        net.sendToServer(&pc, sizeof(pc));
                    } else {
                        uint32_t myId = (net.role == net::NetRole::Host) ? net::HOST_PLAYER_ID : 0;
                        core::CellState cs = board.getState(hIdx);
                        Vector2 groundPos = renderer.getFlagBasePosition(hIdx, board);
                        Vector2 shipPos = renderer.localShip.position;
                        if (cs == core::CellState::Flagged) {
                            uint8_t skinId = board.getFlagSkin(hIdx, static_cast<uint8_t>(menu.flagSkin));
                            renderer.triggerFlagPickup(groundPos, shipPos, myId, true, skinId);
                            renderer.removeFlagDrop(hIdx);
                            board.unflag(hIdx);
                            if (net.role == net::NetRole::Host) {
                                net::PacketResult pr;
                                pr.index = hIdx;
                                pr.state = 1; // Hidden / unflagged
                                pr.placerId = myId;
                                pr.flagSkin = skinId;
                                net.broadcast(&pr, sizeof(pr));
                            }
                        } else if (cs == core::CellState::Hidden) {
                            board.setFlag(hIdx, myId, static_cast<uint8_t>(menu.flagSkin));
                            renderer.triggerFlagDrop(hIdx, groundPos, shipPos, static_cast<uint8_t>(menu.flagSkin));
                            if (net.role == net::NetRole::Host) {
                                net::PacketResult pr;
                                pr.index = hIdx;
                                pr.state = 2; // Flagged
                                pr.placerId = myId;
                                pr.flagSkin = static_cast<uint8_t>(menu.flagSkin);
                                net.broadcast(&pr, sizeof(pr));
                            }
                        }
                        saveCurrentSlot();
                    }
                }
            }

            // Chording: Key C or Middle Mouse Button click (ONLY if not dragging/panning!)
            bool triggerChord = IsKeyPressed(KEY_C);
            if (IsMouseButtonReleased(MOUSE_MIDDLE_BUTTON) && !renderer.camera.isMiddleDragging()) {
                triggerChord = true;
            }

            if (triggerChord) {
                Vector2 toMouse = { worldMouse.x - renderer.localShip.position.x, worldMouse.y - renderer.localShip.position.y };
                float distToMouse = std::sqrt(toMouse.x * toMouse.x + toMouse.y * toMouse.y);
                if (distToMouse > 0.001f) {
                    renderer.localShip.angle = std::atan2(toMouse.y, toMouse.x) * RAD2DEG + 90.0f;
                }
                renderer.fireLaser(renderer.localShip.getNosePosition(), worldMouse, renderer.getLaserColorForSkin(menu.cursorSkin));

                if (hovered >= 0) {
                    size_t hIdx = static_cast<size_t>(hovered);
                    if (board.getState(hIdx) == core::CellState::Revealed) {
                        Vector2 cellPos = renderer.getCellWorldPosition(hIdx, board);
                        Vector2 cellCenter = { cellPos.x + renderer.cellSize * 0.5f, cellPos.y + renderer.cellSize * 0.5f };
                        float dist = Vector2Distance(renderer.localShip.position, cellCenter);

                        if (dist > renderer.localShip.range) {
                            renderer.triggerOutOfReach(static_cast<int64_t>(hIdx), cellPos);
                        } else {
                            renderer.clearOutOfReach();
                            pendingUncoverCell = -1;
                            if (net.role == net::NetRole::Client) {
                                net::PacketClick pc;
                                pc.index = hIdx;
                                pc.action = 1; // Chord
                                pc.flagSkin = 0;
                                net.sendToServer(&pc, sizeof(pc));
                            } else {
                                std::vector<size_t> newlyRevealed;
                                bool hitBomb = false;
                                if (board.chord(hIdx, newlyRevealed, hitBomb)) {
                                    for (size_t revIdx : newlyRevealed) {
                                        renderer.removeFlagDrop(revIdx);
                                        Vector2 pos = renderer.getCellWorldPosition(revIdx, board);
                                        if (board.isBomb(revIdx)) {
                                            renderer.emitExplosion(pos, ui::Colors::CellFlag);
                                        } else {
                                            renderer.emitDebris(pos, ui::Colors::Zinc400);
                                            if (render::ScrapSystem::isScrapCell(board.config.seed, revIdx, board.totalCells(), board.config.bombs)) {
                                                scrapSystem.spawn({ pos.x + renderer.cellSize * 0.5f, pos.y + renderer.cellSize * 0.5f });
                                            }
                                        }

                                        if (net.role == net::NetRole::Host) {
                                            net::PacketResult pr;
                                            pr.index = revIdx;
                                            pr.state = 0;
                                            net.broadcast(&pr, sizeof(pr));
                                        }
                                    }
                                    saveCurrentSlot();
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
            for (size_t i = 0; i < board.totalCells(); ++i) {
                if (!board.isBomb(i)) {
                    board.reveal(i);
                }
            }
        }

        // Timer progression
        if (board.revealedCount > 0 && !board.isGameOver && !board.isVictory) {
            timePlayed += dt;
        }

        // Restart hotkey ('R')
        if ((board.isGameOver || board.isVictory) && IsKeyPressed(KEY_R)) {
            if (net.role != net::NetRole::Client) {
                restartCurrentGame();
            }
        }

        // Update Scrap System
        Vector2 hudScrapPos = hud.getScrapBadgeScreenPos();
        bool mouseClicked = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
        scrapSystem.update(dt, hudScrapPos, renderer.camera.camera, worldMouse, mouseClicked);

        int collected = scrapSystem.collectPending();
        if (collected > 0) {
            scrapCount += collected;
            hud.scrapCount = scrapCount;
            menu.scrapCount = scrapCount;
            hud.triggerScrapPulse();
            saveSettings();
            if (net.role != net::NetRole::Client) {
                saveCurrentSlot();
            }
        }

        // Periodic auto-save while in-game (every 3 seconds)
        static float autoSaveTimer = 0.0f;
        autoSaveTimer += dt;
        if (autoSaveTimer >= 3.0f) {
            autoSaveTimer = 0.0f;
            if (net.role != net::NetRole::Client) {
                saveCurrentSlot();
            }
        }
    }
}

void App::draw() {
    float scale = std::clamp(menu.guiScale, 0.75f, 1.50f);
    ui::Widgets::setScale(scale);
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

        if (menuAct.toggleCRT || menuAct.vsyncChanged || menuAct.fpsLimitChanged || menuAct.guiScaleChanged) {
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

        if (menuAct.playSolo) {
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
            int64_t hovered = renderer.getHoveredCellIndex(board);
            renderer.render(board, hovered, net);

            BeginMode2D(renderer.camera.camera);
            scrapSystem.drawWorld(renderer.camera.camera);
            EndMode2D();

            BeginMode2D(uiCam);
            ui::HUDActions hudAct = hud.drawAndProcess(uiW, uiH, board, timePlayed, net, voiceMgr.isTransmitting(), voiceMgr.getSettings().enabled, voiceMgr.getSettings().pushToTalk);
            scrapSystem.drawScreen(scale);
            EndMode2D();

            if (hudAct.returnToMenu) {
                int leftover = scrapSystem.collectAll();
                if (leftover > 0) {
                    scrapCount += leftover;
                    hud.scrapCount = scrapCount;
                    menu.scrapCount = scrapCount;
                    saveSettings();
                }
                saveCurrentSlot();
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
                    restartCurrentGame();
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
}

void App::run() {
    init();

    while (!WindowShouldClose() && !shouldQuit) {
        handleNetEvents();
        update(GetFrameTime());
        draw();
    }
}

} // namespace minesweeper
