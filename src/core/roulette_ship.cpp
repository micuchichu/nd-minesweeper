#include "roulette_ship.hpp"
#include "ship_config.hpp"
#include "ui/theme.hpp"
#include <cmath>
#include <algorithm>

namespace minesweeper::core {

// ============================================================================
// Dedicated Roulette Casino Ship Implementation
// ============================================================================

RouletteShip::RouletteShip()
    : MerchantShip(8.0f, 160.0f, 140.0f)
{
    scale = 1.8f;
    collisionRadius = 26.0f;
    name = "roulette";
    color = Color{ 168, 85, 247, 255 };
    setAnchor({ 0.0f, 0.0f }, 90.0f);
    setupThrusters();
}

RouletteShip::RouletteShip(Texture2D tex, Vector2 anchor, const std::string& shipName)
    : MerchantShip(8.0f, 160.0f, 140.0f, tex, 0)
{
    scale = 1.8f;
    name = shipName.empty() ? "roulette" : shipName;
    color = Color{ 168, 85, 247, 255 };
    setAnchor(anchor, 90.0f);
    setupThrusters();
}

RouletteShip::RouletteShip(Texture2D tex, Vector2 anchor, const ShipConfig& config)
    : MerchantShip(config.mass, config.range, config.speed, tex, 0)
{
    name = config.name.empty() ? "roulette" : config.name;
    color = Color{ 168, 85, 247, 255 };
    setAnchor(anchor, 90.0f);
    applyConfig(config);
}

void RouletteShip::setupThrusters() {
    ShipConfig cfg = ShipConfig::createDefault(texture.width, texture.height, name);
    applyConfig(cfg);
}

void RouletteShip::startSpin(int winNum, float duration) {
    if (winNum < 0) {
        winNum = rand() % ROULETTE_POCKET_COUNT;
    }
    roulette.winningNumber = winNum;
    roulette.winningPocketIndex = getPocketIndexForNumber(winNum);
    roulette.spinDuration = (duration >= 0.1f) ? duration : 4.5f;
    roulette.spinTimer = 0.0f;
    roulette.startAngle = angle;
    roulette.targetAngle = getTargetAngleForNumber(winNum);

    // Calculate total rotation: ~6 full revolutions (2160 deg) + delta to target angle
    float curNormalized = std::fmod(roulette.startAngle, 360.0f);
    if (curNormalized < 0.0f) curNormalized += 360.0f;
    float delta = roulette.targetAngle - curNormalized;
    while (delta < 0.0f) delta += 360.0f;
    roulette.totalRotation = (360.0f * 6.0f) + delta;

    roulette.spinState = RouletteSpinState::Spinning;
    roulette.payoutAwarded = false;
    roulette.lastWon = false;
    roulette.lastPayout = 0;
    roulette.resultMessage.clear();

    // Reset alignment timers
    isAligning = false;
    alignTimer = 0.0f;
    resultDisplayTimer = 0.0f;
}

void RouletteShip::alignBackToDock() {
    isAligning = true;
    alignTimer = 0.0f;
    alignStartAngle = angle;

    float curNorm = std::fmod(alignStartAngle, 360.0f);
    if (curNorm < 0.0f) curNorm += 360.0f;
    float diff = 90.0f - curNorm;
    while (diff > 180.0f) diff -= 360.0f;
    while (diff < -180.0f) diff += 360.0f;
    alignTargetAngle = alignStartAngle + diff;
}

void RouletteShip::emitVortexParticles(float dt, float spinIntensity) {
    if (thrusters.empty() || spinIntensity <= 0.01f) return;

    emitTimer += dt * std::clamp(spinIntensity * 2.5f, 0.5f, 3.5f);
    float theta = angle * DEG2RAD;
    float cosA = std::cos(theta);
    float sinA = std::sin(theta);

    while (emitTimer >= 0.015f) {
        emitTimer -= 0.015f;
        if (exhaust.size() >= 220) break;

        for (const auto& th : thrusters) {
            Vector2 worldNozzle = {
                position.x + (th.offset.x * cosA - th.offset.y * sinA) * scale,
                position.y + (th.offset.x * sinA + th.offset.y * cosA) * scale
            };

            float len = std::hypot(th.offset.x, th.offset.y);
            if (len < 0.001f) continue;
            // Tangential exhaust direction for clockwise torque
            Vector2 localExhaustDir = { th.offset.y / len, -th.offset.x / len };

            Vector2 worldDir = {
                localExhaustDir.x * cosA - localExhaustDir.y * sinA,
                localExhaustDir.x * sinA + localExhaustDir.y * cosA
            };
            Vector2 worldPerp = { -worldDir.y, worldDir.x };

            float pSpeed = (70.0f + static_cast<float>(rand() % 50)) * spinIntensity;
            float spread = ((rand() % 100) - 50) * 0.006f;
            Vector2 pVel = {
                (worldDir.x + worldPerp.x * spread) * pSpeed,
                (worldDir.y + worldPerp.y * spread) * pSpeed
            };

            Color col = (rand() % 2 == 0) ? Color{ 168, 85, 247, 255 } : Color{ 106, 75, 191, 255 };
            float sz = th.nozzleWidth * 1.5f;
            exhaust.push_back({ worldNozzle, pVel, 0.35f, 0.35f, sz, col });
        }
    }
}

void RouletteShip::update(float dt) {
    if (roulette.spinState == RouletteSpinState::Spinning) {
        roulette.spinTimer += dt;
        float p = std::clamp(roulette.spinTimer / roulette.spinDuration, 0.0f, 1.0f);

        // Smooth Acceleration Ramp (p in [0, 0.20]) + Deceleration (p in [0.20, 1.0])
        const float p_a = 0.20f;
        const float PI_VAL = 3.14159265358979323846f;
        const float I_total = (p_a * 0.5f) + ((1.0f - p_a) / 3.0f);
        float ease = 0.0f;
        float intensity = 0.0f;

        if (p <= p_a) {
            float normP = p / p_a;
            float integral = 0.5f * (p - (p_a / PI_VAL) * std::sin(PI_VAL * normP));
            ease = integral / I_total;
            intensity = 0.4f + 0.6f * normP;
        } else {
            float u = (1.0f - p) / (1.0f - p_a);
            float u3 = u * u * u;
            float integral = (p_a * 0.5f) + ((1.0f - p_a) / 3.0f) * (1.0f - u3);
            ease = integral / I_total;
            intensity = u * u;
        }
        ease = std::clamp(ease, 0.0f, 1.0f);

        angle = roulette.startAngle + roulette.totalRotation * ease;
        restAngle = angle; // Follow spin angle

        emitVortexParticles(dt, intensity);

        // Ball physics: counter-clockwise orbit, spiral inward, settle into winning pocket
        const float rTrack = 49.0f;
        const float rPocket = 31.0f;
        float oneMinusP = 1.0f - p;
        float ballTotalRot = (360.0f * 10.0f);
        float ballEase = 1.0f - (oneMinusP * oneMinusP * oneMinusP);

        if (p < 0.65f) {
            // Phase 1: High-speed orbit on outer track
            roulette.ball.radius = rTrack;
            roulette.ball.angle = -90.0f + (1.0f - ballEase) * ballTotalRot;
        } else if (p < 0.88f) {
            // Phase 2: Inward spiral drop from rim to pocket ring
            float tDrop = (p - 0.65f) / 0.23f;
            float baseR = rTrack - (rTrack - rPocket) * tDrop;
            float jitter = std::sin(tDrop * 35.0f) * 2.0f * (1.0f - tDrop);
            roulette.ball.radius = baseR + jitter;
            roulette.ball.angle = -90.0f + (1.0f - ballEase) * ballTotalRot;
        } else {
            // Phase 3: Settled in the pocket ring, locks onto the winning pocket
            roulette.ball.radius = rPocket;
            float pocketWorldAngle = angle + getPocketTextureAngle(roulette.winningPocketIndex);
            float tLock = (p - 0.88f) / 0.12f;
            roulette.ball.angle = -90.0f * (1.0f - tLock) + pocketWorldAngle * tLock;
        }

        if (p >= 1.0f) {
            angle = roulette.targetAngle;
            restAngle = angle;
            roulette.spinState = RouletteSpinState::Result;
            resultDisplayTimer = 0.0f;
            isAligning = false;
            roulette.ball.radius = rPocket;
            roulette.ball.angle = angle + getPocketTextureAngle(roulette.winningPocketIndex);

            roulette.lastPayout = evaluateRoulettePayout(
                roulette.activeBet.type,
                roulette.activeBet.selectedNumber,
                roulette.activeBet.amount,
                roulette.winningNumber
            );
            roulette.lastWon = (roulette.lastPayout > 0);
            roulette.payoutAwarded = false;

            std::string colName = (roulette.winningNumber == 0) ? "GREEN" : (isRouletteRed(roulette.winningNumber) ? "RED" : "BLACK");
            if (roulette.lastWon) {
                roulette.resultMessage = "WINNER! +" + std::to_string(roulette.lastPayout) + " SCRAP [" + std::to_string(roulette.winningNumber) + " " + colName + "]";
            } else {
                roulette.resultMessage = "NO WIN: [" + std::to_string(roulette.winningNumber) + " " + colName + "]";
            }
        }
    } else if (isAligning) {
        alignTimer += dt;
        float tAlign = std::clamp(alignTimer / alignDuration, 0.0f, 1.0f);
        // Cubic ease out
        float aEase = 1.0f - std::pow(1.0f - tAlign, 3.0f);
        angle = alignStartAngle + (alignTargetAngle - alignStartAngle) * aEase;
        restAngle = angle;

        // Ball remains locked in the winning pocket and rotates smoothly with the wheel
        roulette.ball.radius = 31.0f;
        roulette.ball.angle = angle + getPocketTextureAngle(roulette.winningPocketIndex);

        if (tAlign >= 1.0f) {
            angle = 90.0f;
            restAngle = 90.0f;
            isAligning = false;
            roulette.spinState = RouletteSpinState::Idle;
        }
    } else if (roulette.spinState == RouletteSpinState::Result) {
        // Result state: ball sits in winning pocket and rotates with the wheel
        roulette.ball.radius = 31.0f;
        roulette.ball.angle = angle + getPocketTextureAngle(roulette.winningPocketIndex);

        resultDisplayTimer += dt;
        if (resultDisplayTimer >= resultDisplayDuration) {
            alignBackToDock();
        }
    } else if (roulette.spinState == RouletteSpinState::Idle) {
        roulette.ball.radius = 31.0f;
        roulette.ball.angle = angle + getPocketTextureAngle(roulette.winningPocketIndex);
    }

    MerchantShip::update(dt);

    float curSpeed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
    if (curSpeed > 15.0f) {
        emitThrusterParticles(dt, curSpeed / speed);
    }
}

void RouletteShip::draw(const char* label, Color tint, bool speaking) const {
    (void)tint;
    (void)speaking;
    if (texture.id == 0) {
        Ship::draw(label, color, false);
        return;
    }

    Rectangle src = { 0.0f, 0.0f, static_cast<float>(texture.width), static_cast<float>(texture.height) };
    float w = static_cast<float>(texture.width) * scale;
    float h = static_cast<float>(texture.height) * scale;
    Vector2 origin = { w * 0.5f, h * 0.5f };

    float hoverY = (!isMoving && roulette.spinState != RouletteSpinState::Spinning && !isAligning)
        ? (std::sin(static_cast<float>(GetTime()) * 1.8f) * 2.0f) : 0.0f;
    Vector2 drawPos = { position.x, position.y + hoverY };

    // 1. Hovering Drop Shadow
    Vector2 shadowOffset = { 3.5f * scale, 5.0f * scale };
    Rectangle shadowOuter = { drawPos.x + shadowOffset.x, drawPos.y + shadowOffset.y, w * 1.05f, h * 1.05f };
    Vector2 originOuter = { shadowOuter.width * 0.5f, shadowOuter.height * 0.5f };
    DrawTexturePro(texture, src, shadowOuter, originOuter, angle, Fade(BLACK, 0.20f));
    Rectangle shadowDest = { drawPos.x + shadowOffset.x, drawPos.y + shadowOffset.y, w, h };
    DrawTexturePro(texture, src, shadowDest, origin, angle, Fade(BLACK, 0.40f));

    // 2. Multi-Nozzle Thrusters
    drawThrusters(drawPos, angle);

    // 3. Ship Sprite
    DrawTexturePro(texture, src, { drawPos.x, drawPos.y, w, h }, origin, angle, WHITE);

    // 4. Roulette 12 O'Clock Indicator Needle & Ball
    float rimRadius = (static_cast<float>(texture.width) * 0.5f - 4.0f) * scale;
    Vector2 needleTip = { drawPos.x, drawPos.y - rimRadius + 6.0f * scale };
    Vector2 needleBaseL = { drawPos.x - 7.0f * scale, drawPos.y - rimRadius - 10.0f * scale };
    Vector2 needleBaseR = { drawPos.x + 7.0f * scale, drawPos.y - rimRadius - 10.0f * scale };

    // Outer needle border
    DrawTriangle(
        { needleTip.x, needleTip.y + 1.5f },
        { needleBaseL.x - 1.5f, needleBaseL.y - 1.5f },
        { needleBaseR.x + 1.5f, needleBaseR.y - 1.5f },
        ui::Colors::Zinc950
    );

    // Vibrant gold needle fill
    DrawTriangle(needleTip, needleBaseL, needleBaseR, ui::Colors::Amber400);

    // Highlight center line
    DrawLineEx({ drawPos.x, needleBaseL.y }, needleTip, 1.5f, WHITE);

    // Glowing indicator bracket at top
    DrawCircleV({ drawPos.x, drawPos.y - rimRadius - 10.0f * scale }, 4.0f * scale, Color{ 168, 85, 247, 255 });
    DrawCircleV({ drawPos.x, drawPos.y - rimRadius - 10.0f * scale }, 2.0f * scale, WHITE);

    // 4b. Draw Metallic Roulette Ball
    float bRad = roulette.ball.angle * DEG2RAD;
    float bDist = roulette.ball.radius * scale;
    Vector2 ballPos = { drawPos.x + std::cos(bRad) * bDist, drawPos.y + std::sin(bRad) * bDist };

    // Drop shadow under ball
    DrawCircleV({ ballPos.x + 1.5f * scale, ballPos.y + 2.0f * scale }, 3.2f * scale, Fade(BLACK, 0.45f));
    // Ball chrome body
    DrawCircleV(ballPos, 3.0f * scale, Color{ 230, 235, 245, 255 });
    // Edge shading
    DrawCircleLinesV(ballPos, 3.0f * scale, Color{ 148, 163, 184, 255 });
    // Specular highlight
    DrawCircleV({ ballPos.x - 1.0f * scale, ballPos.y - 1.0f * scale }, 1.2f * scale, WHITE);

    // 5. Floating Badge
    const char* displayName = (label && label[0] != '\0') ? label : "CASINO ROULETTE";
    int nameW = MeasureText(displayName, 12);
    float rad = angle * DEG2RAD;
    float halfExtentY = (std::abs(w * std::sin(rad)) + std::abs(h * std::cos(rad))) * 0.5f;
    float badgeY = drawPos.y + halfExtentY + 6.0f;
    Rectangle badge = { drawPos.x - static_cast<float>(nameW + 16) * 0.5f, badgeY, static_cast<float>(nameW + 16), 16.0f };
    DrawRectangleRec(badge, Fade(BLACK, 0.85f));
    DrawRectangleLinesEx(badge, 1.0f, Color{ 168, 85, 247, 255 });
    DrawText(displayName, static_cast<int>(badge.x + 8), static_cast<int>(badge.y + 2), 12, Color{ 216, 180, 254, 255 });
}

Vector2 RouletteShip::getNosePosition() const {
    float theta = angle * DEG2RAD;
    float sinA = std::sin(theta);
    float cosA = std::cos(theta);
    float noseOffset = (texture.width > 0) ? (static_cast<float>(texture.width) * 0.5f - 4.0f) : 28.0f;
    return {
        position.x + cosA * (noseOffset * scale),
        position.y + sinA * (noseOffset * scale)
    };
}

} // namespace minesweeper::core
