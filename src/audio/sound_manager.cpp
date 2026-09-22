#include "sound_manager.hpp"
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <chrono>

namespace fs = std::filesystem;

namespace minesweeper::audio {

SoundManager* SoundManager::s_instance = nullptr;

void SoundManager::playButton() {
    if (s_instance) s_instance->playButtonSound();
}

void SoundManager::playIncrement() {
    if (s_instance) s_instance->playIncrementSound();
}

void SoundManager::playUncover() {
    if (s_instance) s_instance->playUncoverSound();
}

void SoundManager::playExplosion() {
    if (s_instance) s_instance->playExplosionSound();
}

void SoundManager::playBanana() {
    if (s_instance) s_instance->playBananaSound();
}

void SoundManager::playLaser() {
    if (s_instance) s_instance->playLaserSound();
}

void SoundManager::playFlag() {
    if (s_instance) s_instance->playFlagSound();
}

SoundManager::SoundManager() {
    s_instance = this;
    std::random_device rd;
    m_rng.seed(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count()) ^ rd());
}

SoundManager::~SoundManager() {
    cleanup();
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

bool SoundManager::init() {
    if (!IsAudioDeviceReady()) {
        InitAudioDevice();
    }

    if (!IsAudioDeviceReady()) {
        std::cerr << "[AUDIO] Failed to initialize Raylib audio device!" << std::endl;
        m_initialized = false;
        return false;
    }

    m_initialized = true;
    scanBackgroundSounds();
    loadSoundEffects();

    if (!m_trackPaths.empty() && m_enabled) {
        m_state = BgmPlaybackState::Waiting;
        m_intervalTimer = getRandomInterval(4.0f, 12.0f);
        std::cout << "[AUDIO] SoundManager initialized. Next background sound in " << m_intervalTimer << "s." << std::endl;
    }

    return true;
}

void SoundManager::cleanup() {
    stop();
    unloadSoundEffects();

    if (m_initialized) {
        if (IsAudioDeviceReady()) {
            CloseAudioDevice();
        }
        m_initialized = false;
    }

    m_state = BgmPlaybackState::Idle;
    m_trackPaths.clear();
}

void SoundManager::scanBackgroundSounds() {
    m_trackPaths.clear();

    const std::vector<std::string> searchDirs = {
        "assets/sounds/background",
        "../assets/sounds/background",
        std::string(GetApplicationDirectory()) + "assets/sounds/background",
        std::string(GetApplicationDirectory()) + "../assets/sounds/background"
    };

    fs::path targetDir;
    for (const auto& dirStr : searchDirs) {
        std::error_code ec;
        if (fs::exists(dirStr, ec) && fs::is_directory(dirStr, ec)) {
            targetDir = fs::canonical(dirStr, ec);
            if (ec) targetDir = dirStr;
            break;
        }
    }

    if (targetDir.empty()) {
        std::cout << "[AUDIO] Background sounds directory not found." << std::endl;
        return;
    }

    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(targetDir, ec)) {
        if (entry.is_regular_file(ec)) {
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });

            if (ext == ".ogg" || ext == ".wav" || ext == ".mp3" || ext == ".flac") {
                m_trackPaths.push_back(entry.path().string());
            }
        }
    }

    std::sort(m_trackPaths.begin(), m_trackPaths.end());
    std::cout << "[AUDIO] Discovered " << m_trackPaths.size() << " background track(s) in " << targetDir.string() << std::endl;
    for (size_t i = 0; i < m_trackPaths.size(); ++i) {
        std::cout << "  [" << (i + 1) << "] " << fs::path(m_trackPaths[i]).filename().string() << std::endl;
    }
}

float SoundManager::getRandomInterval(float minSec, float maxSec) {
    if (minSec > maxSec) std::swap(minSec, maxSec);
    std::uniform_real_distribution<float> dist(minSec, maxSec);
    return dist(m_rng);
}

int SoundManager::pickNextTrackIndex() {
    if (m_trackPaths.empty()) return -1;
    if (m_trackPaths.size() == 1) return 0;

    std::uniform_int_distribution<int> dist(0, static_cast<int>(m_trackPaths.size()) - 1);
    int idx = dist(m_rng);
    if (idx == m_lastTrackIndex) {
        idx = (idx + 1) % static_cast<int>(m_trackPaths.size());
    }
    return idx;
}

