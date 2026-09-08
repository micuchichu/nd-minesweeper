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

        bool isSold = slot.isPurchased;
        bool bagFull = !playerInv.hasFreeSlot();

        bool hotkeyPressed = false;
        if (i == 0 && (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1))) hotkeyPressed = true;
        if (i == 1 && (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2))) hotkeyPressed = true;
        if (i == 2 && (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3))) hotkeyPressed = true;
        if (i == 3 && (IsKeyPressed(KEY_FOUR) || IsKeyPressed(KEY_KP_4))) hotkeyPressed = true;

        if (isSold) {
            DrawRectangleRec(btnRect, Fade(Colors::Zinc950, alpha));
            DrawRectangleLinesEx(btnRect, 1.0f, Fade(Colors::Zinc700, alpha));
            int soldW = MeasureText("SOLD", 11);
            DrawText("SOLD", static_cast<int>(btnRect.x + (btnW - static_cast<float>(soldW)) * 0.5f), static_cast<int>(btnRect.y + 8.0f), 11, Fade(Colors::Zinc500, alpha));
        } else if (bagFull) {
            DrawRectangleRec(btnRect, Fade(Colors::Zinc950, alpha));
            DrawRectangleLinesEx(btnRect, 1.0f, Fade(Colors::Red700, alpha * 0.7f));
            int fullW = MeasureText("BAG FULL", 10);
            DrawText("BAG FULL", static_cast<int>(btnRect.x + (btnW - static_cast<float>(fullW)) * 0.5f), static_cast<int>(btnRect.y + 9.0f), 10, Fade(Colors::Red400, alpha));
        } else {
            bool canAfford = (scrapCount >= slot.item.cost);

            Color btnBg = canAfford ? (isBtnHovered ? Colors::Amber500 : Colors::Zinc800) : Colors::Zinc950;
            Color btnBorder = canAfford ? (isBtnHovered ? Colors::Amber300 : Colors::Amber500) : Colors::Zinc700;
            Color btnTextCol = canAfford ? (isBtnHovered ? Colors::Zinc950 : Colors::Amber300) : Colors::Zinc500;

            DrawRectangleRec(btnRect, Fade(btnBg, alpha));
            DrawRectangleLinesEx(btnRect, 1.0f, Fade(btnBorder, alpha));

            char btnLabel[32];
            std::snprintf(btnLabel, sizeof(btnLabel), "[%d] %llu S", static_cast<int>(i + 1), slot.item.cost);
            int bTextW = MeasureText(btnLabel, 11);
            DrawText(btnLabel, static_cast<int>(btnRect.x + (btnW - static_cast<float>(bTextW)) * 0.5f), static_cast<int>(btnRect.y + 8.0f), 11, Fade(btnTextCol, alpha));

            // Purchase trigger
            if (canAfford && ((isBtnHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) || hotkeyPressed)) {
                scrapCount -= slot.item.cost;
                playerInv.addItem(slot.item);
                slot.isPurchased = true;
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
    core::PlayerInventory& playerInv,
    float alpha
) {
    (void)screenW;
    if (alpha <= 0.01f) return false;

    pulseTimer += GetFrameTime();
    bool itemClicked = false;
    Vector2 mousePos = GetMousePosition();

    // Hotbar geometry: 5 slots
    const int numSlots = core::PlayerInventory::CAPACITY;
    const float slotW = 50.0f;
    const float slotH = 50.0f;
    const float gap = 6.0f;
    const float dockX = 20.0f;
    const float dockY = static_cast<float>(screenH) - 92.0f - slotH - 12.0f;

    // Draw active buff indicators above the hotbar if any buffs are running
    float buffY = dockY - 22.0f;
    if (playerInv.bananaBoostTimer > 0.0f) {
        char buffBuf[32];
        std::snprintf(buffBuf, sizeof(buffBuf), "BOOST +30%%: %.1fs", playerInv.bananaBoostTimer);
        int bw = MeasureText(buffBuf, 10);
        Rectangle bRect = { dockX, buffY, static_cast<float>(bw) + 12.0f, 18.0f };
        DrawRectangleRec(bRect, Fade(Colors::Zinc950, alpha * 0.9f));
        DrawRectangleLinesEx(bRect, 1.0f, Fade(Colors::Green400, alpha));
        DrawText(buffBuf, static_cast<int>(bRect.x + 6.0f), static_cast<int>(bRect.y + 4.0f), 10, Fade(Colors::Green400, alpha));
        buffY -= 22.0f;
    }
    if (playerInv.radarActiveTimer > 0.0f) {
        char buffBuf[32];
        std::snprintf(buffBuf, sizeof(buffBuf), "RADAR SCAN: %.1fs", playerInv.radarActiveTimer);
        int bw = MeasureText(buffBuf, 10);
        Rectangle bRect = { dockX, buffY, static_cast<float>(bw) + 12.0f, 18.0f };
        DrawRectangleRec(bRect, Fade(Colors::Zinc950, alpha * 0.9f));
        DrawRectangleLinesEx(bRect, 1.0f, Fade(Colors::Amber400, alpha));
        DrawText(buffBuf, static_cast<int>(bRect.x + 6.0f), static_cast<int>(bRect.y + 4.0f), 10, Fade(Colors::Amber300, alpha));
    }

    // Draw 5 hotbar slots
    for (int i = 0; i < numSlots; ++i) {
        Rectangle r = { dockX + i * (slotW + gap), dockY, slotW, slotH };
        bool isHovered = CheckCollisionPointRec(mousePos, r);
        bool isSelected = (playerInv.selectedSlot == i);

        // Click to select slot
        if (isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            playerInv.selectedSlot = i;
            itemClicked = true;
        }

        // Slot Background
        Color slotBg = isSelected
            ? Fade(Colors::Zinc850, alpha * 0.95f)
            : Fade(isHovered ? Colors::Zinc900 : Colors::Zinc950, alpha * 0.85f);
        DrawRectangleRec(r, slotBg);

        // Slot Border & Selection Highlight
        if (isSelected) {
            float pulse = 0.5f + 0.5f * std::sin(pulseTimer * 6.0f);
            Color borderCol = Fade(Colors::Amber400, alpha * (0.85f + 0.15f * pulse));
            DrawRectangleLinesEx(r, 2.0f, borderCol);

            // Tech brackets for selected slot
            float blen = 6.0f;
            DrawLineEx({ r.x, r.y }, { r.x + blen, r.y }, 2.5f, Colors::Amber300);
            DrawLineEx({ r.x, r.y }, { r.x, r.y + blen }, 2.5f, Colors::Amber300);
            DrawLineEx({ r.x + slotW, r.y }, { r.x + slotW - blen, r.y }, 2.5f, Colors::Amber300);
            DrawLineEx({ r.x + slotW, r.y }, { r.x + slotW, r.y + blen }, 2.5f, Colors::Amber300);
            DrawLineEx({ r.x, r.y + slotH }, { r.x + blen, r.y + slotH }, 2.5f, Colors::Amber300);
            DrawLineEx({ r.x, r.y + slotH }, { r.x, r.y + slotH - blen }, 2.5f, Colors::Amber300);
            DrawLineEx({ r.x + slotW, r.y + slotH }, { r.x + slotW - blen, r.y + slotH }, 2.5f, Colors::Amber300);
            DrawLineEx({ r.x + slotW, r.y + slotH }, { r.x + slotW, r.y + slotH - blen }, 2.5f, Colors::Amber300);
        } else {
            DrawRectangleLinesEx(r, 1.0f, Fade(isHovered ? Colors::Zinc600 : Colors::Zinc800, alpha));
        }

        // Hotkey number at top-left
        char numStr[4];
        std::snprintf(numStr, sizeof(numStr), "%d", i + 1);
        DrawText(numStr, static_cast<int>(r.x + 4.0f), static_cast<int>(r.y + 3.0f), 8, Fade(isSelected ? Colors::Amber400 : Colors::Zinc600, alpha));

        const auto& slot = playerInv.slots[i];
        if (slot.occupied && slot.item.icon.id != 0) {
            // Draw item icon
            const float iconSz = 26.0f;
            Rectangle src = { 0.0f, 0.0f, static_cast<float>(slot.item.icon.width), static_cast<float>(slot.item.icon.height) };
            Rectangle dst = { r.x + (slotW - iconSz) * 0.5f, r.y + 6.0f, iconSz, iconSz };
            DrawTexturePro(slot.item.icon, src, dst, { 0, 0 }, 0.0f, Fade(WHITE, alpha));

            // Tier accent bar under the icon
            Color tierCol = Colors::Green400;
            if (slot.item.tier == core::ItemTier::Tier2) tierCol = Colors::Amber400;
            else if (slot.item.tier == core::ItemTier::Tier3) tierCol = Colors::Purple400;
            DrawRectangleRec({ r.x + 6.0f, r.y + 34.0f, slotW - 12.0f, 1.5f }, Fade(tierCol, alpha * 0.7f));

            // Durability Bar (for bubbles or any continuous item)
            if (slot.item.id == core::ItemId::Bubbles && slot.maxDurability > 0.0f) {
                float pct = std::clamp(slot.durability / slot.maxDurability, 0.0f, 1.0f);
                float barW = slotW - 8.0f;
                float barH = 4.0f;
                float barX = r.x + 4.0f;
                float barY = r.y + slotH - 8.0f;
                Rectangle bgBar = { barX, barY, barW, barH };
                Rectangle fgBar = { barX, barY, barW * pct, barH };
                DrawRectangleRec(bgBar, Fade(Colors::Zinc900, alpha * 0.9f));
                Color durCol = (pct > 0.5f) ? Colors::Green400 : (pct > 0.25f ? Colors::Amber400 : Colors::Red500);
                DrawRectangleRec(fgBar, Fade(durCol, alpha));
                DrawRectangleLinesEx(bgBar, 1.0f, Fade(Colors::Zinc700, alpha * 0.8f));
            } else {
                // Short status label for consumables
                const char* lbl = isSelected ? "[E] USE" : "READY";
                int lw = MeasureText(lbl, 7);
                DrawText(lbl, static_cast<int>(r.x + (slotW - static_cast<float>(lw)) * 0.5f), static_cast<int>(r.y + slotH - 11.0f), 7, Fade(isSelected ? Colors::Amber300 : Colors::Zinc500, alpha));
            }

            // Tooltip on hover
            if (isHovered) {
                float tipW = 165.0f;
                float tipH = 38.0f;
                float tipX = std::clamp(r.x + (slotW - tipW) * 0.5f, 10.0f, static_cast<float>(screenW) - tipW - 10.0f);
                float tipY = r.y - tipH - 6.0f;
                Rectangle tipRect = { tipX, tipY, tipW, tipH };
                DrawRectangleRec(tipRect, Fade(Color{ 10, 12, 16, 250 }, alpha));
                DrawRectangleLinesEx(tipRect, 1.0f, Fade(tierCol, alpha));
                DrawText(slot.item.name.c_str(), static_cast<int>(tipX + 6.0f), static_cast<int>(tipY + 5.0f), 10, Fade(tierCol, alpha));
                DrawText(slot.item.description.c_str(), static_cast<int>(tipX + 6.0f), static_cast<int>(tipY + 20.0f), 8, Fade(Colors::Zinc300, alpha));
            }
        } else {
            // Empty slot label
            const char* emp = "EMPTY";
            int ew = MeasureText(emp, 8);
            DrawText(emp, static_cast<int>(r.x + (slotW - static_cast<float>(ew)) * 0.5f), static_cast<int>(r.y + 20.0f), 8, Fade(Colors::Zinc700, alpha));
        }
    }

    return itemClicked;
}

} // namespace minesweeper::ui
