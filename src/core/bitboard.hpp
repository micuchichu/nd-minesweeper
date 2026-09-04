#pragma once

#include "types.hpp"
#include <vector>
#include <cstdint>
#include <bit>
#include <algorithm>

namespace minesweeper::core {

struct BombBoard {
    std::vector<uint64_t> data;
    size_t totalCells = 0;

    void init(size_t total) {
        totalCells = total;
        data.assign((total + 63) / 64, 0ULL);
    }

    void clear() {
        std::fill(data.begin(), data.end(), 0ULL);
    }

    inline void setBomb(size_t index) {
        data[index >> 6] |= (1ULL << (index & 63));
    }

    inline void clearBomb(size_t index) {
        data[index >> 6] &= ~(1ULL << (index & 63));
    }

    inline bool getBomb(size_t index) const {
        return (data[index >> 6] >> (index & 63)) & 1ULL;
    }

    inline size_t countBombs() const {
        size_t count = 0;
        for (uint64_t word : data) {
            count += std::popcount(word);
        }
        return count;
    }

    template <typename Func>
    inline void forEachBomb(Func&& func) const {
        for (size_t b = 0; b < data.size(); ++b) {
            uint64_t word = data[b];
            while (word != 0) {
                int bit = std::countr_zero(word);
                size_t index = (b << 6) + bit;
                if (index < totalCells) {
                    func(index);
                }
                word &= (word - 1);
            }
        }
    }
};

struct StateBoard {
    std::vector<uint64_t> data;
    size_t totalCells = 0;

    void init(size_t total) {
        totalCells = total;
        data.assign((total + 31) / 32, 0ULL);
    }

    void clear() {
        std::fill(data.begin(), data.end(), 0ULL);
    }

    inline void set(size_t index, CellState state) {
        size_t block = index >> 5;
        if (block >= data.size()) return;
        size_t offset = (index & 31) << 1;

        data[block] &= ~(3ULL << offset);
        data[block] |= (static_cast<uint64_t>(state) << offset);
    }

    inline CellState get(size_t index) const {
        size_t block = index >> 5;
        if (block >= data.size()) return CellState::Hidden;
        size_t offset = (index & 31) << 1;

        return static_cast<CellState>((data[block] >> offset) & 3ULL);
    }

    inline size_t countRevealed() const {
        size_t count = 0;
        for (uint64_t block : data) {
            // State 1 is binary 01 (Revealed)
            uint64_t lower = block & 0x5555555555555555ULL;
            uint64_t upper = (block >> 1) & 0x5555555555555555ULL;
            uint64_t revealed = lower & ~upper;
            count += std::popcount(revealed);
        }
        return count;
    }
};

struct CountBoard {
    std::vector<uint8_t> data;

    void init(size_t total) {
        data.assign(total, 0);
    }

    void clear() {
        std::fill(data.begin(), data.end(), static_cast<uint8_t>(0));
    }

    inline void increment(size_t index) {
        ++data[index];
    }

    inline void set(size_t index, uint8_t count) {
        data[index] = count;
    }

    inline uint8_t get(size_t index) const {
        return data[index];
    }

    inline const uint8_t* rawData() const {
        return data.data();
    }
};

} // namespace minesweeper::core