void SoundManager::playRandomTrack() {
    if (!m_initialized || !m_enabled || m_trackPaths.empty()) {
        return;
    }

    if (m_hasCurrentMusic) {
        StopMusicStream(m_currentMusic);
        UnloadMusicStream(m_currentMusic);
        m_hasCurrentMusic = false;
    }

    int idx = pickNextTrackIndex();
    if (idx < 0) return;

    m_lastTrackIndex = idx;
    m_currentTrackPath = m_trackPaths[idx];

    m_currentMusic = LoadMusicStream(m_currentTrackPath.c_str());
    if (m_currentMusic.ctxData == nullptr || m_currentMusic.frameCount == 0) {
        std::cerr << "[AUDIO] Failed to load audio stream from: " << m_currentTrackPath << std::endl;
        m_hasCurrentMusic = false;
        m_state = BgmPlaybackState::Waiting;
        m_intervalTimer = getRandomInterval(m_minInterval, m_maxInterval);
        return;
    }

    m_currentMusic.looping = false; // Do not loop track endlessly; let it end to trigger interval
    m_hasCurrentMusic = true;
    m_state = BgmPlaybackState::Playing;

    m_fadeTimer = 0.0f;
    m_fadeFactor = 0.0f;
    applyCurrentVolume(); // Start at 0 volume for smooth gradual fade-in
    PlayMusicStream(m_currentMusic);

    float len = GetMusicTimeLength(m_currentMusic);
    std::cout << "[AUDIO] Started playing background sound: " 
              << fs::path(m_currentTrackPath).filename().string() 
              << " (" << static_cast<int>(len / 60.0f) << "m " 
              << static_cast<int>(std::fmod(len, 60.0f)) << "s)" << std::endl;
}

void SoundManager::stop() {
    if (m_hasCurrentMusic) {
        StopMusicStream(m_currentMusic);
        UnloadMusicStream(m_currentMusic);
        m_hasCurrentMusic = false;
    }
    m_fadeTimer = 0.0f;
    m_fadeFactor = 0.0f;
    m_currentTrackPath.clear();
    m_state = m_trackPaths.empty() ? BgmPlaybackState::Idle : BgmPlaybackState::Waiting;
    m_intervalTimer = getRandomInterval(m_minInterval, m_maxInterval);
}

void SoundManager::skip() {
    stop();
    playRandomTrack();
}

void SoundManager::setEnabled(bool enabled) {
    if (m_enabled == enabled) return;
    m_enabled = enabled;

    if (!m_enabled) {
        stop();
        m_state = BgmPlaybackState::Idle;
    } else {
        if (!m_trackPaths.empty()) {
            m_state = BgmPlaybackState::Waiting;
            m_intervalTimer = getRandomInterval(2.0f, 6.0f);
        }
    }
}

void SoundManager::applyCurrentVolume() {
    if (m_hasCurrentMusic && m_state == BgmPlaybackState::Playing) {
        float effectiveVol = m_volume * m_fadeFactor * MASTER_VOLUME_SCALE;
        SetMusicVolume(m_currentMusic, effectiveVol);
    }
}

void SoundManager::setVolume(float volume) {
    m_volume = std::clamp(volume, 0.0f, 1.0f);
    applyCurrentVolume();
}

void SoundManager::setIntervalRange(float minSeconds, float maxSeconds) {
    m_minInterval = std::max(1.0f, minSeconds);
    m_maxInterval = std::max(m_minInterval, maxSeconds);
}

void SoundManager::update(float dt) {
    if (!m_initialized || !m_enabled) return;

    if (m_state == BgmPlaybackState::Waiting) {
        m_intervalTimer -= dt;
        if (m_intervalTimer <= 0.0f) {
            playRandomTrack();
        }
    } else if (m_state == BgmPlaybackState::Playing) {
        if (!m_hasCurrentMusic) {
            m_state = BgmPlaybackState::Waiting;
            m_intervalTimer = getRandomInterval(m_minInterval, m_maxInterval);
            return;
        }

        UpdateMusicStream(m_currentMusic);

        // Smooth gradual fade-in using sine curve for natural perceptual volume rise
        m_fadeTimer += dt;
        float fadeIn = (m_fadeDuration > 0.0f) ? std::clamp(m_fadeTimer / m_fadeDuration, 0.0f, 1.0f) : 1.0f;
        float fadeInFactor = std::sin(fadeIn * 1.57079632679f);

        float played = GetMusicTimePlayed(m_currentMusic);
        float total = GetMusicTimeLength(m_currentMusic);

        // Smooth fade-out towards end of track
        float fadeOutFactor = 1.0f;
        if (m_fadeDuration > 0.0f && total > (m_fadeDuration * 2.0f) && played > (total - m_fadeDuration)) {
            float fadeOut = std::clamp((total - played) / m_fadeDuration, 0.0f, 1.0f);
            fadeOutFactor = std::sin(fadeOut * 1.57079632679f);
        }

        m_fadeFactor = std::min(fadeInFactor, fadeOutFactor);
        applyCurrentVolume();

        // Detect completion: stream stopped playing or reached end
        bool finished = !IsMusicStreamPlaying(m_currentMusic) || (total > 0.0f && played >= (total - 0.05f));
        if (finished) {
            StopMusicStream(m_currentMusic);
            UnloadMusicStream(m_currentMusic);
            m_hasCurrentMusic = false;
            m_fadeTimer = 0.0f;
            m_fadeFactor = 0.0f;

            m_state = BgmPlaybackState::Waiting;
            m_intervalTimer = getRandomInterval(m_minInterval, m_maxInterval);
            std::cout << "[AUDIO] Background sound completed. Waiting " 
                      << static_cast<int>(m_intervalTimer) 
                      << "s before next track." << std::endl;
        }
    }
}

