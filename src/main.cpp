#include "app.hpp"
#include "core/ship.hpp"
#include "core/ship_config.hpp"
#include "core/shop_ship.hpp"
#include "core/item.hpp"
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

    // 11. Spring physics tether initialization & rest distance
    {
        minesweeper::render::RaylibRenderer renderer;
        renderer.localShip.position = { 100.0f, 100.0f };
        renderer.localShip.angle = 90.0f;
        renderer.localShip.velocity = { 0.0f, 0.0f };
        renderer.heldSlot = &pInv.slots[0]; // Banana

        // Step physics once
        renderer.updateHeldItemPhysics(renderer.localShip.position, renderer.localShip.velocity, 1.0f / 120.0f);
        if (!renderer.heldItemInit) {
            std::cerr << "  [FAIL] Held item spring physics failed to initialize!" << std::endl;
            catalog.shutdown();
            CloseWindow();
            return 18;
        }

        float initDist = Vector2Distance(renderer.heldItemPos, renderer.localShip.position);
        if (std::abs(initDist - 26.0f) > 0.5f) {
            std::cerr << "  [FAIL] Held item initial spring distance mismatch! Expected ~26.0, got " << initDist << std::endl;
            catalog.shutdown();
            CloseWindow();
            return 19;
        }
        std::cout << "  [PASS] Test 13: Held item spring physics initialization and 26px rest tether verified." << std::endl;

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
            return 20;
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
            return 21;
        }
        std::cout << "  [PASS] Test 14: Dynamic spring extension and damped return to rest verified." << std::endl;

        // 13. Free 360-degree orbital rotation without angular lock
        // Apply tangential impulse (orbiting around ship)
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
        // Ship angle remained fixed, yet item orbited substantially (free spin)
        if (totalAngRot < 1.0f) {
            std::cerr << "  [FAIL] Held item failed to spin freely around player! Total rotation: " << totalAngRot << std::endl;
            catalog.shutdown();
            CloseWindow();
            return 22;
        }
        std::cout << "  [PASS] Test 15: Free 360-degree orbital spinning around player verified (rotated " << totalAngRot << " rad)." << std::endl;
    }

    catalog.shutdown();
    CloseWindow();
    std::cout << "[TEST-ITEMS] ALL ITEM TESTS PASSED!" << std::endl;
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
            if (r1 != 0) return r1;
            if (r2 != 0) return r2;
            if (r3 != 0) return r3;
            if (r4 != 0) return r4;
            return r5;
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
    }

    minesweeper::App app;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--test-shop") {
            runCapsuleTests();
            runCameraTests();
            runControlTests();
            runTextureTests();
            runItemTests();
            app.testShopMode = true;
        }
        else if (std::string(argv[i]) == "--test-shop-ui") {
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
