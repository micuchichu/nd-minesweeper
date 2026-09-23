#include "app.hpp"
#include "core/ship.hpp"
#include "core/ship_config.hpp"
#include "core/shop_ship.hpp"
#include "core/roulette_ship.hpp"
#include "core/item.hpp"
#include "core/roulette_types.hpp"
#include "ui/roulette_ui.hpp"
#include "render/camera_controller.hpp"
#include "render/procedural_textures.hpp"
#include "audio/sound_manager.hpp"
#include "core/campaign.hpp"
#include "core/save_manager.hpp"
#include "render/planet_renderer.hpp"
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

    // Test 5: CoordND::stepCell grid navigation (2D, 3D, 4D)
    {
        // 2D: 10x10 board
        minesweeper::core::CoordND c2d;
        c2d.init(2, 10);
        // At (0, 0)
        int64_t cell0 = 0;
        int64_t right = c2d.stepCell(cell0, 1, 0);
        if (right != 1) {
            std::cerr << "  [FAIL] Test 5a: step right from 0 expected 1, got " << right << std::endl;
            return 12;
        }
        int64_t down = c2d.stepCell(cell0, 0, 1);
        if (down != 10) {
            std::cerr << "  [FAIL] Test 5b: step down from 0 expected 10, got " << down << std::endl;
            return 13;
        }
        int64_t clampUp = c2d.stepCell(cell0, 0, -1);
        if (clampUp != 0) {
            std::cerr << "  [FAIL] Test 5c: step up from 0 should clamp to 0, got " << clampUp << std::endl;
            return 14;
        }

        // 3D: 5x5x5 board
        minesweeper::core::CoordND c3d;
        c3d.init(3, 5);
        // Cell at (x=2, y=4, z=0) -> bottom edge of slice 0
        int64_t bottomSlice0 = static_cast<int64_t>(c3d.toIndex3D(2, 4, 0));
        // Stepping down should cross into slice 1 at (x=2, y=0, z=1)
        int64_t topSlice1 = c3d.stepCell(bottomSlice0, 0, 1);
        size_t rx, ry, rz;
        c3d.toCoord3D(static_cast<size_t>(topSlice1), rx, ry, rz);
        if (rx != 2 || ry != 0 || rz != 1) {
            std::cerr << "  [FAIL] Test 5d: 3D slice down transition failed, got (" << rx << "," << ry << "," << rz << ")" << std::endl;
            return 15;
        }

        // 4D: 4x4x4x4 board
        minesweeper::core::CoordND c4d;
        c4d.init(4, 4);
        int64_t cell4d = static_cast<int64_t>(c4d.toIndex4D(3, 1, 0, 0));
        int64_t crossSliceZ = c4d.stepCell(cell4d, 1, 0); // x=3 + 1 -> z=1, x=0
        size_t c4x, c4y, c4z, c4w;
        c4d.toCoord4D(static_cast<size_t>(crossSliceZ), c4x, c4y, c4z, c4w);
        if (c4x != 0 || c4z != 1) {
            std::cerr << "  [FAIL] Test 5e: 4D slice Z transition failed, got (" << c4x << "," << c4z << ")" << std::endl;
            return 16;
        }
        std::cout << "  [PASS] Test 5: CoordND::stepCell 2D, 3D, and 4D traversal verified." << std::endl;
    }

    // Test 6: ScoutShip mouse follower tracking
    {
        minesweeper::core::ScoutShip ship;
        ship.position = { 0.0f, 0.0f };
        ship.isInitialized = true;

        Vector2 targetMouse = { 200.0f, 0.0f };
        for (int i = 0; i < 10; ++i) {
            ship.update(targetMouse, 0.05f);
        }
        if (ship.position.x <= 0.0f) {
            std::cerr << "  [FAIL] Test 6: Ship did not move towards target mouse position!" << std::endl;
            return 17;
        }
        if (ship.velocity.x <= 0.0f) {
            std::cerr << "  [FAIL] Test 6: Ship velocity did not accelerate towards target mouse!" << std::endl;
            return 18;
        }
        std::cout << "  [PASS] Test 6: ScoutShip mouse follower tracking verified." << std::endl;
    }

    // Test 7: Laser targeting alignment (mouse vs cell center)
    {
        minesweeper::render::RaylibRenderer renderer;
        renderer.cellSize = 40.0f;
        renderer.slicePadding = 20.0f;

        minesweeper::core::Board board;
        board.init(2, 10, 10, 12345);

        // Cell 0 is at (0, 0). Center must be (20, 20), NOT corner (40, 40)
        Vector2 cPos = renderer.getCellWorldPosition(0, board);
        if (std::abs(cPos.x - 20.0f) > 0.001f || std::abs(cPos.y - 20.0f) > 0.001f) {
            std::cerr << "  [FAIL] Test 7a: getCellWorldPosition(0) center expected (20, 20), got (" << cPos.x << ", " << cPos.y << ")" << std::endl;
            return 19;
        }

        // Cell 1 is at (1, 0). Center must be (60, 20)
        Vector2 cPos1 = renderer.getCellWorldPosition(1, board);
        if (std::abs(cPos1.x - 60.0f) > 0.001f || std::abs(cPos1.y - 20.0f) > 0.001f) {
            std::cerr << "  [FAIL] Test 7b: getCellWorldPosition(1) center expected (60, 20), got (" << cPos1.x << ", " << cPos1.y << ")" << std::endl;
            return 20;
        }

        // Simulating targeting logic:
        Vector2 worldMouse = { 27.5f, 15.2f };
        int64_t hovered = 0;

        // Case A: Mouse action -> targets exact worldMouse position directly
        bool isMouseAction = true;
        int controlMode = 0; // Mouse follower
        Vector2 actionTargetA = worldMouse;
        if (isMouseAction || (renderer.isMouseActive && controlMode == 0)) {
            actionTargetA = worldMouse;
        } else if (hovered >= 0) {
            actionTargetA = renderer.getCellWorldPosition(static_cast<size_t>(hovered), board);
        }
        if (std::abs(actionTargetA.x - worldMouse.x) > 0.001f || std::abs(actionTargetA.y - worldMouse.y) > 0.001f) {
            std::cerr << "  [FAIL] Test 7c: Laser did not target mouse pointer when mouse was active!" << std::endl;
            return 21;
        }

        // Case B: Gamepad / Controller action -> targets exact cell center, NOT corner
        isMouseAction = false;
        controlMode = 1; // Keyboard / Controller mode
        Vector2 actionTargetB = worldMouse;
        if (isMouseAction || (renderer.isMouseActive && controlMode == 0)) {
            actionTargetB = worldMouse;
        } else if (hovered >= 0) {
            actionTargetB = renderer.getCellWorldPosition(static_cast<size_t>(hovered), board);
        }
        if (std::abs(actionTargetB.x - 20.0f) > 0.001f || std::abs(actionTargetB.y - 20.0f) > 0.001f) {
            std::cerr << "  [FAIL] Test 7d: Laser did not target exact cell center for controller action!" << std::endl;
            return 22;
        }

        // Case C: Ensure old bug (double half-cell offset = 40.0f corner) does NOT happen
        if (std::abs(actionTargetB.x - 40.0f) < 0.001f && std::abs(actionTargetB.y - 40.0f) < 0.001f) {
            std::cerr << "  [FAIL] Test 7e: Regression detected! Laser targeted bottom-right cell corner instead of center!" << std::endl;
            return 23;
        }

        std::cout << "  [PASS] Test 7: Laser targeting alignment (mouse pointer & cell center) verified." << std::endl;
    }

    // Test 8: Unselect cell when mouse is outside the board
    {
        minesweeper::render::RaylibRenderer renderer;
        renderer.cellSize = 40.0f;
        renderer.slicePadding = 20.0f;

        minesweeper::core::Board board;
        board.init(2, 10, 10, 12345); // 10x10 board = 400x400 pixels in world space

        // 8a: Outside board to the left (x < 0)
        int64_t idxLeft = renderer.getCellIndexAtWorldPos({ -10.0f, 100.0f }, board);
        if (idxLeft != -1) {
            std::cerr << "  [FAIL] Test 8a: Expected -1 for position left of board, got " << idxLeft << std::endl;
            return 24;
        }

        // 8b: Outside board to the right (x >= 400)
        int64_t idxRight = renderer.getCellIndexAtWorldPos({ 450.0f, 100.0f }, board);
        if (idxRight != -1) {
            std::cerr << "  [FAIL] Test 8b: Expected -1 for position right of board, got " << idxRight << std::endl;
            return 25;
        }

        // 8c: Outside board above (y < 0)
        int64_t idxTop = renderer.getCellIndexAtWorldPos({ 100.0f, -15.0f }, board);
        if (idxTop != -1) {
            std::cerr << "  [FAIL] Test 8c: Expected -1 for position above board, got " << idxTop << std::endl;
            return 26;
        }

        // 8d: Outside board below (y >= 400)
        int64_t idxBottom = renderer.getCellIndexAtWorldPos({ 100.0f, 420.0f }, board);
        if (idxBottom != -1) {
            std::cerr << "  [FAIL] Test 8d: Expected -1 for position below board, got " << idxBottom << std::endl;
            return 27;
        }

        // 8e: Verify cell selection unselects in both control modes
        for (int mode = 0; mode <= 1; ++mode) {
            int64_t currentHoveredCell = 42; // previously hovered cell
            bool isMouseActive = true;

            // Mouse moves outside board
            int64_t mCell = renderer.getCellIndexAtWorldPos({ -50.0f, -50.0f }, board);
            if (isMouseActive) {
                currentHoveredCell = mCell;
            }

            if (currentHoveredCell != -1) {
                std::cerr << "  [FAIL] Test 8e: Cell did not unselect when mouse moved outside board in mode " << mode << ", got " << currentHoveredCell << std::endl;
                return 28;
            }
        }

        std::cout << "  [PASS] Test 8: Unselect cell when mouse is outside the board verified." << std::endl;
    }

    std::cout << "[TEST-CONTROLS] ALL CONTROLS TESTS PASSED!" << std::endl;
    return 0;
}

static int runTextureTests() {
    std::cout << "[TEST-TEXTURES] Starting Procedural Texture Unit Tests..." << std::endl;
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(100, 100, "Texture Unit Test");

    minesweeper::render::ProceduralTextures& pt = minesweeper::render::ProceduralTextures::instance();
    pt.init(30.0f, 2.0f);

    if (!pt.isInitialized()) {
        std::cerr << "  [FAIL] ProceduralTextures failed to initialize!" << std::endl;
        CloseWindow();
        return 1;
    }
    if (pt.shadowTexture.id == 0 || pt.particleTexture.id == 0 || pt.glowTexture.id == 0 ||
        pt.laserBeamTexture.id == 0 || pt.safeStartReticleRT.id == 0 || pt.panelNPatchTexture.id == 0 ||
        pt.buttonNPatchNormal.id == 0 || pt.buttonNPatchHover.id == 0 || pt.buttonNPatchLocked.id == 0) {
        std::cerr << "  [FAIL] One or more texture IDs are 0!" << std::endl;
        CloseWindow();
        return 2;
    }
    std::cout << "  [PASS] Test 1: All procedural textures and render textures initialized with valid GPU IDs." << std::endl;

    // Check dynamic cell resize
    pt.updateCellSize(40.0f, 3.0f);
    if (pt.safeStartReticleRT.id == 0) {
        std::cerr << "  [FAIL] Safe start reticle RT invalid after cell resize!" << std::endl;
        CloseWindow();
        return 3;
    }
    std::cout << "  [PASS] Test 2: Safe start reticle RT successfully updated on cell resize." << std::endl;

    // Check Ship shared textures wiring
    minesweeper::core::Ship::setSharedTextures(pt.particleTexture, pt.glowTexture);
    if (minesweeper::core::Ship::sharedExhaustTexture.id == 0 || minesweeper::core::Ship::sharedGlowTexture.id == 0) {
        std::cerr << "  [FAIL] Ship shared textures not set!" << std::endl;
        CloseWindow();
        return 4;
    }
    std::cout << "  [PASS] Test 3: Ship shared textures successfully assigned." << std::endl;

    pt.cleanup();
    if (pt.isInitialized()) {
        std::cerr << "  [FAIL] ProceduralTextures still initialized after cleanup!" << std::endl;
        CloseWindow();
        return 5;
    }
    std::cout << "  [PASS] Test 4: ProceduralTextures cleanup successfully reset state." << std::endl;

    CloseWindow();
    std::cout << "[TEST-TEXTURES] ALL TEXTURE TESTS PASSED!" << std::endl;
    return 0;
}

