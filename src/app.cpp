#include "app.hpp"
#include "net/steam_manager.hpp"
#include "ui/widgets.hpp"
#include <fstream>
#include <iostream>
#include <cmath>
#include <cstring>
#include <algorithm>

namespace minesweeper {

struct SaveData {
    char joinIp[64] = "127.0.0.1:7777";
    char hostPort[16] = "7777";
    char playerName[16] = "Player";
    int dim = 2;
    int size = 10;
    int bombs = 10;
    uint64_t seed = 12345;
    bool crtEnabled = true;
    uint8_t cursorSkin = 0;
    bool randomizeSeed = true;
    uint8_t flagSkin = 0;
    uint8_t playerSkin = 0;
    bool voiceEnabled = true;
    bool voiceProximity = true;
    bool voicePushToTalk = true;
    float voiceVolume = 1.0f;
    float micGain = 1.0f;
    bool vsyncEnabled = true;
    bool showFPS = false;
    float fpsLimit = 144.0f;
    float guiScale = 1.0f;
    uint64_t scrapCount = 0;
};

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

    hud.init(board.config);
    startNewGame(board.config.dim, board.config.size, board.config.bombs, board.config.seed);

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
    saveSettings();
    scrapSystem.cleanup();
    voiceMgr.cleanup();
    renderer.cleanup();
    net.cleanup();
    CloseWindow();
}

void App::loadSettings() {
    std::ifstream file("savegame.dat", std::ios::binary);
    if (file.is_open()) {
        SaveData data;
        file.read(reinterpret_cast<char*>(&data), sizeof(SaveData));
        std::streamsize bytesRead = file.gcount();
        if (bytesRead >= static_cast<std::streamsize>(sizeof(data.joinIp) + sizeof(data.hostPort) + sizeof(data.playerName) + sizeof(int) * 3 + sizeof(uint64_t))) {
            std::strncpy(menu.joinIpBuf, data.joinIp, sizeof(menu.joinIpBuf));
            std::strncpy(menu.hostPortBuf, data.hostPort, sizeof(menu.hostPortBuf));
            std::strncpy(menu.playerName, data.playerName, sizeof(menu.playerName));
            std::strncpy(net.playerName, data.playerName, sizeof(net.playerName));

            board.config.dim = std::clamp(data.dim, 2, 4);
            board.config.size = std::clamp(data.size, 4, 1000);
            board.config.bombs = std::max(1, data.bombs);
            board.config.seed = (data.seed != 0) ? data.seed : 12345;
            hud.nextSeed = board.config.seed;
            std::snprintf(hud.seedBuf, sizeof(hud.seedBuf), "%llu", hud.nextSeed);
            menu.crtEnabled = data.crtEnabled;
            menu.cursorSkin = std::max(0, static_cast<int>(data.cursorSkin));
            if (bytesRead >= static_cast<std::streamsize>(sizeof(SaveData) - sizeof(uint8_t) * 2 - (sizeof(bool) * 5 + sizeof(float) * 4))) {
                hud.randomizeSeed = data.randomizeSeed;
            }
            if (bytesRead >= static_cast<std::streamsize>(sizeof(SaveData) - sizeof(uint8_t) - (sizeof(bool) * 5 + sizeof(float) * 4))) {
                menu.flagSkin = std::max(0, static_cast<int>(data.flagSkin));
            }
            if (bytesRead >= static_cast<std::streamsize>(sizeof(SaveData) - (sizeof(bool) * 5 + sizeof(float) * 4))) {
                menu.playerSkin = std::max(0, static_cast<int>(data.playerSkin));
            }
            if (bytesRead >= static_cast<std::streamsize>(sizeof(SaveData) - (sizeof(bool) * 2 + sizeof(float) * 2))) {
                menu.voiceSettings.enabled = data.voiceEnabled;
                menu.voiceSettings.proximity = data.voiceProximity;
                menu.voiceSettings.pushToTalk = data.voicePushToTalk;
                menu.voiceSettings.voiceVolume = data.voiceVolume;
                menu.voiceSettings.micGain = data.micGain;
            }
            if (bytesRead >= static_cast<std::streamsize>(sizeof(SaveData) - sizeof(uint64_t))) {
                menu.vsyncEnabled = data.vsyncEnabled;
                menu.showFPS = data.showFPS;
                menu.fpsLimit = data.fpsLimit;
                menu.guiScale = std::clamp(data.guiScale, 0.75f, 1.50f);
            }
            if (bytesRead >= static_cast<std::streamsize>(sizeof(SaveData))) {
                scrapCount = data.scrapCount;
                menu.scrapCount = scrapCount;
                hud.scrapCount = scrapCount;
            }
        }
        file.close();
    }
}

