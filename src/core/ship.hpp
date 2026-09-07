#pragma once

#include "raylib.h"
#include <vector>
#include <string>

namespace minesweeper::core {

struct ShipExhaustParticle {
    Vector2 pos;
    Vector2 vel;
    float life;
    float maxLife;
    float size;
    Color color;
};

class Ship {
public:
    // Core ship attributes requested:
    float mass = 1.0f;           // Mass determines collision inertia & pushback resistance
    float range = 140.0f;        // Laser reach / interaction distance threshold (default 140px)
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

    // Visuals & effects:
    std::vector<ShipExhaustParticle> exhaust;
    float emitTimer = 0.0f;

    // Labeling & identification:
    std::string name;
    Color color = WHITE;
    bool isSpeaking = false;

    // Merchant & anchoring properties:
    bool isMerchant = false;
    Vector2 anchorPosition = { 0.0f, 0.0f };
    float restAngle = 0.0f;

    Ship();
    Ship(float mass, float range, float speed, Texture2D texture = {0}, int skinId = 0);

    // Physics & steering towards a target position with arrival deceleration (player / remote ships)
    void update(Vector2 targetPos, float dt);

    // Physics & steering for merchant ship returning to anchor position with damped deceleration
    void updateMerchant(float dt);

    // Resolves pairwise circular collision between two ships with mass-proportional separation & impulse
    static bool resolveCollision(Ship& a, Ship& b, float restitution = 0.15f);

    // Visual rendering (shadow, thruster flames, hull texture, idle glow, label, voice ring)
    void draw(const char* label = nullptr, Color tint = WHITE, bool speaking = false) const;

    // Draw trailing exhaust particle embers in world space
    void drawExhaust() const;

    // Get position of ship front nose / blaster cannon in world space
    Vector2 getNosePosition() const;

    // Reset position and clear velocity/exhaust
    void reset(Vector2 newPos, float newAngle = 0.0f);
};

} // namespace minesweeper::core