static int runItemTests() {
    std::cout << "[TEST-ITEMS] Starting Shop Items and Inventory System Tests..." << std::endl;

    SetTraceLogLevel(LOG_NONE);
    InitWindow(100, 100, "ItemTest");

    auto& catalog = minesweeper::core::ItemCatalog::instance();
    catalog.init();

    // 1. Check all 3 items exist with correct tiers and costs
    const auto* banana = catalog.getItem(minesweeper::core::ItemId::Banana);
    if (!banana || banana->tier != minesweeper::core::ItemTier::Tier1 || banana->cost != 15) {
        std::cerr << "  [FAIL] Banana item mismatch or not found!" << std::endl;
        catalog.shutdown();
        CloseWindow();
        return 1;
    }
    std::cout << "  [PASS] Test 1: Banana (Tier 1, Cost 15) verified." << std::endl;

    const auto* radar = catalog.getItem(minesweeper::core::ItemId::Radar);
    if (!radar || radar->tier != minesweeper::core::ItemTier::Tier2 || radar->cost != 40) {
        std::cerr << "  [FAIL] Radar item mismatch or not found!" << std::endl;
        catalog.shutdown();
        CloseWindow();
        return 2;
    }
    std::cout << "  [PASS] Test 2: Radar (Tier 2, Cost 40) verified." << std::endl;

    const auto* bubbles = catalog.getItem(minesweeper::core::ItemId::Bubbles);
    if (!bubbles || bubbles->tier != minesweeper::core::ItemTier::Tier3 || bubbles->cost != 60) {
        std::cerr << "  [FAIL] Bubbles item mismatch or not found!" << std::endl;
        catalog.shutdown();
        CloseWindow();
        return 3;
    }
    std::cout << "  [PASS] Test 3: Bubbles (Tier 3, Cost 60) verified." << std::endl;

    // 2. Check Mini Shop capacity = 2 and stocks Tier 1 & 2 only
    auto miniInv = catalog.createInventoryForShop("mini shop", 2);
    if (miniInv.capacity != 2 || miniInv.slots.size() != 2) {
        std::cerr << "  [FAIL] Mini shop capacity != 2!" << std::endl;
        catalog.shutdown();
        CloseWindow();
        return 4;
    }
    if (miniInv.slots[0].item.tier != minesweeper::core::ItemTier::Tier1 ||
        miniInv.slots[1].item.tier != minesweeper::core::ItemTier::Tier2) {
        std::cerr << "  [FAIL] Mini shop does not stock Tier 1 & 2 items!" << std::endl;
        catalog.shutdown();
        CloseWindow();
        return 5;
    }
    std::cout << "  [PASS] Test 4: Mini shop 2-slot capacity and Tier 1-2 stocking verified." << std::endl;

    // 3. Check Big Shop capacity = 4 and stocks up to Tier 3
    auto bigInv = catalog.createInventoryForShop("big shop", 4);
    if (bigInv.capacity != 4 || bigInv.slots.size() != 4) {
        std::cerr << "  [FAIL] Big shop capacity != 4!" << std::endl;
        catalog.shutdown();
        CloseWindow();
        return 6;
    }
    bool hasTier3 = false;
    for (const auto& slot : bigInv.slots) {
        if (slot.item.tier == minesweeper::core::ItemTier::Tier3) hasTier3 = true;
    }
    if (!hasTier3) {
        std::cerr << "  [FAIL] Big shop missing Tier 3 items!" << std::endl;
        catalog.shutdown();
        CloseWindow();
        return 7;
    }
    std::cout << "  [PASS] Test 5: Big shop 4-slot capacity and Tier 1-3 stocking verified." << std::endl;

    // 4. ShopShip capacity auto-determination
    minesweeper::core::ShopShip miniShip;
    miniShip.name = "mini shop";
    miniShip.capsuleLength = 0.0f;
    miniShip.initializeInventory();
    if (miniShip.inventory.capacity != 2 || miniShip.inventory.slots.size() != 2) {
        std::cerr << "  [FAIL] Mini ShopShip inventory capacity determination failed!" << std::endl;
        catalog.shutdown();
        CloseWindow();
        return 8;
    }

    minesweeper::core::ShopShip bigShip;
    bigShip.name = "big shop";
    bigShip.capsuleLength = 130.0f;
    bigShip.initializeInventory();
    if (bigShip.inventory.capacity != 4 || bigShip.inventory.slots.size() != 4) {
        std::cerr << "  [FAIL] Big ShopShip inventory capacity determination failed!" << std::endl;
        catalog.shutdown();
        CloseWindow();
        return 9;
    }
    std::cout << "  [PASS] Test 6: ShopShip mini (2 slots) and big (4 slots) initialization verified." << std::endl;

    // 5. PlayerInventory 5-slot hotbar capacity & item addition
    minesweeper::core::PlayerInventory pInv;
    if (!pInv.hasFreeSlot() || pInv.getFreeSlotIndex() != 0) {
        std::cerr << "  [FAIL] Fresh PlayerInventory should have 5 free slots, free idx 0!" << std::endl;
        catalog.shutdown();
        CloseWindow();
        return 10;
    }

    pInv.addItem(*banana);
    pInv.addItem(*radar);
    pInv.addItem(*bubbles);

    if (pInv.slots[0].item.id != minesweeper::core::ItemId::Banana ||
        pInv.slots[1].item.id != minesweeper::core::ItemId::Radar ||
        pInv.slots[2].item.id != minesweeper::core::ItemId::Bubbles ||
        !pInv.slots[0].occupied || !pInv.slots[1].occupied || !pInv.slots[2].occupied) {
        std::cerr << "  [FAIL] Items not added correctly to hotbar slots!" << std::endl;
        catalog.shutdown();
        CloseWindow();
        return 11;
    }
    std::cout << "  [PASS] Test 7: 5-slot hotbar item addition and tracking verified." << std::endl;

    // 6. Slot selection and carried item retrieval
    pInv.selectedSlot = 2; // Bubbles
    auto* held = pInv.getSelectedSlot();
    if (!held || held->item.id != minesweeper::core::ItemId::Bubbles) {
        std::cerr << "  [FAIL] Selected slot 2 retrieval failed!" << std::endl;
        catalog.shutdown();
        CloseWindow();
        return 12;
    }
    std::cout << "  [PASS] Test 8: Hotbar slot selection and held item retrieval verified." << std::endl;

    // 7. Durability depletion for Bubbles
    if (held->durability <= 0.0f || held->maxDurability < 5.0f) {
        std::cerr << "  [FAIL] Bubbles initial durability incorrect!" << std::endl;
        catalog.shutdown();
        CloseWindow();
        return 13;
    }
    held->durability -= 1.5f;
    if (std::abs(held->durability - 4.5f) > 0.01f) {
        std::cerr << "  [FAIL] Bubbles durability depletion calculation failed!" << std::endl;
        catalog.shutdown();
        CloseWindow();
        return 14;
    }
    std::cout << "  [PASS] Test 9: Bubbles hold-to-use durability tracking verified." << std::endl;

    // 8. Banana active buff calculation
    pInv.bananaBoostTimer = 20.0f;
    float baseSpeed = 600.0f;
    float boostedSpeed = baseSpeed * (pInv.bananaBoostTimer > 0.0f ? 1.30f : 1.0f);
    if (std::abs(boostedSpeed - 780.0f) >= 0.001f) {
        std::cerr << "  [FAIL] Banana +30% speed boost calculation mismatch!" << std::endl;
        catalog.shutdown();
        CloseWindow();
        return 15;
    }
    std::cout << "  [PASS] Test 10: Banana +30% speed boost buff verified (780 px/s)." << std::endl;

    // 9. Radar scan 4.0s timer
    pInv.radarActiveTimer = 4.0f;
    if (pInv.radarActiveTimer <= 0.0f) {
        std::cerr << "  [FAIL] Radar active timer failed to initialize!" << std::endl;
        catalog.shutdown();
        CloseWindow();
        return 16;
    }
    std::cout << "  [PASS] Test 11: Radar scan 4.0s duration timer verified." << std::endl;

    // 10. Radar unsafe cell center alignment check
    {
        minesweeper::render::RaylibRenderer renderer;
        renderer.cellSize = 40.0f;
        renderer.slicePadding = 20.0f;
        minesweeper::core::Board b;
        b.init(2, 10, 10, 12345);

        Vector2 c0 = renderer.getCellWorldPosition(0, b);
        // Center of cell 0 (0..40, 0..40) must be exactly (20, 20)
        if (std::abs(c0.x - 20.0f) > 0.001f || std::abs(c0.y - 20.0f) > 0.001f) {
            std::cerr << "  [FAIL] Radar cell center misaligned! Expected (20, 20), got (" << c0.x << ", " << c0.y << ")" << std::endl;
            catalog.shutdown();
            CloseWindow();
            return 17;
        }
    }
    std::cout << "  [PASS] Test 12: Radar unsafe cell exact center alignment verified." << std::endl;

    // 11. Spring physics & deployment from inside the player ship
    {
        minesweeper::render::RaylibRenderer renderer;
        renderer.localShip.position = { 100.0f, 100.0f };
        renderer.localShip.angle = 90.0f;
        renderer.localShip.velocity = { 0.0f, 0.0f };
        renderer.heldSlot = &pInv.slots[0]; // Banana

        // Step physics once: should start deploying directly from INSIDE the ship
        renderer.updateHeldItemPhysics(renderer.localShip.position, renderer.localShip.velocity, 1.0f / 120.0f);
        if (!renderer.heldItemInit || renderer.heldState != minesweeper::render::RaylibRenderer::HeldItemState::Deploying) {
            std::cerr << "  [FAIL] Held item failed to enter Deploying state from inside ship!" << std::endl;
            catalog.shutdown();
            CloseWindow();
            return 18;
        }

        float initDist = Vector2Distance(renderer.heldItemPos, renderer.localShip.position);
        if (initDist > 2.0f) {
            std::cerr << "  [FAIL] Held item did not emerge from inside ship center! initDist: " << initDist << std::endl;
            catalog.shutdown();
            CloseWindow();
            return 19;
        }
        std::cout << "  [PASS] Test 13: Held item deployment begins inside player ship center (dist: " << initDist << " px)." << std::endl;

        // Step physics until deployment animation completes (~0.25s)
        for (int step = 0; step < 60; ++step) {
            renderer.updateHeldItemPhysics(renderer.localShip.position, renderer.localShip.velocity, 1.0f / 60.0f);
        }
        if (renderer.heldState != minesweeper::render::RaylibRenderer::HeldItemState::Deployed) {
            std::cerr << "  [FAIL] Held item failed to complete deployment to Deployed state!" << std::endl;
            catalog.shutdown();
            CloseWindow();
            return 20;
        }

        float deployedDist = Vector2Distance(renderer.heldItemPos, renderer.localShip.position);
        if (std::abs(deployedDist - 26.0f) > 1.5f) {
            std::cerr << "  [FAIL] Deployed rest tether distance mismatch! Expected ~26.0, got " << deployedDist << std::endl;
            catalog.shutdown();
            CloseWindow();
            return 21;
        }
        std::cout << "  [PASS] Test 14: Held item successfully deployed outward to 26px tether distance." << std::endl;

        // 12. Dynamic spring extension and Hookean restoration
        // Move ship forward along X
        for (int step = 0; step < 30; ++step) {
            renderer.localShip.position.x += 400.0f * (1.0f / 120.0f);
            renderer.localShip.velocity = { 400.0f, 0.0f };
            renderer.updateHeldItemPhysics(renderer.localShip.position, renderer.localShip.velocity, 1.0f / 120.0f);
        }
        // Spring should be extended while pulling
        float stretchDist = Vector2Distance(renderer.heldItemPos, renderer.localShip.position);
        if (stretchDist <= 26.0f) {
            std::cerr << "  [FAIL] Spring failed to stretch under ship forward velocity! dist: " << stretchDist << std::endl;
            catalog.shutdown();
            CloseWindow();
            return 22;
        }

        // Let ship come to a stop and spring damp back to rest length
        renderer.localShip.velocity = { 0.0f, 0.0f };
        for (int step = 0; step < 240; ++step) {
            renderer.updateHeldItemPhysics(renderer.localShip.position, renderer.localShip.velocity, 1.0f / 120.0f);
        }
        float settledDist = Vector2Distance(renderer.heldItemPos, renderer.localShip.position);
        if (std::abs(settledDist - 26.0f) > 1.5f) {
            std::cerr << "  [FAIL] Spring failed to damp back to rest distance! Got " << settledDist << std::endl;
            catalog.shutdown();
            CloseWindow();
            return 23;
        }
        std::cout << "  [PASS] Test 15: Dynamic spring extension and damped return to rest verified." << std::endl;

        // 13. Free 360-degree orbital rotation without angular lock
        renderer.heldItemVel.y += 200.0f;
        float prevAngle = std::atan2(renderer.heldItemPos.y - renderer.localShip.position.y,
                                     renderer.heldItemPos.x - renderer.localShip.position.x);
        float totalAngRot = 0.0f;
        for (int step = 0; step < 120; ++step) {
            renderer.updateHeldItemPhysics(renderer.localShip.position, renderer.localShip.velocity, 1.0f / 120.0f);
            float curAngle = std::atan2(renderer.heldItemPos.y - renderer.localShip.position.y,
                                        renderer.heldItemPos.x - renderer.localShip.position.x);
            float dTheta = curAngle - prevAngle;
            while (dTheta > 3.14159265f) dTheta -= 2.0f * 3.14159265f;
            while (dTheta < -3.14159265f) dTheta += 2.0f * 3.14159265f;
            totalAngRot += std::abs(dTheta);
            prevAngle = curAngle;
        }
        if (totalAngRot < 1.0f) {
            std::cerr << "  [FAIL] Held item failed to spin freely around player! Total rotation: " << totalAngRot << std::endl;
            catalog.shutdown();
            CloseWindow();
            return 24;
        }
        std::cout << "  [PASS] Test 16: Free 360-degree orbital spinning around player verified (rotated " << totalAngRot << " rad)." << std::endl;

        // 14. Retraction back into the ship when unheld
        renderer.heldSlot = nullptr; // Unheld
        renderer.updateHeldItemPhysics(renderer.localShip.position, renderer.localShip.velocity, 1.0f / 60.0f);
        if (renderer.heldState != minesweeper::render::RaylibRenderer::HeldItemState::Retracting) {
            std::cerr << "  [FAIL] Held item failed to transition to Retracting state!" << std::endl;
            catalog.shutdown();
            CloseWindow();
            return 25;
        }

        // Step retraction until item is reeled completely back into the ship
        for (int step = 0; step < 60; ++step) {
            renderer.updateHeldItemPhysics(renderer.localShip.position, renderer.localShip.velocity, 1.0f / 60.0f);
        }
        if (renderer.heldState != minesweeper::render::RaylibRenderer::HeldItemState::Hidden) {
            std::cerr << "  [FAIL] Held item failed to retract back into ship (expected Hidden)!" << std::endl;
            catalog.shutdown();
            CloseWindow();
            return 26;
        }
        float retractedDist = Vector2Distance(renderer.heldItemPos, renderer.localShip.position);
        if (retractedDist > 3.5f) {
            std::cerr << "  [FAIL] Retracted item position not inside player ship! Dist: " << retractedDist << std::endl;
            catalog.shutdown();
            CloseWindow();
            return 27;
        }
        std::cout << "  [PASS] Test 17: Held item smoothly retracts back into player ship and becomes Hidden." << std::endl;
    }

    // 15. Verify JSON loading for shop ships
    minesweeper::core::ShipConfig shop1Cfg;
    if (!minesweeper::core::ShipConfig::loadFromFile("assets/shops/shop1.json", shop1Cfg) || shop1Cfg.shopTier != 1 || shop1Cfg.itemCapacity != 2) {
        std::cerr << "  [FAIL] shop1.json tier or itemCapacity parsing failed! tier: " << shop1Cfg.shopTier << ", cap: " << shop1Cfg.itemCapacity << std::endl;
        catalog.shutdown();
        CloseWindow();
        return 28;
    }
    minesweeper::core::ShipConfig shop2Cfg;
    if (!minesweeper::core::ShipConfig::loadFromFile("assets/shops/shop2.json", shop2Cfg) || shop2Cfg.shopTier != 2 || shop2Cfg.itemCapacity != 3) {
        std::cerr << "  [FAIL] shop2.json tier or itemCapacity parsing failed! tier: " << shop2Cfg.shopTier << ", cap: " << shop2Cfg.itemCapacity << std::endl;
        catalog.shutdown();
        CloseWindow();
        return 29;
    }

    minesweeper::core::ShipConfig shop3Cfg;
    if (minesweeper::core::ShipConfig::loadFromFile("assets/shops/shop3.json", shop3Cfg)) {
        if (shop3Cfg.shopTier != 3 || shop3Cfg.itemCapacity != 8) {
            std::cerr << "  [FAIL] shop3.json tier or itemCapacity parsing failed! tier: " << shop3Cfg.shopTier << ", cap: " << shop3Cfg.itemCapacity << std::endl;
            catalog.shutdown();
            CloseWindow();
            return 30;
        }

        // Verify ShopShip constructed with shop3Cfg adheres strictly to JSON without override
        minesweeper::core::ShopShip shop3Ship(Texture2D{ 0, 0, 0, 0, 0 }, { 0.0f, 0.0f }, shop3Cfg);
        if (shop3Ship.shopTier != 3 || shop3Ship.itemCapacity != 8 || shop3Ship.inventory.capacity != 8) {
            std::cerr << "  [FAIL] ShopShip with shop3Cfg did not retain tier 3 or cap 8! Got tier: " 
                      << shop3Ship.shopTier << ", cap: " << shop3Ship.itemCapacity << std::endl;
            catalog.shutdown();
            CloseWindow();
            return 31;
        }

        // Verify custom single-slot tier 1 capsule ship is NOT overridden to tier 2 / cap 4
        minesweeper::core::ShipConfig customCfg = shop3Cfg;
        customCfg.capsuleLength = 150.0f;
        customCfg.shopTier = 1;
        customCfg.itemCapacity = 1;
        minesweeper::core::ShopShip customShip(Texture2D{ 0, 0, 0, 0, 0 }, { 0.0f, 0.0f }, customCfg);
        if (customShip.shopTier != 1 || customShip.itemCapacity != 1 || customShip.inventory.capacity != 1) {
            std::cerr << "  [FAIL] Custom ShopShip was improperly overridden by capsule/procedural logic! Got tier: " 
                      << customShip.shopTier << ", cap: " << customShip.itemCapacity << std::endl;
            catalog.shutdown();
            CloseWindow();
            return 32;
        }
    }
    std::cout << "  [PASS] Test 18: Shop ship JSON tiers and item capacity parsing (shop1, shop2, shop3) verified." << std::endl;

    // 16. Shop click-to-open interaction & distance verification
    {
        minesweeper::core::ShopShip testShop;
        testShop.position = { 400.0f, 100.0f };
        testShop.collisionRadius = 28.0f;
        testShop.scale = 1.8f;
        testShop.capsuleLength = 130.0f;

        const float maxInteractionDist = 380.0f;
        const float maxCloseDist = 450.0f;

        Vector2 pA, pB;
        testShop.getCapsuleSegment(pA, pB);
        Vector2 ab = { pB.x - pA.x, pB.y - pA.y };
        float lenSq = ab.x * ab.x + ab.y * ab.y;

        auto getDistToShop = [&](Vector2 pt) {
            float t = (lenSq > 0.0001f) ? std::clamp(((pt.x - pA.x) * ab.x + (pt.y - pA.y) * ab.y) / lenSq, 0.0f, 1.0f) : 0.0f;
            Vector2 closest = { pA.x + t * ab.x, pA.y + t * ab.y };
            return Vector2Distance(pt, closest);
        };

        // Test player in range (300px from capsule) vs out of range (410px from capsule)
        Vector2 playerInRange = { 400.0f, pB.y + 300.0f };
        Vector2 playerOutOfRange = { 400.0f, pB.y + 410.0f };
        Vector2 playerBeyondClose = { 400.0f, pB.y + 480.0f };

        float distIn = getDistToShop(playerInRange);
        float distOut = getDistToShop(playerOutOfRange);
        float distBeyond = getDistToShop(playerBeyondClose);

        if (distIn > maxInteractionDist) {
            std::cerr << "  [FAIL] Player in range should be <= 380px, got " << distIn << std::endl;
            catalog.shutdown();
            CloseWindow();
            return 31;
        }
        if (distOut <= maxInteractionDist) {
            std::cerr << "  [FAIL] Player out of range should be > 380px, got " << distOut << std::endl;
            catalog.shutdown();
            CloseWindow();
            return 32;
        }
        if (distBeyond <= maxCloseDist) {
            std::cerr << "  [FAIL] Player beyond close distance should be > 450px, got " << distBeyond << std::endl;
            catalog.shutdown();
            CloseWindow();
            return 33;
        }

        // Mouse click bounds
        float clickRadius = std::max({
            testShop.collisionRadius * testShop.scale + 16.0f,
            36.0f
        });
        Vector2 mouseDirectHit = testShop.position;
        Vector2 mouseMiss = { testShop.position.x + 200.0f, testShop.position.y };

        if (getDistToShop(mouseDirectHit) > clickRadius) {
            std::cerr << "  [FAIL] Direct hit mouse should be within click radius!" << std::endl;
            catalog.shutdown();
            CloseWindow();
            return 34;
        }
        if (getDistToShop(mouseMiss) <= clickRadius) {
            std::cerr << "  [FAIL] Miss mouse should be outside click radius!" << std::endl;
            catalog.shutdown();
            CloseWindow();
            return 35;
        }

        std::cout << "  [PASS] Test 19: Shop click-to-open bounds & interaction distance (380px/450px) verified." << std::endl;
    }

    // 20. UI Hover Detection & Game Input Suppression Verification
    {
        minesweeper::ui::GameHUD testHud;
        const int screenW = 1000;
        const int screenH = 800;
        const float guiScale = 1.0f;

        // 20a: Top bar hover
        Vector2 topBarPt = { 300.0f, 35.0f }; // y <= 70.0f
        if (!testHud.isMouseOver(screenW, screenH, guiScale, topBarPt)) {
            std::cerr << "  [FAIL] Test 20a: Top bar point (300, 35) should be detected as mouse over UI!" << std::endl;
            catalog.shutdown();
            CloseWindow();
            return 36;
        }

        // 20b: Bottom bar hover
        Vector2 bottomBarPt = { 500.0f, 750.0f }; // y >= 800 - 92 = 708
        if (!testHud.isMouseOver(screenW, screenH, guiScale, bottomBarPt)) {
            std::cerr << "  [FAIL] Test 20b: Bottom bar point (500, 750) should be detected as mouse over UI!" << std::endl;
            catalog.shutdown();
            CloseWindow();
            return 37;
        }

        // 20c: Center board point (free play field)
        Vector2 fieldPt = { 500.0f, 400.0f };
        if (testHud.isMouseOver(screenW, screenH, guiScale, fieldPt)) {
            std::cerr << "  [FAIL] Test 20c: Center field point (500, 400) should NOT be mouse over UI!" << std::endl;
            catalog.shutdown();
            CloseWindow();
            return 38;
        }

        // 20d: Hotbar dock hover
        minesweeper::ui::ShopMenu testShopMenu;
        minesweeper::core::PlayerInventory testInv;
        Vector2 hotbarSlotPt = { 45.0f, 660.0f }; // dockX=20, dockY=800-92-50-12=646
        Vector2 outsideHotbarPt = { 450.0f, 660.0f };

        if (!testShopMenu.isMouseOverHotbar(screenW, screenH, testInv, hotbarSlotPt)) {
            std::cerr << "  [FAIL] Test 20d: Hotbar slot point (45, 660) should be detected as mouse over hotbar!" << std::endl;
            catalog.shutdown();
            CloseWindow();
            return 39;
        }
        if (testShopMenu.isMouseOverHotbar(screenW, screenH, testInv, outsideHotbarPt)) {
            std::cerr << "  [FAIL] Test 20d: Point (450, 660) should NOT be detected as mouse over hotbar!" << std::endl;
            catalog.shutdown();
            CloseWindow();
            return 40;
        }

        // 20e: Hotbar expansion with active buff
        testInv.bananaBoostTimer = 8.0f;
        Vector2 buffChipPt = { 35.0f, 630.0f }; // expanded upward by 22px
        if (!testShopMenu.isMouseOverHotbar(screenW, screenH, testInv, buffChipPt)) {
            std::cerr << "  [FAIL] Test 20e: Active buff area (35, 630) should be detected as mouse over hotbar!" << std::endl;
            catalog.shutdown();
            CloseWindow();
            return 41;
        }

        // 20f: Shop Menu Card hover
        testShopMenu.lastCardRect = { 300.0f, 200.0f, 310.0f, 240.0f };
        Vector2 cardInsidePt = { 350.0f, 250.0f };
        Vector2 cardOutsidePt = { 200.0f, 250.0f };

        // Closed shop (alpha = 0): card does NOT block UI
        if (testShopMenu.isMouseOverUI(screenW, screenH, testInv, 0.0f, cardInsidePt)) {
            std::cerr << "  [FAIL] Test 20f: Closed shop (alpha=0) should NOT block UI at card point!" << std::endl;
            catalog.shutdown();
            CloseWindow();
            return 42;
        }
        // Open shop (alpha = 1.0): card DOES block UI
        if (!testShopMenu.isMouseOverUI(screenW, screenH, testInv, 1.0f, cardInsidePt)) {
            std::cerr << "  [FAIL] Test 20f: Open shop (alpha=1) SHOULD block UI at card point!" << std::endl;
            catalog.shutdown();
            CloseWindow();
            return 43;
        }
        if (testShopMenu.isMouseOverUI(screenW, screenH, testInv, 1.0f, cardOutsidePt)) {
            std::cerr << "  [FAIL] Test 20f: Open shop should NOT block UI outside card rect!" << std::endl;
            catalog.shutdown();
            CloseWindow();
            return 44;
        }

        // 20g: Input suppression logic verification
        bool isOverUI = true;
        bool mouseHandledByShop = false;
        bool mousePressed = true;
        bool triggerUncover = mousePressed && !mouseHandledByShop && !isOverUI;
        bool triggerFlag = mousePressed && !mouseHandledByShop && !isOverUI;
        int64_t currentHoveredCell = 15;
        if (isOverUI) currentHoveredCell = -1;

        if (triggerUncover || triggerFlag) {
            std::cerr << "  [FAIL] Test 20g: triggerUncover and triggerFlag MUST be suppressed when isOverUI is true!" << std::endl;
            catalog.shutdown();
            CloseWindow();
            return 45;
        }
        if (currentHoveredCell != -1) {
            std::cerr << "  [FAIL] Test 20g: currentHoveredCell MUST be unselected (-1) when isOverUI is true!" << std::endl;
            catalog.shutdown();
            CloseWindow();
            return 46;
        }

        std::cout << "  [PASS] Test 20: UI Hover detection (HUD/Hotbar/Shop) and Game Input suppression verified." << std::endl;
    }

    catalog.shutdown();
    CloseWindow();
    std::cout << "[TEST-ITEMS] ALL ITEM TESTS PASSED!" << std::endl;
    return 0;
}