void App::saveSettings() {
    std::ofstream file("savegame.dat", std::ios::binary);
    if (file.is_open()) {
        SaveData data;
        std::strncpy(data.joinIp, menu.joinIpBuf, sizeof(data.joinIp));
        std::strncpy(data.hostPort, menu.hostPortBuf, sizeof(data.hostPort));
        std::strncpy(data.playerName, menu.playerName, sizeof(data.playerName));
        data.dim = board.config.dim;
        data.size = board.config.size;
        data.bombs = board.config.bombs;
        data.seed = board.config.seed;
        data.crtEnabled = menu.crtEnabled;
        data.cursorSkin = static_cast<uint8_t>(menu.cursorSkin);
        data.randomizeSeed = hud.randomizeSeed;
        data.flagSkin = static_cast<uint8_t>(menu.flagSkin);
        data.playerSkin = static_cast<uint8_t>(menu.playerSkin);
        data.voiceEnabled = menu.voiceSettings.enabled;
        data.voiceProximity = menu.voiceSettings.proximity;
        data.voicePushToTalk = menu.voiceSettings.pushToTalk;
        data.voiceVolume = menu.voiceSettings.voiceVolume;
        data.micGain = menu.voiceSettings.micGain;
        data.vsyncEnabled = menu.vsyncEnabled;
        data.showFPS = menu.showFPS;
        data.fpsLimit = menu.fpsLimit;
        data.guiScale = menu.guiScale;
        data.scrapCount = scrapCount;

        file.write(reinterpret_cast<const char*>(&data), sizeof(SaveData));
        file.close();
    }
}

