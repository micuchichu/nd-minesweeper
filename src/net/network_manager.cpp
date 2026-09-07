#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#endif

#include "network_manager.hpp"
#include "steam_manager.hpp"
#include <enet/enet.h>
#include <iostream>
#include <vector>
#include <deque>
#include <cstring>
#include <algorithm>
#include <set>

namespace minesweeper::net {

struct NetworkManager::Impl {
    ENetHost* host = nullptr;
    ENetPeer* serverPeer = nullptr;
    std::vector<ENetPeer*> connectedClients;
    std::deque<NetEvent> eventQueue;

    std::map<uint64_t, uint32_t> steamToPeerId;
    std::map<uint32_t, uint64_t> peerIdToSteam;
    std::set<uint64_t> knownSteamClients;
    uint32_t nextSteamPeerId = 1;

    uint32_t getOrCreatePeerId(uint64_t steamId) {
        auto it = steamToPeerId.find(steamId);
        if (it != steamToPeerId.end()) return it->second;
        uint32_t id = nextSteamPeerId++;
        steamToPeerId[steamId] = id;
        peerIdToSteam[id] = steamId;
        return id;
    }

    void cleanupHost() {
        if (serverPeer) {
            enet_peer_disconnect_now(serverPeer, 0);
            serverPeer = nullptr;
        }
        for (ENetPeer* peer : connectedClients) {
            enet_peer_disconnect_now(peer, 0);
        }
        connectedClients.clear();

        if (host) {
            enet_host_destroy(host);
            host = nullptr;
        }
        eventQueue.clear();
        steamToPeerId.clear();
        peerIdToSteam.clear();
        knownSteamClients.clear();
        nextSteamPeerId = 1;
    }
};

NetworkManager::NetworkManager() : pimpl(std::make_unique<Impl>()) {}

NetworkManager::~NetworkManager() {
    cleanup();
}

bool NetworkManager::initialize() {
    bool enetOk = enet_initialize() == 0;
    SteamManager::instance().init();

    SteamManager::instance().setUserDisconnectedCallback([this](uint64_t userSteamId) {
        auto it = pimpl->steamToPeerId.find(userSteamId);
        if (it != pimpl->steamToPeerId.end()) {
            uint32_t peerId = it->second;
            remoteCursors.erase(peerId);
            pimpl->knownSteamClients.erase(userSteamId);

            PacketDisconnect dc;
            dc.playerID = peerId;
            broadcast(&dc, sizeof(dc), true);

            NetEvent ne{};
            ne.type = NetEventType::ClientDisconnected;
            ne.peerId = peerId;
            pimpl->eventQueue.push_back(ne);
        }
    });

    return enetOk;
}

void NetworkManager::cleanup() {
    disconnect();
    SteamManager::instance().shutdown();
    enet_deinitialize();
}

bool NetworkManager::startHost(uint16_t port) {
    disconnect();

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;

    pimpl->host = enet_host_create(&address, 32, 2, 0, 0);
    if (!pimpl->host) {
        return false;
    }

    role = NetRole::Host;
    isSteamMode = false;
    remoteCursors.clear();

    if (SteamManager::instance().isSteamActive()) {
        SteamManager::instance().createLobby(true);
    }
    return true;
}

bool NetworkManager::startSteamHost(bool friendsOnly) {
    disconnect();

    if (!SteamManager::instance().isSteamActive()) {
        std::cerr << "[STEAM] Cannot start Steam host: Steam is not active." << std::endl;
        return false;
    }

    isSteamMode = true;
    role = NetRole::Host;
    remoteCursors.clear();
    pimpl->steamToPeerId.clear();
    pimpl->peerIdToSteam.clear();
    pimpl->knownSteamClients.clear();
    pimpl->nextSteamPeerId = 1;

    return SteamManager::instance().createLobby(friendsOnly);
}

bool NetworkManager::connectToHost(const std::string& addressAndPort) {
    disconnect();

    pimpl->host = enet_host_create(nullptr, 1, 2, 0, 0);
    if (!pimpl->host) {
        return false;
    }

    std::string ip = "127.0.0.1";
    uint16_t port = 7777;

    size_t colon = addressAndPort.find(':');
    if (colon != std::string::npos) {
        ip = addressAndPort.substr(0, colon);
        try {
            port = static_cast<uint16_t>(std::stoi(addressAndPort.substr(colon + 1)));
        } catch (...) {
            port = 7777;
        }
    } else if (!addressAndPort.empty()) {
        ip = addressAndPort;
    }

    ENetAddress address;
    enet_address_set_host(&address, ip.c_str());
    address.port = port;

    pimpl->serverPeer = enet_host_connect(pimpl->host, &address, 2, 0);
    if (!pimpl->serverPeer) {
        enet_host_destroy(pimpl->host);
        pimpl->host = nullptr;
        return false;
    }

    role = NetRole::Client;
    isSteamMode = false;
    remoteCursors.clear();
    return true;
}

bool NetworkManager::connectToSteamLobby(uint64_t lobbyId) {
    disconnect();

    if (!SteamManager::instance().isSteamActive()) {
        std::cerr << "[STEAM] Cannot join Steam lobby: Steam is not active." << std::endl;
        return false;
    }

    isSteamMode = true;
    role = NetRole::Client;
    remoteCursors.clear();
    pimpl->steamToPeerId.clear();
    pimpl->peerIdToSteam.clear();
    pimpl->knownSteamClients.clear();
    pimpl->nextSteamPeerId = 1;

    SteamManager::instance().joinLobby(lobbyId);
    return true;
}

void NetworkManager::disconnect() {
    if (isSteamMode || SteamManager::instance().isInLobby()) {
        SteamManager::instance().leaveLobby();
    }
    isSteamMode = false;

    if (role == NetRole::Client && pimpl->serverPeer) {
        enet_peer_disconnect(pimpl->serverPeer, 0);

        ENetEvent event;
        bool done = false;
        for (int i = 0; i < 10; ++i) {
            if (pimpl->host && enet_host_service(pimpl->host, &event, 10) > 0) {
                if (event.type == ENET_EVENT_TYPE_RECEIVE) {
                    enet_packet_destroy(event.packet);
                } else if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
                    done = true;
                    break;
                }
            }
        }
        if (!done && pimpl->serverPeer) {
            enet_peer_disconnect_now(pimpl->serverPeer, 0);
        }
    }
    else if (role == NetRole::Host) {
        for (ENetPeer* peer : pimpl->connectedClients) {
            enet_peer_disconnect(peer, 0);
        }
        ENetEvent event;
        for (int i = 0; i < 10; ++i) {
            if (pimpl->host && enet_host_service(pimpl->host, &event, 10) > 0) {
                if (event.type == ENET_EVENT_TYPE_RECEIVE) {
                    enet_packet_destroy(event.packet);
                }
            }
        }
    }