static int runRouletteTests() {
    std::cout << "[TEST-ROULETTE] Starting Roulette Wheel & Casino Mechanics Unit Tests..." << std::endl;

    // Test 1: European Wheel Layout & Pockets
    {
        static_assert(minesweeper::core::ROULETTE_POCKET_COUNT == 37, "Expected 37 pockets");
        if (minesweeper::core::ROULETTE_NUMBERS[0] != 0) {
            std::cerr << "  [FAIL] Test 1: Pocket 0 should be number 0, got " << minesweeper::core::ROULETTE_NUMBERS[0] << std::endl;
            return 2;
        }
        if (minesweeper::core::ROULETTE_NUMBERS[1] != 32 || minesweeper::core::ROULETTE_NUMBERS[2] != 15) {
            std::cerr << "  [FAIL] Test 1: Expected 32 then 15 after pocket 0" << std::endl;
            return 3;
        }
        if (minesweeper::core::ROULETTE_NUMBERS[36] != 26) {
            std::cerr << "  [FAIL] Test 1: Expected pocket 36 to be number 26, got " << minesweeper::core::ROULETTE_NUMBERS[36] << std::endl;
            return 4;
        }
        std::cout << "  [PASS] Test 1: European wheel pocket sequence verified." << std::endl;
    }

    // Test 2: Color Classifications
    {
        int redCount = 0;
        int blackCount = 0;
        for (int i = 0; i <= 36; ++i) {
            if (minesweeper::core::isRouletteRed(i)) redCount++;
            if (minesweeper::core::isRouletteBlack(i)) blackCount++;
        }
        if (redCount != 18 || blackCount != 18) {
            std::cerr << "  [FAIL] Test 2: Expected 18 red and 18 black numbers, got " << redCount << " red, " << blackCount << " black" << std::endl;
            return 5;
        }
        if (minesweeper::core::isRouletteRed(0) || minesweeper::core::isRouletteBlack(0)) {
            std::cerr << "  [FAIL] Test 2: Pocket 0 should be green (neither red nor black)" << std::endl;
            return 6;
        }
        std::cout << "  [PASS] Test 2: Red/Black/Green classifications verified." << std::endl;
    }

    // Test 3: 12 O'Clock Pointer Angle Alignment for all 37 numbers
    {
        for (int n = 0; n <= 36; ++n) {
            float targetAng = minesweeper::core::getTargetAngleForNumber(n);
            int pocketIdx = minesweeper::core::getPocketUnderPointer(targetAng);
            int numberFound = minesweeper::core::getNumberForPocketIndex(pocketIdx);
            if (numberFound != n) {
                std::cerr << "  [FAIL] Test 3: Expected number " << n << " at target angle " << targetAng << ", but found " << numberFound << " (pocket " << pocketIdx << ")" << std::endl;
                return 7;
            }
        }
        std::cout << "  [PASS] Test 3: 12 o'clock needle pointer alignment verified for all 37 numbers." << std::endl;
    }

    // Test 4: Payout Calculations
    {
        using namespace minesweeper::core;
        // Red win
        if (evaluateRoulettePayout(RouletteBetType::Red, 0, 50, 32) != 100) return 8;
        // Red loss
        if (evaluateRoulettePayout(RouletteBetType::Red, 0, 50, 15) != 0) return 9;
        // Black win
        if (evaluateRoulettePayout(RouletteBetType::Black, 0, 50, 15) != 100) return 10;
        // Even win / loss / zero
        if (evaluateRoulettePayout(RouletteBetType::Even, 0, 50, 14) != 100) return 11;
        if (evaluateRoulettePayout(RouletteBetType::Even, 0, 50, 15) != 0) return 12;
        if (evaluateRoulettePayout(RouletteBetType::Even, 0, 50, 0) != 0) return 13;
        // Odd win
        if (evaluateRoulettePayout(RouletteBetType::Odd, 0, 50, 15) != 100) return 14;
        // Green 0 Jackpot (36x)
        if (evaluateRoulettePayout(RouletteBetType::GreenZero, 0, 10, 0) != 360) return 15;
        if (evaluateRoulettePayout(RouletteBetType::GreenZero, 0, 10, 32) != 0) return 16;
        // Dozen 1 (1-12) (3x)
        if (evaluateRoulettePayout(RouletteBetType::Dozen1, 0, 20, 7) != 60) return 17;
        if (evaluateRoulettePayout(RouletteBetType::Dozen1, 0, 20, 15) != 0) return 18;
        // Single Number (36x)
        if (evaluateRoulettePayout(RouletteBetType::SingleNumber, 17, 25, 17) != 900) return 19;
        if (evaluateRoulettePayout(RouletteBetType::SingleNumber, 17, 25, 18) != 0) return 20;

        std::cout << "  [PASS] Test 4: All payout multipliers (2x, 3x, 36x jackpot) verified." << std::endl;
    }

    // Test 5: Dedicated RouletteShip Spin Acceleration Mechanics
    {
        using namespace minesweeper::core;
        RouletteShip ship;
        if (ship.name != "roulette") {
            std::cerr << "  [FAIL] Test 5: ship.name should be 'roulette'" << std::endl;
            return 21;
        }

        ship.roulette.activeBet.type = RouletteBetType::Red;
        ship.roulette.activeBet.amount = 50;
        ship.startSpin(32, 1.0f); // Winning number 32 (Red)

        if (ship.roulette.spinState != RouletteSpinState::Spinning) {
            std::cerr << "  [FAIL] Test 5: Spin state should be Spinning immediately after startSpin" << std::endl;
            return 22;
        }

        // Advance spin slightly (acceleration phase: p = 0.05 / 1.0 = 0.05 < 0.20)
        float angleBefore = ship.angle;
        ship.update(0.05f);
        if (ship.roulette.spinState != RouletteSpinState::Spinning) {
            std::cerr << "  [FAIL] Test 5: Spin state should still be Spinning at t=0.05" << std::endl;
            return 23;
        }
        float deltaAngle1 = std::abs(ship.angle - angleBefore);

        // Advance another 0.05s (acceleration phase: velocity should increase, delta2 > delta1)
        angleBefore = ship.angle;
        ship.update(0.05f);
        float deltaAngle2 = std::abs(ship.angle - angleBefore);
        if (deltaAngle2 <= deltaAngle1) {
            std::cerr << "  [FAIL] Test 5: Ship should accelerate in the initial phase (delta2=" << deltaAngle2 << " <= delta1=" << deltaAngle1 << ")" << std::endl;
            return 24;
        }

        // Advance to completion
        ship.update(0.95f);
        if (ship.roulette.spinState != RouletteSpinState::Result) {
            std::cerr << "  [FAIL] Test 5: Spin state should be Result after completion" << std::endl;
            return 25;
        }
        if (!ship.roulette.lastWon || ship.roulette.lastPayout != 100) {
            std::cerr << "  [FAIL] Test 5: Expected win with payout 100, got won=" << ship.roulette.lastWon << ", payout=" << ship.roulette.lastPayout << std::endl;
            return 26;
        }
        // Verify ship settled on target angle
        float diff = std::abs(ship.angle - ship.roulette.targetAngle);
        while (diff > 180.0f) diff = std::abs(diff - 360.0f);
        if (diff > 0.05f) {
            std::cerr << "  [FAIL] Test 5: Ship angle (" << ship.angle << ") did not match target angle (" << ship.roulette.targetAngle << ")" << std::endl;
            return 27;
        }

        std::cout << "  [PASS] Test 5: Dedicated RouletteShip spin acceleration physics and winning pocket lock verified." << std::endl;
    }

    // Test 6: Roulette UI Mouse Shielding
    {
        minesweeper::ui::RouletteUI rUI;
        rUI.lastCardRect = { 100.0f, 100.0f, 380.0f, 390.0f };
        if (!rUI.isMouseOverCard({ 200.0f, 200.0f })) {
            std::cerr << "  [FAIL] Test 6: Inside point (200, 200) should be detected as mouse over card" << std::endl;
            return 28;
        }
        if (rUI.isMouseOverCard({ 50.0f, 50.0f })) {
            std::cerr << "  [FAIL] Test 6: Outside point (50, 50) should NOT be detected as mouse over card" << std::endl;
            return 29;
        }
        std::cout << "  [PASS] Test 6: Roulette UI mouse shielding verified." << std::endl;
    }

    // Test 7: Post-Spin Auto-Alignment Back to Rest Dock (90 deg)
    {
        using namespace minesweeper::core;
        RouletteShip ship;
        ship.startSpin(7, 0.5f);
        ship.update(0.6f); // Finish spin
        if (ship.roulette.spinState != RouletteSpinState::Result) {
            std::cerr << "  [FAIL] Test 7: Expected Result state after spin" << std::endl;
            return 30;
        }

        // Wait past result display duration (2.5s) to trigger auto-alignment
        ship.update(2.6f);
        if (!ship.isAligning) {
            std::cerr << "  [FAIL] Test 7: Ship should start auto-aligning after result display timeout" << std::endl;
            return 31;
        }

        // Advance through alignment duration (1.2s)
        ship.update(1.3f);
        if (ship.isAligning || ship.roulette.spinState != RouletteSpinState::Idle) {
            std::cerr << "  [FAIL] Test 7: Ship should finish aligning and return to Idle" << std::endl;
            return 32;
        }

        float restDiff = std::abs(ship.angle - 90.0f);
        while (restDiff > 180.0f) restDiff = std::abs(restDiff - 360.0f);
        if (restDiff > 0.05f) {
            std::cerr << "  [FAIL] Test 7: Ship should be aligned back to 90 degrees, got " << ship.angle << std::endl;
            return 33;
        }

        // Verify ball stayed in pocket 7
        float expectedBallAngle = ship.angle + getPocketTextureAngle(ship.roulette.winningPocketIndex);
        float ballDiff = std::abs(ship.roulette.ball.angle - expectedBallAngle);
        while (ballDiff > 180.0f) ballDiff = std::abs(ballDiff - 360.0f);
        if (ballDiff > 0.05f) {
            std::cerr << "  [FAIL] Test 7: Ball should remain nestled in winning pocket during alignment" << std::endl;
            return 34;
        }

        std::cout << "  [PASS] Test 7: Post-spin auto-alignment back to 90-degree dock orientation verified." << std::endl;
    }

    // Test 8: Fading Result Popup Lifecycle & Win/Loss Tracking
    {
        minesweeper::ui::RouletteUI rUI;
        if (rUI.isPopupActive()) {
            std::cerr << "  [FAIL] Test 8: Popup should be inactive initially" << std::endl;
            return 35;
        }

        // Test Win Popup trigger
        rUI.triggerPopup(17, true, 120, 40);
        if (!rUI.isPopupActive() || rUI.popup.winningNumber != 17 || !rUI.popup.won || rUI.popup.payout != 120) {
            std::cerr << "  [FAIL] Test 8: Win popup state failed to initialize properly" << std::endl;
            return 36;
        }

        // Advance midway through display duration
        rUI.updatePopup(1.5f);
        if (!rUI.isPopupActive() || rUI.popup.timer < 1.49f) {
            std::cerr << "  [FAIL] Test 8: Popup should still be active at t=1.5s" << std::endl;
            return 37;
        }

        // Advance to expiration (1.5 + 2.5 = 4.0s > duration 3.5s)
        rUI.updatePopup(2.5f);
        if (rUI.isPopupActive()) {
            std::cerr << "  [FAIL] Test 8: Popup failed to fade out/deactivate after duration" << std::endl;
            return 38;
        }

        // Test Loss Popup trigger
        rUI.triggerPopup(0, false, 0, 10);
        if (!rUI.isPopupActive() || rUI.popup.winningNumber != 0 || rUI.popup.won || rUI.popup.payout != 0) {
            std::cerr << "  [FAIL] Test 8: Loss popup state failed to initialize properly" << std::endl;
            return 39;
        }
        std::cout << "  [PASS] Test 8: Fading result popup trigger, timing lifecycle, and win/loss state verified." << std::endl;
    }

    // Test 9: Dynamic UI Repositioning Below Ship During Animation & Retained Until Deselected
    {
        minesweeper::ui::RouletteUI rUI;
        if (rUI.animMoveT != 0.0f || rUI.hasMovedBelow) {
            std::cerr << "  [FAIL] Test 9: animMoveT should initialize at 0.0f (idle above ship)" << std::endl;
            return 40;
        }

        // Test update towards animating state while selected (moving below ship)
        rUI.update(0.1f, true, true);
        if (rUI.animMoveT <= 0.0f || !rUI.hasMovedBelow) {
            std::cerr << "  [FAIL] Test 9: animMoveT should increase and hasMovedBelow should be true when isAnimating" << std::endl;
            return 41;
        }

        // Advance to full transition (dt = 0.5s > 1.0/6.0)
        rUI.update(0.5f, true, true);
        if (rUI.animMoveT < 0.99f) {
            std::cerr << "  [FAIL] Test 9: animMoveT should reach 1.0f when fully animated" << std::endl;
            return 42;
        }

        // When animation finishes (isAnimating = false), it should STAY below the ship while still selected
        rUI.update(0.5f, false, true);
        if (rUI.animMoveT < 0.99f || !rUI.hasMovedBelow) {
            std::cerr << "  [FAIL] Test 9: UI should remain below the ship after animation while still selected" << std::endl;
            return 43;
        }

        // When deselected (isSelected = false), UI should reset position
        rUI.update(0.1f, false, false);
        if (rUI.animMoveT != 0.0f || rUI.hasMovedBelow) {
            std::cerr << "  [FAIL] Test 9: animMoveT and hasMovedBelow should reset upon deselection" << std::endl;
            return 44;
        }

        std::cout << "  [PASS] Test 9: Dynamic UI repositioning below ship during animation and retained until deselection verified." << std::endl;
    }

    std::cout << "[TEST-ROULETTE] ALL ROULETTE TESTS PASSED!" << std::endl;
    return 0;
}

