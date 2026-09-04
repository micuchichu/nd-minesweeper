#pragma once

#include "types.hpp"
#include "coord.hpp"
#include "bitboard.hpp"
#include <vector>
#include <cstdint>
#include <unordered_map>

namespace minesweeper::core {

struct CellFlagInfo {
    uint32_t placerId = 0;
    uint8_t skinId = 0;
};

class Board {
public:
    BoardConfig config;
    CoordND coord;
    BombBoard bombs;
    StateBoard state;
    CountBoard counts;
    std::unordered_map<size_t, CellFlagInfo> flagOwners;

    size_t revealedCount = 0;
    size_t flaggedCount = 0;
    bool isGameOver = false;
    bool isVictory = false;

    Board() = default;

    void init(int dim, int size, int bombs, uint64_t seed);
    void reset();
    void generateBombs(uint64_t seed);
    void buildCache();

    RevealResult reveal(size_t index, std::vector<size_t>* outRevealedCells = nullptr);
    void toggleFlag(size_t index, uint32_t placerId = 0, uint8_t skinId = 0);
    void setFlag(size_t index, uint32_t placerId, uint8_t skinId);
    void unflag(size_t index);
    std::vector<size_t> removeFlagsByPlacer(uint32_t placerId);
    uint8_t getFlagSkin(size_t index, uint8_t fallbackSkin = 0) const;
    bool chord(size_t index, std::vector<size_t>& outNewlyRevealed, bool& hitBomb);

    inline bool isBomb(size_t index) const { return bombs.getBomb(index); }
    inline CellState getState(size_t index) const { return state.get(index); }
    inline uint8_t getCount(size_t index) const { return counts.get(index); }
    inline size_t totalCells() const { return coord.totalCells; }
    inline size_t safeCells() const { return coord.totalCells - config.bombs; }
};

} // namespace minesweeper::core
