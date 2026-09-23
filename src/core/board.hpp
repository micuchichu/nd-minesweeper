#pragma once

#include "types.hpp"
#include "coord.hpp"
#include "bitboard.hpp"
#include "raylib.h"
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

    // Desolate Terrain Mask (Non-Rectangular Board Generation)
    std::vector<bool> playableMask;
    bool hasTerrainMask = false;
    size_t playableCellCount = 0;
    TerrainShape terrainShape = TerrainShape::Rectangle;

    size_t revealedCount = 0;
    size_t flaggedCount = 0;
    bool isGameOver = false;
    bool isVictory = false;
    int64_t startingCell = -1;

    Board() = default;

    void init(int dim, int size, int bombs, uint64_t seed, TerrainShape shape = TerrainShape::Rectangle);
    void init(const BoardConfig& cfg) { init(cfg.dim, cfg.size, cfg.bombs, cfg.seed, cfg.shape); }
    void reset();
    void generateTerrainMask(TerrainShape shape, uint64_t seed);
    void generatePerlinIsland(uint64_t seed);
    void generateCellularAutomata(uint64_t seed);
    void generateVoronoiFaultLine(uint64_t seed);
    void validateBFSConnectivity();
    Vector2 findDockPlacement(float cellSize = 40.0f) const;

    void generateBombs(uint64_t seed);
    void buildCache();
    int64_t findStartingCell() const;

    RevealResult reveal(size_t index, std::vector<size_t>* outRevealedCells = nullptr);
    void toggleFlag(size_t index, uint32_t placerId = 0, uint8_t skinId = 0);
    void setFlag(size_t index, uint32_t placerId, uint8_t skinId);
    void unflag(size_t index);
    std::vector<size_t> removeFlagsByPlacer(uint32_t placerId);
    uint8_t getFlagSkin(size_t index, uint8_t fallbackSkin = 0) const;
    bool chord(size_t index, std::vector<size_t>& outNewlyRevealed, bool& hitBomb);

    inline bool isPlayable(size_t index) const {
        return !hasTerrainMask || (index < playableMask.size() && playableMask[index]);
    }
    inline bool isVoid(size_t index) const {
        return hasTerrainMask && (index >= playableMask.size() || !playableMask[index]);
    }
    bool isEdge(size_t index) const;

    inline bool isBomb(size_t index) const { return isPlayable(index) && bombs.getBomb(index); }
    inline CellState getState(size_t index) const { return state.get(index); }
    inline uint8_t getCount(size_t index) const { return counts.get(index); }
    inline size_t totalCells() const { return coord.totalCells; }
    inline size_t totalPlayableCells() const { return hasTerrainMask ? playableCellCount : coord.totalCells; }
    inline size_t safeCells() const { return totalPlayableCells() - config.bombs; }
};

} // namespace minesweeper::core
