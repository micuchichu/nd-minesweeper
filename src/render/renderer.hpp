#pragma once

#include "../core/board.hpp"
#include "../net/network_manager.hpp"
#include "raylib.h"
#include <cstdint>

namespace minesweeper::render {

class IRenderer {
public:
    virtual ~IRenderer() = default;

    virtual void init() = 0;
    virtual void update(float dt) = 0;
    virtual void render(const core::Board& board, int64_t hoveredIndex, const net::NetworkManager& net) = 0;
    virtual void cleanup() = 0;

    virtual int64_t getHoveredCellIndex(const core::Board& board) const = 0;
    virtual Vector2 getCellWorldPosition(size_t index, const core::Board& board) const = 0;
};

} // namespace minesweeper::render
