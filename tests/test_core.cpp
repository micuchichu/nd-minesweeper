#include "../src/core/board.hpp"
#include "../src/core/rng.hpp"
#include <iostream>
#include <cassert>
#include <chrono>

using namespace minesweeper::core;

void test2D() {
    std::cout << "[TEST] Running 2D logic test...\n";
    Board board;
    board.init(2, 10, 10, 42);

    assert(board.coord.totalCells == 100);
    assert(board.bombs.countBombs() == 10);

    // Verify neighbor counts around every cell
    for (size_t i = 0; i < 100; ++i) {
        int expected = 0;
        board.coord.forEachNeighbor(i, [&](size_t nIdx) {
            if (board.bombs.getBomb(nIdx)) ++expected;
        });
        assert(board.counts.get(i) == expected);
    }

    // Test reveal on a safe cell
    size_t safeIdx = 0;
    while (board.bombs.getBomb(safeIdx)) ++safeIdx;

    RevealResult res = board.reveal(safeIdx);
    assert(res == RevealResult::RevealedSafe || res == RevealResult::Won);
    assert(board.state.get(safeIdx) == CellState::Revealed);
    assert(board.revealedCount > 0);

    // Test flag
    size_t flagIdx = (safeIdx + 1) % 100;
    if (board.state.get(flagIdx) == CellState::Hidden) {
        board.toggleFlag(flagIdx);
        assert(board.state.get(flagIdx) == CellState::Flagged);
        assert(board.flaggedCount == 1);
        board.toggleFlag(flagIdx);
        assert(board.state.get(flagIdx) == CellState::Hidden);
        assert(board.flaggedCount == 0);
    }

    std::cout << "[TEST] 2D logic test PASSED!\n";
}

void test3D() {
    std::cout << "[TEST] Running 3D logic test (size 10, 1000 cells)...\n";
    Board board;
    board.init(3, 10, 50, 12345);

    assert(board.coord.totalCells == 1000);
    assert(board.bombs.countBombs() == 50);

    for (size_t i = 0; i < 1000; ++i) {
        int expected = 0;
        board.coord.forEachNeighbor(i, [&](size_t nIdx) {
            if (board.bombs.getBomb(nIdx)) ++expected;
        });
        assert(board.counts.get(i) == expected);
    }
    std::cout << "[TEST] 3D logic test PASSED!\n";
}

void test4D() {
    std::cout << "[TEST] Running 4D logic test (size 5, 625 cells)...\n";
    Board board;
    board.init(4, 5, 40, 9999);

    assert(board.coord.totalCells == 625);
    assert(board.bombs.countBombs() == 40);

    for (size_t i = 0; i < 625; ++i) {
        int expected = 0;
        board.coord.forEachNeighbor(i, [&](size_t nIdx) {
            if (board.bombs.getBomb(nIdx)) ++expected;
        });
        assert(board.counts.get(i) == expected);
    }
    std::cout << "[TEST] 4D logic test PASSED!\n";
}

void testStress1MillionCells() {
    std::cout << "[TEST] Running STRESS TEST: 1,000,000 cells (1000x1000) with 150,000 mines...\n";
    auto t0 = std::chrono::high_resolution_clock::now();

    Board board;
    board.init(2, 1000, 150000, 777);

    auto t1 = std::chrono::high_resolution_clock::now();
    double initMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

    assert(board.coord.totalCells == 1000000);
    assert(board.bombs.countBombs() == 150000);

    std::cout << "  -> Initialized 1M cells & generated 150k mines & built cache in: " << initMs << " ms\n";

    // Perform flood-fill reveal
    auto t2 = std::chrono::high_resolution_clock::now();
    // Find a zero cell
    size_t zeroIdx = 0;
    for (size_t i = 0; i < 1000000; ++i) {
        if (!board.bombs.getBomb(i) && board.counts.get(i) == 0) {
            zeroIdx = i;
            break;
        }
    }
    board.reveal(zeroIdx);
    auto t3 = std::chrono::high_resolution_clock::now();
    double revealMs = std::chrono::duration<double, std::milli>(t3 - t2).count();

    std::cout << "  -> Flood-fill revealed " << board.revealedCount << " cells in: " << revealMs << " ms\n";
    std::cout << "[TEST] 1 Million Cells Stress Test PASSED!\n";
}

