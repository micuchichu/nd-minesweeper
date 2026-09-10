#pragma once

#include <cstdint>
#include <string>
#include <array>
#include <cmath>
#include <algorithm>

namespace minesweeper::core {

// European Roulette constants (37 pockets: 0 to 36)
constexpr int ROULETTE_POCKET_COUNT = 37;

// Pockets in clockwise order starting from Green 0
constexpr std::array<int, ROULETTE_POCKET_COUNT> ROULETTE_NUMBERS = {
    0, 32, 15, 19, 4, 21, 2, 25, 17, 34, 6, 27, 13, 36, 11, 30, 8, 23, 10,
    5, 24, 16, 33, 1, 20, 14, 31, 9, 22, 18, 29, 7, 28, 12, 35, 3, 26
};

// Angle of Green 0 pocket on the unrotated roulette.png texture (measured in degrees)
constexpr float ROULETTE_ZERO_TEXTURE_ANGLE = -64.14f;
constexpr float ROULETTE_POCKET_STEP = 360.0f / static_cast<float>(ROULETTE_POCKET_COUNT);

// Needle pointer is placed at 12 o'clock (-90.0 degrees)
constexpr float ROULETTE_POINTER_ANGLE = -90.0f;

enum class RouletteBetType {
    None,
    Red,
    Black,
    Even,
    Odd,
    Low,         // 1-18
    High,        // 19-36
    Dozen1,      // 1-12
    Dozen2,      // 13-24
    Dozen3,      // 25-36
    GreenZero,   // 0 (Jackpot 36x)
    SingleNumber // 0-36 (36x)
};

enum class RouletteSpinState {
    Idle,
    Spinning,
    Result
};

struct RouletteBet {
    RouletteBetType type = RouletteBetType::None;
    int selectedNumber = 0; // Only relevant for SingleNumber (0-36)
    uint64_t amount = 0;
};

struct RouletteBallState {
    bool active = false;
    float angle = 0.0f;      // Current world angle (deg) relative to wheel center
    float radius = 31.0f;    // Distance from wheel center in local texture pixels (pre-scale)
    float speed = 0.0f;      // Current angular velocity
};

struct RouletteState {
    RouletteSpinState spinState = RouletteSpinState::Idle;
    float spinTimer = 0.0f;
    float spinDuration = 4.5f;
    float startAngle = 0.0f;
    float targetAngle = 0.0f;
    float totalRotation = 0.0f;
    int winningNumber = 0;
    int winningPocketIndex = 0;
    
    RouletteBallState ball;

    RouletteBet activeBet;
    uint64_t lastPayout = 0;
    bool lastWon = false;
    std::string resultMessage;
    bool payoutAwarded = false;
};

// ============================================================================
// Helper Functions
// ============================================================================

inline bool isRouletteRed(int number) {
    switch (number) {
        case 1: case 3: case 5: case 7: case 9: case 12:
        case 14: case 16: case 18: case 19: case 21: case 23:
        case 25: case 27: case 30: case 32: case 34: case 36:
            return true;
        default:
            return false;
    }
}

inline bool isRouletteBlack(int number) {
    return (number > 0 && number <= 36 && !isRouletteRed(number));
}

inline int getPocketIndexForNumber(int number) {
    for (int i = 0; i < ROULETTE_POCKET_COUNT; ++i) {
        if (ROULETTE_NUMBERS[static_cast<size_t>(i)] == number) {
            return i;
        }
    }
    return 0;
}

inline int getNumberForPocketIndex(int index) {
    int idx = index % ROULETTE_POCKET_COUNT;
    if (idx < 0) idx += ROULETTE_POCKET_COUNT;
    return ROULETTE_NUMBERS[static_cast<size_t>(idx)];
}

// Computes the texture-local angle (degrees) for pocket index k
inline float getPocketTextureAngle(int pocketIndex) {
    return ROULETTE_ZERO_TEXTURE_ANGLE + static_cast<float>(pocketIndex) * ROULETTE_POCKET_STEP;
}

// Computes the exact ship angle (degrees, [0, 360)) so pocket for `number` aligns with 12 o'clock pointer (-90 deg)
inline float getTargetAngleForNumber(int number) {
    int pocketIndex = getPocketIndexForNumber(number);
    float texAngle = getPocketTextureAngle(pocketIndex);
    float target = ROULETTE_POINTER_ANGLE - texAngle;
    target = std::fmod(target, 360.0f);
    if (target < 0.0f) target += 360.0f;
    return target;
}

// Computes which pocket is currently under the 12 o'clock pointer given ship rotation angle
inline int getPocketUnderPointer(float shipAngle) {
    // Under pointer (-90 deg), texture angle = -90 - shipAngle
    float texAngle = ROULETTE_POINTER_ANGLE - shipAngle;
    float offsetFromZero = texAngle - ROULETTE_ZERO_TEXTURE_ANGLE;
    offsetFromZero = std::fmod(offsetFromZero, 360.0f);
    if (offsetFromZero < 0.0f) offsetFromZero += 360.0f;
    int idx = static_cast<int>(std::round(offsetFromZero / ROULETTE_POCKET_STEP)) % ROULETTE_POCKET_COUNT;
    return idx;
}

// Calculates payout amount given a bet and the winning number
inline uint64_t evaluateRoulettePayout(RouletteBetType betType, int selectedNumber, uint64_t betAmount, int winningNumber) {
    if (betAmount == 0 || betType == RouletteBetType::None) {
        return 0;
    }

    switch (betType) {
        case RouletteBetType::Red:
            return isRouletteRed(winningNumber) ? (betAmount * 2) : 0;

        case RouletteBetType::Black:
            return isRouletteBlack(winningNumber) ? (betAmount * 2) : 0;

        case RouletteBetType::Even:
            return (winningNumber != 0 && winningNumber % 2 == 0) ? (betAmount * 2) : 0;

        case RouletteBetType::Odd:
            return (winningNumber % 2 == 1) ? (betAmount * 2) : 0;

        case RouletteBetType::Low:
            return (winningNumber >= 1 && winningNumber <= 18) ? (betAmount * 2) : 0;

        case RouletteBetType::High:
            return (winningNumber >= 19 && winningNumber <= 36) ? (betAmount * 2) : 0;

        case RouletteBetType::Dozen1:
            return (winningNumber >= 1 && winningNumber <= 12) ? (betAmount * 3) : 0;

        case RouletteBetType::Dozen2:
            return (winningNumber >= 13 && winningNumber <= 24) ? (betAmount * 3) : 0;

        case RouletteBetType::Dozen3:
            return (winningNumber >= 25 && winningNumber <= 36) ? (betAmount * 3) : 0;

        case RouletteBetType::GreenZero:
            return (winningNumber == 0) ? (betAmount * 36) : 0;

        case RouletteBetType::SingleNumber:
            return (winningNumber == selectedNumber) ? (betAmount * 36) : 0;

        default:
            return 0;
    }
}

} // namespace minesweeper::core