    pimpl->cleanupHost();
    role = NetRole::Offline;
    remoteCursors.clear();
}

void NetworkManager::update() {
    SteamManager::instance().update();

    // 1. Process Steam P2P packets
    if (isSteamMode || SteamManager::instance().isSteamActive()) {
        SteamP2PPacket p2p;
        while (SteamManager::instance().pollP2PPacket(p2p)) {
            if (p2p.data.size() < sizeof(PacketHeader)) continue;

            uint32_t peerId = pimpl->getOrCreatePeerId(p2p.senderSteamID);
            auto* header = reinterpret_cast<PacketHeader*>(p2p.data.data());

            if (role == NetRole::Host) {
                bool isNewClient = pimpl->knownSteamClients.insert(p2p.senderSteamID).second;
                if (isNewClient || header->type == PacketType::Handshake) {
                    std::cout << "[NET] Host registered Steam client (SteamID: " << p2p.senderSteamID << ", peerId: " << peerId << ")" << std::endl;
                    NetEvent ne{};
                    ne.type = NetEventType::ClientConnected;
                    ne.peerId = peerId;
                    pimpl->eventQueue.push_back(ne);
                }
            }

            if (header->type == PacketType::Handshake) {
                // Handshake handled above
            }
            else if (header->type == PacketType::Init && p2p.data.size() >= sizeof(PacketInit)) {
                auto* p = reinterpret_cast<PacketInit*>(p2p.data.data());
                NetEvent ne{};
                ne.type = NetEventType::InitBoard;
                ne.initData = *p;
                pimpl->eventQueue.push_back(ne);
            }
            else if (header->type == PacketType::Click && p2p.data.size() >= sizeof(PacketClick)) {
                auto* p = reinterpret_cast<PacketClick*>(p2p.data.data());
                NetEvent ne{};
                ne.type = NetEventType::PlayerClick;
                ne.peerId = peerId;
                ne.clickData = *p;
                pimpl->eventQueue.push_back(ne);
            }
            else if (header->type == PacketType::Result && p2p.data.size() >= sizeof(PacketResult)) {
                auto* p = reinterpret_cast<PacketResult*>(p2p.data.data());
                NetEvent ne{};
                ne.type = NetEventType::BoardResult;
                ne.resultData = *p;
                pimpl->eventQueue.push_back(ne);
            }
            else if (header->type == PacketType::Cursor && p2p.data.size() >= sizeof(PacketCursor)) {
                auto* p = reinterpret_cast<PacketCursor*>(p2p.data.data());
                uint32_t senderId = (role == NetRole::Host) ? peerId : p->playerID;

                RemoteCursor rc;
                rc.x = p->x;
                rc.y = p->y;
                rc.angle = p->angle;
                rc.mass = p->mass;
                rc.isMoving = p->isMoving;
                rc.skin = p->skin;
                std::memcpy(rc.name, p->name, sizeof(rc.name));
                rc.name[sizeof(rc.name) - 1] = '\0';
                auto it = remoteCursors.find(senderId);
                if (it != remoteCursors.end()) {
                    rc.isSpeaking = it->second.isSpeaking;
                    rc.speakingTimer = it->second.speakingTimer;
                }
                remoteCursors[senderId] = rc;

                if (role == NetRole::Host) {
                    PacketCursor forwardPacket = *p;
                    forwardPacket.playerID = senderId;
                    for (uint64_t otherSteamId : SteamManager::instance().getConnectedPeers()) {
                        if (otherSteamId != p2p.senderSteamID) {
                            SteamManager::instance().sendP2PPacket(otherSteamId, &forwardPacket, sizeof(forwardPacket), false);
                        }
                    }
                    if (pimpl->host) {
                        for (ENetPeer* client : pimpl->connectedClients) {
                            ENetPacket* fwd = enet_packet_create(&forwardPacket, sizeof(PacketCursor), 0);
                            enet_peer_send(client, 1, fwd);
                        }
                    }
                }
            }
            else if (header->type == PacketType::Voice && p2p.data.size() >= sizeof(PacketVoice)) {
                auto* p = reinterpret_cast<PacketVoice*>(p2p.data.data());
                uint32_t senderId = (role == NetRole::Host) ? peerId : p->playerID;

                PacketVoice voicePkt = *p;
                voicePkt.playerID = senderId;

                auto it = remoteCursors.find(senderId);
                if (it != remoteCursors.end()) {
                    it->second.isSpeaking = true;
                    it->second.speakingTimer = 0.35f;
                }

                if (onVoiceReceived) {
                    onVoiceReceived(voicePkt);
                }

                if (role == NetRole::Host) {
                    for (uint64_t otherSteamId : SteamManager::instance().getConnectedPeers()) {
                        if (otherSteamId != p2p.senderSteamID) {
                            SteamManager::instance().sendP2PPacket(otherSteamId, &voicePkt, sizeof(voicePkt), false);
                        }
                    }
                    if (pimpl->host) {
                        for (ENetPeer* client : pimpl->connectedClients) {
                            ENetPacket* fwd = enet_packet_create(&voicePkt, sizeof(PacketVoice), 0);
                            enet_peer_send(client, 1, fwd);
                        }
                    }
                }
            }
            else if (header->type == PacketType::Laser && p2p.data.size() >= sizeof(PacketLaser)) {
                auto* p = reinterpret_cast<PacketLaser*>(p2p.data.data());
                uint32_t senderId = (role == NetRole::Host) ? peerId : p->playerID;

                PacketLaser laserPkt = *p;
                laserPkt.playerID = senderId;

                NetEvent ne{};
                ne.type = NetEventType::LaserFired;
                ne.peerId = senderId;
                ne.laserData = laserPkt;
                pimpl->eventQueue.push_back(ne);

                if (role == NetRole::Host) {
                    for (uint64_t otherSteamId : SteamManager::instance().getConnectedPeers()) {
                        if (otherSteamId != p2p.senderSteamID) {
                            SteamManager::instance().sendP2PPacket(otherSteamId, &laserPkt, sizeof(laserPkt), false);
                        }
                    }
                    if (pimpl->host) {
                        for (ENetPeer* client : pimpl->connectedClients) {
                            ENetPacket* fwd = enet_packet_create(&laserPkt, sizeof(PacketLaser), 0);
                            enet_peer_send(client, 1, fwd);
                        }
                    }
                }
            }
            else if (header->type == PacketType::Disconnect && p2p.data.size() >= sizeof(PacketDisconnect)) {
                auto* p = reinterpret_cast<PacketDisconnect*>(p2p.data.data());
                remoteCursors.erase(p->playerID);
                NetEvent ne{};
                ne.type = NetEventType::ClientDisconnected;
                ne.peerId = p->playerID;
                pimpl->eventQueue.push_back(ne);
            }
            else if (header->type == PacketType::Sync && p2p.data.size() >= sizeof(PacketSyncHeader)) {
                auto* p = reinterpret_cast<PacketSyncHeader*>(p2p.data.data());
                NetEvent ne{};
                ne.type = NetEventType::SyncBoard;
                ne.syncData.header = *p;

                size_t stateBytes = p->stateWordCount * sizeof(uint64_t);
                size_t flagBytes = p->flagCount * sizeof(PacketFlagSync);
                size_t expectedBytes = sizeof(PacketSyncHeader) + stateBytes + flagBytes;

                if (p2p.data.size() >= sizeof(PacketSyncHeader) + stateBytes && p->stateWordCount > 0) {
                    ne.syncData.stateWords.resize(p->stateWordCount);
                    std::memcpy(ne.syncData.stateWords.data(), p2p.data.data() + sizeof(PacketSyncHeader), stateBytes);
                }
                if (p2p.data.size() >= expectedBytes && p->flagCount > 0) {
                    ne.syncData.flagEntries.resize(p->flagCount);
                    std::memcpy(ne.syncData.flagEntries.data(), p2p.data.data() + sizeof(PacketSyncHeader) + stateBytes, flagBytes);
                }
                pimpl->eventQueue.push_back(ne);
            }
        }
    }

    // 2. Process ENet packets
    if (!pimpl->host) return;

    ENetEvent event;
    while (enet_host_service(pimpl->host, &event, 0) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT: {
                if (role == NetRole::Host) {
                    pimpl->connectedClients.push_back(event.peer);
                    NetEvent ne{};
                    ne.type = NetEventType::ClientConnected;
                    ne.peerId = event.peer->incomingPeerID;
                    pimpl->eventQueue.push_back(ne);
                }
                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT: {
                uint32_t disconnectedId = event.peer->incomingPeerID;
                if (role == NetRole::Host) {
                    auto it = std::find(pimpl->connectedClients.begin(), pimpl->connectedClients.end(), event.peer);
                    if (it != pimpl->connectedClients.end()) {
                        pimpl->connectedClients.erase(it);
                    }

                    PacketDisconnect dc;
                    dc.playerID = disconnectedId;
                    broadcast(&dc, sizeof(dc), true);

                    NetEvent ne{};
                    ne.type = NetEventType::ClientDisconnected;
                    ne.peerId = disconnectedId;
                    pimpl->eventQueue.push_back(ne);
                }
                else if (role == NetRole::Client) {
                    pimpl->serverPeer = nullptr;
                    role = NetRole::Offline;
                }
                remoteCursors.erase(disconnectedId);
                break;
            }

            case ENET_EVENT_TYPE_RECEIVE: {
                if (event.packet->dataLength >= sizeof(PacketHeader)) {
                    auto* header = reinterpret_cast<PacketHeader*>(event.packet->data);

                    if (header->type == PacketType::Init && event.packet->dataLength >= sizeof(PacketInit)) {
                        auto* p = reinterpret_cast<PacketInit*>(event.packet->data);
                        NetEvent ne{};
                        ne.type = NetEventType::InitBoard;
                        ne.initData = *p;
                        pimpl->eventQueue.push_back(ne);
                    }
                    else if (header->type == PacketType::Click && event.packet->dataLength >= sizeof(PacketClick)) {
                        auto* p = reinterpret_cast<PacketClick*>(event.packet->data);
                        NetEvent ne{};
                        ne.type = NetEventType::PlayerClick;
                        ne.peerId = event.peer ? event.peer->incomingPeerID : 0;
                        ne.clickData = *p;
                        pimpl->eventQueue.push_back(ne);
                    }
                    else if (header->type == PacketType::Result && event.packet->dataLength >= sizeof(PacketResult)) {
                        auto* p = reinterpret_cast<PacketResult*>(event.packet->data);
                        NetEvent ne{};
                        ne.type = NetEventType::BoardResult;
                        ne.resultData = *p;
                        pimpl->eventQueue.push_back(ne);
                    }
                    else if (header->type == PacketType::Cursor && event.packet->dataLength >= sizeof(PacketCursor)) {
                        auto* p = reinterpret_cast<PacketCursor*>(event.packet->data);
                        uint32_t senderId = (role == NetRole::Host && event.peer) ? event.peer->incomingPeerID : p->playerID;

                        RemoteCursor rc;
                        rc.x = p->x;
                        rc.y = p->y;
                        rc.angle = p->angle;
                        rc.mass = p->mass;
                        rc.isMoving = p->isMoving;
                        rc.skin = p->skin;
                        std::memcpy(rc.name, p->name, sizeof(rc.name));
                        rc.name[sizeof(rc.name) - 1] = '\0';
                        auto it = remoteCursors.find(senderId);
                        if (it != remoteCursors.end()) {
                            rc.isSpeaking = it->second.isSpeaking;
                            rc.speakingTimer = it->second.speakingTimer;
                        }
                        remoteCursors[senderId] = rc;

                        if (role == NetRole::Host) {
                            PacketCursor forwardPacket = *p;
                            forwardPacket.playerID = senderId;
                            for (ENetPeer* client : pimpl->connectedClients) {
                                if (client != event.peer) {
                                    ENetPacket* fwd = enet_packet_create(&forwardPacket, sizeof(PacketCursor), 0);
                                    enet_peer_send(client, 1, fwd);
                                }
                            }
                            for (uint64_t steamId : SteamManager::instance().getConnectedPeers()) {
                                SteamManager::instance().sendP2PPacket(steamId, &forwardPacket, sizeof(forwardPacket), false);
                            }
                        }
                    }
                    else if (header->type == PacketType::Voice && event.packet->dataLength >= sizeof(PacketVoice)) {
                        auto* p = reinterpret_cast<PacketVoice*>(event.packet->data);
                        uint32_t senderId = (role == NetRole::Host && event.peer) ? event.peer->incomingPeerID : p->playerID;

                        PacketVoice voicePkt = *p;
                        voicePkt.playerID = senderId;

                        auto it = remoteCursors.find(senderId);
                        if (it != remoteCursors.end()) {
                            it->second.isSpeaking = true;
                            it->second.speakingTimer = 0.35f;
                        }

                        if (onVoiceReceived) {
                            onVoiceReceived(voicePkt);
                        }

                        if (role == NetRole::Host) {
                            for (ENetPeer* client : pimpl->connectedClients) {
                                if (client != event.peer) {
                                    ENetPacket* fwd = enet_packet_create(&voicePkt, sizeof(PacketVoice), 0);
                                    enet_peer_send(client, 1, fwd);
                                }
                            }
                            for (uint64_t steamId : SteamManager::instance().getConnectedPeers()) {
                                SteamManager::instance().sendP2PPacket(steamId, &voicePkt, sizeof(voicePkt), false);
                            }
                        }
                    }
                    else if (header->type == PacketType::Laser && event.packet->dataLength >= sizeof(PacketLaser)) {
                        auto* p = reinterpret_cast<PacketLaser*>(event.packet->data);
                        uint32_t senderId = (role == NetRole::Host && event.peer) ? event.peer->incomingPeerID : p->playerID;

                        PacketLaser laserPkt = *p;
                        laserPkt.playerID = senderId;

                        NetEvent ne{};
                        ne.type = NetEventType::LaserFired;
                        ne.peerId = senderId;
                        ne.laserData = laserPkt;
                        pimpl->eventQueue.push_back(ne);

                        if (role == NetRole::Host) {
                            for (ENetPeer* client : pimpl->connectedClients) {
                                if (client != event.peer) {
                                    ENetPacket* fwd = enet_packet_create(&laserPkt, sizeof(PacketLaser), 0);
                                    enet_peer_send(client, 1, fwd);
                                }
                            }
                            for (uint64_t steamId : SteamManager::instance().getConnectedPeers()) {
                                SteamManager::instance().sendP2PPacket(steamId, &laserPkt, sizeof(laserPkt), false);
                            }
                        }
                    }
                    else if (header->type == PacketType::Disconnect && event.packet->dataLength >= sizeof(PacketDisconnect)) {
                        auto* p = reinterpret_cast<PacketDisconnect*>(event.packet->data);
                        remoteCursors.erase(p->playerID);
                        NetEvent ne{};
                        ne.type = NetEventType::ClientDisconnected;
                        ne.peerId = p->playerID;
                        pimpl->eventQueue.push_back(ne);
                    }
                    else if (header->type == PacketType::Sync && event.packet->dataLength >= sizeof(PacketSyncHeader)) {
                        auto* p = reinterpret_cast<PacketSyncHeader*>(event.packet->data);
                        NetEvent ne{};
                        ne.type = NetEventType::SyncBoard;
                        ne.syncData.header = *p;

                        size_t stateBytes = p->stateWordCount * sizeof(uint64_t);
                        size_t flagBytes = p->flagCount * sizeof(PacketFlagSync);
                        size_t expectedBytes = sizeof(PacketSyncHeader) + stateBytes + flagBytes;

                        if (event.packet->dataLength >= sizeof(PacketSyncHeader) + stateBytes && p->stateWordCount > 0) {
                            ne.syncData.stateWords.resize(p->stateWordCount);
                            std::memcpy(ne.syncData.stateWords.data(), event.packet->data + sizeof(PacketSyncHeader), stateBytes);
                        }
                        if (event.packet->dataLength >= expectedBytes && p->flagCount > 0) {
                            ne.syncData.flagEntries.resize(p->flagCount);
                            std::memcpy(ne.syncData.flagEntries.data(), event.packet->data + sizeof(PacketSyncHeader) + stateBytes, flagBytes);
                        }
                        pimpl->eventQueue.push_back(ne);
                    }
                }
                enet_packet_destroy(event.packet);
                break;
            }
            default:
                break;
        }
    }

    for (auto& [id, cursor] : remoteCursors) {
        if (cursor.speakingTimer > 0.0f) {
            cursor.speakingTimer -= 0.016f;
            if (cursor.speakingTimer <= 0.0f) {
                cursor.isSpeaking = false;
                cursor.speakingTimer = 0.0f;
            }
        }
    }
}

