#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

namespace minesweeper::net {

enum class NetRole : uint8_t {
    Offline = 0,
    Host = 1,
    Client = 2
};

enum class PacketType : uint8_t {
    Init = 0,
    Click = 1,
    Result = 2,
    Cursor = 3,
    Disconnect = 4,
    Sync = 5,
    Handshake = 6,
    Voice = 7,
    Laser = 8,
    Bubble = 9
};

constexpr uint32_t HOST_PLAYER_ID = 0xFFFFFFFF;

#pragma pack(push, 1)

struct PacketHeader {
    PacketType type;
};

struct PacketHandshake {
    PacketType type = PacketType::Handshake;
    char name[16] = {0};
    uint8_t cursorSkin = 0;
    uint8_t flagSkin = 0;
};

struct PacketInit {
    PacketType type = PacketType::Init;
    uint64_t seed = 0;
    int32_t dim = 2;
    int32_t size = 10;
    int32_t bombs = 10;
};

struct PacketClick {
    PacketType type = PacketType::Click;
    uint64_t index = 0;
    uint8_t action = 0; // 0 = Reveal, 1 = Chord, 2 = Flag
    uint8_t flagSkin = 0;
};

struct PacketResult {
    PacketType type = PacketType::Result;
    uint64_t index = 0;
    uint8_t state = 0; // 0 = Reveal, 1 = Hidden (unflagged), 2 = Flagged
    uint32_t placerId = 0;
    uint8_t flagSkin = 0;
};

struct PacketCursor {
    PacketType type = PacketType::Cursor;
    uint32_t playerID = 0;
    float x = 0.0f;
    float y = 0.0f;
    float angle = 0.0f;
    float mass = 8.0f;
    bool isMoving = false;
    uint8_t skin = 0;
    char name[16] = {0};
};

struct PacketVoice {
    PacketType type = PacketType::Voice;
    uint32_t playerID = 0;
    float x = 0.0f;
    float y = 0.0f;
    uint16_t sampleCount = 640;
    uint16_t dataSize = 320;
    uint8_t data[320] = {0};
};

struct PacketDisconnect {
    PacketType type = PacketType::Disconnect;
    uint32_t playerID = 0;
};

struct PacketLaser {
    PacketType type = PacketType::Laser;
    uint32_t playerID = 0;
    float fromX = 0.0f;
    float fromY = 0.0f;
    float toX = 0.0f;
    float toY = 0.0f;
    uint8_t skinId = 0;
    uint8_t laserType = 0; // 0 = skin laser, 1 = red flag laser
};

struct PacketBubble {
    PacketType type = PacketType::Bubble;
    uint32_t playerID = 0;
    float x = 0.0f;
    float y = 0.0f;
};

struct PacketFlagSync {
    uint64_t index = 0;
    uint32_t placerId = 0;
    uint8_t skinId = 0;
};

struct PacketSyncHeader {
    PacketType type = PacketType::Sync;
    uint64_t seed = 0;
    int32_t dim = 2;
    int32_t size = 10;
    int32_t bombs = 10;
    float timePlayed = 0.0f;
    uint8_t isGameOver = 0;
    uint8_t isVictory = 0;
    uint32_t stateWordCount = 0;
    uint32_t flagCount = 0;
};

#pragma pack(pop)

struct PacketSyncData {
    PacketSyncHeader header{};
    std::vector<uint64_t> stateWords;
    std::vector<PacketFlagSync> flagEntries;
};

enum class NetEventType : uint8_t {
    ClientConnected,
    ClientDisconnected,
    InitBoard,
    PlayerClick,
    BoardResult,
    SyncBoard,
    LaserFired,
    BubbleTriggered
};

struct NetEvent {
    NetEventType type;
    uint32_t peerId = 0;

    PacketInit initData{};
    PacketClick clickData{};
    PacketResult resultData{};
    PacketSyncData syncData{};
    PacketLaser laserData{};
    PacketBubble bubbleData{};
};

struct RemoteCursor {
    float x = 0.0f;
    float y = 0.0f;
    float angle = 0.0f;
    float mass = 8.0f;
    bool isMoving = false;
    uint8_t skin = 0;
    char name[16] = {0};
    bool isSpeaking = false;
    float speakingTimer = 0.0f;
};

} // namespace minesweeper::net
