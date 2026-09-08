#include "item.hpp"
#include "render/asset_manager.hpp"
#include <algorithm>

namespace minesweeper::core {

ItemCatalog& ItemCatalog::instance() {
    static ItemCatalog s_instance;
    return s_instance;
}

void ItemCatalog::init() {
    if (isInitialized) return;
    items.clear();

    auto& am = render::AssetManager::instance();

    // 1. Banana (Tier 1, Common)
    Item banana;
    banana.id = ItemId::Banana;
    banana.strId = "banana";
    banana.name = "BANANA BOOST";
    banana.description = "Use [E] • +30% Speed for 20s";
    banana.tier = ItemTier::Tier1;
    banana.cost = 15;
    banana.icon = am.loadTexture("assets/items/banana.png");
    items.push_back(banana);

    // 2. Radar (Tier 2, Advanced)
    Item radar;
    radar.id = ItemId::Radar;
    radar.strId = "radar";
    radar.name = "MINE RADAR";
    radar.description = "Use [E] • Scans 3 cells for 4s";
    radar.tier = ItemTier::Tier2;
    radar.cost = 40;
    radar.icon = am.loadTexture("assets/items/radar.png");
    items.push_back(radar);

    // 3. Bubbles (Tier 3, High-Tech)
    Item bubbles;
    bubbles.id = ItemId::Bubbles;
    bubbles.strId = "bubbles";
    bubbles.name = "BUBBLES";
    bubbles.description = "Hold [E] • Emits bubbles & blurs";
    bubbles.tier = ItemTier::Tier3;
    bubbles.cost = 60;
    bubbles.icon = am.loadTexture("assets/items/bubbles.png");
    items.push_back(bubbles);

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

ShopInventory ItemCatalog::createInventoryForShop(const std::string& shopName, int capacity) const {
    (void)shopName;
    ShopInventory inv;
    inv.capacity = capacity;

    const Item* banana = getItem(ItemId::Banana);
    const Item* radar = getItem(ItemId::Radar);
    const Item* bubbles = getItem(ItemId::Bubbles);

    if (capacity <= 2) {
        // Mini shop: 2 slots (Tiers 1 & 2)
        if (banana) inv.slots.push_back({ *banana, false, 1 });
        if (radar) inv.slots.push_back({ *radar, false, 1 });
    } else {
        // Big shop: 4 slots (Tiers 1, 2, 3)
        if (banana) inv.slots.push_back({ *banana, false, 1 });
        if (radar) inv.slots.push_back({ *radar, false, 1 });
        if (bubbles) inv.slots.push_back({ *bubbles, false, 1 });
        if (bubbles) inv.slots.push_back({ *bubbles, false, 2 });
    }

    while (inv.slots.size() > static_cast<size_t>(capacity)) {
        inv.slots.pop_back();
    }

    return inv;
}

} // namespace minesweeper::core