std::string SoundManager::getCurrentTrackName() const {
    if (!m_currentTrackPath.empty()) {
        return fs::path(m_currentTrackPath).filename().string();
    }
    return "";
}

float SoundManager::getCurrentTrackTimePlayed() const {
    if (m_hasCurrentMusic) {
        return GetMusicTimePlayed(m_currentMusic);
    }
    return 0.0f;
}

float SoundManager::getCurrentTrackTimeLength() const {
    if (m_hasCurrentMusic) {
        return GetMusicTimeLength(m_currentMusic);
    }
    return 0.0f;
}

void SoundManager::setSfxEnabled(bool enabled) {
    m_sfxEnabled = enabled;
}

void SoundManager::setSfxVolume(float volume) {
    m_sfxVolume = std::clamp(volume, 0.0f, 1.0f);
}

static std::string resolveSoundAsset(const std::string& relPath) {
    const std::vector<std::string> prefixes = {
        "",
        "assets/sounds/",
        "../assets/sounds/",
        std::string(GetApplicationDirectory()) + "assets/sounds/",
        std::string(GetApplicationDirectory()) + "../assets/sounds/",
        "assets/",
        "../assets/",
        std::string(GetApplicationDirectory()) + "assets/",
        std::string(GetApplicationDirectory()) + "../assets/"
    };
    for (const auto& prefix : prefixes) {
        std::string candidate = prefix + relPath;
        std::error_code ec;
        if (fs::exists(candidate, ec) && !fs::is_directory(candidate, ec)) {
            return candidate;
        }
    }
    return "";
}

static std::vector<std::string> findSoundAssetsInDir(const std::string& subDir, const std::string& prefixName) {
    std::vector<std::string> matches;
    const std::vector<std::string> searchBases = {
        "assets/sounds/" + subDir,
        "../assets/sounds/" + subDir,
        std::string(GetApplicationDirectory()) + "assets/sounds/" + subDir,
        std::string(GetApplicationDirectory()) + "../assets/sounds/" + subDir
    };
    fs::path targetDir;
    for (const auto& base : searchBases) {
        std::error_code ec;
        if (fs::exists(base, ec) && fs::is_directory(base, ec)) {
            targetDir = fs::canonical(base, ec);
            if (ec) targetDir = base;
            break;
        }
    }
    if (!targetDir.empty()) {
        std::error_code ec;
        for (const auto& entry : fs::directory_iterator(targetDir, ec)) {
            if (entry.is_regular_file(ec)) {
                std::string fname = entry.path().filename().string();
                if (fname.rfind(prefixName, 0) == 0) { // starts with prefixName
                    std::string ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
                        return static_cast<char>(std::tolower(c));
                    });
                    if (ext == ".wav" || ext == ".ogg" || ext == ".mp3" || ext == ".flac") {
                        matches.push_back(entry.path().string());
                    }
                }
            }
        }
    }
    std::sort(matches.begin(), matches.end());
    return matches;
}

static inline bool isSoundLoaded(const Sound& s) {
    return s.frameCount > 0;
}

