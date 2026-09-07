#pragma once

#include "raylib.h"
#include <vector>
#include <string>

namespace minesweeper::core {

struct ShipConfig;

struct ShipExhaustParticle {
    Vector2 pos;
    Vector2 vel;
    float life;
    float maxLife;
    float size;
    Color color;
};

// ============================================================================
// Configurable Ship Thruster
// ============================================================================
struct ShipThruster {
    Vector2 offset = { 0.0f, 0.0f };         // Local offset relative to sprite center (texture pixels)
    Vector2 direction = { -1.0f, 0.0f };      // Exhaust plume direction in local space
    float nozzleWidth = 2.0f;                 // Width/diameter of nozzle in texture pixels
    float flameLength = 6.0f;                 // Maximum flame length in texture pixels
    Color outerColor = { 255, 140, 0, 255 };  // Outer plume / flame color
    Color innerColor = { 255, 220, 50, 255 }; // Hot inner core color
};

// ============================================================================
// Base Ship Class
// ============================================================================
class Ship {
public:
    // Core ship attributes:
    float mass = 1.0f;           // Mass determines collision inertia & pushback resistance
    float range = 150.0f;        // Laser reach / interaction distance threshold (default 150px)
    float speed = 600.0f;        // Maximum travel speed (px/s)
    Texture2D texture = { 0 };   // Ship hull texture
    int skinId = 0;              // Skin palette index

    // Physical & simulation state:
    Vector2 position = { 0.0f, 0.0f };
    Vector2 velocity = { 0.0f, 0.0f };
    float angle = 0.0f;
    float collisionRadius = 14.0f; // Bounding circle radius for collision detection
    float scale = 1.8f;
    bool isMoving = false;
    bool isInitialized = false;

    // Configurable thrusters:
    Color thrusterColor = { 255, 179, 0, 255 }; // Unified thruster color across ship
    std::vector<ShipThruster> thrusters;
    void addThruster(Vector2 offset, Vector2 direction, float width, float length, Color outer, Color inner);
    void applyConfig(const ShipConfig& cfg);

    // Visuals & effects:
    std::vector<ShipExhaustParticle> exhaust;
    float emitTimer = 0.0f;

    // Labeling & identification:
    std::string name;
    Color color = WHITE;
    bool isSpeaking = false;

    Ship();
    Ship(float mass, float range, float speed, Texture2D texture = { 0 }, int skinId = 0);
    virtual ~Ship() = default;

    // Physics & simulation step
    virtual void update(float dt);

    // Visual rendering (shadow, thruster flames, hull texture, idle glow, label, voice ring)
    virtual void draw(const char* label = nullptr, Color tint = WHITE, bool speaking = false) const;

    // Position of ship front nose / blaster cannon in world space
    virtual Vector2 getNosePosition() const;

    // Draw trailing exhaust particle embers in world space
    void drawExhaust() const;

    // Reset position and clear velocity/exhaust
    void reset(Vector2 newPos, float newAngle = 0.0f);

    // Bump / collision recoil timer
    float bumpTimer = 0.0f;

    // Resolves pairwise circular collision between two ships with controlled bumper dynamics & mass-proportional impulse
    static bool resolveCollision(Ship& a, Ship& b, float restitution = 0.60f);

protected:
    // Helper to draw thrusters for any ship configuration
    void drawThrusters(Vector2 drawPos, float baseAngle) const;
    void emitThrusterParticles(float dt, float speedRatio);
};

// ============================================================================
// Scout / Player Ship Class
// ============================================================================
class ScoutShip : public Ship {
public:
    Vector2 targetPosition = { 0.0f, 0.0f };
    float targetFollowDistance = 70.0f;  // 70px follow distance
    float slowRadius = 140.0f;
    float maxAccel = 2200.0f;

    ScoutShip();
    ScoutShip(float mass, float range, float speed, Texture2D texture = { 0 }, int skinId = 0);

    // Physics & steering towards a target position with arrival deceleration (player / remote ships)
    void update(Vector2 targetPos, float dt);
    void update(float dt) override;

    void draw(const char* label = nullptr, Color tint = WHITE, bool speaking = false) const override;
    Vector2 getNosePosition() const override;
};

using PlayerShip = ScoutShip;

// ============================================================================
// Merchant Base Ship Class
// ============================================================================
class MerchantShip : public Ship {
public:
    Vector2 anchorPosition = { 0.0f, 0.0f };
    float restAngle = 0.0f;
    float returnAccel = 1200.0f;
    float slowRadius = 90.0f;

    MerchantShip();
    MerchantShip(float mass, float range, float speed, Texture2D texture = { 0 }, int skinId = 0);

    void setAnchor(Vector2 anchor, float anchorAngle = 0.0f);

    // Physics & steering for returning to anchor station with damped deceleration
    void update(float dt) override;
    void updateMerchant(float dt) { update(dt); }
};

// ============================================================================
// Shop Freighter Ship Class (Docked Merchant)
// ============================================================================
class ShopShip : public MerchantShip {
public:
    ShopShip();
    ShopShip(Texture2D texture, Vector2 anchor, const std::string& shipName = "SHOP");
    ShopShip(Texture2D texture, Vector2 anchor, const ShipConfig& config);

    void setupThrusters();
    void update(float dt) override;
    void draw(const char* label = nullptr, Color tint = WHITE, bool speaking = false) const override;
    Vector2 getNosePosition() const override;
};

} // namespace minesweeper::core
