#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_FLAC
#define MA_NO_MP3
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_NODE_GRAPH
#define MA_NO_ENGINE

#ifdef _MSC_VER
#pragma warning(push, 0)
#pragma warning(disable: 4456 4245 4244 4100 4267)
#endif

#include "../../third_party/miniaudio/miniaudio.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include "voice_manager.hpp"

#include <cmath>
#include <cstring>
#include <mutex>
#include <deque>
#include <unordered_map>
#include <algorithm>
#include <atomic>
#include <iostream>

namespace minesweeper::audio {

// IMA-ADPCM lookup tables
static const int16_t STEP_TABLE[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

static const int8_t INDEX_TABLE[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
};

static void encodeImaAdpcm(const int16_t* pcm, size_t sampleCount, uint8_t* outAdpcm, int16_t& prevSample, int8_t& stepIndex) {
    size_t outIdx = 0;
    for (size_t i = 0; i < sampleCount; i += 2) {
        uint8_t byteVal = 0;
        for (int nib = 0; nib < 2; ++nib) {
            int16_t sample = (i + nib < sampleCount) ? pcm[i + nib] : 0;
            int32_t diff = static_cast<int32_t>(sample) - static_cast<int32_t>(prevSample);
            uint8_t code = 0;
            if (diff < 0) {
                code = 8;
                diff = -diff;
            }

            int32_t step = STEP_TABLE[stepIndex];
            int32_t vpdiff = step >> 3;

            if (diff >= step) {
                code |= 4;
                diff -= step;
                vpdiff += step;
            }
            step >>= 1;
            if (diff >= step) {
                code |= 2;
                diff -= step;
                vpdiff += step;
            }
            step >>= 1;
            if (diff >= step) {
                code |= 1;
                vpdiff += step;
            }

            if (code & 8) {
                prevSample = static_cast<int16_t>(std::clamp(static_cast<int32_t>(prevSample) - vpdiff, -32768, 32767));
            } else {
                prevSample = static_cast<int16_t>(std::clamp(static_cast<int32_t>(prevSample) + vpdiff, -32768, 32767));
            }

            stepIndex = static_cast<int8_t>(std::clamp(static_cast<int>(stepIndex) + INDEX_TABLE[code & 7], 0, 88));

            if (nib == 0) {
                byteVal = code & 0x0F;
            } else {
                byteVal |= (code << 4);
            }
        }
        outAdpcm[outIdx++] = byteVal;
    }
}

static void decodeImaAdpcm(const uint8_t* inAdpcm, size_t byteCount, int16_t* outPcm, size_t maxSamples, int16_t& prevSample, int8_t& stepIndex) {
    size_t outIdx = 0;
    for (size_t i = 0; i < byteCount && outIdx < maxSamples; ++i) {
        uint8_t byteVal = inAdpcm[i];
        for (int nib = 0; nib < 2 && outIdx < maxSamples; ++nib) {
            uint8_t code = (nib == 0) ? (byteVal & 0x0F) : ((byteVal >> 4) & 0x0F);
            int32_t step = STEP_TABLE[stepIndex];
            int32_t vpdiff = step >> 3;

            if (code & 4) vpdiff += step;
            if (code & 2) vpdiff += (step >> 1);
            if (code & 1) vpdiff += (step >> 2);

            if (code & 8) {
                prevSample = static_cast<int16_t>(std::clamp(static_cast<int32_t>(prevSample) - vpdiff, -32768, 32767));
            } else {
                prevSample = static_cast<int16_t>(std::clamp(static_cast<int32_t>(prevSample) + vpdiff, -32768, 32767));
            }

            stepIndex = static_cast<int8_t>(std::clamp(static_cast<int>(stepIndex) + INDEX_TABLE[code & 7], 0, 88));
            outPcm[outIdx++] = prevSample;
        }
    }
}

struct PeerAudioStream {
    uint32_t peerID = 0;
    float posX = 0.0f;
    float posY = 0.0f;
    std::deque<int16_t> sampleBuffer;
    float inactiveTimer = 0.0f;
    int16_t adpcmPrev = 0;
    int8_t adpcmIndex = 0;
};

struct VoiceManager::Impl {
    VoiceSettings settings;

    ma_context context{};
    ma_device captureDevice{};
    ma_device playbackDevice{};
    bool contextInitialized = false;
    bool captureInitialized = false;
    bool playbackInitialized = false;

    std::atomic<float> localCursorX{0.0f};
    std::atomic<float> localCursorY{0.0f};
    std::atomic<bool> pttActive{false};

    std::atomic<float> currentMicLevel{0.0f};
    std::atomic<bool> isCurrentlyTransmitting{false};
    float vadHangoverTimer = 0.0f;

    int16_t localAdpcmPrev = 0;
    int8_t localAdpcmIndex = 0;
    std::vector<int16_t> captureAccumulator;

    std::mutex outgoingMutex;
    std::deque<net::PacketVoice> outgoingPackets;

    std::mutex peersMutex;
    std::unordered_map<uint32_t, PeerAudioStream> peers;

    static void captureCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
    static void playbackCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
};

void VoiceManager::Impl::captureCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    (void)pOutput;
    VoiceManager::Impl* impl = reinterpret_cast<VoiceManager::Impl*>(pDevice->pUserData);
    if (!impl || !impl->settings.enabled || !pInput || frameCount == 0) return;

    const int16_t* inSamples = reinterpret_cast<const int16_t*>(pInput);
    float gain = impl->settings.micGain;

    // Calculate RMS energy of this audio chunk
    double sumSq = 0.0;
    for (ma_uint32 i = 0; i < frameCount; ++i) {
        float s = inSamples[i] * gain;
        sumSq += s * s;
    }
    float rms = static_cast<float>(std::sqrt(sumSq / frameCount) / 32768.0);
    float oldLevel = impl->currentMicLevel.load(std::memory_order_relaxed);
    float newLevel = oldLevel * 0.65f + rms * 0.35f;
    impl->currentMicLevel.store(newLevel, std::memory_order_relaxed);

    bool shouldTransmit = false;
    if (impl->settings.pushToTalk) {
        shouldTransmit = impl->pttActive.load(std::memory_order_relaxed);
    } else {
        if (rms >= impl->settings.vadThreshold) {
            impl->vadHangoverTimer = 0.35f;
            shouldTransmit = true;
        } else if (impl->vadHangoverTimer > 0.0f) {
            shouldTransmit = true;
        }
    }

    impl->isCurrentlyTransmitting.store(shouldTransmit, std::memory_order_relaxed);

    if (shouldTransmit) {
        for (ma_uint32 i = 0; i < frameCount; ++i) {
            float s = inSamples[i] * gain;
            int32_t clamped = std::clamp(static_cast<int32_t>(s), -32768, 32767);
            impl->captureAccumulator.push_back(static_cast<int16_t>(clamped));

            if (impl->captureAccumulator.size() >= 640) {
                net::PacketVoice pkt;
                pkt.type = net::PacketType::Voice;
                pkt.playerID = 0;
                pkt.x = impl->localCursorX.load(std::memory_order_relaxed);
                pkt.y = impl->localCursorY.load(std::memory_order_relaxed);
                pkt.sampleCount = 640;
                pkt.dataSize = 320;

                encodeImaAdpcm(impl->captureAccumulator.data(), 640, pkt.data, impl->localAdpcmPrev, impl->localAdpcmIndex);
                impl->captureAccumulator.clear();

                std::lock_guard<std::mutex> lock(impl->outgoingMutex);
                if (impl->outgoingPackets.size() < 24) {
                    impl->outgoingPackets.push_back(pkt);
                }
            }
        }
    } else {
        impl->captureAccumulator.clear();
        impl->localAdpcmPrev = 0;
        impl->localAdpcmIndex = 0;
    }
}

void VoiceManager::Impl::playbackCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    (void)pInput;
    VoiceManager::Impl* impl = reinterpret_cast<VoiceManager::Impl*>(pDevice->pUserData);
    if (!impl || !pOutput || frameCount == 0) return;

