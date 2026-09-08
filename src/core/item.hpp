#pragma once

#include "raylib.h"
#include <string>
#include <vector>
#include <cstdint>

namespace minesweeper::core {

enum class ItemTier : uint8_t {
    Tier1 = 1, // Common (Banana)
    Tier2 = 2, // Advanced (Radar)
    Tier3 = 3  // High-Tech (Bubbles)
};

enum class ItemId : uint8_t {
    None = 0,
    Banana = 1,
    Radar = 2,
    Bubbles = 3
};

struct Item {
    ItemId id = ItemId::None;
    std::string strId;
    std::string name;
    std::string description;
    ItemTier tier = ItemTier::Tier1;
    uint64_t cost = 15;
    Texture2D icon = { 0 };
};

struct ShopSlot {
    Item item;
    bool isPurchased = false;
    int charges = 1;
};

struct ShopInventory {
    int capacity = 2; // 2 for mini shop, 4 for big shop
    std::vector<ShopSlot> slots;
};

struct PlayerInventory {
    bool hasBanana = false;
    bool hasRadar = false;
    int bubbleCharges = 0;

    void reset() {
        hasBanana = false;
        hasRadar = false;
        bubbleCharges = 0;
    }
};

class ItemCatalog {
public:
    static ItemCatalog& instance();

    void init();
    void shutdown();

    const Item* getItem(ItemId id) const;
    const Item* getItem(const std::string& strId) const;
    const std::vector<Item>& getAllItems() const { return items; }

    // Generates inventory for shops based on capacity and tier constraints
    ShopInventory createInventoryForShop(const std::string& shopName, int capacity) const;

private:
    ItemCatalog() = default;
    ~ItemCatalog() = default;
    ItemCatalog(const ItemCatalog&) = delete;
    ItemCatalog& operator=(const ItemCatalog&) = delete;

    std::vector<Item> items;
    bool isInitialized = false;
};

} // namespace minesweeper::core
