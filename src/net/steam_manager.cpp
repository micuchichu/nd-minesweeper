#include "steam_manager.hpp"
#include "protocol.hpp"
#include <steam/steam_api.h>
#include <iostream>
#include <fstream>
#include <algorithm>

namespace minesweeper::net {

struct SteamManager::Impl {
    bool active = false;
    CSteamID localID;
    CSteamID currentLobbyID;
    std::vector<uint64_t> connectedPeers;

    LobbyJoinRequestedCallback onJoinRequested;
    LobbyEnteredCallback onLobbyEntered;
    UserDisconnectedCallback onUserDisconnected;

    CCallResult<Impl, LobbyCreated_t> callResultLobbyCreated;
    CCallResult<Impl, LobbyEnter_t> callResultLobbyEnter;

    void OnLobbyCreated(LobbyCreated_t* pCallback, bool bIOFailure);
    void OnLobbyEntered(LobbyEnter_t* pCallback, bool bIOFailure);

    CCallback<Impl, GameLobbyJoinRequested_t> callbackGameLobbyJoinRequested;
    CCallback<Impl, GameRichPresenceJoinRequested_t> callbackGameRichPresenceJoinRequested;
    CCallback<Impl, P2PSessionRequest_t> callbackP2PSessionRequest;
    CCallback<Impl, P2PSessionConnectFail_t> callbackP2PSessionConnectFail;
    CCallback<Impl, LobbyChatUpdate_t> callbackLobbyChatUpdate;

    void OnGameLobbyJoinRequested(GameLobbyJoinRequested_t* pCallback);
    void OnGameRichPresenceJoinRequested(GameRichPresenceJoinRequested_t* pCallback);
    void OnP2PSessionRequest(P2PSessionRequest_t* pCallback);
    void OnP2PSessionConnectFail(P2PSessionConnectFail_t* pCallback);
    void OnLobbyChatUpdate(LobbyChatUpdate_t* pCallback);