void SoundManager::loadSoundEffects() {
    unloadSoundEffects();

    // 1. UI Sounds
    std::string btnPath = resolveSoundAsset("ui/button.wav");
    if (!btnPath.empty()) {
        m_sndButton = LoadSound(btnPath.c_str());
        m_hasSndButton = isSoundLoaded(m_sndButton);
        if (m_hasSndButton) {
            std::cout << "[AUDIO] Loaded UI button sound: " << btnPath << std::endl;
        }
    } else {
        std::cout << "[AUDIO] UI button sound (ui/button.wav) not found." << std::endl;
    }

    std::string incPath = resolveSoundAsset("ui/increment.wav");
    if (!incPath.empty()) {
        m_sndIncrement = LoadSound(incPath.c_str());
        m_hasSndIncrement = isSoundLoaded(m_sndIncrement);
        if (m_hasSndIncrement) {
            std::cout << "[AUDIO] Loaded UI increment sound: " << incPath << std::endl;
        }
    } else {
        std::cout << "[AUDIO] UI increment sound (ui/increment.wav) not found." << std::endl;
    }

    // 2. Game Sounds - Uncover
    std::vector<std::string> uncoverPaths = findSoundAssetsInDir("game", "uncover");
    if (uncoverPaths.empty()) {
        std::string directUncover = resolveSoundAsset("game/uncover1.wav");
        if (!directUncover.empty()) uncoverPaths.push_back(directUncover);
    }
    for (const auto& p : uncoverPaths) {
        Sound s = LoadSound(p.c_str());
        if (isSoundLoaded(s)) {
            m_sndUncovers.push_back(s);
            std::cout << "[AUDIO] Loaded game uncover sound: " << p << std::endl;
        }
    }

    // 3. Game Sounds - Explosions
    std::vector<std::string> explodePaths = findSoundAssetsInDir("game", "explode");
    if (explodePaths.empty()) {
        for (int i = 1; i <= 3; ++i) {
            std::string directExp = resolveSoundAsset("game/explode" + std::to_string(i) + ".wav");
            if (!directExp.empty()) explodePaths.push_back(directExp);
        }
    }
    for (const auto& p : explodePaths) {
        Sound s = LoadSound(p.c_str());
        if (isSoundLoaded(s)) {
            m_sndExplosions.push_back(s);
            std::cout << "[AUDIO] Loaded game explosion sound: " << p << std::endl;
        }
    }

    // 4. Game Sounds - Banana
    std::string bananaPath = resolveSoundAsset("game/banana.wav");
    if (!bananaPath.empty()) {
        m_sndBanana = LoadSound(bananaPath.c_str());
        m_hasSndBanana = isSoundLoaded(m_sndBanana);
        if (m_hasSndBanana) {
            std::cout << "[AUDIO] Loaded game banana sound: " << bananaPath << std::endl;
        }
    } else {
        std::cout << "[AUDIO] Game banana sound (game/banana.wav) not found." << std::endl;
    }

    // 5. Game Sounds - Laser
    std::vector<std::string> laserPaths = findSoundAssetsInDir("game", "laser");
    if (laserPaths.empty()) {
        std::string directLaser = resolveSoundAsset("game/laser1.wav");
        if (!directLaser.empty()) laserPaths.push_back(directLaser);
    }
    for (const auto& p : laserPaths) {
        Sound s = LoadSound(p.c_str());
        if (isSoundLoaded(s)) {
            m_sndLasers.push_back(s);
            std::cout << "[AUDIO] Loaded game laser sound: " << p << std::endl;
        }
    }

    // 6. Game Sounds - Flag
    std::vector<std::string> flagPaths = findSoundAssetsInDir("game", "flag");
    if (flagPaths.empty()) {
        std::string directFlag = resolveSoundAsset("game/flag1.wav");
        if (!directFlag.empty()) flagPaths.push_back(directFlag);
    }
    for (const auto& p : flagPaths) {
        Sound s = LoadSound(p.c_str());
        if (isSoundLoaded(s)) {
            m_sndFlags.push_back(s);
            std::cout << "[AUDIO] Loaded game flag sound: " << p << std::endl;
        }
    }
}

void SoundManager::unloadSoundEffects() {
    if (m_hasSndButton) {
        if (isSoundLoaded(m_sndButton)) UnloadSound(m_sndButton);
        m_hasSndButton = false;
        m_sndButton = {};
    }
    if (m_hasSndIncrement) {
        if (isSoundLoaded(m_sndIncrement)) UnloadSound(m_sndIncrement);
        m_hasSndIncrement = false;
        m_sndIncrement = {};
    }
    for (auto& s : m_sndUncovers) {
        if (isSoundLoaded(s)) UnloadSound(s);
    }
    m_sndUncovers.clear();

    for (auto& s : m_sndExplosions) {
        if (isSoundLoaded(s)) UnloadSound(s);
    }
    m_sndExplosions.clear();

    if (m_hasSndBanana) {
        if (isSoundLoaded(m_sndBanana)) UnloadSound(m_sndBanana);
        m_hasSndBanana = false;
        m_sndBanana = {};
    }

    for (auto& s : m_sndLasers) {
        if (isSoundLoaded(s)) UnloadSound(s);
    }
    m_sndLasers.clear();

    for (auto& s : m_sndFlags) {
        if (isSoundLoaded(s)) UnloadSound(s);
    }
    m_sndFlags.clear();
}

