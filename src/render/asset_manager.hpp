#pragma once

#include "raylib.h"
#include "core/ship_config.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace minesweeper::render {

struct ShopShipAsset {
    std::string id;          // e.g. "shop1", "shop2"
    std::string path;        // Resolved file path
    std::string displayName; // e.g. "SHOP 1", "SHOP 2"
    Texture2D texture;       // Loaded GPU texture
    core::ShipConfig config; // Loaded ship configuration
};

class AssetManager {
public:
    static AssetManager& instance();

    void init();
    void shutdown();

    // Loads or retrieves cached texture by path. Sets TEXTURE_FILTER_POINT automatically.
    Texture2D loadTexture(const std::string& path);
    Texture2D getTexture(const std::string& path);
    bool hasTexture(const std::string& path) const;
    void unloadTexture(const std::string& path);
    void unloadAll();

    // Scans assets/shops/ and loads all available ship textures in alphabetical order
    std::vector<ShopShipAsset> loadShopShipAssets();

    // Resolves a relative asset path by testing common fallback directories
    std::string resolvePath(const std::string& relativePath) const;

private:
    AssetManager() = default;
    ~AssetManager();
    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    std::unordered_map<std::string, Texture2D> textures;
    std::vector<ShopShipAsset> shopShipAssets;
    bool isInitialized = false;
};

} // namespace minesweeper::render
