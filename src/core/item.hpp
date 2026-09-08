#pragma once

#include "raylib.h"
#include <string>
#include <vector>
#include <array>
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

struct InventorySlot {
    Item item;
    bool occupied = false;
    float durability = 1.0f;
    float maxDurability = 1.0f;
};

struct PlayerInventory {
    static constexpr int CAPACITY = 5;
    std::array<InventorySlot, CAPACITY> slots;
    int selectedSlot = 0; // 0 .. 4

    // Active buffs granted when items are used
    float bananaBoostTimer = 0.0f; // > 0 when speed boost is active
    float radarActiveTimer = 0.0f; // > 0 when radar scan is active (3-5s)

    bool hasFreeSlot() const {
        for (const auto& s : slots) {
            if (!s.occupied) return true;
        }
        return false;
    }

    int getFreeSlotIndex() const {
        for (int i = 0; i < CAPACITY; ++i) {
            if (!slots[i].occupied) return i;
        }
        return -1;
    }

    bool addItem(const Item& item) {
        int idx = getFreeSlotIndex();
        if (idx == -1) return false;
        slots[idx].item = item;
        slots[idx].occupied = true;
        if (item.id == ItemId::Bubbles) {
            slots[idx].durability = 6.0f;
            slots[idx].maxDurability = 6.0f;
        } else {
            slots[idx].durability = 1.0f;
            slots[idx].maxDurability = 1.0f;
        }
        return true;
    }

    void clearSlot(int index) {
        if (index >= 0 && index < CAPACITY) {
            slots[index].occupied = false;
            slots[index].item = Item();
            slots[index].durability = 0.0f;
            slots[index].maxDurability = 1.0f;
        }
    }

    InventorySlot* getSelectedSlot() {
        if (selectedSlot >= 0 && selectedSlot < CAPACITY && slots[selectedSlot].occupied) {
            return &slots[selectedSlot];
        }
        return nullptr;
    }

    const InventorySlot* getSelectedSlot() const {
        if (selectedSlot >= 0 && selectedSlot < CAPACITY && slots[selectedSlot].occupied) {
            return &slots[selectedSlot];
        }
        return nullptr;
    }

    int getItemCount(ItemId id) const {
        int count = 0;
        for (const auto& s : slots) {
            if (s.occupied && s.item.id == id) ++count;
        }
        return count;
    }

    void reset() {
        for (auto& s : slots) {
            s.occupied = false;
            s.item = Item();
            s.durability = 0.0f;
            s.maxDurability = 1.0f;
        }
        selectedSlot = 0;
        bananaBoostTimer = 0.0f;
        radarActiveTimer = 0.0f;
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
