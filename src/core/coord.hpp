#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <concepts>
#include <algorithm>

namespace minesweeper::core {

struct CoordND {
    int dim = 2;
    size_t size = 10;
    size_t totalCells = 100;
    size_t stride1 = 10;
    size_t stride2 = 100;
    size_t stride3 = 1000;

    void init(int d, size_t s) {
        dim = d;
        size = s;
        stride1 = size;
        stride2 = size * size;
        stride3 = size * size * size;
        totalCells = 1;
        for (int i = 0; i < dim; ++i) {
            totalCells *= size;
        }
    }

    inline size_t toIndex2D(size_t x, size_t y) const {
        return x + y * stride1;
    }

    inline size_t toIndex3D(size_t x, size_t y, size_t z) const {
        return x + y * stride1 + z * stride2;
    }

    inline size_t toIndex4D(size_t x, size_t y, size_t z, size_t w) const {
        return x + y * stride1 + z * stride2 + w * stride3;
    }

    inline void toCoord2D(size_t index, size_t& x, size_t& y) const {
        x = index % stride1;
        y = index / stride1;
    }

    inline void toCoord3D(size_t index, size_t& x, size_t& y, size_t& z) const {
        x = index % stride1;
        y = (index / stride1) % stride1;
        z = index / stride2;
    }

    inline void toCoord4D(size_t index, size_t& x, size_t& y, size_t& z, size_t& w) const {
        x = index % stride1;
        y = (index / stride1) % stride1;
        z = (index / stride2) % stride1;
        w = index / stride3;
    }

    // Inlined high-performance neighbor traversal
    template <typename Func>
    inline void forEachNeighbor(size_t index, Func&& func) const {
        const size_t S = size;
        const size_t S1 = stride1;
        const size_t S2 = stride2;
        const size_t S3 = stride3;

        if (dim == 2) {
            const size_t x = index % S1;
            const size_t y = index / S1;

            const size_t minX = (x > 0) ? x - 1 : 0;
            const size_t maxX = (x + 1 < S) ? x + 1 : S - 1;
            const size_t minY = (y > 0) ? y - 1 : 0;
            const size_t maxY = (y + 1 < S) ? y + 1 : S - 1;

            for (size_t ny = minY; ny <= maxY; ++ny) {
                const size_t rowBase = ny * S1;
                for (size_t nx = minX; nx <= maxX; ++nx) {
                    const size_t nIdx = rowBase + nx;
                    if (nIdx != index) {
                        func(nIdx);
                    }
                }
            }
        }
        else if (dim == 3) {
            const size_t x = index % S1;
            const size_t y = (index / S1) % S1;
            const size_t z = index / S2;

            const size_t minX = (x > 0) ? x - 1 : 0;
            const size_t maxX = (x + 1 < S) ? x + 1 : S - 1;
            const size_t minY = (y > 0) ? y - 1 : 0;
            const size_t maxY = (y + 1 < S) ? y + 1 : S - 1;
            const size_t minZ = (z > 0) ? z - 1 : 0;
            const size_t maxZ = (z + 1 < S) ? z + 1 : S - 1;

            for (size_t nz = minZ; nz <= maxZ; ++nz) {
                const size_t zBase = nz * S2;
                for (size_t ny = minY; ny <= maxY; ++ny) {
                    const size_t yBase = zBase + ny * S1;
                    for (size_t nx = minX; nx <= maxX; ++nx) {
                        const size_t nIdx = yBase + nx;
                        if (nIdx != index) {
                            func(nIdx);
                        }
                    }
                }
            }
        }
        else if (dim >= 4) {
            const size_t x = index % S1;
            const size_t y = (index / S1) % S1;
            const size_t z = (index / S2) % S1;
            const size_t w = index / S3;

            const size_t minX = (x > 0) ? x - 1 : 0;
            const size_t maxX = (x + 1 < S) ? x + 1 : S - 1;
            const size_t minY = (y > 0) ? y - 1 : 0;
            const size_t maxY = (y + 1 < S) ? y + 1 : S - 1;
            const size_t minZ = (z > 0) ? z - 1 : 0;
            const size_t maxZ = (z + 1 < S) ? z + 1 : S - 1;
            const size_t minW = (w > 0) ? w - 1 : 0;
            const size_t maxW = (w + 1 < S) ? w + 1 : S - 1;

            for (size_t nw = minW; nw <= maxW; ++nw) {
                const size_t wBase = nw * S3;
                for (size_t nz = minZ; nz <= maxZ; ++nz) {
                    const size_t zBase = wBase + nz * S2;
                    for (size_t ny = minY; ny <= maxY; ++ny) {
                        const size_t yBase = zBase + ny * S1;
                        for (size_t nx = minX; nx <= maxX; ++nx) {
                            const size_t nIdx = yBase + nx;
                            if (nIdx != index) {
                                func(nIdx);
                            }
                        }
                    }
                }
            }
        }
    }
};

} // namespace minesweeper::core