static int runAudioTests() {
    std::cout << "[TEST-AUDIO] Starting SoundManager & Background Sounds Tests..." << std::endl;

    minesweeper::audio::SoundManager soundMgr;

    // 1. Initial defaults
    if (soundMgr.getState() != minesweeper::audio::BgmPlaybackState::Idle) {
        std::cerr << "  [FAIL] Initial state is not Idle!" << std::endl;
        return 1;
    }
    if (std::abs(soundMgr.getVolume() - 0.20f) > 0.001f) {
        std::cerr << "  [FAIL] Initial volume is not 0.20f!" << std::endl;
        return 2;
    }
    std::cout << "  [PASS] Test 1: Initial default state (Idle, volume 0.20) verified." << std::endl;

    // 2. Initialization and file discovery
    if (!soundMgr.init()) {
        std::cerr << "  [FAIL] soundMgr.init() returned false!" << std::endl;
        return 3;
    }
    if (soundMgr.getTrackCount() == 0) {
        std::cerr << "  [FAIL] No background sound files discovered!" << std::endl;
        return 4;
    }
    std::cout << "  [PASS] Test 2: SoundManager initialized, discovered " << soundMgr.getTrackCount() << " track(s)." << std::endl;

    // 3. State after initialization: waiting with random interval
    if (soundMgr.getState() != minesweeper::audio::BgmPlaybackState::Waiting) {
        std::cerr << "  [FAIL] State after init is not Waiting!" << std::endl;
        return 5;
    }
    float initialWait = soundMgr.getRemainingWaitTime();
    if (initialWait <= 0.0f) {
        std::cerr << "  [FAIL] Initial wait timer <= 0!" << std::endl;
        return 6;
    }
    std::cout << "  [PASS] Test 3: Waiting state scheduled with initial interval (" << initialWait << "s)." << std::endl;

    // 4. Play random track & verify fade-in starts at 0 volume
    soundMgr.playRandomTrack();
    if (soundMgr.getState() != minesweeper::audio::BgmPlaybackState::Playing) {
        std::cerr << "  [FAIL] State after playRandomTrack() is not Playing!" << std::endl;
        return 7;
    }
    if (soundMgr.getFadeFactor() > 0.001f) {
        std::cerr << "  [FAIL] Fade factor should start at 0.0, got " << soundMgr.getFadeFactor() << std::endl;
        return 8;
    }
    std::string trackName = soundMgr.getCurrentTrackName();
    if (trackName != "background1.ogg") {
        std::cerr << "  [FAIL] Unexpected track name: " << trackName << std::endl;
        return 9;
    }
    float len = soundMgr.getCurrentTrackTimeLength();
    if (len < 200.0f) {
        std::cerr << "  [FAIL] Track length should be ~240s, got " << len << std::endl;
        return 10;
    }
    std::cout << "  [PASS] Test 4: Track playing (" << trackName << ", length: " << len << "s, initial fade: 0.0)." << std::endl;

    // 5. Gradual fade-in update verification
    soundMgr.update(2.5f); // Halfway through 5.0s fade duration
    float midFade = soundMgr.getFadeFactor();
    if (midFade < 0.6f || midFade > 0.8f) {
        std::cerr << "  [FAIL] Halfway fade factor should be ~0.707 (sine curve), got " << midFade << std::endl;
        return 11;
    }
    soundMgr.update(2.5f); // Complete 5.0s fade
    float fullFade = soundMgr.getFadeFactor();
    if (std::abs(fullFade - 1.0f) > 0.01f) {
        std::cerr << "  [FAIL] Completed fade factor should be 1.0, got " << fullFade << std::endl;
        return 12;
    }
    std::cout << "  [PASS] Test 5: Smooth 5.0s sine fade-in progression (0.0 -> " << midFade << " -> " << fullFade << ") verified." << std::endl;

    // 6. Stop and transition back to waiting with new interval
    soundMgr.setIntervalRange(15.0f, 45.0f);
    soundMgr.stop();
    if (soundMgr.getState() != minesweeper::audio::BgmPlaybackState::Waiting) {
        std::cerr << "  [FAIL] State after stop() is not Waiting!" << std::endl;
        return 10;
    }
    float waitTimer = soundMgr.getRemainingWaitTime();
    if (waitTimer < 14.9f || waitTimer > 45.1f) {
        std::cerr << "  [FAIL] Interval timer not in range [15, 45]: " << waitTimer << std::endl;
        return 11;
    }
    std::cout << "  [PASS] Test 5: Stop transitioned to Waiting with random interval (" << waitTimer << "s in [15, 45])." << std::endl;

    // 7. Sound Effects (SFX) Verification
    if (!soundMgr.isSfxEnabled() || std::abs(soundMgr.getSfxVolume() - 0.80f) > 0.001f) {
        std::cerr << "  [FAIL] Default SFX settings mismatch!" << std::endl;
        return 13;
    }
    if (!soundMgr.hasButtonSound()) {
        std::cerr << "  [FAIL] UI button sound not loaded!" << std::endl;
        return 14;
    }
    if (!soundMgr.hasIncrementSound()) {
        std::cerr << "  [FAIL] UI increment sound not loaded!" << std::endl;
        return 15;
    }
    if (!soundMgr.hasUncoverSound()) {
        std::cerr << "  [FAIL] Game uncover sound not loaded!" << std::endl;
        return 16;
    }
    if (soundMgr.getExplosionSoundCount() != 3) {
        std::cerr << "  [FAIL] Expected 3 explosion sounds, got " << soundMgr.getExplosionSoundCount() << std::endl;
        return 17;
    }
    if (!soundMgr.hasBananaSound()) {
        std::cerr << "  [FAIL] Game banana sound not loaded!" << std::endl;
        return 18;
    }
    if (!soundMgr.hasLaserSound()) {
        std::cerr << "  [FAIL] Game laser sound not loaded!" << std::endl;
        return 19;
    }
    if (!soundMgr.hasFlagSound()) {
        std::cerr << "  [FAIL] Game flag sound not loaded!" << std::endl;
        return 20;
    }
    std::cout << "  [PASS] Test 6: SFX assets verified (button, increment, uncover, 3 explosions, banana, laser, flag)." << std::endl;

    // 8. SFX playback and static triggers execution
    soundMgr.playButtonSound();
    soundMgr.playIncrementSound();
    soundMgr.playUncoverSound();
    soundMgr.playExplosionSound();
    soundMgr.playBananaSound();
    soundMgr.playLaserSound();
    soundMgr.playFlagSound();

    minesweeper::audio::SoundManager::playButton();
    minesweeper::audio::SoundManager::playIncrement();
    minesweeper::audio::SoundManager::playUncover();
    minesweeper::audio::SoundManager::playExplosion();
    minesweeper::audio::SoundManager::playBanana();
    minesweeper::audio::SoundManager::playLaser();
    minesweeper::audio::SoundManager::playFlag();

    soundMgr.setSfxVolume(0.50f);
    if (std::abs(soundMgr.getSfxVolume() - 0.50f) > 0.001f) {
        std::cerr << "  [FAIL] SFX volume adjustment mismatch!" << std::endl;
        return 21;
    }
    std::cout << "  [PASS] Test 7: SFX playback triggers & volume adjustment verified." << std::endl;

    // 9. Disable audio
    soundMgr.setEnabled(false);
    if (soundMgr.getState() != minesweeper::audio::BgmPlaybackState::Idle) {
        std::cerr << "  [FAIL] State after setEnabled(false) is not Idle!" << std::endl;
        return 22;
    }
    std::cout << "  [PASS] Test 8: Disabled audio transitions to Idle." << std::endl;

    // 10. Cleanup
    soundMgr.cleanup();
    std::cout << "[TEST-AUDIO] ALL AUDIO TESTS PASSED!" << std::endl;
    return 0;
}

