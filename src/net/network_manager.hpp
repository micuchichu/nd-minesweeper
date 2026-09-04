#pragma once

#include "protocol.hpp"
#include <deque>
#include <map>
#include <memory>
#include <string>

namespace minesweeper::net {

class NetworkManager {
public:
    NetRole role = NetRole::Offline;
    char playerName[16] = "Player";
    std::map<uint32_t, RemoteCursor> remoteCursors;

    NetworkManager();
    ~NetworkManager();

    bool isSteamMode = false;

    bool initialize();
    void cleanup();

    bool startHost(uint16_t port);
    bool startSteamHost(bool friendsOnly = true);
    bool connectToHost(const std::string& addressAndPort);
    bool connectToSteamLobby(uint64_t lobbyId);
    void disconnect();

    void update();
    bool pollEvent(NetEvent& outEvent);

    void sendToServer(const void* data, size_t size, bool reliable = true);
    void sendToPeer(uint32_t peerId, const void* data, size_t size, bool reliable = true);
    void broadcast(const void* data, size_t size, bool reliable = true);

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};

} // namespace minesweeper::net
