#pragma once

#include <cstdint>
#include <cstddef>

namespace minesweeper::core {

// High-speed, 100% cross-platform deterministic 64-bit Pseudo-Random Number Generator.
// Implements SplitMix64 with Lemire / unbiased threshold rejection sampling.
//
// Guaranteed bit-exact behavior across MSVC (Windows), Clang (macOS), and GCC (Linux)
// on both x86_64 and ARM64 (Apple Silicon). Eliminates platform divergence caused
// by compiler-specific std::uniform_int_distribution implementations.
class Rng {
public:
    uint64_t state;

    constexpr explicit Rng(uint64_t seed = 0)
        : state(seed == 0 ? 0x853c49e6748fea9bULL : seed) {}

    // Generates the next pseudo-random 64-bit integer
    inline uint64_t nextU64() {
        uint64_t z = (state += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }

    // Generates the next pseudo-random 32-bit integer
    inline uint32_t nextU32() {
        return static_cast<uint32_t>(nextU64() >> 32);
    }

    // Unbiased uniform integer in range [0, bound - 1]
    // Rejection sampling on the incomplete bucket: threshold = (2^64 - bound) % bound
    // Mathematically guaranteed 0 modulo bias and bit-exact consistency across all platforms.
    inline uint64_t nextBounded(uint64_t bound) {
        if (bound <= 1) return 0;
        uint64_t threshold = (0ULL - bound) % bound;
        while (true) {
            uint64_t r = nextU64();
            if (r >= threshold) {
                return r % bound;
            }
        }
    }
};

} // namespace minesweeper::core
