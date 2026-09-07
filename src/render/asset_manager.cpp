#include "asset_manager.hpp"
#include <filesystem>
#include <algorithm>
#include <iostream>

namespace fs = std::filesystem;

namespace minesweeper::render {

AssetManager& AssetManager::instance() {
    static AssetManager inst;
    return inst;
}

AssetManager::~AssetManager() {
    shutdown();
}

void AssetManager::init() {
    isInitialized = true;
}

void AssetManager::shutdown() {
    unloadAll();
    isInitialized = false;
}

std::string AssetManager::resolvePath(const std::string& relativePath) const {
    if (relativePath.empty()) return "";
    if (fs::exists(relativePath)) return relativePath;

    const std::vector<std::string> prefixes = {
        "",
        "assets/",
        "../assets/",
        "../../assets/",
        "minesweeper/assets/",
        "../",
        "../../"
    };

    for (const auto& pre : prefixes) {
        std::string candidate = pre + relativePath;
        if (fs::exists(candidate)) {
            return candidate;
        }
    }

    return relativePath;
}

Texture2D AssetManager::loadTexture(const std::string& path) {
    std::string resolved = resolvePath(path);
    auto it = textures.find(resolved);
    if (it != textures.end()) {
        return it->second;
    }

    if (!fs::exists(resolved)) {
        return Texture2D{ 0 };
    }

    Texture2D tex = LoadTexture(resolved.c_str());
    if (tex.id != 0) {
        SetTextureFilter(tex, TEXTURE_FILTER_POINT);
        textures[resolved] = tex;
    }
    return tex;
}

Texture2D AssetManager::getTexture(const std::string& path) {
    std::string resolved = resolvePath(path);
    auto it = textures.find(resolved);
    if (it != textures.end()) {
        return it->second;
    }
    return loadTexture(path);
}

bool AssetManager::hasTexture(const std::string& path) const {
    std::string resolved = resolvePath(path);
    return textures.find(resolved) != textures.end();
}

void AssetManager::unloadTexture(const std::string& path) {
    std::string resolved = resolvePath(path);
    auto it = textures.find(resolved);
    if (it != textures.end()) {
        if (it->second.id != 0) {
            UnloadTexture(it->second);
        }
        textures.erase(it);
    }
}

void AssetManager::unloadAll() {
    for (auto& [path, tex] : textures) {
        if (tex.id != 0) {
            UnloadTexture(tex);
        }
    }
    textures.clear();
    shopShipAssets.clear();
}

std::vector<ShopShipAsset> AssetManager::loadShopShipAssets() {
    if (!shopShipAssets.empty()) {
        return shopShipAssets;
    }

    const std::vector<std::string> shopDirs = {
        "assets/shops",
        "../assets/shops",
        "../../assets/shops",
        "minesweeper/assets/shops"
    };

    std::string validDir;
    for (const auto& dir : shopDirs) {
        if (fs::exists(dir) && fs::is_directory(dir)) {
            validDir = dir;
            break;
        }
    }

    if (validDir.empty()) {
        return shopShipAssets;
    }

    std::vector<fs::path> filePaths;
    for (const auto& entry : fs::directory_iterator(validDir)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            if (ext == ".png") {
                filePaths.push_back(entry.path());
            }
        }
    }

    std::sort(filePaths.begin(), filePaths.end());

    int shopIndex = 1;
    for (const auto& p : filePaths) {
        std::string pathStr = p.string();
        std::string stem = p.stem().string(); // e.g. "shop1"

        Texture2D tex = loadTexture(pathStr);
        if (tex.id != 0) {
            std::string displayName;
            if (stem.rfind("shop", 0) == 0 && stem.length() > 4) {
                displayName = "SHOP " + stem.substr(4);
            } else {
                displayName = "SHOP " + std::to_string(shopIndex);
            }

            core::ShipConfig cfg = core::ShipConfig::createDefault(tex.width, tex.height, displayName);
            fs::path jsonPath = p.parent_path() / (stem + ".json");
            if (fs::exists(jsonPath)) {
                core::ShipConfig::loadFromFile(jsonPath.string(), cfg);
            } else {
                std::string resolvedJson = resolvePath("shops/" + stem + ".json");
                if (!fs::exists(resolvedJson)) {
                    resolvedJson = resolvePath("assets/shops/" + stem + ".json");
                }
                if (fs::exists(resolvedJson)) {
                    core::ShipConfig::loadFromFile(resolvedJson, cfg);
                }
            }

            if (!cfg.name.empty()) {
                displayName = cfg.name;
            }

            shopShipAssets.push_back({ stem, pathStr, displayName, tex, cfg });
            ++shopIndex;
        }
    }

    return shopShipAssets;
}

} // namespace minesweeper::render
