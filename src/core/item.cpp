#include "item.hpp"
#include "json_parser.hpp"
#include "render/asset_manager.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

namespace minesweeper::core {

ItemCatalog& ItemCatalog::instance() {
    static ItemCatalog s_instance;
    return s_instance;
}

void ItemCatalog::init() {
    if (isInitialized) return;
    items.clear();

    auto& am = render::AssetManager::instance();

    // 1. Scan candidate directories for data-driven item JSONs
    const std::vector<std::string> itemDirs = {
        "assets/items",
        "../assets/items",
        "../../assets/items",
        "minesweeper/assets/items"
    };

    std::string validDir;
    for (const auto& dir : itemDirs) {
        if (fs::exists(dir) && fs::is_directory(dir)) {
            validDir = dir;
            break;
        }
    }

    if (!validDir.empty()) {
        std::vector<fs::path> jsonFiles;
        for (const auto& entry : fs::directory_iterator(validDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                jsonFiles.push_back(entry.path());
            }
        }
        std::sort(jsonFiles.begin(), jsonFiles.end());

        for (const auto& p : jsonFiles) {
            std::ifstream f(p);
            if (!f.is_open()) continue;
            std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
            f.close();

            JsonValue root;
            if (!SimpleJsonParser::parse(content, root) || !root.isObject()) continue;

            Item item;
            std::string stem = p.stem().string();
            item.strId = root.contains("id") ? root["id"].asString(stem) : stem;

            if (item.strId == "banana") item.id = ItemId::Banana;
            else if (item.strId == "radar") item.id = ItemId::Radar;
            else if (item.strId == "bubbles") item.id = ItemId::Bubbles;

            item.name = root.contains("name") ? root["name"].asString(stem) : stem;
            item.description = root.contains("description") ? root["description"].asString("") : "";
            int t = root.contains("tier") ? root["tier"].asInt(1) : 1;
            item.tier = static_cast<ItemTier>(std::clamp(t, 1, 3));
            item.cost = root.contains("cost") ? static_cast<uint64_t>(root["cost"].asInt(15)) : 15;
            item.defaultDurability = root.contains("durability") ? root["durability"].asFloat(1.0f) : 1.0f;
            item.maxDurability = item.defaultDurability;

            std::string iconPath = root.contains("icon") ? root["icon"].asString("") : "";
            if (iconPath.empty()) {
                fs::path pngPath = p.parent_path() / (stem + ".png");
                if (fs::exists(pngPath)) iconPath = pngPath.string();
            }
            if (!iconPath.empty()) {
                item.icon = am.loadTexture(iconPath);
            }
            items.push_back(item);
        }
    }

    // Fallback: If no JSONs found (e.g. headless unit tests without assets), supply standard items
    if (items.empty()) {
        Item banana;
        banana.id = ItemId::Banana;
        banana.strId = "banana";
        banana.name = "BANANA BOOST";
        banana.description = "Use [E] • +30% Speed for 20s";
        banana.tier = ItemTier::Tier1;
        banana.cost = 15;
        banana.defaultDurability = 1.0f;
        banana.maxDurability = 1.0f;
        banana.icon = am.loadTexture("assets/items/banana.png");
        items.push_back(banana);

        Item radar;
        radar.id = ItemId::Radar;
        radar.strId = "radar";
        radar.name = "MINE RADAR";
        radar.description = "Use [E] • Scans 3 cells for 4s";
        radar.tier = ItemTier::Tier2;
        radar.cost = 40;
        radar.defaultDurability = 1.0f;
        radar.maxDurability = 1.0f;
        radar.icon = am.loadTexture("assets/items/radar.png");
        items.push_back(radar);

        Item bubbles;
        bubbles.id = ItemId::Bubbles;
        bubbles.strId = "bubbles";
        bubbles.name = "BUBBLES";
        bubbles.description = "Hold [E] • Emits bubbles & blurs";
        bubbles.tier = ItemTier::Tier3;
        bubbles.cost = 60;
        bubbles.defaultDurability = 6.0f;
        bubbles.maxDurability = 6.0f;
        bubbles.icon = am.loadTexture("assets/items/bubbles.png");
        items.push_back(bubbles);
    }

    isInitialized = true;
}

void ItemCatalog::shutdown() {
    items.clear();
    isInitialized = false;
}

const Item* ItemCatalog::getItem(ItemId id) const {
    for (const auto& it : items) {
        if (it.id == id) return &it;
    }
    return nullptr;
}

const Item* ItemCatalog::getItem(const std::string& strId) const {
    for (const auto& it : items) {
        if (it.strId == strId) return &it;
    }
    return nullptr;
}

ShopInventory ItemCatalog::createInventoryForShop(int shopTier, int capacity) const {
    ShopInventory inv;
    inv.capacity = capacity;

    std::vector<const Item*> eligibleItems;
    for (const auto& it : items) {
        if (shopTier <= 1) {
            if (it.tier <= ItemTier::Tier2) eligibleItems.push_back(&it);
        } else {
            eligibleItems.push_back(&it);
        }
    }

    std::sort(eligibleItems.begin(), eligibleItems.end(), [](const Item* a, const Item* b) {
        if (a->tier != b->tier) return static_cast<int>(a->tier) < static_cast<int>(b->tier);
        return a->cost < b->cost;
    });

    if (eligibleItems.empty()) {
        const Item* banana = getItem(ItemId::Banana);
        const Item* radar = getItem(ItemId::Radar);
        const Item* bubbles = getItem(ItemId::Bubbles);
        if (banana) eligibleItems.push_back(banana);
        if (radar) eligibleItems.push_back(radar);
        if (shopTier > 1 && bubbles) eligibleItems.push_back(bubbles);
    }

    if (!eligibleItems.empty()) {
        for (int i = 0; i < capacity; ++i) {
            const Item* item = eligibleItems[static_cast<size_t>(i) % eligibleItems.size()];
            inv.slots.push_back({ *item, false, 1 });
        }
    }

    return inv;
}

ShopInventory ItemCatalog::createInventoryForShop(const std::string& shopName, int capacity) const {
    int tier = (capacity > 2 || shopName.find("big") != std::string::npos) ? 2 : 1;
    return createInventoryForShop(tier, capacity);
}

} // namespace minesweeper::core
