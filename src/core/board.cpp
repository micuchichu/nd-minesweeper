#include "board.hpp"
#include "rng.hpp"
#include <vector>
#include <algorithm>

namespace minesweeper::core {

void Board::init(int dim, int size, int bombsCount, uint64_t seed) {
    config.dim = dim;
    config.size = size;
    coord.init(dim, static_cast<size_t>(size));

    size_t total = coord.totalCells;
    if (bombsCount >= static_cast<int>(total)) {
        bombsCount = static_cast<int>(total) - 1;
    }
    if (bombsCount < 1) {
        bombsCount = 1;
    }
    config.bombs = bombsCount;
    config.seed = seed;

    bombs.init(total);
    state.init(total);
    counts.init(total);

    reset();
    generateBombs(seed);
    buildCache();
}

void Board::reset() {
    bombs.clear();
    state.clear();
    counts.clear();
    flagOwners.clear();
    revealedCount = 0;
    flaggedCount = 0;
    isGameOver = false;
    isVictory = false;
    startingCell = -1;
}

void Board::generateBombs(uint64_t seed) {
    bombs.clear();
    size_t total = coord.totalCells;
    size_t numBombs = static_cast<size_t>(config.bombs);
    if (total == 0 || numBombs == 0) return;

    Rng rng(seed);

    size_t placed = 0;
    while (placed < numBombs) {
        size_t index = static_cast<size_t>(rng.nextBounded(total));
        if (!bombs.getBomb(index)) {
            bombs.setBomb(index);
            ++placed;
        }
    }
}

int64_t Board::findStartingCell() const {
    if (coord.totalCells == 0 || config.bombs >= static_cast<int>(coord.totalCells)) {
        return -1;
    }

    size_t dim = static_cast<size_t>(config.dim);
    float centerCoord = (static_cast<float>(config.size) - 1.0f) * 0.5f;

    auto calcDistSq = [&](size_t idx) -> float {
        float d2 = 0.0f;
        if (dim == 2) {
            size_t x = 0, y = 0;
            coord.toCoord2D(idx, x, y);
            float dx = static_cast<float>(x) - centerCoord;
            float dy = static_cast<float>(y) - centerCoord;
            d2 = dx * dx + dy * dy;
        } else if (dim == 3) {
            size_t x = 0, y = 0, z = 0;
            coord.toCoord3D(idx, x, y, z);
            float dx = static_cast<float>(x) - centerCoord;
            float dy = static_cast<float>(y) - centerCoord;
            float dz = static_cast<float>(z) - centerCoord;
            d2 = dx * dx + dy * dy + dz * dz;
        } else {
            size_t x = 0, y = 0, z = 0, w = 0;
            coord.toCoord4D(idx, x, y, z, w);
            float dx = static_cast<float>(x) - centerCoord;
            float dy = static_cast<float>(y) - centerCoord;
            float dz = static_cast<float>(z) - centerCoord;
            float dw = static_cast<float>(w) - centerCoord;
            d2 = dx * dx + dy * dy + dz * dz + dw * dw;
        }
        return d2;
    };

    int64_t bestZeroIdx = -1;
    float bestZeroDist = 1e18f;

    int64_t bestFallbackIdx = -1;
    uint8_t minFallbackCount = 255;
    float bestFallbackDist = 1e18f;

    for (size_t i = 0; i < coord.totalCells; ++i) {
        if (bombs.getBomb(i)) continue;

        uint8_t c = counts.get(i);
        float d = calcDistSq(i);

        if (c == 0) {
            if (d < bestZeroDist) {
                bestZeroDist = d;
                bestZeroIdx = static_cast<int64_t>(i);
            }
        } else {
            if (c < minFallbackCount || (c == minFallbackCount && d < bestFallbackDist)) {
                minFallbackCount = c;
                bestFallbackDist = d;
                bestFallbackIdx = static_cast<int64_t>(i);
            }
        }
    }

    if (bestZeroIdx >= 0) return bestZeroIdx;
    return bestFallbackIdx;
}

void Board::buildCache() {
    counts.clear();

    bombs.forEachBomb([this](size_t bombIndex) {
        coord.forEachNeighbor(bombIndex, [this](size_t neighborIndex) {
            counts.increment(neighborIndex);
        });
    });

    startingCell = findStartingCell();
}

RevealResult Board::reveal(size_t startIndex, std::vector<size_t>* outRevealedCells) {
    if (startIndex >= coord.totalCells) {
        return RevealResult::AlreadyRevealed;
    }

    CellState current = state.get(startIndex);
    if (current != CellState::Hidden) {
        return RevealResult::AlreadyRevealed;
    }

    if (bombs.getBomb(startIndex)) {
        state.set(startIndex, CellState::Revealed);
        isGameOver = true;
        if (outRevealedCells) {
            outRevealedCells->push_back(startIndex);
        }
        return RevealResult::HitBomb;
    }

    std::vector<size_t> stack;
    stack.reserve(256);
    stack.push_back(startIndex);
    state.set(startIndex, CellState::Revealed);
    if (outRevealedCells) {
        outRevealedCells->push_back(startIndex);
    }

    while (!stack.empty()) {
        size_t idx = stack.back();
        stack.pop_back();

        if (counts.get(idx) > 0 || bombs.getBomb(idx)) {
            continue;
        }

        coord.forEachNeighbor(idx, [&](size_t neighborIdx) {
            if (state.get(neighborIdx) == CellState::Hidden) {
                state.set(neighborIdx, CellState::Revealed);
                if (outRevealedCells) {
                    outRevealedCells->push_back(neighborIdx);
                }
                if (counts.get(neighborIdx) == 0 && !bombs.getBomb(neighborIdx)) {
                    stack.push_back(neighborIdx);
                }
            }
        });
    }

    revealedCount = state.countRevealed();
    if (revealedCount == coord.totalCells - static_cast<size_t>(config.bombs)) {
        isVictory = true;
        return RevealResult::Won;
    }

    return RevealResult::RevealedSafe;
}

void Board::setFlag(size_t index, uint32_t placerId, uint8_t skinId) {
    if (index >= coord.totalCells) return;
    CellState s = state.get(index);
    if (s != CellState::Flagged) {
        state.set(index, CellState::Flagged);
        ++flaggedCount;
    }
    flagOwners[index] = { placerId, skinId };
}

void Board::unflag(size_t index) {
    if (index >= coord.totalCells) return;
    CellState s = state.get(index);
    if (s == CellState::Flagged) {
        state.set(index, CellState::Hidden);
        if (flaggedCount > 0) --flaggedCount;
    }
    flagOwners.erase(index);
}

void Board::toggleFlag(size_t index, uint32_t placerId, uint8_t skinId) {
    if (index >= coord.totalCells) return;
    CellState s = state.get(index);
    if (s == CellState::Hidden) {
        setFlag(index, placerId, skinId);
    } else if (s == CellState::Flagged) {
        unflag(index);
    }
}

std::vector<size_t> Board::removeFlagsByPlacer(uint32_t placerId) {
    std::vector<size_t> removed;
    for (auto it = flagOwners.begin(); it != flagOwners.end(); ) {
        if (it->second.placerId == placerId) {
            size_t idx = it->first;
            if (state.get(idx) == CellState::Flagged) {
                state.set(idx, CellState::Hidden);
                if (flaggedCount > 0) --flaggedCount;
                removed.push_back(idx);
            }
            it = flagOwners.erase(it);
        } else {
            ++it;
        }
    }
    return removed;
}

uint8_t Board::getFlagSkin(size_t index, uint8_t fallbackSkin) const {
    auto it = flagOwners.find(index);
    if (it != flagOwners.end()) {
        return it->second.skinId;
    }
    return fallbackSkin;
}

bool Board::chord(size_t index, std::vector<size_t>& outNewlyRevealed, bool& hitBomb) {
    hitBomb = false;
    if (index >= coord.totalCells) return false;
    if (state.get(index) != CellState::Revealed) return false;

    uint8_t count = counts.get(index);
    if (count == 0) return false;

    int adjacentFlags = 0;
    coord.forEachNeighbor(index, [&](size_t nIdx) {
        if (state.get(nIdx) == CellState::Flagged) {
            ++adjacentFlags;
        }
    });

    if (adjacentFlags != static_cast<int>(count)) {
        return false;
    }

    bool didRevealAny = false;
    coord.forEachNeighbor(index, [&](size_t nIdx) {
        if (state.get(nIdx) == CellState::Hidden) {
            RevealResult res = reveal(nIdx, &outNewlyRevealed);
            didRevealAny = true;
            if (res == RevealResult::HitBomb) {
                hitBomb = true;
            }
        }
    });

    return didRevealAny;
}

} // namespace minesweeper::core
