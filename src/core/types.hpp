#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

namespace minesweeper::core {

enum class CellState : uint8_t {
    Hidden = 0,
    Revealed = 1,
    Flagged = 2
};

enum class RevealResult : uint8_t {
    AlreadyRevealed = 0,
    RevealedSafe = 1,
    HitBomb = 2,
    Won = 3
};

struct BoardConfig {
    int dim = 2;
    int size = 10;
    int bombs = 10;
    uint64_t seed = 12345;

    size_t totalCells() const {
        size_t total = 1;
        for (int i = 0; i < dim; ++i) {
            total *= static_cast<size_t>(size);
        }
        return total;
    }
};

} // namespace minesweeper::core