    float* out = reinterpret_cast<float*>(pOutput);
    std::fill(out, out + frameCount * 2, 0.0f);

    if (!impl->settings.enabled || impl->settings.voiceVolume <= 0.001f) return;

    float localX = impl->localCursorX.load(std::memory_order_relaxed);
    float localY = impl->localCursorY.load(std::memory_order_relaxed);
    float masterVol = impl->settings.voiceVolume;
    bool proximity = impl->settings.proximity;
    float maxDist = impl->settings.maxAudibleDistance;
    float minDist = impl->settings.minAudibleDistance;

    std::lock_guard<std::mutex> lock(impl->peersMutex);
    for (auto& [id, peer] : impl->peers) {
        if (peer.sampleBuffer.empty()) continue;

        float leftGain = masterVol;
        float rightGain = masterVol;

        if (proximity) {
            float dx = peer.posX - localX;
            float dy = peer.posY - localY;
            float dist = std::sqrt(dx * dx + dy * dy);

            if (dist >= maxDist) {
                size_t toPop = std::min(static_cast<size_t>(frameCount), peer.sampleBuffer.size());
                peer.sampleBuffer.erase(peer.sampleBuffer.begin(), peer.sampleBuffer.begin() + toPop);
                continue;
            }

            float distFactor = 1.0f;
            if (dist > minDist) {
                distFactor = std::clamp(1.0f - ((dist - minDist) / (maxDist - minDist)), 0.0f, 1.0f);
            }
            float gain = distFactor * distFactor * masterVol;

            float pan = std::clamp(dx / (maxDist * 0.5f), -0.85f, 0.85f);
            leftGain = std::cos((pan + 1.0f) * 0.25f * 3.14159265f) * gain;
            rightGain = std::sin((pan + 1.0f) * 0.25f * 3.14159265f) * gain;
        }

        ma_uint32 framesToRead = std::min(frameCount, static_cast<ma_uint32>(peer.sampleBuffer.size()));
        for (ma_uint32 f = 0; f < framesToRead; ++f) {
            float s = static_cast<float>(peer.sampleBuffer[f]) / 32768.0f;
            out[f * 2 + 0] += s * leftGain;
            out[f * 2 + 1] += s * rightGain;
        }
        peer.sampleBuffer.erase(peer.sampleBuffer.begin(), peer.sampleBuffer.begin() + framesToRead);
    }

