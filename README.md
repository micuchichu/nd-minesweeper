# N-Dimensional Minesweeper (nd-minesweeper)

A super-optimized, high-performance **N-Dimensional Minesweeper** remake built in modern **C++20** with **Raylib**, featuring 2D, 3D, and 4D hypercube gameplay, custom CRT post-processing, and online multiplayer powered by **ENet** and **Steamworks P2P (Spacewar 480)**.

---

## Key Features

- **N-Dimensional Gameplay**:
  - **2D**: Classic Minesweeper grid with ultra-fast flood-fill cascades.
  - **3D**: 3D cube decomposed into interactive 2D slices with full 26-neighbor connectivity.
  - **4D**: 4D tesseract/hypercube sliced along $ and $ axes with 80-neighbor connectivity.
- **Extreme Performance**:
  - Ultra-compact **1-bit BombBoard** (1,000,000 cells fit in just **125 KB** of L2 cache).
  - Fast bitwise population count (std::popcount) and hardware bit-scan (std::countr_zero).
  - Generates 1,000,000 cells with 150,000 mines and builds neighbor count caches in **< 12 ms**.
  - Iterative non-recursive flood fill and chording.
- **Cross-Platform Deterministic PRNG**:
  - Custom 64-bit **SplitMix64** generator with Lemire threshold rejection sampling.
  - Eliminates compiler/OS discrepancies (std::uniform_int_distribution variance between MSVC, Apple Clang, and GCC).
  - Identical seeds generate bit-exact identical boards across Windows, macOS, and Linux.
- **Rendering & Visuals**:
  - Raylib backend with viewport frustum culling and dynamic Level of Detail (LOD).
  - Smooth pan, zoom, and coordinate unprojection.
  - Subtle retro CRT shader pass with curvature, scanlines, bloom, and chromatic aberration.
  - Contiguous DOD particle pools for mine explosions and cell debris.
- **Multiplayer & Networking**:
  - Direct IP (ENet) and **Steam P2P** overlay support (via Spacewar App ID 480).
  - Full server-authoritative state synchronization supporting mid-game late joins.
  - Multiplayer chording (middle-click / 'C' key).
  - Remote player cursor tracking with custom colors and name tags.
  - Attribution-based flag placement (flags display who placed them and despawn if a player leaves).
- **Customization & Skins**:
  - Dynamic skin discovery from ssets/skins/cursors/ and ssets/skins/flags/.
  - Horizontal reel selector with live preview in the Customize menu.

---

## Building and Running

### Windows (Visual Studio 2022)
1. Open minesweeper.sln in Visual Studio 2022.
2. Select **Release** or **Debug** with the **x64** architecture.
3. Build and run (Ctrl+F5 or F5).

### Cross-Platform (CMake)
`ash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
`

### macOS
Run the automated macOS build script:
`ash
./build_macos.sh
`

---

## Controls

| Action | Control |
| :--- | :--- |
| **Reveal Cell** | Left Click |
| **Flag / Unflag Cell** | Right Click |
| **Chord (Reveal Neighbors)** | Middle Click / C key |
| **Pan Camera** | Right-Click Drag / Middle-Click Drag |
| **Zoom Camera** | Mouse Scroll Wheel |
| **Restart Game** | R key (Host / Solo) |
| **Toggle Menu** | Escape key |
| **Step Config Values by 10** | Hold Shift while clicking + / - |

---

## License
MIT License