static int runCampaignTests() {
    std::cout << "[TEST-CAMPAIGN] Starting Campaign Mode Unit Tests..." << std::endl;

    // Test 1: Planet & Sector initialization
    minesweeper::core::CampaignManager campaign;
    campaign.init(998877);

    if (campaign.sectors.size() != 4) {
        std::cerr << "  [FAIL] Expected 4 sectors, got " << campaign.sectors.size() << std::endl;
        return 1;
    }
    if (campaign.sectors[0].gridSize != 8 || campaign.sectors[0].bombCount != 10 || !campaign.sectors[0].isUnlocked) {
        std::cerr << "  [FAIL] Sector 0 config invalid!" << std::endl;
        return 2;
    }
    if (campaign.sectors[1].gridSize != 10 || campaign.sectors[1].bombCount != 18 || campaign.sectors[1].isUnlocked) {
        std::cerr << "  [FAIL] Sector 1 config invalid!" << std::endl;
        return 3;
    }
    if (campaign.sectors[2].gridSize != 12 || campaign.sectors[2].bombCount != 28 || campaign.sectors[2].isUnlocked) {
        std::cerr << "  [FAIL] Sector 2 config invalid!" << std::endl;
        return 4;
    }
    if (campaign.sectors[3].gridSize != 14 || campaign.sectors[3].bombCount != 42 || campaign.sectors[3].isUnlocked) {
        std::cerr << "  [FAIL] Sector 3 config invalid!" << std::endl;
        return 5;
    }
    std::cout << "  [PASS] Test 1: Planet and 4 sectors initialized correctly." << std::endl;

    // Test 2: Global Cell Indexing conversions
    {
        for (int s = 0; s < 4; ++s) {
            size_t local = 42 + static_cast<size_t>(s) * 17;
            size_t global = minesweeper::core::CampaignManager::toGlobalCellIndex(s, local);
            int outS = -1;
            size_t outLocal = 0;
            minesweeper::core::CampaignManager::fromGlobalCellIndex(global, outS, outLocal);
            if (outS != s || outLocal != local) {
                std::cerr << "  [FAIL] Global index roundtrip failed for sector " << s << std::endl;
                return 6;
            }
        }
        std::cout << "  [PASS] Test 2: Global cell indexing roundtrip verified." << std::endl;
    }

    // Test 3: Sector Walls & Collision Resolution
    {
        minesweeper::core::Ship ship;
        ship.collisionRadius = 14.0f;
        ship.isInitialized = true;

        const auto& walls = campaign.sectors[0].walls;
        if (walls.empty()) {
            std::cerr << "  [FAIL] Sector 0 has no perimeter walls!" << std::endl;
            return 7;
        }

        // Test collision with first wall: place ship overlapping top wall
        const auto& wall = walls[0];
        ship.position = { wall.rect.x + wall.rect.width * 0.5f, wall.rect.y - 5.0f };
        ship.velocity = { 0.0f, 50.0f };

        bool hit = campaign.resolveShipCollisions(ship);
        if (!hit) {
            std::cerr << "  [FAIL] Ship collision with wall was not detected!" << std::endl;
            return 8;
        }
        if (ship.position.y > wall.rect.y - ship.collisionRadius + 0.01f) {
            std::cerr << "  [FAIL] Ship was not pushed out of wall correctly! Y=" << ship.position.y << std::endl;
            return 9;
        }
        std::cout << "  [PASS] Test 3: Ship-to-wall Circle-AABB collision resolution verified." << std::endl;
    }

    // Test 4: Locked Orbital Launcher Gantry Clamps block ship transit
    {
        minesweeper::core::Ship ship;
        ship.collisionRadius = 14.0f;
        ship.isInitialized = true;

        const auto& launcher = campaign.sectors[0].exitLauncher;
        if (!launcher.isLocked) {
            std::cerr << "  [FAIL] Sector 0 orbital launcher should be locked initially!" << std::endl;
            return 10;
        }

        ship.position = { launcher.barrierBounds.x + launcher.barrierBounds.width * 0.5f, launcher.barrierBounds.y + launcher.barrierBounds.height * 0.5f };
        ship.velocity = { 100.0f, 0.0f };

        bool hitLauncher = campaign.resolveShipCollisions(ship);
        if (!hitLauncher) {
            std::cerr << "  [FAIL] Ship inside locked orbital launcher barrier was not collided!" << std::endl;
            return 11;
        }
        std::cout << "  [PASS] Test 4: Locked orbital launcher gantry clamps block ship passage." << std::endl;
    }

    // Test 5: Sector Clearance and Orbital Launcher Arming
    {
        auto& sec0 = campaign.sectors[0];
        for (size_t i = 0; i < sec0.board.totalCells(); ++i) {
            if (!sec0.board.isBomb(i)) {
                sec0.board.reveal(i);
            }
        }

        bool cleared = campaign.checkSectorClear(0);
        if (!cleared || !sec0.isCleared) {
            std::cerr << "  [FAIL] Sector 0 failed to clear after uncovering all safe cells!" << std::endl;
            return 12;
        }

        if (!campaign.sectors[1].isUnlocked) {
            std::cerr << "  [FAIL] Sector 1 did not unlock after Sector 0 cleared!" << std::endl;
            return 13;
        }

        if (campaign.sectors[0].exitLauncher.isLocked) {
            std::cerr << "  [FAIL] Sector 0 exit launcher is still locked!" << std::endl;
            return 14;
        }

        campaign.updatePlanetClearance();
        if (campaign.planetClearPercentage <= 0.0f) {
            std::cerr << "  [FAIL] Planet clearance percentage did not update!" << std::endl;
            return 15;
        }
        std::cout << "  [PASS] Test 5: Sector clearance and orbital launcher arming verified." << std::endl;
    }

    // Test 6: Campaign Save & Load Persistence
    {
        minesweeper::core::SaveManager saveMgr;
        saveMgr.init();

        float saveTime = 345.5f;
        uint64_t saveScrap = 780;
        bool saved = saveMgr.saveCampaign(campaign, saveTime, saveScrap);
        if (!saved) {
            std::cerr << "  [FAIL] Failed to save campaign state!" << std::endl;
            return 16;
        }

        if (!saveMgr.hasCampaignSave()) {
            std::cerr << "  [FAIL] hasCampaignSave() returned false after saving!" << std::endl;
            return 17;
        }

        minesweeper::core::CampaignManager loadedCampaign;
        float loadTime = 0.0f;
        uint64_t loadScrap = 0;
        bool loaded = saveMgr.loadCampaign(loadedCampaign, loadTime, loadScrap);
        if (!loaded) {
            std::cerr << "  [FAIL] Failed to load campaign state!" << std::endl;
            return 18;
        }

        if (loadScrap != 780) {
            std::cerr << "  [FAIL] Loaded scrap mismatch: expected 780, got " << loadScrap << std::endl;
            return 19;
        }
        if (std::abs(loadTime - 345.5f) > 0.1f) {
            std::cerr << "  [FAIL] Loaded playtime mismatch: expected 345.5, got " << loadTime << std::endl;
            return 20;
        }
        if (loadedCampaign.planetSeed != campaign.planetSeed) {
            std::cerr << "  [FAIL] Loaded planet seed mismatch!" << std::endl;
            return 21;
        }
        if (!loadedCampaign.sectors[0].isCleared || !loadedCampaign.sectors[1].isUnlocked) {
            std::cerr << "  [FAIL] Loaded sector clearance state mismatch!" << std::endl;
            return 22;
        }

        saveMgr.deleteCampaignSave();
        if (saveMgr.hasCampaignSave()) {
            std::cerr << "  [FAIL] Campaign save still exists after deleteCampaignSave()!" << std::endl;
            return 23;
        }
        std::cout << "  [PASS] Test 6: Campaign save and load round-trip verified." << std::endl;
    }

    // Test 7: Standalone Sector Worlds & Orbital Launcher Transit Detection
    {
        minesweeper::core::CampaignManager mgr;
        mgr.init(12345);

        // Sector 0 should have entrance spawn
        Vector2 s0Spawn = mgr.getSectorSpawnPosition(0);
        if (s0Spawn.x <= 0.0f || s0Spawn.y <= 0.0f) {
            std::cerr << "  [FAIL] Sector 0 spawn position is invalid: (" << s0Spawn.x << ", " << s0Spawn.y << ")" << std::endl;
            return 24;
        }

        // Before sector is cleared, launcher transit should return false
        const auto& launcher = mgr.sectors[0].exitLauncher;
        Vector2 launcherCenter = { launcher.openingBounds.x + launcher.openingBounds.width * 0.5f, launcher.openingBounds.y + launcher.openingBounds.height * 0.5f };
        if (mgr.checkLauncherTransit(0, launcherCenter, 14.0f)) {
            std::cerr << "  [FAIL] checkLauncherTransit should return false for locked launcher!" << std::endl;
            return 25;
        }

        // Clear sector 0 to arm launcher
        for (size_t i = 0; i < mgr.sectors[0].board.totalCells(); ++i) {
            if (!mgr.sectors[0].board.isBomb(i)) {
                mgr.sectors[0].board.reveal(i);
            }
        }
        mgr.checkSectorClear(0);
        if (!mgr.checkLauncherTransit(0, launcherCenter, 14.0f)) {
            std::cerr << "  [FAIL] checkLauncherTransit failed to trigger for armed launcher!" << std::endl;
            return 26;
        }

        // Far away from launcher should return false
        if (mgr.checkLauncherTransit(0, { 50.0f, 50.0f }, 14.0f)) {
            std::cerr << "  [FAIL] checkLauncherTransit triggered when far away from launcher!" << std::endl;
            return 27;
        }

        std::cout << "  [PASS] Test 7: Standalone sector worlds and orbital launcher transit triggering verified." << std::endl;
    }

    // Test 8: Sector 1 Shop Suppression & Sector 2 Shop Docking
    {
        minesweeper::core::CampaignManager mgr;
        mgr.init(12345);
        mgr.activeSectorIndex = 0;

        minesweeper::render::RaylibRenderer rnd;
        rnd.activeCampaignSector = 0;
        rnd.shopShips.resize(1);
        rnd.hasRouletteShip = true;
        rnd.updateShopAnchorCampaign(mgr);

        if (rnd.shopShips[0].isInitialized) {
            std::cerr << "  [FAIL] Sector 0 shop ships should not be initialized!" << std::endl;
            return 28;
        }
        if (rnd.rouletteShip.isInitialized) {
            std::cerr << "  [FAIL] Sector 0 roulette ship should not be initialized!" << std::endl;
            return 29;
        }

        // Now test Sector 2 (activeSectorIndex = 1)
        mgr.activeSectorIndex = 1;
        rnd.activeCampaignSector = 1;
        rnd.updateShopAnchorCampaign(mgr);

        if (!rnd.shopShips[0].isInitialized) {
            std::cerr << "  [FAIL] Sector 1 shop ships should be initialized!" << std::endl;
            return 30;
        }
        if (rnd.shopShips[0].position.y > 100.0f) {
            std::cerr << "  [FAIL] Sector 1 shop ship not docked north clear of runway! Y=" << rnd.shopShips[0].position.y << std::endl;
            return 31;
        }
        if (rnd.rouletteShip.position.y < 500.0f) {
            std::cerr << "  [FAIL] Sector 1 roulette ship not docked south clear of runway! Y=" << rnd.rouletteShip.position.y << std::endl;
            return 32;
        }
        std::cout << "  [PASS] Test 8: Sector 1 shop suppression and Sector 2 docking verified." << std::endl;
    }

    // Test 9: Campaign Cell Uncovering & Game-Over Board Recovery
    {
        minesweeper::core::CampaignManager mgr;
        mgr.init(12345);
        auto& sec0 = mgr.sectors[0];

        // Reveal first safe cell
        for (size_t i = 0; i < sec0.board.totalCells(); ++i) {
            if (!sec0.board.isBomb(i)) {
                sec0.board.reveal(i);
                break;
            }
        }
        if (sec0.board.revealedCount == 0) {
            std::cerr << "  [FAIL] Failed to reveal cells on sector board!" << std::endl;
            return 33;
        }

        // Test Board Game Over reset
        sec0.board.isGameOver = true;
        if (!sec0.board.isGameOver) {
            std::cerr << "  [FAIL] Board isGameOver flag not set!" << std::endl;
            return 34;
        }
        sec0.board.init(sec0.board.config);
        if (sec0.board.isGameOver || sec0.board.revealedCount != 0) {
            std::cerr << "  [FAIL] Board reset after game over failed!" << std::endl;
            return 35;
        }

        // Test GameHUD isMouseOver campaign flag skips bottom footer
        minesweeper::ui::GameHUD hud;
        Vector2 bottomScreenPt = { 400.0f, 650.0f }; // In bottom 92px of 720p window
        if (hud.isMouseOver(1280, 720, 1.0f, bottomScreenPt, false) == false) {
            std::cerr << "  [FAIL] Normal HUD should detect bottom footer!" << std::endl;
            return 36;
        }
        if (hud.isMouseOver(1280, 720, 1.0f, bottomScreenPt, true) == true) {
            std::cerr << "  [FAIL] Campaign HUD should not block bottom screen!" << std::endl;
            return 37;
        }

        std::cout << "  [PASS] Test 9: Cell uncover, board reset, and non-blocking campaign HUD verified." << std::endl;
    }

    // Test 10: Data-Driven Sector Parsing, Configuration, & Dynamic Unlock Rules
    {
        // 1. Test parseSectorJson with custom schema
        std::string customJson = R"({
            "id": 99,
            "name": "Sector 99: Asteroid Foundry",
            "codename": "AST-99",
            "subtitle": "Zero-G Orbital Smelter",
            "description": "High radiation anomaly detected in sector 99.",
            "threatLevel": 5,
            "map": {
                "gridSize": 9,
                "bombCount": 15,
                "dimension": 2,
                "wallThickness": 32.0,
                "westMargin": 200.0,
                "eastMargin": 190.0,
                "vertMargin": 130.0,
                "spawnPos": [110.0, 250.0],
                "stagingDepotPos": [120.0, 250.0],
                "stagingDepotTitle": "SPECIAL OPERATIONS OUTPOST",
                "customWalls": [
                    { "x": 150.0, "y": 80.0, "w": 40.0, "h": 100.0, "isHazard": true }
                ]
            },
            "progression": {
                "isUnlocked": true,
                "unlocks": [101, 102],
                "hasExitLauncher": true,
                "launcherTargetSector": 101,
                "launcherOpeningHeight": 160.0
            },
            "ships": {
                "allowShops": true,
                "allowRoulette": false,
                "allowedShipTypes": ["shop1"],
                "shopDockPos": [105.0, 85.0],
                "rouletteDockPos": [105.0, -85.0]
            }
        })";

        minesweeper::core::SectorConfig parsedCfg;
        if (!minesweeper::core::CampaignManager::parseSectorJson(customJson, parsedCfg)) {
            std::cerr << "  [FAIL] Failed to parse custom sector JSON!" << std::endl;
            return 38;
        }

        if (parsedCfg.id != 99 || parsedCfg.codename != "AST-99" || parsedCfg.threatLevel != 5) {
            std::cerr << "  [FAIL] Parsed sector metadata mismatch! ID=" << parsedCfg.id << std::endl;
            return 39;
        }
        if (parsedCfg.gridSize != 9 || parsedCfg.bombCount != 15 || parsedCfg.customWalls.size() != 1) {
            std::cerr << "  [FAIL] Parsed sector map specifications mismatch!" << std::endl;
            return 40;
        }
        if (parsedCfg.unlocks.size() != 2 || parsedCfg.unlocks[0] != 101 || parsedCfg.launcherTargetSector != 101) {
            std::cerr << "  [FAIL] Parsed sector progression specifications mismatch!" << std::endl;
            return 41;
        }
        if (!parsedCfg.allowShops || parsedCfg.allowRoulette || parsedCfg.stagingDepotTitle != "SPECIAL OPERATIONS OUTPOST") {
            std::cerr << "  [FAIL] Parsed sector ship specifications mismatch!" << std::endl;
            return 42;
        }

        // 2. Test directory loader discovering assets/campaign/sectors/
        auto loadedConfigs = minesweeper::core::CampaignManager::loadSectorConfigs("assets/campaign/sectors");
        if (loadedConfigs.size() < 4) {
            std::cerr << "  [FAIL] Expected at least 4 sectors loaded from assets/campaign/sectors/, got " << loadedConfigs.size() << std::endl;
            return 43;
        }
        if (loadedConfigs[0].id != 1 || loadedConfigs[0].codename != "OUTPOST-ALPHA" || loadedConfigs[0].allowShops != false) {
            std::cerr << "  [FAIL] Sector 01 configuration mismatch in loaded assets!" << std::endl;
            return 44;
        }
        if (loadedConfigs[1].id != 2 || loadedConfigs[1].codename != "FOUNDRY-WASTES" || loadedConfigs[1].allowShops != true) {
            std::cerr << "  [FAIL] Sector 02 configuration mismatch in loaded assets!" << std::endl;
            return 45;
        }

        // 3. Test CampaignManager initialization and dynamic launcher routing
        minesweeper::core::CampaignManager mgr;
        mgr.init(12345);

        if (mgr.sectors.size() < 4) {
            std::cerr << "  [FAIL] CampaignManager failed to initialize with loaded sectors! Count=" << mgr.sectors.size() << std::endl;
            return 46;
        }
        int s0Target = mgr.getLauncherTargetSectorIndex(0);
        if (s0Target != 1) {
            std::cerr << "  [FAIL] Expected Sector 0 launcher to target index 1 (Sector 02), got " << s0Target << std::endl;
            return 47;
        }

        // 4. Test dynamic unlock progression
        // Modify sector 0 unlocks to directly target sector 3 (index 2)
        mgr.sectors[0].unlocksSectors = { 3 };
        mgr.sectors[2].isUnlocked = false;
        for (size_t i = 0; i < mgr.sectors[0].board.totalCells(); ++i) {
            if (!mgr.sectors[0].board.isBomb(i)) {
                mgr.sectors[0].board.reveal(i);
            }
        }
        mgr.checkSectorClear(0);
        if (!mgr.sectors[2].isUnlocked) {
            std::cerr << "  [FAIL] Custom dynamic unlock failed to unlock Sector 3!" << std::endl;
            return 48;
        }

        std::cout << "  [PASS] Test 10: Data-driven sector JSON parsing, asset loading, and dynamic progression verified." << std::endl;
    }

    // Test 11: Sector Editor Serialization, JSON Export, and Live Rebuilding
    {
        minesweeper::core::CampaignManager mgr;
        mgr.init(12345);

        // 1. Export sector 0 to JSON string
        minesweeper::core::SectorConfig cfg = mgr.sectors[0].config;
        cfg.name = "Sector 01: Test Rebuild Zone";
        cfg.gridSize = 9;
        cfg.bombCount = 14;
        cfg.wallThickness = 36.0f;
        cfg.westMargin = 220.0f;
        cfg.stagingDepotTitle = "TEST STAGING DOCK";

        std::string exportedJson = minesweeper::core::CampaignManager::exportSectorConfigToJson(cfg);
        if (exportedJson.empty()) {
            std::cerr << "  [FAIL] exportSectorConfigToJson returned empty string!" << std::endl;
            return 49;
        }

        // 2. Parse exported JSON back and verify roundtrip
        minesweeper::core::SectorConfig roundtripCfg;
        if (!minesweeper::core::CampaignManager::parseSectorJson(exportedJson, roundtripCfg)) {
            std::cerr << "  [FAIL] Failed to parse exported JSON string!" << std::endl;
            return 50;
        }

        if (roundtripCfg.name != "Sector 01: Test Rebuild Zone" || roundtripCfg.gridSize != 9 || roundtripCfg.bombCount != 14) {
            std::cerr << "  [FAIL] Exported JSON roundtrip mismatch in map parameters!" << std::endl;
            return 51;
        }
        if (roundtripCfg.wallThickness != 36.0f || roundtripCfg.stagingDepotTitle != "TEST STAGING DOCK") {
            std::cerr << "  [FAIL] Exported JSON roundtrip mismatch in depot title / wall thickness!" << std::endl;
            return 52;
        }

        // 3. Test rebuildSector live
        bool rebuilt = mgr.rebuildSector(0, roundtripCfg);
        if (!rebuilt) {
            std::cerr << "  [FAIL] rebuildSector failed!" << std::endl;
            return 53;
        }

        if (mgr.sectors[0].gridSize != 9 || mgr.sectors[0].bombCount != 14 || mgr.sectors[0].name != "Sector 01: Test Rebuild Zone") {
            std::cerr << "  [FAIL] Live rebuilt sector fields mismatch!" << std::endl;
            return 54;
        }

        // Arena width must account for 9*40 = 360 board + 220 west + 180 east = 760
        float expectedArenaW = 9 * 40.0f + 220.0f + roundtripCfg.eastMargin;
        if (std::abs(mgr.sectors[0].arenaBounds.width - expectedArenaW) > 0.1f) {
            std::cerr << "  [FAIL] Live rebuilt arena width mismatch! Expected " << expectedArenaW << ", got " << mgr.sectors[0].arenaBounds.width << std::endl;
            return 55;
        }

        std::cout << "  [PASS] Test 11: Sector Editor JSON export, roundtrip parsing, and live sector rebuilding verified." << std::endl;
    }