    for (ma_uint32 i = 0; i < frameCount * 2; ++i) {
        out[i] = std::clamp(out[i], -1.0f, 1.0f);
    }
}

VoiceManager::VoiceManager() : pimpl(std::make_unique<Impl>()) {}

VoiceManager::~VoiceManager() {
    cleanup();
}

bool VoiceManager::init() {
    if (ma_context_init(NULL, 0, NULL, &pimpl->context) != MA_SUCCESS) {
        std::cerr << "[VOICE] Failed to initialize miniaudio context." << std::endl;
        return false;
    }
    pimpl->contextInitialized = true;

    // 1. Initialize Microphone Capture Device (16 kHz, Mono, S16)
    ma_device_config captureConfig = ma_device_config_init(ma_device_type_capture);
    captureConfig.capture.format = ma_format_s16;
    captureConfig.capture.channels = 1;
    captureConfig.sampleRate = 16000;
    captureConfig.dataCallback = Impl::captureCallback;
    captureConfig.pUserData = pimpl.get();

    if (ma_device_init(&pimpl->context, &captureConfig, &pimpl->captureDevice) == MA_SUCCESS) {
        if (ma_device_start(&pimpl->captureDevice) == MA_SUCCESS) {
            pimpl->captureInitialized = true;
            std::cout << "[VOICE] Microphone capture device started successfully (16 kHz mono)." << std::endl;
        } else {
            ma_device_uninit(&pimpl->captureDevice);
            std::cerr << "[VOICE] Warning: Failed to start capture device." << std::endl;
        }
    } else {
        std::cerr << "[VOICE] Warning: No capture device found or failed to initialize." << std::endl;
    }

    // 2. Initialize Stereo Playback Device (16 kHz, Stereo, F32)
    ma_device_config playbackConfig = ma_device_config_init(ma_device_type_playback);
    playbackConfig.playback.format = ma_format_f32;
    playbackConfig.playback.channels = 2;
    playbackConfig.sampleRate = 16000;
    playbackConfig.dataCallback = Impl::playbackCallback;
    playbackConfig.pUserData = pimpl.get();

    if (ma_device_init(&pimpl->context, &playbackConfig, &pimpl->playbackDevice) == MA_SUCCESS) {
        if (ma_device_start(&pimpl->playbackDevice) == MA_SUCCESS) {
            pimpl->playbackInitialized = true;
            std::cout << "[VOICE] Audio playback device started successfully." << std::endl;
        } else {
            ma_device_uninit(&pimpl->playbackDevice);
            std::cerr << "[VOICE] Warning: Failed to start playback device." << std::endl;
        }
    } else {
        std::cerr << "[VOICE] Warning: Failed to initialize playback device." << std::endl;
    }

    return (pimpl->captureInitialized || pimpl->playbackInitialized);
}

