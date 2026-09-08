#include "app.hpp"
#include "core/ship.hpp"
#include "core/ship_config.hpp"
#include "render/camera_controller.hpp"
#include <string>
#include <iostream>
#include <cassert>
#include <cmath>

static int runCapsuleTests() {
    std::cout << "[TEST-CAPSULE] Starting 2D Capsule Collision Unit Tests..." << std::endl;

    // Test 1: Circle vs Circle (degenerate segments)
    {
        minesweeper::core::Ship a;
        a.position = { 0.0f, 0.0f };
        a.collisionRadius = 14.0f;
        a.capsuleLength = 0.0f;
        a.isInitialized = true;

        minesweeper::core::Ship b;
        b.position = { 20.0f, 0.0f };
        b.collisionRadius = 14.0f;
        b.capsuleLength = 0.0f;
        b.isInitialized = true;

        Vector2 pA1, pA2, pB1, pB2, cA, cB;
        a.getCapsuleSegment(pA1, pA2);
        b.getCapsuleSegment(pB1, pB2);
        if (pA1.x != 0.0f || pA1.y != 0.0f || pA2.x != 0.0f || pA2.y != 0.0f) return 1;
        if (pB1.x != 20.0f || pB1.y != 0.0f || pB2.x != 20.0f || pB2.y != 0.0f) return 1;

        float dist = minesweeper::core::Ship::segmentToSegmentDist(pA1, pA2, pB1, pB2, cA, cB);
        if (std::abs(dist - 20.0f) >= 0.001f) return 1;
        if (std::abs(cA.x - 0.0f) >= 0.001f || std::abs(cA.y - 0.0f) >= 0.001f) return 1;
        if (std::abs(cB.x - 20.0f) >= 0.001f || std::abs(cB.y - 0.0f) >= 0.001f) return 1;
        std::cout << "  [PASS] Test 1: Circle-Circle degenerate segment distance verified." << std::endl;
    }

    // Test 2: Horizontal Capsule vs Circle (Big Shop vs Player)
    {
        minesweeper::core::Ship shop; // Capsule
        shop.position = { 100.0f, 100.0f };
        shop.angle = 0.0f;
        shop.capsuleLength = 130.0f;
        shop.collisionRadius = 28.0f;
        shop.bumpable = false;
        shop.isInitialized = true;

        Vector2 p1, p2;
        shop.getCapsuleSegment(p1, p2);
        if (std::abs(p1.x - 35.0f) >= 0.001f || std::abs(p1.y - 100.0f) >= 0.001f) return 2;
        if (std::abs(p2.x - 165.0f) >= 0.001f || std::abs(p2.y - 100.0f) >= 0.001f) return 2;

        minesweeper::core::Ship player;
        player.collisionRadius = 14.0f;
        player.capsuleLength = 0.0f;
        player.isInitialized = true;
        player.position = { 100.0f, 130.0f };

        bool hitCenter = minesweeper::core::Ship::resolveCollision(player, shop);
        if (!hitCenter) return 3;
        std::cout << "  [PASS] Test 2a: Capsule center side collision resolved." << std::endl;

        shop.position = { 100.0f, 100.0f };
        shop.velocity = { 0.0f, 0.0f };
        player.position = { 185.0f, 100.0f };
        player.velocity = { 0.0f, 0.0f };
        bool hitNose = minesweeper::core::Ship::resolveCollision(player, shop);
        if (!hitNose) return 4;
        std::cout << "  [PASS] Test 2b: Capsule front nose cap collision resolved." << std::endl;

        shop.position = { 100.0f, 100.0f };
        shop.velocity = { 0.0f, 0.0f };
        player.position = { 15.0f, 100.0f };
        player.velocity = { 0.0f, 0.0f };
        bool hitStern = minesweeper::core::Ship::resolveCollision(player, shop);
        if (!hitStern) return 5;
        std::cout << "  [PASS] Test 2c: Capsule rear stern cap collision resolved." << std::endl;

        shop.position = { 100.0f, 100.0f };
        shop.velocity = { 0.0f, 0.0f };
        player.position = { 100.0f, 150.0f };
        player.velocity = { 0.0f, 0.0f };
        bool hitVoid = minesweeper::core::Ship::resolveCollision(player, shop);
        if (hitVoid) return 6;
        std::cout << "  [PASS] Test 2d: Outside capsule hull correctly registers NO collision." << std::endl;
    }

    // Test 3: Rotated Capsule (90 degrees)
    {
        minesweeper::core::Ship shop;
        shop.position = { 200.0f, 200.0f };
        shop.angle = 90.0f;
        shop.capsuleLength = 100.0f;
        shop.collisionRadius = 25.0f;
        shop.bumpable = false;
        shop.isInitialized = true;

        Vector2 p1, p2;
        shop.getCapsuleSegment(p1, p2);
        if (std::abs(p1.x - 200.0f) >= 0.01f || std::abs(p1.y - 150.0f) >= 0.01f) return 7;
        if (std::abs(p2.x - 200.0f) >= 0.01f || std::abs(p2.y - 250.0f) >= 0.01f) return 7;

        minesweeper::core::Ship player;
        player.collisionRadius = 15.0f;
        player.capsuleLength = 0.0f;
        player.isInitialized = true;

        player.position = { 225.0f, 200.0f };
        bool hitRotatedSide = minesweeper::core::Ship::resolveCollision(player, shop);
        if (!hitRotatedSide) return 8;
        std::cout << "  [PASS] Test 3: Rotated 90-degree capsule collision verified." << std::endl;
    }

    // Test 4: Capsule vs Capsule Collision
    {
        minesweeper::core::Ship capA;
        capA.position = { 100.0f, 100.0f };
        capA.angle = 0.0f;
        capA.capsuleLength = 80.0f;
        capA.collisionRadius = 20.0f;
        capA.isInitialized = true;

        minesweeper::core::Ship capB;
        capB.position = { 100.0f, 130.0f };
        capB.angle = 0.0f;
        capB.capsuleLength = 80.0f;
        capB.collisionRadius = 20.0f;
        capB.isInitialized = true;

        bool hitCapsules = minesweeper::core::Ship::resolveCollision(capA, capB);
        if (!hitCapsules) return 9;
        std::cout << "  [PASS] Test 4: Capsule-to-Capsule collision resolved." << std::endl;
    }

    std::cout << "[TEST-CAPSULE] ALL 2D CAPSULE TESTS PASSED!" << std::endl;
    return 0;
}

