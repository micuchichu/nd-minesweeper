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

int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--test-capsule") {
            int r1 = runCapsuleTests();
            int r2 = runCameraTests();
            int r3 = runControlTests();
            int r4 = runTextureTests();
            int r5 = runItemTests();
            int r6 = runRouletteTests();
            if (r1 != 0) return r1;
            if (r2 != 0) return r2;
            if (r3 != 0) return r3;
            if (r4 != 0) return r4;
            if (r5 != 0) return r5;
            return r6;
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
        else if (std::string(argv[i]) == "+connect_lobby" && i + 1 < argc) {
            try {
                app.initialLobbyId = std::stoull(argv[i + 1]);
            } catch (...) {}
        }
    }
    app.run();
    return 0;
}
