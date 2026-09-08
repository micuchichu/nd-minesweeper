#include "shop_menu.hpp"
#include "theme.hpp"
#include <cmath>
#include <cstdio>
#include <algorithm>

namespace minesweeper::ui {

bool ShopMenu::drawHoverMenu(
    core::ShopShip& shop,
    uint64_t& scrapCount,
    core::PlayerInventory& playerInv,
    float alpha,
    const render::CameraController& camera,
    int screenW,
    int screenH
) {
    if (alpha <= 0.01f) return false;

    pulseTimer += GetFrameTime();
    bool purchasedItem = false;

    Vector2 shopScreen = camera.getWorldToScreen(shop.position);

    // Menu geometry
    const float cardW = 310.0f;
    const float headerH = 42.0f;
    const float rowH = 48.0f;
    const float footerH = 22.0f;
    const float padding = 8.0f;
    const float slotCount = static_cast<float>(shop.inventory.slots.size());
    const float cardH = headerH + (slotCount * rowH) + ((slotCount > 0 ? slotCount - 1 : 0) * 4.0f) + footerH + padding * 2.0f;

    // Screen clamping: prefer above the ship, flip below if too close to top
    float cardX = std::clamp(shopScreen.x - cardW * 0.5f, 16.0f, static_cast<float>(screenW) - cardW - 16.0f);
    float cardY = shopScreen.y - cardH - 30.0f;
    if (cardY < 75.0f) {
        cardY = shopScreen.y + 40.0f;
    }
    cardY = std::clamp(cardY, 75.0f, static_cast<float>(screenH) - cardH - 16.0f);

    // 1. Holographic connector line from shop to menu
    Vector2 anchorCard = (cardY > shopScreen.y)
        ? Vector2{ cardX + cardW * 0.5f, cardY }
        : Vector2{ cardX + cardW * 0.5f, cardY + cardH };

    Color reticleCol = Fade(Colors::Amber400, alpha * 0.45f);
    DrawLineEx(shopScreen, anchorCard, 1.5f, reticleCol);
    DrawCircleV(shopScreen, 3.5f, Fade(Colors::Amber300, alpha * 0.7f));
    DrawCircleLines(static_cast<int>(shopScreen.x), static_cast<int>(shopScreen.y), 6.0f, reticleCol);

    // 2. Card Background & Mindustry sci-fi frame
    Rectangle cardRect = { cardX, cardY, cardW, cardH };
    DrawRectangleRec(cardRect, Fade(Color{ 14, 15, 19, 248 }, alpha));
    DrawRectangleLinesEx(cardRect, 1.5f, Fade(Colors::Zinc700, alpha));

    // Sci-fi corner brackets
    Color accentCol = Fade(Colors::Amber400, alpha);
    float bLen = 8.0f;
    // Top-left
    DrawLineEx({ cardX, cardY }, { cardX + bLen, cardY }, 2.0f, accentCol);
    DrawLineEx({ cardX, cardY }, { cardX, cardY + bLen }, 2.0f, accentCol);
    // Top-right
    DrawLineEx({ cardX + cardW, cardY }, { cardX + cardW - bLen, cardY }, 2.0f, accentCol);
    DrawLineEx({ cardX + cardW, cardY }, { cardX + cardW, cardY + bLen }, 2.0f, accentCol);
    // Bottom-left
    DrawLineEx({ cardX, cardY + cardH }, { cardX + bLen, cardY + cardH }, 2.0f, accentCol);
    DrawLineEx({ cardX, cardY + cardH }, { cardX, cardY + cardH - bLen }, 2.0f, accentCol);
    // Bottom-right
    DrawLineEx({ cardX + cardW, cardY + cardH }, { cardX + cardW - bLen, cardY + cardH }, 2.0f, accentCol);
    DrawLineEx({ cardX + cardW, cardY + cardH }, { cardX + cardW, cardY + cardH - bLen }, 2.0f, accentCol);

    // 3. Card Header
    std::string titleStr = shop.name.empty() ? "SHOP" : shop.name;
    std::transform(titleStr.begin(), titleStr.end(), titleStr.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });

    const char* tierCapStr = (shop.inventory.capacity > 2) ? "TIER 1-3 • 4 SLOTS" : "TIER 1-2 • 2 SLOTS";