void App::startNewGame(int dim, int size, int bombs, uint64_t seed) {
    renderer.clearParticles();
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

    // Center camera on board
    float boardWidth = size * renderer.cellSize;
    float sliceStride = boardWidth + renderer.slicePadding;
    Vector2 center = { boardWidth * 0.5f, boardWidth * 0.5f };

    if (dim == 3) {
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
                            std::vector<size_t> newlyRevealed;
                            core::RevealResult res = board.reveal(idx, &newlyRevealed);
                            Vector2 pos = renderer.getCellWorldPosition(idx, board);
                            if (res == core::RevealResult::HitBomb) {
                                renderer.emitExplosion(pos, ui::Colors::CellFlag);
                            } else {
                                renderer.emitDebris(pos, ui::Colors::Zinc400);
                                for (size_t cIdx : newlyRevealed) {
                                    if (!board.isBomb(cIdx) && render::ScrapSystem::isScrapCell(board.config.seed, cIdx)) {
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
                                        if (render::ScrapSystem::isScrapCell(board.config.seed, revIdx)) {
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
                        if (cs == core::CellState::Flagged) {
                            board.unflag(idx);
                            net::PacketResult pr;
                            pr.index = idx;
                            pr.state = 1; // Hidden / unflagged
                            pr.placerId = ev.peerId;
                            pr.flagSkin = 0;
                            net.broadcast(&pr, sizeof(pr));
                        } else if (cs == core::CellState::Hidden) {
                            board.setFlag(idx, ev.peerId, ev.clickData.flagSkin);
                            net::PacketResult pr;
                            pr.index = idx;
                            pr.state = 2; // Flagged
                            pr.placerId = ev.peerId;
                            pr.flagSkin = ev.clickData.flagSkin;
                            net.broadcast(&pr, sizeof(pr));
                        }
                    }
                }
                break;
            }
            case net::NetEventType::BoardResult: {
                size_t idx = ev.resultData.index;
                if (ev.resultData.state == 0) {
                    if (board.getState(idx) == core::CellState::Flagged) {
                        board.unflag(idx);
                    }
                    std::vector<size_t> newlyRevealed;
                    core::RevealResult res = board.reveal(idx, &newlyRevealed);
                    Vector2 pos = renderer.getCellWorldPosition(idx, board);
                    if (res == core::RevealResult::HitBomb) {
                        renderer.emitExplosion(pos, ui::Colors::CellFlag);
                    } else {
                        renderer.emitDebris(pos, ui::Colors::Zinc400);
                        for (size_t cIdx : newlyRevealed) {
                            if (!board.isBomb(cIdx) && render::ScrapSystem::isScrapCell(board.config.seed, cIdx)) {
                                Vector2 cPos = renderer.getCellWorldPosition(cIdx, board);
                                scrapSystem.spawn({ cPos.x + renderer.cellSize * 0.5f, cPos.y + renderer.cellSize * 0.5f });
                            }
                        }
                    }
                    board.flagOwners.erase(idx);
                } else if (ev.resultData.state == 1) { // Unflagged
                    board.unflag(idx);
                } else if (ev.resultData.state == 2) { // Flagged
                    board.setFlag(idx, ev.resultData.placerId, ev.resultData.flagSkin);
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
    renderer.activePlayerSkin = menu.playerSkin;
    renderer.update(dt);

    // Synchronize voice settings and live mic meter with menu
    voiceMgr.getSettings() = menu.voiceSettings;
    menu.micInputLevel = voiceMgr.getMicLevel();

    Vector2 worldMouse = renderer.camera.getScreenToWorld(renderer.camera.getCRTMousePosition());
    voiceMgr.setLocalCursorPos(worldMouse.x, worldMouse.y);
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

        // Multiplayer Cursor Broadcast
        if (net.role != net::NetRole::Offline) {
            static Vector2 lastSent = { -9999.0f, -9999.0f };
            if (Vector2Distance(worldMouse, lastSent) > 3.0f) {
                lastSent = worldMouse;
                net::PacketCursor pc;
                pc.playerID = (net.role == net::NetRole::Host) ? net::HOST_PLAYER_ID : 0;
                pc.x = worldMouse.x;
                pc.y = worldMouse.y;
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
            // Left Mouse Button: Reveal
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hovered >= 0) {
                size_t hIdx = static_cast<size_t>(hovered);
                if (net.role == net::NetRole::Client) {
                    net::PacketClick pc;
                    pc.index = hIdx;
                    pc.action = 0;
                    pc.flagSkin = 0;
                    net.sendToServer(&pc, sizeof(pc));
                } else {
                    if (board.getState(hIdx) == core::CellState::Hidden) {
                        std::vector<size_t> newlyRevealed;
                        core::RevealResult res = board.reveal(hIdx, &newlyRevealed);
                        Vector2 pos = renderer.getCellWorldPosition(hIdx, board);
                        if (res == core::RevealResult::HitBomb) {
                            renderer.emitExplosion(pos, ui::Colors::CellFlag);
                        } else {
                            renderer.emitDebris(pos, ui::Colors::Zinc400);
                            for (size_t cIdx : newlyRevealed) {
                                if (!board.isBomb(cIdx) && render::ScrapSystem::isScrapCell(board.config.seed, cIdx)) {
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
                    }
                }
            }

            // Right Mouse Button: Flag / Unflag
            if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && hovered >= 0) {
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
                    if (cs == core::CellState::Flagged) {
                        board.unflag(hIdx);
                        if (net.role == net::NetRole::Host) {
                            net::PacketResult pr;
                            pr.index = hIdx;
                            pr.state = 1; // Hidden / unflagged
                            pr.placerId = myId;
                            pr.flagSkin = 0;
                            net.broadcast(&pr, sizeof(pr));
                        }
                    } else if (cs == core::CellState::Hidden) {
                        board.setFlag(hIdx, myId, static_cast<uint8_t>(menu.flagSkin));
                        if (net.role == net::NetRole::Host) {
                            net::PacketResult pr;
                            pr.index = hIdx;
                            pr.state = 2; // Flagged
                            pr.placerId = myId;
                            pr.flagSkin = static_cast<uint8_t>(menu.flagSkin);
                            net.broadcast(&pr, sizeof(pr));
                        }
                    }
                }
            }

            // Chording: Key C or Middle Mouse Button click (ONLY if not dragging/panning!)
            bool triggerChord = IsKeyPressed(KEY_C);
            if (IsMouseButtonReleased(MOUSE_MIDDLE_BUTTON) && !renderer.camera.isMiddleDragging()) {
                triggerChord = true;
            }

            if (triggerChord && hovered >= 0) {
                size_t hIdx = static_cast<size_t>(hovered);
                if (board.getState(hIdx) == core::CellState::Revealed) {
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
                                Vector2 pos = renderer.getCellWorldPosition(revIdx, board);
                                if (board.isBomb(revIdx)) {
                                    renderer.emitExplosion(pos, ui::Colors::CellFlag);
                                } else {
                                    renderer.emitDebris(pos, ui::Colors::Zinc400);
                                    if (render::ScrapSystem::isScrapCell(board.config.seed, revIdx)) {
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
            state = AppState::InGame;
        }
        else if (menuAct.hostGame) {
            if (net.startHost(menuAct.hostPort)) {
                state = AppState::InGame;
                hud.init(board.config);
                startNewGame(board.config.dim, board.config.size, board.config.bombs, board.config.seed);
            } else {
                menu.statusMessage = "FAILED TO BIND PORT";
            }
        }
        else if (menuAct.joinGame) {
            if (net.connectToHost(menuAct.joinAddress)) {
                board.init(2, 0, 0, 0);
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