bool NetworkManager::pollEvent(NetEvent& outEvent) {
    if (pimpl->eventQueue.empty()) return false;
    outEvent = pimpl->eventQueue.front();
    pimpl->eventQueue.pop_front();
    return true;
}

void NetworkManager::sendToServer(const void* data, size_t size, bool reliable) {
    if (role != NetRole::Client) return;

    // 1. Steam Client
    if (isSteamMode || SteamManager::instance().isInLobby()) {
        uint64_t hostSteamId = SteamManager::instance().getLobbyOwner();
        if (hostSteamId != 0) {
            SteamManager::instance().sendP2PPacket(hostSteamId, data, static_cast<uint32_t>(size), reliable);
        }
        return;
    }

    // 2. ENet Client
    if (pimpl->serverPeer && pimpl->host) {
        enet_uint32 flags = reliable ? ENET_PACKET_FLAG_RELIABLE : 0;
        ENetPacket* packet = enet_packet_create(data, size, flags);
        enet_peer_send(pimpl->serverPeer, reliable ? 0 : 1, packet);
        enet_host_flush(pimpl->host);
    }
}

void NetworkManager::sendToPeer(uint32_t peerId, const void* data, size_t size, bool reliable) {
    // 1. Check if target is a Steam peer
    auto it = pimpl->peerIdToSteam.find(peerId);
    if (it != pimpl->peerIdToSteam.end()) {
        SteamManager::instance().sendP2PPacket(it->second, data, static_cast<uint32_t>(size), reliable);
        return;
    }

    // 2. Check if target is an ENet peer
    if (role == NetRole::Host && pimpl->host) {
        for (ENetPeer* peer : pimpl->connectedClients) {
            if (peer->incomingPeerID == peerId) {
                enet_uint32 flags = reliable ? ENET_PACKET_FLAG_RELIABLE : 0;
                ENetPacket* packet = enet_packet_create(data, size, flags);
                enet_peer_send(peer, reliable ? 0 : 1, packet);
                enet_host_flush(pimpl->host);
                break;
            }
        }
    }
}

void NetworkManager::broadcast(const void* data, size_t size, bool reliable) {
    if (role != NetRole::Host) return;

    // 1. Broadcast to all Steam peers
    if (SteamManager::instance().isSteamActive()) {
        SteamManager::instance().broadcastP2PPacket(data, static_cast<uint32_t>(size), reliable);
    }

    // 2. Broadcast to all ENet peers
    if (pimpl->host && !pimpl->connectedClients.empty()) {
        enet_uint32 flags = reliable ? ENET_PACKET_FLAG_RELIABLE : 0;
        ENetPacket* packet = enet_packet_create(data, size, flags);
        for (ENetPeer* peer : pimpl->connectedClients) {
            enet_peer_send(peer, reliable ? 0 : 1, packet);
        }
        enet_host_flush(pimpl->host);
    }
}

} // namespace minesweeper::net