    DrawText(titleStr.c_str(), static_cast<int>(cardX + 12.0f), static_cast<int>(cardY + 8.0f), 15, Fade(Colors::Amber400, alpha));
    DrawText(tierCapStr, static_cast<int>(cardX + 12.0f), static_cast<int>(cardY + 25.0f), 10, Fade(Colors::Zinc400, alpha));

    // Scrap counter in header
    char scrapBuf[32];
    std::snprintf(scrapBuf, sizeof(scrapBuf), "SCRAP: %llu", scrapCount);
    int scrapW = MeasureText(scrapBuf, 13);
    DrawText(scrapBuf, static_cast<int>(cardX + cardW - static_cast<float>(scrapW) - 12.0f), static_cast<int>(cardY + 12.0f), 13, Fade(Colors::Amber300, alpha));

    // Header divider line
    DrawLineEx({ cardX + 8.0f, cardY + headerH }, { cardX + cardW - 8.0f, cardY + headerH }, 1.0f, Fade(Colors::Zinc800, alpha));

    // 4. Slots
    Vector2 mousePos = GetMousePosition();
    float curY = cardY + headerH + padding;

    for (size_t i = 0; i < shop.inventory.slots.size(); ++i) {
        auto& slot = shop.inventory.slots[i];
        Rectangle rowRect = { cardX + padding, curY, cardW - padding * 2.0f, rowH };
        bool isRowHovered = CheckCollisionPointRec(mousePos, rowRect);

        DrawRectangleRec(rowRect, Fade(isRowHovered ? Colors::Zinc850 : Colors::Zinc900, alpha));
        DrawRectangleLinesEx(rowRect, 1.0f, Fade(isRowHovered ? Colors::Zinc600 : Colors::Zinc800, alpha));

        // Draw Icon
        float iconSize = 34.0f;
        Rectangle iconRect = { rowRect.x + 6.0f, rowRect.y + (rowH - iconSize) * 0.5f, iconSize, iconSize };
        DrawRectangleRec(iconRect, Fade(Colors::Zinc950, alpha));
        DrawRectangleLinesEx(iconRect, 1.0f, Fade(Colors::Zinc700, alpha));

        if (slot.item.icon.id != 0) {
            Rectangle src = { 0.0f, 0.0f, static_cast<float>(slot.item.icon.width), static_cast<float>(slot.item.icon.height) };
            Rectangle dst = { iconRect.x + 2.0f, iconRect.y + 2.0f, iconSize - 4.0f, iconSize - 4.0f };
            DrawTexturePro(slot.item.icon, src, dst, { 0, 0 }, 0.0f, Fade(WHITE, alpha));
        }

        // Tier badge & Title
        Color tierCol = Colors::Green400;
        const char* tierLabel = "T1";
        if (slot.item.tier == core::ItemTier::Tier2) {
            tierCol = Colors::Amber400;
            tierLabel = "T2";
        } else if (slot.item.tier == core::ItemTier::Tier3) {
            tierCol = Colors::Purple400;
            tierLabel = "T3";
        }

        float textX = iconRect.x + iconSize + 8.0f;
        DrawText(tierLabel, static_cast<int>(textX), static_cast<int>(rowRect.y + 6.0f), 10, Fade(tierCol, alpha));
        DrawText(slot.item.name.c_str(), static_cast<int>(textX + 20.0f), static_cast<int>(rowRect.y + 5.0f), 12, Fade(WHITE, alpha));
        DrawText(slot.item.description.c_str(), static_cast<int>(textX), static_cast<int>(rowRect.y + 24.0f), 10, Fade(Colors::Zinc400, alpha));

        // Action / Buy Button
        float btnW = 86.0f;
        float btnH = 28.0f;
        Rectangle btnRect = { rowRect.x + rowRect.width - btnW - 6.0f, rowRect.y + (rowH - btnH) * 0.5f, btnW, btnH };
        bool isBtnHovered = CheckCollisionPointRec(mousePos, btnRect);

        // Check ownership state
        bool isOwned = false;
        if (slot.item.id == core::ItemId::Banana && playerInv.hasBanana) isOwned = true;
        if (slot.item.id == core::ItemId::Radar && playerInv.hasRadar) isOwned = true;

        bool hotkeyPressed = false;
        if (i == 0 && (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1))) hotkeyPressed = true;
        if (i == 1 && (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2))) hotkeyPressed = true;
        if (i == 2 && (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3))) hotkeyPressed = true;
        if (i == 3 && (IsKeyPressed(KEY_FOUR) || IsKeyPressed(KEY_KP_4))) hotkeyPressed = true;

        if (isOwned) {
            // Already owned 1-time item
            DrawRectangleRec(btnRect, Fade(Colors::Zinc950, alpha));
            DrawRectangleLinesEx(btnRect, 1.0f, Fade(Colors::Zinc700, alpha));
            int ownedW = MeasureText("OWNED", 11);
            DrawText("OWNED", static_cast<int>(btnRect.x + (btnW - static_cast<float>(ownedW)) * 0.5f), static_cast<int>(btnRect.y + 8.0f), 11, Fade(Colors::Zinc500, alpha));
        } else {
            bool canAfford = (scrapCount >= slot.item.cost);

            Color btnBg = canAfford ? (isBtnHovered ? Colors::Amber500 : Colors::Zinc800) : Colors::Zinc950;
            Color btnBorder = canAfford ? (isBtnHovered ? Colors::Amber300 : Colors::Amber500) : Colors::Zinc700;
            Color btnTextCol = canAfford ? (isBtnHovered ? Colors::Zinc950 : Colors::Amber300) : Colors::Zinc500;

            DrawRectangleRec(btnRect, Fade(btnBg, alpha));
            DrawRectangleLinesEx(btnRect, 1.0f, Fade(btnBorder, alpha));

            char btnLabel[32];
            if (slot.item.id == core::ItemId::Bubbles) {
                std::snprintf(btnLabel, sizeof(btnLabel), "[%d] %llu S", static_cast<int>(i + 1), slot.item.cost);
            } else {
                std::snprintf(btnLabel, sizeof(btnLabel), "[%d] %llu S", static_cast<int>(i + 1), slot.item.cost);
            }
            int bTextW = MeasureText(btnLabel, 11);
            DrawText(btnLabel, static_cast<int>(btnRect.x + (btnW - static_cast<float>(bTextW)) * 0.5f), static_cast<int>(btnRect.y + 8.0f), 11, Fade(btnTextCol, alpha));

            // Purchase trigger
            if (canAfford && ((isBtnHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) || hotkeyPressed)) {
                scrapCount -= slot.item.cost;
                if (slot.item.id == core::ItemId::Banana) {
                    playerInv.hasBanana = true;
                } else if (slot.item.id == core::ItemId::Radar) {
                    playerInv.hasRadar = true;
                } else if (slot.item.id == core::ItemId::Bubbles) {
                    playerInv.bubbleCharges += 1;
                }
                purchasedItem = true;
            }
        }

        curY += rowH + 4.0f;
    }

    // 5. Footer hint
    const char* hint = "HOTKEYS [1] - [4] TO BUY • ESC CLOSE";
    int hintW = MeasureText(hint, 9);
    DrawText(hint, static_cast<int>(cardX + (cardW - static_cast<float>(hintW)) * 0.5f), static_cast<int>(cardY + cardH - 16.0f), 9, Fade(Colors::Zinc500, alpha));

    return purchasedItem;
}

