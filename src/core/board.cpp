#include "board.hpp"
#include "rng.hpp"
#include <vector>
#include <algorithm>
#include <queue>
#include <cmath>

namespace minesweeper::core {

static float pseudoNoise(float x, float y, uint64_t seed) {
    int ix = static_cast<int>(std::floor(x));
    int iy = static_cast<int>(std::floor(y));
    float fx = x - static_cast<float>(ix);
    float fy = y - static_cast<float>(iy);
    float u = fx * fx * (3.0f - 2.0f * fx);
    float v = fy * fy * (3.0f - 2.0f * fy);

    auto hash = [&](int px, int py) -> float {
        uint64_t h = seed ^ (static_cast<uint64_t>(px * 374761393) + static_cast<uint64_t>(py * 668265263));
        h = (h ^ (h >> 13)) * 1274126177ULL;
        return static_cast<float>(h & 0xFFFF) / 65535.0f;
    };

    float a = hash(ix, iy);
    float b = hash(ix + 1, iy);
    float c = hash(ix, iy + 1);
    float d = hash(ix + 1, iy + 1);

    return (a * (1.0f - u) + b * u) * (1.0f - v) + (c * (1.0f - u) + d * u) * v;
}

void Board::init(int dim, int size, int bombsCount, uint64_t seed, TerrainShape shape) {
    config.dim = dim;
    config.size = size;
    config.shape = shape;
    config.seed = seed;
    terrainShape = shape;
    coord.init(dim, static_cast<size_t>(size));

    size_t total = coord.totalCells;

    // Initialize terrain mask for 2D boards when non-rectangular shape is requested
    if (dim == 2 && shape != TerrainShape::Rectangle) {
        generateTerrainMask(shape, seed);
    } else {
        hasTerrainMask = false;
        playableMask.assign(total, true);
        playableCellCount = total;
    }

    size_t playables = totalPlayableCells();
    if (bombsCount >= static_cast<int>(playables)) {
        bombsCount = static_cast<int>(playables) - 1;
    }
    if (bombsCount < 1) {
        bombsCount = 1;
    }
    config.bombs = bombsCount;

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

void Board::generateTerrainMask(TerrainShape shape, uint64_t seed) {
    hasTerrainMask = true;
    terrainShape = shape;
    playableMask.assign(coord.totalCells, false);

    switch (shape) {
        case TerrainShape::PerlinIsland:
            generatePerlinIsland(seed);
            break;
        case TerrainShape::CellularAutomata:
            generateCellularAutomata(seed);
            break;
        case TerrainShape::VoronoiFaultLine:
            generateVoronoiFaultLine(seed);
            break;
        case TerrainShape::Rectangle:
        default:
            hasTerrainMask = false;
            playableMask.assign(coord.totalCells, true);
            break;
    }

    validateBFSConnectivity();
}

void Board::generatePerlinIsland(uint64_t seed) {
    int s = static_cast<int>(coord.size);
    float center = (static_cast<float>(s) - 1.0f) * 0.5f;
    float maxR = center * 1.05f;

    for (int y = 0; y < s; ++y) {
        for (int x = 0; x < s; ++x) {
            float dx = (static_cast<float>(x) - center);
            float dy = (static_cast<float>(y) - center);
            float dist = std::sqrt(dx * dx + dy * dy);
            float normDist = dist / (maxR > 0.0f ? maxR : 1.0f);

            // Multi-octave fractional Brownian motion noise
            float n1 = pseudoNoise(static_cast<float>(x) * 0.22f, static_cast<float>(y) * 0.22f, seed);
            float n2 = pseudoNoise(static_cast<float>(x) * 0.44f, static_cast<float>(y) * 0.44f, seed + 101);
            float n3 = pseudoNoise(static_cast<float>(x) * 0.88f, static_cast<float>(y) * 0.88f, seed + 202);
            float n = n1 * 0.55f + n2 * 0.30f + n3 * 0.15f;

            // Radial falloff: center is guaranteed solid, edges drop off to void
            float val = n - (normDist * 0.75f);
            size_t idx = coord.toIndex2D(static_cast<size_t>(x), static_cast<size_t>(y));
            playableMask[idx] = (val > -0.12f);
        }
    }
}

void Board::generateCellularAutomata(uint64_t seed) {
    int s = static_cast<int>(coord.size);
    Rng rng(seed);

    // Initial 55% alive fill
    std::vector<bool> current(coord.totalCells, false);
    for (size_t i = 0; i < coord.totalCells; ++i) {
        current[i] = (rng.nextFloat01() < 0.55f);
    }

    // Always keep 3x3 center core alive
    int cx = s / 2;
    int cy = s / 2;
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            int nx = cx + dx;
            int ny = cy + dy;
            if (nx >= 0 && nx < s && ny >= 0 && ny < s) {
                current[coord.toIndex2D(static_cast<size_t>(nx), static_cast<size_t>(ny))] = true;
            }
        }
    }

