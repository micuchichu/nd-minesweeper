#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace minesweeper::net {

struct SteamP2PPacket {
    uint64_t senderSteamID = 0;
    std::vector<uint8_t> data;
};

class SteamManager {
public:
    static SteamManager& instance();

    bool init();
    void update();
    void shutdown();

    bool isSteamActive() const;
    uint64_t getLocalSteamID() const;
    const char* getPersonaName() const;

    // Lobbies & Matchmaking
    bool createLobby(bool friendsOnly = true);
    void joinLobby(uint64_t lobbyId);
    void leaveLobby();
    bool isInLobby() const;
    uint64_t getLobbyID() const;
    uint64_t getLobbyOwner() const;

    // Steam Overlay
    void openInviteOverlay();
    void openFriendsOverlay();

    // Rich Presence
    void setRichPresence(const char* key, const char* value);
    void clearRichPresence();

    // Steam P2P Networking
    bool sendP2PPacket(uint64_t targetSteamID, const void* data, uint32_t size, bool reliable = true);
    bool broadcastP2PPacket(const void* data, uint32_t size, bool reliable = true);
    bool pollP2PPacket(SteamP2PPacket& outPacket);

    // Callbacks
    using LobbyJoinRequestedCallback = std::function<void(uint64_t lobbyId)>;
    using LobbyEnteredCallback = std::function<void(uint64_t lobbyId, bool success)>;
    using UserDisconnectedCallback = std::function<void(uint64_t userSteamID)>;

    void setLobbyJoinRequestedCallback(LobbyJoinRequestedCallback cb);
    void setLobbyEnteredCallback(LobbyEnteredCallback cb);
    void setUserDisconnectedCallback(UserDisconnectedCallback cb);

    // Connected Steam Peers in active session
    const std::vector<uint64_t>& getConnectedPeers() const;

private:
    SteamManager();
    ~SteamManager();

    struct Impl;
    std::unique_ptr<Impl> pimpl;
};

} // namespace minesweeper::net
