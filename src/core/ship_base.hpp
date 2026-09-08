#pragma once

#include "raylib.h"
#include "ship_types.hpp"
#include <vector>
#include <string>

namespace minesweeper::core {

struct ShipConfig;

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
    float collisionRadius = 14.0f; // Bounding circle / capsule cap radius for collision detection
    float capsuleLength = 0.0f;    // Length of central focal segment for 2D capsule collision (0 for circular ships)
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

    // Shared procedural textures for batched exhaust and thruster glow
    static Texture2D sharedExhaustTexture;
    static Texture2D sharedGlowTexture;
    static void setSharedTextures(Texture2D exhaustTex, Texture2D glowTex);

    // Reset position and clear velocity/exhaust
    void reset(Vector2 newPos, float newAngle = 0.0f);

    // Bump / collision recoil timer
    float bumpTimer = 0.0f;
    bool bumpable = true; // True for player ships (bumper-cars), false for shop freighters (pushable without bumper kick)

    // Calculate world-space segment endpoints for capsule collision
    void getCapsuleSegment(Vector2& outA, Vector2& outB) const;

    // Minimum distance and closest points between two line segments S1:[p1, q1] and S2:[p2, q2]
    static float segmentToSegmentDist(Vector2 p1, Vector2 q1, Vector2 p2, Vector2 q2, Vector2& outC1, Vector2& outC2);

    // Update exhaust particle physics and lifetimes
    void updateExhaust(float dt);
    void emitThrusterParticles(float dt, float speedRatio);

    // Resolves pairwise collision (capsule/circle) between two ships with controlled bumper dynamics & mass-proportional impulse
    static bool resolveCollision(Ship& a, Ship& b, float restitution = 0.60f);

protected:
    // Helper to draw thrusters for any ship configuration
    void drawThrusters(Vector2 drawPos, float baseAngle) const;
};

} // namespace minesweeper::core