    Impl()
        : callbackGameLobbyJoinRequested(this, &Impl::OnGameLobbyJoinRequested)
        , callbackGameRichPresenceJoinRequested(this, &Impl::OnGameRichPresenceJoinRequested)
        , callbackP2PSessionRequest(this, &Impl::OnP2PSessionRequest)
        , callbackP2PSessionConnectFail(this, &Impl::OnP2PSessionConnectFail)
        , callbackLobbyChatUpdate(this, &Impl::OnLobbyChatUpdate)
    {}
};

void SteamManager::Impl::OnLobbyCreated(LobbyCreated_t* pCallback, bool bIOFailure) {
    if (bIOFailure || pCallback->m_eResult != k_EResultOK) {
        std::cerr << "[STEAM] Failed to create lobby, result: " << pCallback->m_eResult << std::endl;
        return;
    }

    currentLobbyID = pCallback->m_ulSteamIDLobby;
    std::cout << "[STEAM] Lobby created successfully: " << currentLobbyID.ConvertToUint64() << std::endl;

    const char* persona = SteamFriends()->GetPersonaName();
    SteamMatchmaking()->SetLobbyData(currentLobbyID, "name", persona);
    SteamMatchmaking()->SetLobbyData(currentLobbyID, "game", "DimensionSweeper");

    std::string lobbyStr = std::to_string(currentLobbyID.ConvertToUint64());
    SteamFriends()->SetRichPresence("connect", lobbyStr.c_str());
    SteamFriends()->SetRichPresence("status", "Hosting Dimension Sweeper");
    SteamFriends()->SetRichPresence("steam_display", "#Status_Playing");

    if (onLobbyEntered) {
        onLobbyEntered(currentLobbyID.ConvertToUint64(), true);
    }
}

void SteamManager::Impl::OnLobbyEntered(LobbyEnter_t* pCallback, bool bIOFailure) {
    if (bIOFailure || pCallback->m_EChatRoomEnterResponse != k_EChatRoomEnterResponseSuccess) {
        std::cerr << "[STEAM] Failed to enter lobby, response: " << pCallback->m_EChatRoomEnterResponse << std::endl;
        if (onLobbyEntered) {
            onLobbyEntered(0, false);
        }
        return;
    }

    currentLobbyID = pCallback->m_ulSteamIDLobby;
    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(currentLobbyID);
    std::cout << "[STEAM] Entered lobby: " << currentLobbyID.ConvertToUint64() << " owned by host: " << hostID.ConvertToUint64() << std::endl;

    if (hostID != localID) {
        connectedPeers.clear();
        connectedPeers.push_back(hostID.ConvertToUint64());

        // Send handshake packet so host accepts P2P session and sends board sync
        PacketHandshake handshake;
        handshake.type = PacketType::Handshake;
        const char* myName = SteamFriends()->GetPersonaName();
        if (myName) {
            std::strncpy(handshake.name, myName, sizeof(handshake.name) - 1);
            handshake.name[sizeof(handshake.name) - 1] = '\0';
        }
        SteamNetworking()->SendP2PPacket(hostID, &handshake, sizeof(handshake), k_EP2PSendReliable);
        std::cout << "[STEAM] Sent PacketHandshake to host: " << hostID.ConvertToUint64() << std::endl;
    }

    if (onLobbyEntered) {
        onLobbyEntered(currentLobbyID.ConvertToUint64(), true);
    }
}

void SteamManager::Impl::OnGameLobbyJoinRequested(GameLobbyJoinRequested_t* pCallback) {
    std::cout << "[STEAM] Overlay Join Requested for lobby: " << pCallback->m_steamIDLobby.ConvertToUint64() << std::endl;
    if (onJoinRequested) {
        onJoinRequested(pCallback->m_steamIDLobby.ConvertToUint64());
    }
}

void SteamManager::Impl::OnGameRichPresenceJoinRequested(GameRichPresenceJoinRequested_t* pCallback) {
    std::cout << "[STEAM] Rich Presence Join Requested: " << pCallback->m_rgchConnect << std::endl;
    try {
        uint64_t lobbyId = std::stoull(pCallback->m_rgchConnect);
        if (onJoinRequested) {
            onJoinRequested(lobbyId);
        }
    } catch (...) {}
}

void SteamManager::Impl::OnP2PSessionRequest(P2PSessionRequest_t* pCallback) {
    std::cout << "[STEAM] P2P Session requested from: " << pCallback->m_steamIDRemote.ConvertToUint64() << std::endl;
    SteamNetworking()->AcceptP2PSessionWithUser(pCallback->m_steamIDRemote);

    uint64_t remoteId = pCallback->m_steamIDRemote.ConvertToUint64();
    auto it = std::find(connectedPeers.begin(), connectedPeers.end(), remoteId);
    if (it == connectedPeers.end()) {
        connectedPeers.push_back(remoteId);
    }
}

void SteamManager::Impl::OnP2PSessionConnectFail(P2PSessionConnectFail_t* pCallback) {
    uint64_t remoteId = pCallback->m_steamIDRemote.ConvertToUint64();
    std::cerr << "[STEAM] P2P Session failed with: " << remoteId << " error: " << static_cast<int>(pCallback->m_eP2PSessionError) << std::endl;

    auto it = std::find(connectedPeers.begin(), connectedPeers.end(), remoteId);
    if (it != connectedPeers.end()) {
        connectedPeers.erase(it);
    }
    if (onUserDisconnected) {
        onUserDisconnected(remoteId);
    }
}

void SteamManager::Impl::OnLobbyChatUpdate(LobbyChatUpdate_t* pCallback) {
    uint64_t changedUser = pCallback->m_ulSteamIDUserChanged;
    uint32_t flags = pCallback->m_rgfChatMemberStateChange;

    if (flags & k_EChatMemberStateChangeEntered) {
        if (changedUser != localID.ConvertToUint64() && changedUser != 0) {
            std::cout << "[STEAM] User entered lobby: " << changedUser << std::endl;
            auto it = std::find(connectedPeers.begin(), connectedPeers.end(), changedUser);
            if (it == connectedPeers.end()) {
                connectedPeers.push_back(changedUser);
            }
        }
    }

    if (flags & (k_EChatMemberStateChangeLeft | k_EChatMemberStateChangeDisconnected | k_EChatMemberStateChangeKicked | k_EChatMemberStateChangeBanned)) {
        std::cout << "[STEAM] User left lobby: " << changedUser << std::endl;
        auto it = std::find(connectedPeers.begin(), connectedPeers.end(), changedUser);
        if (it != connectedPeers.end()) {
            connectedPeers.erase(it);
        }
        if (onUserDisconnected) {
            onUserDisconnected(changedUser);
        }
    }
}

SteamManager& SteamManager::instance() {
    static SteamManager s_instance;
    return s_instance;
}

SteamManager::SteamManager() : pimpl(std::make_unique<Impl>()) {}

SteamManager::~SteamManager() {
    shutdown();
}

bool SteamManager::init() {
    // Ensure steam_appid.txt exists with 480
    std::ifstream checkFile("steam_appid.txt");
    if (!checkFile.good()) {
        checkFile.close();
        std::ofstream outFile("steam_appid.txt");
        if (outFile.is_open()) {
            outFile << "480";
            outFile.close();
        }
    } else {
        checkFile.close();
    }

    if (SteamAPI_RestartAppIfNecessary(480)) {
        // App is being relaunched through Steam
        std::cout << "[STEAM] Restarting app through Steam for App ID 480..." << std::endl;
    }

    if (!SteamAPI_Init()) {
        std::cerr << "[STEAM] SteamAPI_Init() failed. Running in offline/standard IP mode." << std::endl;
        pimpl->active = false;
        return false;
    }

    pimpl->active = true;
    pimpl->localID = SteamUser()->GetSteamID();
    std::cout << "[STEAM] Initialized successfully! User: " << SteamFriends()->GetPersonaName() << " (" << pimpl->localID.ConvertToUint64() << ")" << std::endl;

    return true;
}

void SteamManager::update() {
    if (!pimpl->active) return;
    SteamAPI_RunCallbacks();
}

void SteamManager::shutdown() {
    if (!pimpl->active) return;
    leaveLobby();
    clearRichPresence();
    SteamAPI_Shutdown();
    pimpl->active = false;
    pimpl->connectedPeers.clear();
}

bool SteamManager::isSteamActive() const {
    return pimpl->active;
}

uint64_t SteamManager::getLocalSteamID() const {
    return pimpl->active ? pimpl->localID.ConvertToUint64() : 0;
}

const char* SteamManager::getPersonaName() const {
    if (pimpl->active && SteamFriends()) {
        return SteamFriends()->GetPersonaName();
    }
    return "Player";
}

bool SteamManager::createLobby(bool friendsOnly) {
    if (!pimpl->active) return false;
    leaveLobby();

    ELobbyType lobbyType = friendsOnly ? k_ELobbyTypeFriendsOnly : k_ELobbyTypePublic;
    SteamAPICall_t call = SteamMatchmaking()->CreateLobby(lobbyType, 16);
    pimpl->callResultLobbyCreated.Set(call, pimpl.get(), &Impl::OnLobbyCreated);
    return true;
}

void SteamManager::joinLobby(uint64_t lobbyId) {
    if (!pimpl->active || lobbyId == 0) return;
    leaveLobby();

    std::cout << "[STEAM] Joining lobby: " << lobbyId << std::endl;
    SteamAPICall_t call = SteamMatchmaking()->JoinLobby(CSteamID(lobbyId));
    pimpl->callResultLobbyEnter.Set(call, pimpl.get(), &Impl::OnLobbyEntered);
}

void SteamManager::leaveLobby() {
    if (!pimpl->active || pimpl->currentLobbyID.ConvertToUint64() == 0) return;

    for (uint64_t peer : pimpl->connectedPeers) {
        SteamNetworking()->CloseP2PSessionWithUser(CSteamID(peer));
    }
    pimpl->connectedPeers.clear();

    SteamMatchmaking()->LeaveLobby(pimpl->currentLobbyID);
    pimpl->currentLobbyID = k_steamIDNil;
    clearRichPresence();
}

bool SteamManager::isInLobby() const {
    return pimpl->active && pimpl->currentLobbyID.ConvertToUint64() != 0;
}

uint64_t SteamManager::getLobbyID() const {
    return pimpl->currentLobbyID.ConvertToUint64();
}

uint64_t SteamManager::getLobbyOwner() const {
    if (!isInLobby()) return 0;
    return SteamMatchmaking()->GetLobbyOwner(pimpl->currentLobbyID).ConvertToUint64();
}

void SteamManager::openInviteOverlay() {
    if (!pimpl->active) return;
    if (isInLobby()) {
        SteamFriends()->ActivateGameOverlayInviteDialog(pimpl->currentLobbyID);
    } else {
        SteamFriends()->ActivateGameOverlay("Friends");
    }
}

void SteamManager::openFriendsOverlay() {
    if (!pimpl->active) return;
    SteamFriends()->ActivateGameOverlay("Friends");
}

void SteamManager::setRichPresence(const char* key, const char* value) {
    if (!pimpl->active || !key) return;
    SteamFriends()->SetRichPresence(key, value);
}

void SteamManager::clearRichPresence() {
    if (!pimpl->active) return;
    SteamFriends()->ClearRichPresence();
}

bool SteamManager::sendP2PPacket(uint64_t targetSteamID, const void* data, uint32_t size, bool reliable) {
    if (!pimpl->active || targetSteamID == 0 || targetSteamID == pimpl->localID.ConvertToUint64() || !data || size == 0) return false;
    EP2PSend sendType = reliable ? k_EP2PSendReliable : k_EP2PSendUnreliable;
    return SteamNetworking()->SendP2PPacket(CSteamID(targetSteamID), data, size, sendType);
}

bool SteamManager::broadcastP2PPacket(const void* data, uint32_t size, bool reliable) {
    if (!pimpl->active || !data || size == 0) return false;
    bool anySent = false;
    for (uint64_t peer : pimpl->connectedPeers) {
        if (sendP2PPacket(peer, data, size, reliable)) {
            anySent = true;
        }
    }
    return anySent;
}

bool SteamManager::pollP2PPacket(SteamP2PPacket& outPacket) {
    if (!pimpl->active) return false;

    uint32_t msgSize = 0;
    while (SteamNetworking()->IsP2PPacketAvailable(&msgSize)) {
        outPacket.data.resize(msgSize);
        CSteamID remoteSteamID;
        if (SteamNetworking()->ReadP2PPacket(outPacket.data.data(), msgSize, &msgSize, &remoteSteamID)) {
            outPacket.data.resize(msgSize);
            outPacket.senderSteamID = remoteSteamID.ConvertToUint64();

            auto it = std::find(pimpl->connectedPeers.begin(), pimpl->connectedPeers.end(), outPacket.senderSteamID);
            if (it == pimpl->connectedPeers.end()) {
                pimpl->connectedPeers.push_back(outPacket.senderSteamID);
            }
            return true;
        }
    }
    return false;
}

void SteamManager::setLobbyJoinRequestedCallback(LobbyJoinRequestedCallback cb) {
    pimpl->onJoinRequested = std::move(cb);
}

void SteamManager::setLobbyEnteredCallback(LobbyEnteredCallback cb) {
    pimpl->onLobbyEntered = std::move(cb);
}

void SteamManager::setUserDisconnectedCallback(UserDisconnectedCallback cb) {
    pimpl->onUserDisconnected = std::move(cb);
}

const std::vector<uint64_t>& SteamManager::getConnectedPeers() const {
    return pimpl->connectedPeers;
}

} // namespace minesweeper::net