void SoundManager::playButtonSound() {
    if (!m_initialized || !m_sfxEnabled || m_sfxVolume <= 0.0f) return;
    if (m_hasSndButton) {
        SetSoundVolume(m_sndButton, m_sfxVolume * MASTER_VOLUME_SCALE);
        PlaySound(m_sndButton);
    }
}

void SoundManager::playIncrementSound() {
    if (!m_initialized || !m_sfxEnabled || m_sfxVolume <= 0.0f) return;
    if (m_hasSndIncrement) {
        SetSoundVolume(m_sndIncrement, m_sfxVolume * MASTER_VOLUME_SCALE);
        PlaySound(m_sndIncrement);
    }
}

void SoundManager::playUncoverSound() {
    if (!m_initialized || !m_sfxEnabled || m_sfxVolume <= 0.0f || m_sndUncovers.empty()) return;
    size_t idx = 0;
    if (m_sndUncovers.size() > 1) {
        std::uniform_int_distribution<size_t> dist(0, m_sndUncovers.size() - 1);
        idx = dist(m_rng);
    }
    Sound& snd = m_sndUncovers[idx];
    std::uniform_real_distribution<float> pitchDist(0.96f, 1.04f);
    SetSoundPitch(snd, pitchDist(m_rng));
    SetSoundVolume(snd, m_sfxVolume * MASTER_VOLUME_SCALE);
    PlaySound(snd);
}

void SoundManager::playExplosionSound() {
    if (!m_initialized || !m_sfxEnabled || m_sfxVolume <= 0.0f || m_sndExplosions.empty()) return;
    size_t idx = 0;
    if (m_sndExplosions.size() > 1) {
        std::uniform_int_distribution<size_t> dist(0, m_sndExplosions.size() - 1);
        idx = dist(m_rng);
    }
    Sound& snd = m_sndExplosions[idx];
    std::uniform_real_distribution<float> pitchDist(0.92f, 1.08f);
    SetSoundPitch(snd, pitchDist(m_rng));
    SetSoundVolume(snd, m_sfxVolume * MASTER_VOLUME_SCALE);
    PlaySound(snd);
}

void SoundManager::playBananaSound() {
    if (!m_initialized || !m_sfxEnabled || m_sfxVolume <= 0.0f) return;
    if (m_hasSndBanana) {
        SetSoundVolume(m_sndBanana, m_sfxVolume * MASTER_VOLUME_SCALE);
        PlaySound(m_sndBanana);
    }
}

void SoundManager::playLaserSound() {
    if (!m_initialized || !m_sfxEnabled || m_sfxVolume <= 0.0f || m_sndLasers.empty()) return;
    size_t idx = 0;
    if (m_sndLasers.size() > 1) {
        std::uniform_int_distribution<size_t> dist(0, m_sndLasers.size() - 1);
        idx = dist(m_rng);
    }
    Sound& snd = m_sndLasers[idx];
    std::uniform_real_distribution<float> pitchDist(0.95f, 1.05f);
    SetSoundPitch(snd, pitchDist(m_rng));
    SetSoundVolume(snd, m_sfxVolume * MASTER_VOLUME_SCALE * 0.25f);
    PlaySound(snd);
}

void SoundManager::playFlagSound() {
    if (!m_initialized || !m_sfxEnabled || m_sfxVolume <= 0.0f || m_sndFlags.empty()) return;
    size_t idx = 0;
    if (m_sndFlags.size() > 1) {
        std::uniform_int_distribution<size_t> dist(0, m_sndFlags.size() - 1);
        idx = dist(m_rng);
    }
    Sound& snd = m_sndFlags[idx];
    std::uniform_real_distribution<float> pitchDist(0.95f, 1.05f);
    SetSoundPitch(snd, pitchDist(m_rng));
    SetSoundVolume(snd, m_sfxVolume * MASTER_VOLUME_SCALE);
    PlaySound(snd);
}

} // namespace minesweeper::audio