void testFlagAttributionAndDisconnect() {
    std::cout << "[TEST] Running multiplayer flag attribution & disconnect test...\n";
    Board board;
    board.init(2, 10, 10, 1234);

    uint32_t hostId = 0xFFFFFFFF;
    uint32_t player1 = 1;
    uint32_t player2 = 2;

    // Player 1 places skin 2 on cell 10
    board.setFlag(10, player1, 2);
    // Player 2 places skin 1 on cell 20
    board.setFlag(20, player2, 1);
    // Host places skin 0 on cell 30
    board.setFlag(30, hostId, 0);

    assert(board.flaggedCount == 3);
    assert(board.state.get(10) == CellState::Flagged);
    assert(board.state.get(20) == CellState::Flagged);
    assert(board.state.get(30) == CellState::Flagged);
    assert(board.getFlagSkin(10) == 2);
    assert(board.getFlagSkin(20) == 1);
    assert(board.getFlagSkin(30) == 0);

    // Player 1 disconnects -> their flags are removed
    auto removedP1 = board.removeFlagsByPlacer(player1);
    assert(removedP1.size() == 1);
    assert(removedP1[0] == 10);
    assert(board.state.get(10) == CellState::Hidden);
    assert(board.flaggedCount == 2);
    assert(board.state.get(20) == CellState::Flagged);
    assert(board.state.get(30) == CellState::Flagged);

    // Player 2 disconnects -> their flags are removed
    auto removedP2 = board.removeFlagsByPlacer(player2);
    assert(removedP2.size() == 1);
    assert(removedP2[0] == 20);
    assert(board.state.get(20) == CellState::Hidden);
    assert(board.flaggedCount == 1);
    assert(board.state.get(30) == CellState::Flagged);

    // Host unflags cell 30
    board.unflag(30);
    assert(board.state.get(30) == CellState::Hidden);
    assert(board.flaggedCount == 0);
    assert(board.flagOwners.empty());

    std::cout << "[TEST] Multiplayer flag attribution & disconnect test PASSED!\n";
}

void testDeterministicRng() {
    std::cout << "[TEST] Running cross-platform deterministic RNG test...\n";
    Rng rng1(12345);
    uint64_t v1 = rng1.nextU64();
    uint64_t v2 = rng1.nextU64();
    uint64_t b1 = rng1.nextBounded(100);
    uint64_t b2 = rng1.nextBounded(100);

    // Verify independent instances with the same seed produce the exact same sequence
    Rng rng2(12345);
    assert(rng2.nextU64() == v1);
    assert(rng2.nextU64() == v2);
    assert(rng2.nextBounded(100) == b1);
    assert(rng2.nextBounded(100) == b2);

    // Verify board generation with seed produces exact same bomb layout every time
    Board bA, bB;
    bA.init(2, 20, 50, 99999);
    bB.init(2, 20, 50, 99999);
    assert(bA.bombs.countBombs() == 50);
    assert(bB.bombs.countBombs() == 50);
    for (size_t i = 0; i < 400; ++i) {
        assert(bA.bombs.getBomb(i) == bB.bombs.getBomb(i));
        assert(bA.counts.get(i) == bB.counts.get(i));
    }

    std::cout << "[TEST] Deterministic RNG test PASSED!\n";
}

int main() {
    testDeterministicRng();
    test2D();
    test3D();
    test4D();
    testStress1MillionCells();
    testFlagAttributionAndDisconnect();
    std::cout << "\nALL CORE ENGINE TESTS PASSED SUCCESSFULLY!\n";
    return 0;
}