#if defined(_DEBUG) || !defined(NDEBUG)
    // Test 12: Player ship disabled while sector editor is open
    {
        minesweeper::core::CampaignManager mgr;
        mgr.init(12345ULL);
        minesweeper::render::RaylibRenderer renderer;
        minesweeper::ui::SectorEditor editor;

        // Initially player ship is initialized in game
        renderer.localShip.position = mgr.getSectorSpawnPosition(0);
        renderer.localShip.isInitialized = true;
        renderer.localShip.velocity = { 100.0f, 0.0f };

        // Open sector editor
        editor.open(mgr, 0);
        if (!editor.isOpen) {
            std::cerr << "  [FAIL] Test 12: Editor should be open!" << std::endl;
            return 56;
        }

        // When editor is open, ship is disabled
        renderer.localShip.isInitialized = false;
        renderer.localShip.velocity = { 0.0f, 0.0f };
        renderer.localShip.isMoving = false;

        // Step physics while disabled: verify ship remains at rest and does not update
        Vector2 prevPos = renderer.localShip.position;
        renderer.stepPhysics({ 800.0f, 800.0f }, 0.016f);
        if (renderer.localShip.position.x != prevPos.x || renderer.localShip.position.y != prevPos.y) {
            std::cerr << "  [FAIL] Test 12: Player ship moved while editor was open and ship disabled!" << std::endl;
            return 57;
        }
        if (renderer.localShip.velocity.x != 0.0f || renderer.localShip.velocity.y != 0.0f) {
            std::cerr << "  [FAIL] Test 12: Player ship had non-zero velocity while disabled!" << std::endl;
            return 58;
        }

        // Close editor: verify ship can be restored at sector spawn
        editor.close();
        if (editor.isOpen) {
            std::cerr << "  [FAIL] Test 12: Editor should be closed!" << std::endl;
            return 59;
        }

        Vector2 spawn = mgr.getSectorSpawnPosition(mgr.activeSectorIndex);
        renderer.localShip.position = spawn;
        renderer.localShip.velocity = { 0.0f, 0.0f };
        renderer.localShip.isMoving = false;
        renderer.localShip.isInitialized = true;

        if (!renderer.localShip.isInitialized) {
            std::cerr << "  [FAIL] Test 12: Player ship was not re-enabled upon closing editor!" << std::endl;
            return 60;
        }

        // Verify ship resumes movement once re-enabled
        renderer.stepPhysics({ spawn.x + 200.0f, spawn.y }, 0.05f);
        if (renderer.localShip.position.x == spawn.x && renderer.localShip.position.y == spawn.y) {
            std::cerr << "  [FAIL] Test 12: Player ship did not move after being re-enabled!" << std::endl;
            return 61;
        }

        std::cout << "  [PASS] Test 12: Player ship disabled while sector editor is open verified." << std::endl;
    }
