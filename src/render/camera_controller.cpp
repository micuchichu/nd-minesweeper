#include "camera_controller.hpp"
#include <algorithm>
#include <cmath>

namespace minesweeper::render {

CameraController::CameraController() {
    reset();
}

void CameraController::reset(Vector2 targetPos, float zoom) {
    camera.offset = {0, 0};
    camera.target = targetPos;
    camera.rotation = 0.0f;
    camera.zoom = std::clamp(zoom, 0.01f, 30.0f);
}

Vector2 CameraController::getCRTMousePosition() const {
    Vector2 mouse = GetMousePosition();
    if (!enableCRT) return mouse;

    float screenW = static_cast<float>(GetScreenWidth());
    float screenH = static_cast<float>(GetScreenHeight());
    if (screenW <= 0.0f || screenH <= 0.0f) return mouse;

    Vector2 centered = { (mouse.x / screenW) - 0.5f, (mouse.y / screenH) - 0.5f };
    float r2 = (centered.x * centered.x) + (centered.y * centered.y);

    return {
        (mouse.x / screenW + centered.x * (r2 * 0.05f)) * screenW,
        (mouse.y / screenH + centered.y * (r2 * 0.05f)) * screenH
    };
}

Vector2 CameraController::getScreenToWorld(Vector2 screenPos) const {
    return GetScreenToWorld2D(screenPos, camera);
}

Vector2 CameraController::getWorldToScreen(Vector2 worldPos) const {
    return GetWorldToScreen2D(worldPos, camera);
}

void CameraController::handleInput(bool allowPanAndZoom) {
    if (!allowPanAndZoom) return;

    // Pan with middle mouse button or right mouse button while holding space/ctrl
    if (IsMouseButtonDown(MOUSE_MIDDLE_BUTTON)) {
        Vector2 delta = GetMouseDelta();
        camera.target.x -= delta.x / camera.zoom;
        camera.target.y -= delta.y / camera.zoom;
    }

    // Zoom with mouse wheel towards cursor position
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        float zoomFactor = 1.15f;
        if (wheel < 0.0f) zoomFactor = 1.0f / zoomFactor;

        Vector2 mouseWorldBefore = getScreenToWorld(getCRTMousePosition());
        camera.zoom = std::clamp(camera.zoom * zoomFactor, 0.02f, 20.0f);
        Vector2 mouseWorldAfter = getScreenToWorld(getCRTMousePosition());

        camera.target.x += mouseWorldBefore.x - mouseWorldAfter.x;
        camera.target.y += mouseWorldBefore.y - mouseWorldAfter.y;
    }
}

} // namespace minesweeper::render