    // 4 iterations of 4-5 cave generation rules
    std::vector<bool> next = current;
    for (int iter = 0; iter < 4; ++iter) {
        for (int y = 0; y < s; ++y) {
            for (int x = 0; x < s; ++x) {
                int count = 0;
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        int nx = x + dx;
                        int ny = y + dy;
                        if (nx < 0 || nx >= s || ny < 0 || ny >= s) {
                            // Out of bounds acts as wall
                            ++count;
                        } else {
                            if (current[coord.toIndex2D(static_cast<size_t>(nx), static_cast<size_t>(ny))]) {
                                ++count;
                            }
                        }
                    }
                }
                size_t idx = coord.toIndex2D(static_cast<size_t>(x), static_cast<size_t>(y));
                next[idx] = (count >= 5);
            }
        }
        current = next;
    }

    playableMask = current;
}

void Board::generateVoronoiFaultLine(uint64_t seed) {
    int s = static_cast<int>(coord.size);
    playableMask.assign(coord.totalCells, true);
    Rng rng(seed);

    // Generate 2 tectonic chasms (void ribbons 2 cells wide)
    int numLines = 2;
    for (int l = 0; l < numLines; ++l) {
        float x1 = rng.nextFloat01() * static_cast<float>(s);
        float y1 = (l == 0) ? 0.0f : rng.nextFloat01() * static_cast<float>(s);
        float x2 = rng.nextFloat01() * static_cast<float>(s);
        float y2 = (l == 0) ? static_cast<float>(s) : (s - y1);

        float bridgePos = 0.3f + rng.nextFloat01() * 0.4f;

        for (int y = 0; y < s; ++y) {
            for (int x = 0; x < s; ++x) {
                // Distance from point (x, y) to line segment (x1,y1)-(x2,y2)
                float px = static_cast<float>(x);
                float py = static_cast<float>(y);
                float ldx = x2 - x1;
                float ldy = y2 - y1;
                float lenSq = ldx * ldx + ldy * ldy;
                float t = (lenSq > 0.0f) ? std::clamp(((px - x1) * ldx + (py - y1) * ldy) / lenSq, 0.0f, 1.0f) : 0.0f;
                float nearX = x1 + t * ldx;
                float nearY = y1 + t * ldy;
                float dist = std::sqrt((px - nearX) * (px - nearX) + (py - nearY) * (py - nearY));

                // Carve 2-tile void ribbon except near bridge
                if (dist < 1.25f) {
                    bool isBridge = (std::abs(t - bridgePos) < 0.12f);
                    if (!isBridge) {
                        playableMask[coord.toIndex2D(static_cast<size_t>(x), static_cast<size_t>(y))] = false;
                    }
                }
            }
        }
    }
}

