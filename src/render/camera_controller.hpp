#pragma once

#include "raylib.h"
#include "raymath.h"

namespace minesweeper::render {

class CameraController {
public:
    Camera2D camera = {0};
    bool enableCRT = true;

    CameraController();

    void reset(Vector2 targetPos = {0, 0}, float zoom = 1.0f);
    void handleInput(bool allowPanAndZoom = true);

    Vector2 getCRTMousePosition() const;
    Vector2 getScreenToWorld(Vector2 screenPos) const;
    Vector2 getWorldToScreen(Vector2 worldPos) const;

    float getZoom() const { return camera.zoom; }
    void setZoom(float z) { camera.zoom = z; }

    float middleDragDistance = 0.0f;
    bool isMiddleDragging() const { return middleDragDistance > 5.0f; }

    bool enableEdgeFollow = true;
    bool manualPanActive = false;
    float edgeMarginRatio = 0.22f;
    float followSpeed = 6.5f;

    void followShip(Vector2 shipWorldPos, float dt, float customScreenW = 0.0f, float customScreenH = 0.0f);
    void centerOn(Vector2 worldPos, float customScreenW = 0.0f, float customScreenH = 0.0f);
};

} // namespace minesweeper::render
