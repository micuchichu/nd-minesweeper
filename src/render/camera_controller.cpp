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
    manualPanActive = false;
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

    // Pan with middle mouse button
    if (IsMouseButtonPressed(MOUSE_MIDDLE_BUTTON)) {
        middleDragDistance = 0.0f;
    }
    if (IsMouseButtonDown(MOUSE_MIDDLE_BUTTON)) {
        Vector2 delta = GetMouseDelta();
        middleDragDistance += std::abs(delta.x) + std::abs(delta.y);
        if (middleDragDistance > 5.0f) {
            manualPanActive = true;
        }
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

void CameraController::centerOn(Vector2 worldPos, float customScreenW, float customScreenH) {
    float screenW = (customScreenW > 0.0f) ? customScreenW : static_cast<float>(GetScreenWidth());
    float screenH = (customScreenH > 0.0f) ? customScreenH : static_cast<float>(GetScreenHeight());
    if (screenW <= 0.0f) screenW = 1280.0f;
    if (screenH <= 0.0f) screenH = 720.0f;
    camera.target.x = worldPos.x - (screenW * 0.5f) / camera.zoom;
    camera.target.y = worldPos.y - (screenH * 0.5f) / camera.zoom;
    manualPanActive = false;
}

void CameraController::followShip(Vector2 shipWorldPos, float dt, float customScreenW, float customScreenH) {
    if (!enableEdgeFollow) return;
    if (dt <= 0.0f) return;
    if (dt > 0.1f) dt = 0.1f;

    // Do not auto-follow while player is actively holding middle click
    if (IsMouseButtonDown(MOUSE_MIDDLE_BUTTON)) {
        if (middleDragDistance > 5.0f) {
            manualPanActive = true;
        }
        return;
    }

    float screenW = (customScreenW > 0.0f) ? customScreenW : static_cast<float>(GetScreenWidth());
    float screenH = (customScreenH > 0.0f) ? customScreenH : static_cast<float>(GetScreenHeight());
    if (screenW <= 0.0f || screenH <= 0.0f) return;

    Vector2 shipScreen = getWorldToScreen(shipWorldPos);

    // If the user manually dragged the camera away, hold off until the ship is on-screen again
    if (manualPanActive) {
        if (shipScreen.x >= 0.0f && shipScreen.x <= screenW &&
            shipScreen.y >= 0.0f && shipScreen.y <= screenH) {
            manualPanActive = false;
        } else {
            return;
        }
    }

    float marginX = screenW * std::clamp(edgeMarginRatio, 0.05f, 0.45f);
    float marginY = screenH * std::clamp(edgeMarginRatio, 0.05f, 0.45f);

    float minX = marginX;
    float maxX = screenW - marginX;
    float minY = marginY;
    float maxY = screenH - marginY;

    Vector2 pushScreen = { 0.0f, 0.0f };

    if (shipScreen.x < minX) {
        pushScreen.x = shipScreen.x - minX;
    } else if (shipScreen.x > maxX) {
        pushScreen.x = shipScreen.x - maxX;
    }

    if (shipScreen.y < minY) {
        pushScreen.y = shipScreen.y - minY;
    } else if (shipScreen.y > maxY) {
        pushScreen.y = shipScreen.y - maxY;
    }

    if (pushScreen.x == 0.0f && pushScreen.y == 0.0f) {
        return;
    }

    Vector2 pushWorld = {
        pushScreen.x / camera.zoom,
        pushScreen.y / camera.zoom
    };

    float penDist = std::sqrt(pushScreen.x * pushScreen.x + pushScreen.y * pushScreen.y);
    float refMargin = std::min(marginX, marginY);
    float speedMultiplier = 1.0f;
    if (refMargin > 0.0f && penDist > refMargin * 0.5f) {
        float extra = (penDist - refMargin * 0.5f) / refMargin;
        speedMultiplier += extra * 1.5f;
    }

    float effectiveSpeed = std::min(24.0f, followSpeed * speedMultiplier);
    float blend = 1.0f - std::exp(-effectiveSpeed * dt);

    camera.target.x += pushWorld.x * blend;
    camera.target.y += pushWorld.y * blend;
}

} // namespace minesweeper::render