static int runCameraTests() {
    std::cout << "[TEST-CAMERA] Starting Smooth Camera Edge Follow Tests..." << std::endl;

    const float sw = 1280.0f;
    const float sh = 720.0f;

    // Test 1: Deadzone check (ship in center of screen produces no camera movement)
    {
        minesweeper::render::CameraController cc;
        cc.reset({ 0.0f, 0.0f }, 1.0f);
        Vector2 shipPos = { 640.0f, 360.0f };
        Vector2 targetBefore = cc.camera.target;
        cc.followShip(shipPos, 0.016f, sw, sh);
        if (std::abs(cc.camera.target.x - targetBefore.x) > 0.0001f ||
            std::abs(cc.camera.target.y - targetBefore.y) > 0.0001f) {
            std::cerr << "  [FAIL] Test 1: Center deadzone caused unexpected camera movement!" << std::endl;
            return 1;
        }
        std::cout << "  [PASS] Test 1: Center deadzone produces zero camera movement." << std::endl;
    }

    // Test 2: Right edge penetration causes camera to smoothly pan right
    {
        minesweeper::render::CameraController cc;
        cc.reset({ 0.0f, 0.0f }, 1.0f);
        Vector2 shipPos = { 1200.0f, 360.0f };
        cc.followShip(shipPos, 0.016f, sw, sh);
        if (cc.camera.target.x <= 0.0f) {
            std::cerr << "  [FAIL] Test 2: Right edge penetration did not pan camera right!" << std::endl;
            return 2;
        }
        std::cout << "  [PASS] Test 2: Right edge penetration smoothly pans camera right." << std::endl;
    }

    // Test 3: Left edge penetration causes camera to smoothly pan left
    {
        minesweeper::render::CameraController cc;
        cc.reset({ 500.0f, 0.0f }, 1.0f);
        Vector2 shipPos = { 600.0f, 360.0f };
        cc.followShip(shipPos, 0.016f, sw, sh);
        if (cc.camera.target.x >= 500.0f) {
            std::cerr << "  [FAIL] Test 3: Left edge penetration did not pan camera left!" << std::endl;
            return 3;
        }
        std::cout << "  [PASS] Test 3: Left edge penetration smoothly pans camera left." << std::endl;
    }

    // Test 4: centerOn precisely centers worldPos on screen
    {
        minesweeper::render::CameraController cc;
        cc.reset({ 0.0f, 0.0f }, 2.0f);
        Vector2 worldPos = { 300.0f, 400.0f };
        cc.centerOn(worldPos, sw, sh);
        Vector2 screenPos = cc.getWorldToScreen(worldPos);
        if (std::abs(screenPos.x - sw * 0.5f) > 0.01f ||
            std::abs(screenPos.y - sh * 0.5f) > 0.01f) {
            std::cerr << "  [FAIL] Test 4: centerOn did not place worldPos at exact screen center!" << std::endl;
            return 4;
        }
        std::cout << "  [PASS] Test 4: centerOn precisely centers world coordinate." << std::endl;
    }

    // Test 5: Manual pan suspension prevents follow until ship re-enters screen
    {
        minesweeper::render::CameraController cc;
        cc.reset({ 0.0f, 0.0f }, 1.0f);
        cc.manualPanActive = true;
        Vector2 shipPos = { -500.0f, 360.0f };
        Vector2 targetBefore = cc.camera.target;
        cc.followShip(shipPos, 0.016f, sw, sh);
        if (cc.camera.target.x != targetBefore.x) {
            std::cerr << "  [FAIL] Test 5: manualPanActive allowed camera to follow off-screen ship!" << std::endl;
            return 5;
        }
        Vector2 shipOnScreen = { 100.0f, 360.0f };
        cc.followShip(shipOnScreen, 0.016f, sw, sh);
        if (cc.manualPanActive) {
            std::cerr << "  [FAIL] Test 5: manualPanActive did not reset after ship returned to screen!" << std::endl;
            return 5;
        }
        std::cout << "  [PASS] Test 5: Manual pan suspension and viewport re-entry verified." << std::endl;
    }

    std::cout << "[TEST-CAMERA] ALL CAMERA TESTS PASSED!" << std::endl;
    return 0;
}