void Board::validateBFSConnectivity() {
    int s = static_cast<int>(coord.size);
    size_t total = coord.totalCells;
    std::vector<int> component(total, -1);
    std::vector<size_t> componentSizes;

    int currentComp = 0;
    for (size_t i = 0; i < total; ++i) {
        if (!playableMask[i] || component[i] != -1) continue;

        size_t cSize = 0;
        std::queue<size_t> q;
        q.push(i);
        component[i] = currentComp;
        ++cSize;

        while (!q.empty()) {
            size_t curr = q.front();
            q.pop();

            size_t cx = 0, cy = 0;
            coord.toCoord2D(curr, cx, cy);

            const int dx[4] = { 1, -1, 0, 0 };
            const int dy[4] = { 0, 0, 1, -1 };
            for (int k = 0; k < 4; ++k) {
                int nx = static_cast<int>(cx) + dx[k];
                int ny = static_cast<int>(cy) + dy[k];
                if (nx >= 0 && nx < s && ny >= 0 && ny < s) {
                    size_t nIdx = coord.toIndex2D(static_cast<size_t>(nx), static_cast<size_t>(ny));
                    if (playableMask[nIdx] && component[nIdx] == -1) {
                        component[nIdx] = currentComp;
                        ++cSize;
                        q.push(nIdx);
                    }
                }
            }
        }

        componentSizes.push_back(cSize);
        ++currentComp;
    }

    if (componentSizes.empty()) {
        // Fallback: entire board was voided, create solid center
        int cx = s / 2;
        int cy = s / 2;
        for (int y = std::max(0, cy - 2); y <= std::min(s - 1, cy + 2); ++y) {
            for (int x = std::max(0, cx - 2); x <= std::min(s - 1, cx + 2); ++x) {
                playableMask[coord.toIndex2D(static_cast<size_t>(x), static_cast<size_t>(y))] = true;
            }
        }
    } else {
        // Find largest component
        int bestComp = 0;
        size_t bestSize = 0;
        for (size_t c = 0; c < componentSizes.size(); ++c) {
            if (componentSizes[c] > bestSize) {
                bestSize = componentSizes[c];
                bestComp = static_cast<int>(c);
            }
        }

        // Keep only largest contiguous landmass
        for (size_t i = 0; i < total; ++i) {
            if (component[i] != bestComp) {
                playableMask[i] = false;
            }
        }
    }

    // Ensure central core exists and count total playables
    int cx = s / 2;
    int cy = s / 2;
    size_t centerIdx = coord.toIndex2D(static_cast<size_t>(cx), static_cast<size_t>(cy));
    playableMask[centerIdx] = true;

    playableCellCount = 0;
    for (size_t i = 0; i < total; ++i) {
        if (playableMask[i]) ++playableCellCount;
    }
}

Vector2 Board::findDockPlacement(float cellSize) const {
    if (config.dim != 2) return { -95.0f, 150.0f };

    int s = static_cast<int>(coord.size);
    float center = (static_cast<float>(s) - 1.0f) * 0.5f;

    // Raycast from center outward in western directions to find exterior convex edge
    const float angles[] = { 3.14159f, 3.14159f * 0.9f, 3.14159f * 1.1f, 3.14159f * 0.75f, 3.14159f * 1.25f };
    for (float angle : angles) {
        float r = 0.0f;
        int lastPlayableX = -1;
        int lastPlayableY = -1;
        while (r < static_cast<float>(s)) {
            float curX = center + r * std::cos(angle);
            float curY = center + r * std::sin(angle);
            int ix = static_cast<int>(std::round(curX));
            int iy = static_cast<int>(std::round(curY));
            if (ix < 0 || ix >= s || iy < 0 || iy >= s) break;

            size_t idx = coord.toIndex2D(static_cast<size_t>(ix), static_cast<size_t>(iy));
            if (isPlayable(idx)) {
                lastPlayableX = ix;
                lastPlayableY = iy;
            } else if (lastPlayableX != -1) {
                // Stepped off landmass into void
                break;
            }
            r += 0.5f;
        }

        if (lastPlayableX != -1) {
            float dockWorldX = (static_cast<float>(lastPlayableX) + 0.5f) * cellSize + std::cos(angle) * (cellSize * 2.2f);
            float dockWorldY = (static_cast<float>(lastPlayableY) + 0.5f) * cellSize + std::sin(angle) * (cellSize * 2.2f);
            return { dockWorldX, dockWorldY };
        }
    }

    return { -95.0f, center * cellSize };
}

bool Board::isEdge(size_t index) const {
    if (!isPlayable(index)) return false;
    if (config.dim != 2) return false;

    size_t x = 0, y = 0;
    coord.toCoord2D(index, x, y);
    int ix = static_cast<int>(x);
    int iy = static_cast<int>(y);
    int s = static_cast<int>(coord.size);

    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            int nx = ix + dx;
            int ny = iy + dy;
            if (nx < 0 || nx >= s || ny < 0 || ny >= s) return true;
            size_t nIdx = coord.toIndex2D(static_cast<size_t>(nx), static_cast<size_t>(ny));
            if (isVoid(nIdx)) return true;
        }
    }
    return false;
}

void Board::generateBombs(uint64_t seed) {
    bombs.clear();
    size_t total = coord.totalCells;
    size_t numBombs = static_cast<size_t>(config.bombs);
    if (total == 0 || numBombs == 0) return;

    Rng rng(seed);

    size_t placed = 0;
    size_t maxAttempts = total * 20;
    size_t attempts = 0;
    while (placed < numBombs && attempts++ < maxAttempts) {
        size_t index = static_cast<size_t>(rng.nextBounded(total));
        if (isPlayable(index) && !bombs.getBomb(index)) {
            bombs.setBomb(index);
            ++placed;
        }
    }
}

