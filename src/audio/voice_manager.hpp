#pragma once

#include "../net/protocol.hpp"
#include <cstdint>
#include <vector>
#include <memory>
#include <string>

namespace minesweeper::audio {

struct VoiceSettings {
    bool enabled = true;
    bool proximity = true;       // true: spatial 3D/distance falloff & panning, false: global voice
    bool pushToTalk = true;      // true: hold [V] to talk, false: voice activity detection (open mic)
    float voiceVolume = 1.0f;    // 0.0 to 1.5
    float micGain = 1.0f;        // 0.5 to 2.0
    float maxAudibleDistance = 1200.0f;
    float minAudibleDistance = 150.0f;
    float vadThreshold = 0.035f; // RMS threshold for voice activity detection
};

class VoiceManager {
public:
    VoiceManager();
    ~VoiceManager();

    bool init();
    void cleanup();
    void update(float dt);

    void setLocalCursorPos(float worldX, float worldY);
    void setPushToTalkActive(bool active);

    bool isTransmitting() const;
    float getMicLevel() const; // 0.0 to 1.0 (live microphone input level for settings UI)

    // Networking
    bool getOutgoingPacket(net::PacketVoice& outPacket);
    void receiveVoicePacket(const net::PacketVoice& packet);

    VoiceSettings& getSettings();
    const VoiceSettings& getSettings() const;

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};

} // namespace minesweeper::audio