static int runControlTests() {
    std::cout << "[TEST-CONTROLS] Starting Controls & Keyboard/Controller Unit Tests..." << std::endl;

    // Test 1: ScoutShip direct movement acceleration
    {
        minesweeper::core::ScoutShip ship;
        ship.position = { 100.0f, 100.0f };
        ship.velocity = { 0.0f, 0.0f };
        ship.angle = 0.0f;
        ship.isInitialized = true;

        // Move right with full input
        ship.updateDirect({ 1.0f, 0.0f }, 0.05f);
        if (ship.velocity.x <= 0.0f) {
            std::cerr << "  [FAIL] Test 1: Ship velocity.x did not accelerate positively on right input!" << std::endl;
            return 1;
        }
        if (ship.position.x <= 100.0f) {
            std::cerr << "  [FAIL] Test 1: Ship position.x did not advance on right input!" << std::endl;
            return 2;
        }
        if (!ship.isMoving) {
            std::cerr << "  [FAIL] Test 1: Ship isMoving is false during movement input!" << std::endl;
            return 3;
        }
        std::cout << "  [PASS] Test 1: ScoutShip direct movement acceleration verified." << std::endl;
    }

    // Test 2: ScoutShip space braking when input is released
    {
        minesweeper::core::ScoutShip ship;
        ship.position = { 100.0f, 100.0f };
        ship.velocity = { 300.0f, 0.0f };
        ship.isInitialized = true;

        float prevVelX = ship.velocity.x;
        ship.updateDirect({ 0.0f, 0.0f }, 0.05f);
        if (ship.velocity.x >= prevVelX) {
            std::cerr << "  [FAIL] Test 2: Ship did not decelerate when input was neutral!" << std::endl;
            return 4;
        }

        // Run multiple frames to verify complete stop
        for (int i = 0; i < 60; ++i) {
            ship.updateDirect({ 0.0f, 0.0f }, 0.05f);
        }
        if (ship.velocity.x != 0.0f || ship.velocity.y != 0.0f) {
            std::cerr << "  [FAIL] Test 2: Ship did not come to a complete stop after braking!" << std::endl;
            return 5;
        }
        if (ship.isMoving) {
            std::cerr << "  [FAIL] Test 2: Ship isMoving is true when stopped!" << std::endl;
            return 6;
        }
        std::cout << "  [PASS] Test 2: ScoutShip active braking and full stop verified." << std::endl;
    }

    // Test 3: ScoutShip aim override
    {
        minesweeper::core::ScoutShip ship;
        ship.position = { 100.0f, 100.0f };
        ship.velocity = { 0.0f, 0.0f };
        ship.angle = 0.0f;
        ship.isInitialized = true;

        // Move right with stick aim pointing 180 degrees (down)
        for (int i = 0; i < 20; ++i) {
            ship.updateDirect({ 1.0f, 0.0f }, 0.05f, true, 180.0f);
        }
        if (ship.angle <= 45.0f) {
            std::cerr << "  [FAIL] Test 3: Ship angle did not rotate towards aimAngle (180 deg)!" << std::endl;
            return 7;
        }
        std::cout << "  [PASS] Test 3: ScoutShip aimAngle override verified." << std::endl;
    }

    // Test 4: getCellIndexAtWorldPos for 2D, 3D and out of bounds
    {
        minesweeper::render::RaylibRenderer renderer;
        renderer.cellSize = 40.0f;
        renderer.slicePadding = 20.0f;

        // 2D board: 10x10
        minesweeper::core::Board b2d;
        b2d.init(2, 10, 10, 12345);

        int64_t idx0 = renderer.getCellIndexAtWorldPos({ 10.0f, 10.0f }, b2d);
        if (idx0 != 0) {
            std::cerr << "  [FAIL] Test 4a: Expected cell index 0 at (10, 10), got " << idx0 << std::endl;
            return 8;
        }

        int64_t idx1 = renderer.getCellIndexAtWorldPos({ 50.0f, 10.0f }, b2d);
        if (idx1 != 1) {
            std::cerr << "  [FAIL] Test 4b: Expected cell index 1 at (50, 10), got " << idx1 << std::endl;
            return 9;
        }

        int64_t idxOOB = renderer.getCellIndexAtWorldPos({ -10.0f, -10.0f }, b2d);
        if (idxOOB != -1) {
            std::cerr << "  [FAIL] Test 4c: Expected -1 for out of bounds coordinate, got " << idxOOB << std::endl;
            return 10;
        }

        int64_t idxOOB2 = renderer.getCellIndexAtWorldPos({ 500.0f, 500.0f }, b2d);
        if (idxOOB2 != -1) {
            std::cerr << "  [FAIL] Test 4d: Expected -1 for out of bounds coordinate, got " << idxOOB2 << std::endl;
            return 11;
        }
        std::cout << "  [PASS] Test 4: getCellIndexAtWorldPos board space mappings verified." << std::endl;
    }

    std::cout << "[TEST-CONTROLS] ALL CONTROLS TESTS PASSED!" << std::endl;
    return 0;
}

int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--test-capsule") {
            int r1 = runCapsuleTests();
            int r2 = runCameraTests();
            int r3 = runControlTests();
            if (r1 != 0) return r1;
            if (r2 != 0) return r2;
            return r3;
        }
        if (std::string(argv[i]) == "--test-camera") {
            return runCameraTests();
        }
        if (std::string(argv[i]) == "--test-controls") {
            return runControlTests();
        }
    }

    minesweeper::App app;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--test-shop") {
            runCapsuleTests();
            runCameraTests();
            runControlTests();
            app.testShopMode = true;
        }
        else if (std::string(argv[i]) == "--test-customize") {
            app.testCustomizeMode = true;
        }
        else if (std::string(argv[i]) == "+connect_lobby" && i + 1 < argc) {
            try {
                app.initialLobbyId = std::stoull(argv[i + 1]);
            } catch (...) {}
        }
    }
    app.run();
    return 0;
}