int64_t Board::findStartingCell() const {
    if (coord.totalCells == 0 || config.bombs >= static_cast<int>(totalPlayableCells())) {
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
        if (!isPlayable(i) || bombs.getBomb(i)) continue;

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
        if (!isPlayable(bombIndex)) return;
        coord.forEachNeighbor(bombIndex, [this](size_t neighborIndex) {
            if (isPlayable(neighborIndex)) {
                counts.increment(neighborIndex);
            }
        });
    });

    startingCell = findStartingCell();

    // Guaranteed Opening Drop: If no 0-value cell was found on the playable landmass,
    // relocate any bombs adjacent to the best starting candidate to guarantee a 0-value cascade!
    if (startingCell >= 0 && counts.get(static_cast<size_t>(startingCell)) > 0) {
        size_t sIdx = static_cast<size_t>(startingCell);
        std::vector<size_t> bombsToRelocate;
        coord.forEachNeighbor(sIdx, [&](size_t nIdx) {
            if (isPlayable(nIdx) && bombs.getBomb(nIdx)) {
                bombsToRelocate.push_back(nIdx);
                bombs.clearBomb(nIdx);
            }
        });

        // Place these bombs in other playable cells away from start cell and its neighbors
        Rng rng(config.seed + 999);
        for (size_t b = 0; b < bombsToRelocate.size(); ++b) {
            size_t attempts = 0;
            while (attempts++ < coord.totalCells * 4) {
                size_t candidate = static_cast<size_t>(rng.nextBounded(coord.totalCells));
                if (!isPlayable(candidate) || bombs.getBomb(candidate) || candidate == sIdx) continue;

                // Make sure candidate is not an immediate neighbor of sIdx
                bool isAdj = false;
                coord.forEachNeighbor(sIdx, [&](size_t nIdx) {
                    if (nIdx == candidate) isAdj = true;
                });
                if (!isAdj) {
                    bombs.setBomb(candidate);
                    break;
                }
            }
        }

        // Rebuild counts with relocated bombs
        counts.clear();
        bombs.forEachBomb([this](size_t bombIndex) {
            if (!isPlayable(bombIndex)) return;
            coord.forEachNeighbor(bombIndex, [this](size_t neighborIndex) {
                if (isPlayable(neighborIndex)) {
                    counts.increment(neighborIndex);
                }
            });
        });

        startingCell = findStartingCell();
    }
}

RevealResult Board::reveal(size_t startIndex, std::vector<size_t>* outRevealedCells) {
    if (startIndex >= coord.totalCells || isVoid(startIndex)) {
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
            if (isPlayable(neighborIdx) && state.get(neighborIdx) == CellState::Hidden) {
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
    if (revealedCount >= safeCells()) {
        isVictory = true;
        return RevealResult::Won;
    }

    return RevealResult::RevealedSafe;
}

void Board::setFlag(size_t index, uint32_t placerId, uint8_t skinId) {
    if (index >= coord.totalCells || isVoid(index)) return;
    CellState s = state.get(index);
    if (s != CellState::Flagged) {
        state.set(index, CellState::Flagged);
        ++flaggedCount;
    }
    flagOwners[index] = { placerId, skinId };
}

void Board::unflag(size_t index) {
    if (index >= coord.totalCells || isVoid(index)) return;
    CellState s = state.get(index);
    if (s == CellState::Flagged) {
        state.set(index, CellState::Hidden);
        if (flaggedCount > 0) --flaggedCount;
    }
    flagOwners.erase(index);
}

void Board::toggleFlag(size_t index, uint32_t placerId, uint8_t skinId) {
    if (index >= coord.totalCells || isVoid(index)) return;
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
    if (index >= coord.totalCells || isVoid(index)) return false;
    if (state.get(index) != CellState::Revealed) return false;

    uint8_t count = counts.get(index);
    if (count == 0) return false;

    int adjacentFlags = 0;
    coord.forEachNeighbor(index, [&](size_t nIdx) {
        if (isPlayable(nIdx) && state.get(nIdx) == CellState::Flagged) {
            ++adjacentFlags;
        }
    });

    if (adjacentFlags != static_cast<int>(count)) {
        return false;
    }

    bool didRevealAny = false;
    coord.forEachNeighbor(index, [&](size_t nIdx) {
        if (isPlayable(nIdx) && state.get(nIdx) == CellState::Hidden) {
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