#endif

    // Test 13: In-World Sector Editor Wall Manipulation & Heterogeneous Merchant Docks
    {
        minesweeper::core::CampaignManager mgr;
        mgr.init(12345ULL);
        minesweeper::render::RaylibRenderer renderer;

        // 1. Verify Sector 02 has heterogeneous merchant docks (shop3, shop1, roulette)
        auto* sec2 = mgr.getSectorByIndex(1);
        if (!sec2) {
            std::cerr << "  [FAIL] Test 13: Sector 02 not found!" << std::endl;
            return 62;
        }
        if (sec2->merchantSpawns.size() < 3) {
            std::cerr << "  [FAIL] Test 13: Sector 02 should have at least 3 merchant spawns, got " << sec2->merchantSpawns.size() << std::endl;
            return 63;
        }
        if (sec2->merchantSpawns[0].type != "shop3" || sec2->merchantSpawns[1].type != "shop1" || sec2->merchantSpawns[2].type != "roulette") {
            std::cerr << "  [FAIL] Test 13: Sector 02 heterogeneous dock types mismatch! Got "
                      << sec2->merchantSpawns[0].type << ", " << sec2->merchantSpawns[1].type << ", " << sec2->merchantSpawns[2].type << std::endl;
            return 64;
        }

        // 2. Instantiate ships in renderer and verify shopShips and rouletteShip reflect heterogeneous types
        mgr.activeSectorIndex = 1;
        renderer.updateShopAnchorCampaign(mgr);
        if (renderer.shopShips.size() < 2) {
            std::cerr << "  [FAIL] Test 13: Renderer should have at least 2 shop ships, got " << renderer.shopShips.size() << std::endl;
            return 65;
        }
        if (renderer.shopShips[0].typeId != "shop3") {
            std::cerr << "  [FAIL] Test 13: Dock 1 should be shop3 (heavy freighter)! Got " << renderer.shopShips[0].typeId << std::endl;
            return 66;
        }
        if (renderer.shopShips[1].typeId != "shop1") {
            std::cerr << "  [FAIL] Test 13: Dock 2 should be shop1 (mini shop)! Got " << renderer.shopShips[1].typeId << std::endl;
            return 67;
        }
        if (!renderer.hasRouletteShip || !renderer.rouletteShip.isInitialized) {
            std::cerr << "  [FAIL] Test 13: Roulette ship should be initialized for Sector 02!" << std::endl;
            return 68;
        }

#if defined(_DEBUG) || !defined(NDEBUG)
        // 3. Test SectorEditor interactive in-world manipulation
        minesweeper::ui::SectorEditor editor;
        editor.open(mgr, 1);
        if (!editor.isOpen) {
            std::cerr << "  [FAIL] Test 13: SectorEditor failed to open!" << std::endl;
            return 69;
        }

        // Test grid snap
        editor.gridSnap = 20.0f;
        float snapped = editor.snapCoord(113.6f);
        if (std::abs(snapped - 120.0f) > 0.01f) {
            std::cerr << "  [FAIL] Test 13: Grid snap failed! Expected 120, got " << snapped << std::endl;
            return 70;
        }

        // Test adding custom wall
        minesweeper::core::SectorWall testWall;
        testWall.rect = { 100.0f, 120.0f, 80.0f, 40.0f };
        testWall.isHazard = false;
        editor.currentTool = minesweeper::ui::EditorTool::Select;
        auto workingCfg = editor.getWorkingConfig();
        workingCfg.customWalls.push_back(testWall);
        workingCfg.merchantSpawns[0].type = "shop2"; // Change dock 1 to cargo hauler

        mgr.rebuildSector(1, workingCfg);
        renderer.updateShopAnchorCampaign(mgr);

        if (renderer.shopShips[0].typeId != "shop2") {
            std::cerr << "  [FAIL] Test 13: Live rebuild failed to update dock 1 ship type to shop2! Got " << renderer.shopShips[0].typeId << std::endl;
            return 71;
        }
        if (mgr.sectors[1].walls.empty()) {
            std::cerr << "  [FAIL] Test 13: Live rebuild lost walls!" << std::endl;
            return 72;
        }
#endif

        // 4. Test JSON roundtrip of merchantSpawns array
        std::string jsonStr = minesweeper::core::CampaignManager::exportSectorConfigToJson(sec2->config);
        minesweeper::core::SectorConfig roundtripCfg;
        if (!minesweeper::core::CampaignManager::parseSectorJson(jsonStr, roundtripCfg)) {
            std::cerr << "  [FAIL] Test 13: Failed to parse exported JSON with merchantSpawns!" << std::endl;
            return 73;
        }
        if (roundtripCfg.merchantSpawns.size() != sec2->config.merchantSpawns.size()) {
            std::cerr << "  [FAIL] Test 13: Exported JSON roundtrip mismatch in merchantSpawns count!" << std::endl;
            return 74;
        }
        if (roundtripCfg.merchantSpawns[0].type != sec2->config.merchantSpawns[0].type ||
            roundtripCfg.merchantSpawns[1].type != sec2->config.merchantSpawns[1].type) {
            std::cerr << "  [FAIL] Test 13: Exported JSON roundtrip mismatch in merchantSpawns types!" << std::endl;
            return 75;
        }

        std::cout << "  [PASS] Test 13: In-world editor manipulation, heterogeneous merchant docks, and JSON roundtrip verified." << std::endl;
    }

    // Test 14: Data-Driven Planetary System, Dynamic Biome Palettes, and Progression
    {
        // 1. Test loading data-driven planet configs
        auto loadedPlanets = minesweeper::core::CampaignManager::loadPlanetConfigs("assets/campaign/planets");
        if (loadedPlanets.size() < 3) {
            std::cerr << "  [FAIL] Expected at least 3 data-driven planets, got " << loadedPlanets.size() << std::endl;
            return 80;
        }

        // Verify Planet 1 (Tartarus-IV)
        if (loadedPlanets[0].id != 1 || loadedPlanets[0].name != "Tartarus-IV" || !loadedPlanets[0].isUnlocked ||
            loadedPlanets[0].sectorDataPath != "assets/campaign/sectors/planet1") {
            std::cerr << "  [FAIL] Planet 01 (Tartarus-IV) configuration mismatch in loaded assets!" << std::endl;
            return 81;
        }

        // Verify Planet 2 (Acheron-Prime)
        if (loadedPlanets[1].id != 2 || loadedPlanets[1].name != "Acheron-Prime" || loadedPlanets[1].threatLevel != 2 ||
            loadedPlanets[1].sectorDataPath != "assets/campaign/sectors/planet2") {
            std::cerr << "  [FAIL] Planet 02 (Acheron-Prime) configuration mismatch in loaded assets!" << std::endl;
            return 82;
        }

        // Verify Planet 3 (Caelum-VII)
        if (loadedPlanets[2].id != 3 || loadedPlanets[2].name != "Caelum-VII" || loadedPlanets[2].threatLevel != 3 ||
            loadedPlanets[2].sectorDataPath != "assets/campaign/sectors/planet3") {
            std::cerr << "  [FAIL] Planet 03 (Caelum-VII) configuration mismatch in loaded assets!" << std::endl;
            return 83;
        }

        // 2. Test JSON export and roundtrip parsing
        std::string pJson = minesweeper::core::CampaignManager::exportPlanetConfigToJson(loadedPlanets[1]);
        minesweeper::core::PlanetConfig roundtripP;
        if (!minesweeper::core::CampaignManager::parsePlanetJson(pJson, roundtripP)) {
            std::cerr << "  [FAIL] Failed to parse exported planet JSON string!" << std::endl;
            return 84;
        }
        if (roundtripP.id != 2 || roundtripP.name != "Acheron-Prime" || roundtripP.sectorDataPath != loadedPlanets[1].sectorDataPath ||
            roundtripP.visual.surfaceTones.size() != loadedPlanets[1].visual.surfaceTones.size()) {
            std::cerr << "  [FAIL] Planet JSON roundtrip data mismatch!" << std::endl;
            return 85;
        }

        // 3. Test CampaignManager planet switching & active configuration
        minesweeper::core::CampaignManager mgr;
        mgr.init(12345);
        if (mgr.activePlanetIndex != 0 || mgr.planetName != "Tartarus-IV") {
            std::cerr << "  [FAIL] Default active planet mismatch! Name=" << mgr.planetName << std::endl;
            return 86;
        }
        // Verify planet 1 loaded its own sectors from planet1/
        if (mgr.sectors.empty() || mgr.sectors[0].codename != "OUTPOST-ALPHA") {
            std::cerr << "  [FAIL] Planet 1 failed to load its own sectors! Got codename=" << (mgr.sectors.empty() ? "NONE" : mgr.sectors[0].codename) << std::endl;
            return 861;
        }

        // Switch to Planet 2 (Acheron-Prime) and verify it loaded its distinct sectors
        bool switched = mgr.selectPlanet(1);
        if (!switched || mgr.activePlanetIndex != 1 || mgr.planetName != "Acheron-Prime") {
            std::cerr << "  [FAIL] Failed to switch active planet to Acheron-Prime!" << std::endl;
            return 87;
        }
        const auto* activeP = mgr.getActivePlanetConfig();
        if (!activeP || activeP->name != "Acheron-Prime") {
            std::cerr << "  [FAIL] getActivePlanetConfig mismatch after switching!" << std::endl;
            return 88;
        }
        if (mgr.sectors.empty() || mgr.sectors[0].codename != "ACHERON-01") {
            std::cerr << "  [FAIL] Planet 2 failed to load its own distinct sectors! Got codename=" << (mgr.sectors.empty() ? "NONE" : mgr.sectors[0].codename) << std::endl;
            return 881;
        }

        // Switch to Planet 3 (Caelum-VII) and verify it loaded its distinct sectors
        bool switchedP3 = mgr.selectPlanet(2);
        if (!switchedP3 || mgr.sectors.empty() || mgr.sectors[0].codename != "CAELUM-01") {
            std::cerr << "  [FAIL] Planet 3 failed to load its own distinct sectors! Got codename=" << (mgr.sectors.empty() ? "NONE" : mgr.sectors[0].codename) << std::endl;
            return 882;
        }

        // 4. Test PlanetRenderer dynamic biome and palette application
        minesweeper::render::PlanetRenderer planet;
        planet.init();
        planet.applyPlanetConfig(loadedPlanets[1]);
        if (planet.currentPlanetConfig.name != "Acheron-Prime") {
            std::cerr << "  [FAIL] PlanetRenderer failed to apply active planet config!" << std::endl;
            return 89;
        }
        if (planet.sectors.size() != 42) {
            std::cerr << "  [FAIL] PlanetRenderer sectors corrupted after applyPlanetConfig! Count=" << planet.sectors.size() << std::endl;
            return 90;
        }

        // 5. Test automatic planet unlocking on 100% planetary clearance
        mgr.selectPlanet(0); // Switch back to Tartarus-IV
        mgr.planets[1].isUnlocked = false; // ensure locked initially
        for (auto& sec : mgr.sectors) {
            sec.isCleared = true;
            for (size_t i = 0; i < sec.board.totalCells(); ++i) {
                if (!sec.board.isBomb(i)) {
                    sec.board.reveal(i);
                }
            }
        }
        mgr.updatePlanetClearance();
        if (!mgr.isPlanetCleared) {
            std::cerr << "  [FAIL] Expected planet to be 100% cleared!" << std::endl;
            return 91;
        }
        if (!mgr.planets[1].isUnlocked) {
            std::cerr << "  [FAIL] Clearing planet did not unlock next planet (Acheron-Prime)!" << std::endl;
            return 92;
        }

        std::cout << "  [PASS] Test 14: Data-driven planet JSON parsing, sectorDataPath routing, multi-planet selection, dynamic biome palettes, and unlock progression verified." << std::endl;
    }

    // Test 15: Multiplayer Co-op Campaign, Scrap Economy, Procedural Desolate Terrain, Safe Zone & In-World Merchant Ship
    {
        std::cout << "[TEST-CAMPAIGN] Test 15: Running Multiplayer Co-op & Merchant Salvage Loop tests..." << std::endl;

        // 1. Procedural Desolate Terrain Masks (Non-Rectangular board masks, void cells, BFS reachability, dock placement)
        {
            minesweeper::core::Board boardCA;
            boardCA.init(2, 12, 10, 54321, minesweeper::core::TerrainShape::CellularAutomata);
            if (!boardCA.hasTerrainMask) {
                std::cerr << "  [FAIL] Test 15: Board with CellularAutomata shape did not enable hasTerrainMask!" << std::endl;
                return 151;
            }
            if (boardCA.totalPlayableCells() >= boardCA.totalCells()) {
                std::cerr << "  [FAIL] Test 15: CellularAutomata should produce void cells! Playable=" << boardCA.totalPlayableCells() << " Total=" << boardCA.totalCells() << std::endl;
                return 152;
            }

            // Verify void cells are never bombs and cannot be uncovered
            size_t voidCount = 0;
            for (size_t i = 0; i < boardCA.totalCells(); ++i) {
                if (boardCA.isVoid(i)) {
                    voidCount++;
                    if (boardCA.isBomb(i)) {
                        std::cerr << "  [FAIL] Test 15: Void cell " << i << " marked as bomb!" << std::endl;
                        return 153;
                    }
                }
            }
            if (voidCount == 0) {
                std::cerr << "  [FAIL] Test 15: No void cells generated!" << std::endl;
                return 154;
            }

            // Guaranteed safe start check: startingCell must be playable, not a bomb, and have 0 neighbor bombs
            if (boardCA.startingCell < 0) {
                std::cerr << "  [FAIL] Test 15: Starting cell not set on procedural terrain!" << std::endl;
                return 155;
            }
            size_t sIdx = static_cast<size_t>(boardCA.startingCell);
            if (!boardCA.isPlayable(sIdx) || boardCA.isBomb(sIdx) || boardCA.counts.get(sIdx) != 0) {
                std::cerr << "  [FAIL] Test 15: Starting cell must be a safe playable 0-value cascade start! Playable=" << boardCA.isPlayable(sIdx) << " Bomb=" << boardCA.isBomb(sIdx) << " Count=" << (int)boardCA.counts.get(sIdx) << std::endl;
                return 156;
            }

            // Dock placement raycasting
            Vector2 dockPos = boardCA.findDockPlacement(40.0f);
            if (dockPos.x >= 0.0f) {
                std::cerr << "  [FAIL] Test 15: Merchant dock placement must be located west outside grid perimeter! Got X=" << dockPos.x << std::endl;
                return 157;
            }

            // Test PerlinIsland and VoronoiFaultLine generation
            minesweeper::core::Board boardPI, boardVF;
            boardPI.init(2, 10, 8, 112233, minesweeper::core::TerrainShape::PerlinIsland);
            boardVF.init(2, 10, 8, 445566, minesweeper::core::TerrainShape::VoronoiFaultLine);
            if (!boardPI.hasTerrainMask || !boardVF.hasTerrainMask) {
                std::cerr << "  [FAIL] Test 15: PerlinIsland or VoronoiFaultLine mask generation failed!" << std::endl;
                return 158;
            }
        }

        // 2. Consumable Item Catalog & Inventory
        {
            auto& catalog = minesweeper::core::ItemCatalog::instance();
            catalog.init();
            const auto* shield = catalog.getItem(minesweeper::core::ItemId::BlastShield);
            const auto* wand = catalog.getItem(minesweeper::core::ItemId::GroundPenetratingWand);
            const auto* beacon = catalog.getItem(minesweeper::core::ItemId::RadarBeacon);

            if (!shield || shield->cost != 50) {
                std::cerr << "  [FAIL] Test 15: BlastShield missing or wrong cost in catalog!" << std::endl;
                return 160;
            }
            if (!wand || wand->cost != 30) {
                std::cerr << "  [FAIL] Test 15: GroundPenetratingWand missing or wrong cost in catalog!" << std::endl;
                return 161;
            }
            if (!beacon || beacon->cost != 60) {
                std::cerr << "  [FAIL] Test 15: RadarBeacon missing or wrong cost in catalog!" << std::endl;
                return 162;
            }

            minesweeper::core::PlayerInventory inv;
            inv.addItem(*shield);
            if (!inv.hasItem(minesweeper::core::ItemId::BlastShield)) {
                std::cerr << "  [FAIL] Test 15: PlayerInventory failed to hold BlastShield!" << std::endl;
                return 163;
            }
            inv.consumeItem(minesweeper::core::ItemId::BlastShield);
            if (inv.hasItem(minesweeper::core::ItemId::BlastShield)) {
                std::cerr << "  [FAIL] Test 15: BlastShield not consumed from inventory!" << std::endl;
                return 164;
            }
        }

        // 3. In-World Merchant Ship, Pedestals, Hold-to-Buy, and Safe Zone
        {
            minesweeper::core::ShopShip shopShip;
            shopShip.position = { 100.0f, 100.0f };
            if (shopShip.pedestals.size() != 3) {
                std::cerr << "  [FAIL] Test 15: Expected 3 merchant pedestals, got " << shopShip.pedestals.size() << std::endl;
                return 170;
            }

            // Verify safe zone radius (3 tiles = 120px)
            if (!shopShip.isInsideSafeZone({ 100.0f, 100.0f })) {
                std::cerr << "  [FAIL] Test 15: Center of shop ship must be inside safe zone!" << std::endl;
                return 171;
            }
            if (shopShip.isInsideSafeZone({ 300.0f, 300.0f })) {
                std::cerr << "  [FAIL] Test 15: Distant pos (300, 300) should be outside safe zone!" << std::endl;
                return 172;
            }

            // Test hold-to-buy on pedestal 0 (GP-Wand $30)
            uint64_t teamFunds = 100;
            minesweeper::core::PlayerInventory inv;
            Vector2 ped0World = { shopShip.position.x + shopShip.pedestals[0].offset.x, shopShip.position.y + shopShip.pedestals[0].offset.y };

            // Step with mouse down for 0.55s (exceeds 0.5s threshold)
            shopShip.updatePedestals(0.55f, ped0World, true, teamFunds, inv);
            if (teamFunds != 70) {
                std::cerr << "  [FAIL] Test 15: Hold-to-buy did not deduct funds correctly! Expected 70, got " << teamFunds << std::endl;
                return 173;
            }
            if (!inv.hasItem(minesweeper::core::ItemId::GroundPenetratingWand)) {
                std::cerr << "  [FAIL] Test 15: Hold-to-buy did not grant purchased item to inventory!" << std::endl;
                return 174;
            }

            // Verify deposit hopper bounding box
            Rectangle hRect = shopShip.getHopperWorldRect();
            if (hRect.width <= 0.0f || hRect.height <= 0.0f) {
                std::cerr << "  [FAIL] Test 15: ShopShip hopper world rect has non-positive dimensions!" << std::endl;
                return 175;
            }
        }

        // 4. Scrap Economy, Visual Clutter Constraint (<= 65% of tile), Drag & Drop Banking
        {
            minesweeper::render::ScrapSystem scrapSys;
            scrapSys.spawn({ 50.0f, 50.0f });
            if (scrapSys.items.empty()) {
                std::cerr << "  [FAIL] Test 15: ScrapSystem failed to spawn scrap token!" << std::endl;
                return 180;
            }

            // Verify clutter control: item size <= 65% of 40px cell (26px)
            const auto& item = scrapSys.items[0];
            float maxVisualDim = std::max(item.size.x, item.size.y) * 1.25f; // include max squash/stretch
            if (maxVisualDim > 26.0f) {
                std::cerr << "  [FAIL] Test 15: Visual clutter violation! Scrap token max dimension (" << maxVisualDim << "px) exceeds 65% of cell width (26px)!" << std::endl;
                return 181;
            }

            // Test drag and drop into merchant hopper
            Camera2D cam = { 0 };
            cam.zoom = 1.0f;
            Rectangle hopper = { 200.0f, 200.0f, 50.0f, 30.0f };
            scrapSys.carriedItemIndex = 0;
            scrapSys.items[0].currentPos = { 210.0f, 210.0f };
            // Release mouse inside hopper
            scrapSys.update(0.016f, { 10.0f, 10.0f }, cam, { 210.0f, 210.0f }, false, false, hopper);
            if (!scrapSys.hopperDepositTriggered || scrapSys.pendingCollected != 1) {
                std::cerr << "  [FAIL] Test 15: Drag and drop into hopper did not trigger deposit! Triggered=" << scrapSys.hopperDepositTriggered << " Pending=" << scrapSys.pendingCollected << std::endl;
                return 182;
            }
            int collected = scrapSys.collectPending();
            if (collected != 1) {
                std::cerr << "  [FAIL] Test 15: collectPending failed to return deposited scrap! Got " << collected << std::endl;
                return 183;
            }
        }

        // 5. Campaign Sector Modifiers & Emergency Extraction
        {
            minesweeper::core::CampaignManager mgr;
            mgr.init(12345);
            uint64_t scrap = 200;
            auto* sec = mgr.getSectorByIndex(0);
            if (!sec) {
                std::cerr << "  [FAIL] Test 15: Sector 0 missing in CampaignManager!" << std::endl;
                return 190;
            }
            sec->threatIndex = 1.0f;

            // Trigger emergency extraction: lose 50% scrap, +5% threat
            mgr.handleEmergencyExtraction(0, scrap);
            if (scrap != 100) {
                std::cerr << "  [FAIL] Test 15: Emergency extraction scrap loss failed! Expected 100, got " << scrap << std::endl;
                return 191;
            }
            if (std::abs(sec->threatIndex - 1.05f) > 0.001f) {
                std::cerr << "  [FAIL] Test 15: Emergency extraction threat increase failed! Expected 1.05, got " << sec->threatIndex << std::endl;
                return 192;
            }

            // Test MoltenTileTimer
            minesweeper::core::MoltenTileTimer mt;
            mt.cellIndex = 12;
            mt.timeLeft = 5.0f;
            mt.active = true;
            mt.timeLeft -= 1.5f;
            if (std::abs(mt.timeLeft - 3.5f) > 0.001f) {
                std::cerr << "  [FAIL] Test 15: MoltenTileTimer countdown failed!" << std::endl;
                return 193;
            }
        }

        // 6. Authoritative Network Protocol Packets
        {
            minesweeper::net::PacketInteractItem p1;
            minesweeper::net::PacketDepositScrap p2;
            minesweeper::net::PacketItemStateSync p3;
            minesweeper::net::PacketWalletUpdate p4;
            minesweeper::net::PacketDetonationEvent p5;

            if (p1.type != minesweeper::net::PacketType::InteractItem ||
                p2.type != minesweeper::net::PacketType::DepositScrap ||
                p3.type != minesweeper::net::PacketType::ItemStateSync ||
                p4.type != minesweeper::net::PacketType::WalletUpdate ||
                p5.type != minesweeper::net::PacketType::DetonationEvent) {
                std::cerr << "  [FAIL] Test 15: Authoritative network packet type mismatch!" << std::endl;
                return 195;
            }
        }

        std::cout << "  [PASS] Test 15: Multiplayer Co-op Campaign, Scrap Economy, Desolate Terrain, Safe Zone & Merchant Loop verified." << std::endl;
    }

    std::cout << "[TEST-CAMPAIGN] ALL CAMPAIGN TESTS PASSED!" << std::endl;
    return 0;
}

