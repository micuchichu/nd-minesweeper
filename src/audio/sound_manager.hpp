#pragma once

#include <raylib.h>
#include <string>
#include <vector>
#include <random>

namespace minesweeper::audio {

enum class BgmPlaybackState {
    Idle,       // No tracks available or audio device not initialized
    Waiting,    // Silence interval countdown between tracks
    Playing     // Currently playing a background track
};

class SoundManager {
public:
    static SoundManager* s_instance;
    static SoundManager& instance() { return *s_instance; }
    static bool hasInstance() { return s_instance != nullptr; }

    // Static trigger shortcuts (safe to call even if instance is null)
    static void playButton();
    static void playIncrement();
    static void playUncover();
    static void playExplosion();
    static void playBanana();
    static void playLaser();
    static void playFlag();

    SoundManager();
    ~SoundManager();

    bool init();
    void cleanup();
    void update(float dt);

    // Audio settings - Background Music
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }

    void setVolume(float volume);
    float getVolume() const { return m_volume; }

    // Audio settings - Sound Effects (SFX)
    void setSfxEnabled(bool enabled);
    bool isSfxEnabled() const { return m_sfxEnabled; }

    void setSfxVolume(float volume);
    float getSfxVolume() const { return m_sfxVolume; }

    // Silence interval configuration (in seconds)
    void setIntervalRange(float minSeconds, float maxSeconds);
    float getMinInterval() const { return m_minInterval; }
    float getMaxInterval() const { return m_maxInterval; }

    // Playback control - BGM
    void playRandomTrack();
    void stop();
    void skip();

    // SFX playback methods
    void playButtonSound();
    void playIncrementSound();
    void playUncoverSound();
    void playExplosionSound();
    void playBananaSound();
    void playLaserSound();
    void playFlagSound();

    // SFX query status
    bool hasButtonSound() const { return m_hasSndButton; }
    bool hasIncrementSound() const { return m_hasSndIncrement; }
    bool hasUncoverSound() const { return !m_sndUncovers.empty(); }
    size_t getExplosionSoundCount() const { return m_sndExplosions.size(); }
    bool hasBananaSound() const { return m_hasSndBanana; }
    bool hasLaserSound() const { return !m_sndLasers.empty(); }
    size_t getLaserSoundCount() const { return m_sndLasers.size(); }
    bool hasFlagSound() const { return !m_sndFlags.empty(); }
    size_t getFlagSoundCount() const { return m_sndFlags.size(); }

    // Query status - BGM
    BgmPlaybackState getState() const { return m_state; }
    float getRemainingWaitTime() const { return m_intervalTimer; }
    std::string getCurrentTrackName() const;
    size_t getTrackCount() const { return m_trackPaths.size(); }
    float getCurrentTrackTimePlayed() const;
    float getCurrentTrackTimeLength() const;
    float getFadeFactor() const { return m_fadeFactor; }
    float getFadeDuration() const { return m_fadeDuration; }

private:
    bool m_initialized = false;
    bool m_enabled = true;
    float m_volume = 0.20f;

    // SFX settings
    bool m_sfxEnabled = true;
    float m_sfxVolume = 0.80f;

    // SFX sound handles
    Sound m_sndButton{};
    bool m_hasSndButton = false;

    Sound m_sndIncrement{};
    bool m_hasSndIncrement = false;

    std::vector<Sound> m_sndUncovers;
    std::vector<Sound> m_sndExplosions;

    Sound m_sndBanana{};
    bool m_hasSndBanana = false;

    std::vector<Sound> m_sndLasers;
    std::vector<Sound> m_sndFlags;

    // Interval timers
    float m_minInterval = 25.0f;
    float m_maxInterval = 65.0f;
    float m_intervalTimer = 0.0f;

    // Fade configuration
    float m_fadeDuration = 5.0f;
    float m_fadeTimer = 0.0f;
    float m_fadeFactor = 0.0f;

    static constexpr float MASTER_VOLUME_SCALE = 0.5f;

    // Track paths
    std::vector<std::string> m_trackPaths;
    int m_lastTrackIndex = -1;

    // Current playing music stream
    Music m_currentMusic{};
    bool m_hasCurrentMusic = false;
    std::string m_currentTrackPath;

    BgmPlaybackState m_state = BgmPlaybackState::Idle;

    std::mt19937 m_rng;

    void scanBackgroundSounds();
    void loadSoundEffects();
    void unloadSoundEffects();
    float getRandomInterval(float minSec, float maxSec);
    int pickNextTrackIndex();
    void applyCurrentVolume();
};

} // namespace minesweeper::audio
