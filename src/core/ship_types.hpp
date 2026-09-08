#pragma once

#include "raylib.h"

namespace minesweeper::core {

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

} // namespace minesweeper::core