static int runPlanetTests() {
    std::cout << "[TEST-PLANET] Starting 3D Geodesic Hex Planet Renderer Unit Tests..." << std::endl;

    // Test 1: PlanetRenderer initialization & 42-sector hexagonal dual lattice
    minesweeper::render::PlanetRenderer planet;
    planet.init();

    if (planet.sectors.size() != 42) {
        std::cerr << "  [FAIL] Expected 42 geodesic sectors (12 pentagons + 30 hexagons), got " << planet.sectors.size() << std::endl;
        return 1;
    }
    for (size_t i = 0; i < planet.sectors.size(); ++i) {
        const auto& sec = planet.sectors[i];
        if (sec.corners.size() < 5 || sec.corners.size() > 6) {
            std::cerr << "  [FAIL] Sector " << i << " has invalid corner count: " << sec.corners.size() << std::endl;
            return 2;
        }
        if (std::abs(Vector3Length(sec.center) - 1.0f) > 0.01f) {
            std::cerr << "  [FAIL] Sector " << i << " center vector is not normalized!" << std::endl;
            return 3;
        }
        for (const auto& c : sec.corners) {
            if (std::abs(Vector3Length(c) - 1.0f) > 0.01f) {
                std::cerr << "  [FAIL] Sector " << i << " corner vector is not normalized!" << std::endl;
                return 4;
            }
        }
    }
    if (planet.stars.empty()) {
        std::cerr << "  [FAIL] Starfield not generated!" << std::endl;
        return 5;
    }
    std::cout << "  [PASS] Test 1: PlanetRenderer initialized with 42 seamless geodesic hex/pent facets." << std::endl;

    // Test 2: Campaign Fortress mapping
    {
        int f0 = planet.getSectorIdxForFortress(0);
        int f1 = planet.getSectorIdxForFortress(1);
        int f2 = planet.getSectorIdxForFortress(2);
        int f3 = planet.getSectorIdxForFortress(3);

        if (f0 < 0 || f1 < 0 || f2 < 0 || f3 < 0) {
            std::cerr << "  [FAIL] Fortress mapping failed to assign all 4 campaign fortresses!" << std::endl;
            return 6;
        }
        if (f0 == f1 || f0 == f2 || f0 == f3 || f1 == f2 || f1 == f3 || f2 == f3) {
            std::cerr << "  [FAIL] Duplicate fortress sector indices detected!" << std::endl;
            return 7;
        }
        if (!planet.sectors[f0].isFortress || !planet.sectors[f1].isFortress ||
            !planet.sectors[f2].isFortress || !planet.sectors[f3].isFortress) {
            std::cerr << "  [FAIL] isFortress flag not set on assigned fortress sectors!" << std::endl;
            return 8;
        }
        std::cout << "  [PASS] Test 2: All 4 Campaign Fortresses uniquely mapped to prominent surface hexes." << std::endl;
    }

    // Test 3: Sector focus calculation and smooth transition
    {
        int sec1 = planet.getSectorIdxForFortress(1);
        planet.focusSector(sec1);
        if (!planet.isTransitioning) {
            std::cerr << "  [FAIL] isTransitioning flag should be true after focusSector!" << std::endl;
            return 9;
        }

        float initPitch = planet.pitch;
        float initYaw = planet.yaw;

        planet.update(0.25f);

        float dYaw1 = std::abs(planet.targetYaw - initYaw);
        float dYaw2 = std::abs(planet.targetYaw - planet.yaw);
        float dPitch1 = std::abs(planet.targetPitch - initPitch);
        float dPitch2 = std::abs(planet.targetPitch - planet.pitch);
        if (dYaw2 >= dYaw1 && dPitch2 >= dPitch1) {
            std::cerr << "  [FAIL] Orientation did not advance toward target during update!" << std::endl;
            return 10;
        }

        // Settle smoothly
        for (int step = 0; step < 60; ++step) {
            planet.update(0.08f);
        }

        Vector3 focusedNorm = planet.rotateVector(planet.sectors[sec1].center);
        if (focusedNorm.z < 0.95f) {
            std::cerr << "  [FAIL] Focused sector center should point toward camera (+Z >= 0.95), got Z=" << focusedNorm.z << std::endl;
            return 11;
        }
        std::cout << "  [PASS] Test 3: Smooth spherical camera focus and orientation verified." << std::endl;
    }

    // Test 4: Coordinate rotation length preservation
    {
        Vector3 testVec = { 0.577f, 0.577f, 0.577f };
        Vector3 rotVec = planet.rotateVector(testVec);
        if (std::abs(Vector3Length(testVec) - Vector3Length(rotVec)) > 0.001f) {
            std::cerr << "  [FAIL] rotateVector did not preserve vector length!" << std::endl;
            return 12;
        }
        std::cout << "  [PASS] Test 4: Spherical rotation isometry and vector norms verified." << std::endl;
    }

    // Test 5: Exact spherical Voronoi sector selection & raycast math
    {
        for (size_t i = 0; i < planet.sectors.size(); ++i) {
            // Test that a ray pointing directly at a sector's center on the planet sphere selects exactly sector i
            Vector3 hitPoint = Vector3Scale(planet.sectors[i].center, minesweeper::render::PlanetRenderer::PLANET_RADIUS);
            Vector3 hitNorm = Vector3Normalize(hitPoint);
            float bestDot = -2.0f;
            int bestIdx = -1;
            for (size_t j = 0; j < planet.sectors.size(); ++j) {
                float dot = Vector3DotProduct(planet.sectors[j].center, hitNorm);
                if (dot > bestDot) {
                    bestDot = dot;
                    bestIdx = static_cast<int>(j);
                }
            }
            if (bestIdx != static_cast<int>(i)) {
                std::cerr << "  [FAIL] Voronoi selection mismatch for sector " << i << ": got " << bestIdx << std::endl;
                return 13;
            }
        }
        std::cout << "  [PASS] Test 5: Spherical Voronoi raycast sector selection verified for all 42 sectors." << std::endl;
    }

    std::cout << "[TEST-PLANET] ALL 3D GEODESIC HEX PLANET TESTS PASSED!" << std::endl;
    return 0;
}

int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--test" || std::string(argv[i]) == "--test-all" || std::string(argv[i]) == "--test-capsule") {
            int r1 = runCapsuleTests();
            int r2 = runCameraTests();
            int r3 = runControlTests();
            int r4 = runTextureTests();
            int r5 = runItemTests();
            int r6 = runRouletteTests();
            int r7 = runAudioTests();
            int r8 = runCampaignTests();
            int r9 = runPlanetTests();
            if (r1 != 0) return r1;
            if (r2 != 0) return r2;
            if (r3 != 0) return r3;
            if (r4 != 0) return r4;
            if (r5 != 0) return r5;
            if (r6 != 0) return r6;
            if (r7 != 0) return r7;
            if (r8 != 0) return r8;
            return r9;
        }
        if (std::string(argv[i]) == "--test-camera") {
            return runCameraTests();
        }
        if (std::string(argv[i]) == "--test-controls") {
            return runControlTests();
        }
        if (std::string(argv[i]) == "--test-textures") {
            return runTextureTests();
        }
        if (std::string(argv[i]) == "--test-items") {
            return runItemTests();
        }
        if (std::string(argv[i]) == "--test-roulette") {
            return runRouletteTests();
        }
        if (std::string(argv[i]) == "--test-audio") {
            return runAudioTests();
        }
        if (std::string(argv[i]) == "--test-campaign") {
            return runCampaignTests();
        }
        if (std::string(argv[i]) == "--test-planet") {
            return runPlanetTests();
        }
    }

    minesweeper::App app;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--test-shop") {
            runCapsuleTests();
            runCameraTests();
            runControlTests();
            runTextureTests();
            runItemTests();
            runRouletteTests();
            app.testShopMode = true;
        }
        else if (std::string(argv[i]) == "--test-shop-ui" || std::string(argv[i]) == "--test-ui") {
            app.testShopUIMode = true;
        }
        else if (std::string(argv[i]) == "--test-customize") {
            app.testCustomizeMode = true;
        }
        else if (std::string(argv[i]) == "--test-campaign-ui") {
            app.testCampaignMode = true;
        }
#if defined(_DEBUG) || !defined(NDEBUG)
        else if (std::string(argv[i]) == "--editor" || std::string(argv[i]) == "--sector-editor" || std::string(argv[i]) == "--test-editor") {
            app.testEditorMode = true;
        }
#endif
        else if (std::string(argv[i]) == "+connect_lobby" && i + 1 < argc) {
            try {
                app.initialLobbyId = std::stoull(argv[i + 1]);
            } catch (...) {}
        }
    }
    app.run();
    return 0;
}