bool ShopMenu::drawInventoryDock(
    int screenW,
    int screenH,
    const core::PlayerInventory& playerInv,
    float alpha
) {
    (void)screenW;
    if (alpha <= 0.01f) return false;

    // Render compact dock in lower left corner
    const float slotW = 54.0f;
    const float slotH = 50.0f;
    const float gap = 6.0f;
    const float dockX = 20.0f;
    const float dockY = static_cast<float>(screenH) - 92.0f - slotH - 12.0f;

    bool bubblesClicked = false;
    Vector2 mousePos = GetMousePosition();

    auto& catalog = core::ItemCatalog::instance();
    const core::Item* bananaItem = catalog.getItem(core::ItemId::Banana);
    const core::Item* radarItem = catalog.getItem(core::ItemId::Radar);
    const core::Item* bubblesItem = catalog.getItem(core::ItemId::Bubbles);

    // Slot 1: Banana
    Rectangle r1 = { dockX, dockY, slotW, slotH };
    DrawRectangleRec(r1, Fade(Colors::Zinc950, alpha * 0.85f));
    DrawRectangleLinesEx(r1, 1.0f, Fade(playerInv.hasBanana ? Colors::Green500 : Colors::Zinc800, alpha));
    if (bananaItem && bananaItem->icon.id != 0) {
        Rectangle src = { 0.0f, 0.0f, static_cast<float>(bananaItem->icon.width), static_cast<float>(bananaItem->icon.height) };
        Rectangle dst = { r1.x + 13.0f, r1.y + 6.0f, 28.0f, 28.0f };
        DrawTexturePro(bananaItem->icon, src, dst, { 0, 0 }, 0.0f, Fade(WHITE, playerInv.hasBanana ? alpha : alpha * 0.25f));
    }
    const char* t1 = playerInv.hasBanana ? "+30% SPD" : "LOCKED";
    int t1W = MeasureText(t1, 8);
    DrawText(t1, static_cast<int>(r1.x + (slotW - static_cast<float>(t1W)) * 0.5f), static_cast<int>(r1.y + 36.0f), 8, Fade(playerInv.hasBanana ? Colors::Green400 : Colors::Zinc600, alpha));

    // Slot 2: Radar
    Rectangle r2 = { dockX + slotW + gap, dockY, slotW, slotH };
    DrawRectangleRec(r2, Fade(Colors::Zinc950, alpha * 0.85f));
    DrawRectangleLinesEx(r2, 1.0f, Fade(playerInv.hasRadar ? Colors::Amber400 : Colors::Zinc800, alpha));
    if (radarItem && radarItem->icon.id != 0) {
        Rectangle src = { 0.0f, 0.0f, static_cast<float>(radarItem->icon.width), static_cast<float>(radarItem->icon.height) };
        Rectangle dst = { r2.x + 13.0f, r2.y + 6.0f, 28.0f, 28.0f };
        DrawTexturePro(radarItem->icon, src, dst, { 0, 0 }, 0.0f, Fade(WHITE, playerInv.hasRadar ? alpha : alpha * 0.25f));
    }
    const char* t2 = playerInv.hasRadar ? "RADAR ON" : "LOCKED";
    int t2W = MeasureText(t2, 8);
    DrawText(t2, static_cast<int>(r2.x + (slotW - static_cast<float>(t2W)) * 0.5f), static_cast<int>(r2.y + 36.0f), 8, Fade(playerInv.hasRadar ? Colors::Amber300 : Colors::Zinc600, alpha));

    // Slot 3: Bubbles
    Rectangle r3 = { dockX + (slotW + gap) * 2.0f, dockY, slotW, slotH };
    bool isBubblesHovered = CheckCollisionPointRec(mousePos, r3);
    DrawRectangleRec(r3, Fade(isBubblesHovered && playerInv.bubbleCharges > 0 ? Colors::Zinc850 : Colors::Zinc950, alpha * 0.85f));
    Color bColor = (playerInv.bubbleCharges > 0) ? (isBubblesHovered ? Colors::Cyan300 : Colors::Purple400) : Colors::Zinc800;
    DrawRectangleLinesEx(r3, 1.0f, Fade(bColor, alpha));

    if (bubblesItem && bubblesItem->icon.id != 0) {
        Rectangle src = { 0.0f, 0.0f, static_cast<float>(bubblesItem->icon.width), static_cast<float>(bubblesItem->icon.height) };
        Rectangle dst = { r3.x + 13.0f, r3.y + 6.0f, 28.0f, 28.0f };
        DrawTexturePro(bubblesItem->icon, src, dst, { 0, 0 }, 0.0f, Fade(WHITE, (playerInv.bubbleCharges > 0) ? alpha : alpha * 0.25f));
    }

    if (playerInv.bubbleCharges > 0) {
        char chargeBuf[16];
        std::snprintf(chargeBuf, sizeof(chargeBuf), "[B] x%d", playerInv.bubbleCharges);
        int chW = MeasureText(chargeBuf, 9);
        DrawText(chargeBuf, static_cast<int>(r3.x + (slotW - static_cast<float>(chW)) * 0.5f), static_cast<int>(r3.y + 36.0f), 9, Fade(Colors::Cyan300, alpha));

        if (isBubblesHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            bubblesClicked = true;
        }
    } else {
        const char* t3 = "EMPTY";
        int t3W = MeasureText(t3, 8);
        DrawText(t3, static_cast<int>(r3.x + (slotW - static_cast<float>(t3W)) * 0.5f), static_cast<int>(r3.y + 36.0f), 8, Fade(Colors::Zinc600, alpha));
    }

    return bubblesClicked;
}

} // namespace minesweeper::ui