void VoiceManager::cleanup() {
    if (pimpl->captureInitialized) {
        ma_device_stop(&pimpl->captureDevice);
        ma_device_uninit(&pimpl->captureDevice);
        pimpl->captureInitialized = false;
    }
    if (pimpl->playbackInitialized) {
        ma_device_stop(&pimpl->playbackDevice);
        ma_device_uninit(&pimpl->playbackDevice);
        pimpl->playbackInitialized = false;
    }
    if (pimpl->contextInitialized) {
        ma_context_uninit(&pimpl->context);
        pimpl->contextInitialized = false;
    }
}

void VoiceManager::update(float dt) {
    if (!pimpl) return;

    if (pimpl->vadHangoverTimer > 0.0f) {
        pimpl->vadHangoverTimer -= dt;
    }

    // Clean up stale peers after 5 seconds of silence
    std::lock_guard<std::mutex> lock(pimpl->peersMutex);
    for (auto it = pimpl->peers.begin(); it != pimpl->peers.end(); ) {
        it->second.inactiveTimer += dt;
        if (it->second.inactiveTimer > 5.0f && it->second.sampleBuffer.empty()) {
            it = pimpl->peers.erase(it);
        } else {
            ++it;
        }
    }
}

void VoiceManager::setLocalCursorPos(float worldX, float worldY) {
    if (pimpl) {
        pimpl->localCursorX.store(worldX, std::memory_order_relaxed);
        pimpl->localCursorY.store(worldY, std::memory_order_relaxed);
    }
}

void VoiceManager::setPushToTalkActive(bool active) {
    if (pimpl) {
        pimpl->pttActive.store(active, std::memory_order_relaxed);
    }
}

bool VoiceManager::isTransmitting() const {
    return pimpl ? pimpl->isCurrentlyTransmitting.load(std::memory_order_relaxed) : false;
}

float VoiceManager::getMicLevel() const {
    return pimpl ? pimpl->currentMicLevel.load(std::memory_order_relaxed) : 0.0f;
}

bool VoiceManager::getOutgoingPacket(net::PacketVoice& outPacket) {
    if (!pimpl) return false;
    std::lock_guard<std::mutex> lock(pimpl->outgoingMutex);
    if (pimpl->outgoingPackets.empty()) return false;

    outPacket = pimpl->outgoingPackets.front();
    pimpl->outgoingPackets.pop_front();
    return true;
}

void VoiceManager::receiveVoicePacket(const net::PacketVoice& packet) {
    if (!pimpl || !pimpl->settings.enabled) return;
    if (packet.dataSize == 0 || packet.sampleCount == 0) return;

    std::vector<int16_t> pcm(packet.sampleCount);
    std::lock_guard<std::mutex> lock(pimpl->peersMutex);
    auto& peer = pimpl->peers[packet.playerID];
    peer.peerID = packet.playerID;
    peer.posX = packet.x;
    peer.posY = packet.y;
    peer.inactiveTimer = 0.0f;

    decodeImaAdpcm(packet.data, packet.dataSize, pcm.data(), packet.sampleCount, peer.adpcmPrev, peer.adpcmIndex);

    // Limit buffer to prevent excess latency (max ~200ms = 3200 samples)
    if (peer.sampleBuffer.size() + pcm.size() > 3200) {
        peer.sampleBuffer.clear();
    }
    peer.sampleBuffer.insert(peer.sampleBuffer.end(), pcm.begin(), pcm.end());
}

VoiceSettings& VoiceManager::getSettings() {
    return pimpl->settings;
}

const VoiceSettings& VoiceManager::getSettings() const {
    return pimpl->settings;
}

} // namespace minesweeper::audio
