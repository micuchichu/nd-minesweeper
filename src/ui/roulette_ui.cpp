#include "roulette_ui.hpp"
#include "theme.hpp"
#include <cmath>
#include <cstdio>
#include <algorithm>

namespace minesweeper::ui {

bool RouletteUI::draw(
    core::RouletteShip& shop,
    uint64_t& scrapCount,
    float alpha,
    const render::CameraController& camera,
    int screenW,
    int screenH
) {
    if (alpha <= 0.01f) {
        lastCardRect = { 0.0f, 0.0f, 0.0f, 0.0f };
        requestClose = false;
        isEditingBet = false;
        return false;
    }

    pulseTimer += GetFrameTime();
    Vector2 mousePos = GetMousePosition();
    bool mousePressed = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    bool actionTaken = false;

    Vector2 shopScreen = camera.getWorldToScreen(shop.position);

    const float cardW = 480.0f;
    const float cardH = 340.0f;

    // Position anchored near ship, clamping within screen bounds
    float cardX = std::clamp(shopScreen.x - cardW * 0.5f, 16.0f, static_cast<float>(screenW) - cardW - 16.0f);
    float cardY = shopScreen.y - cardH - 30.0f;
    if (cardY < 75.0f) {
        cardY = shopScreen.y + 40.0f;
    }
    cardY = std::clamp(cardY, 75.0f, static_cast<float>(screenH) - cardH - 16.0f);

    // 1. Holographic connector line
    Vector2 anchorCard = (cardY > shopScreen.y)
        ? Vector2{ cardX + cardW * 0.5f, cardY }
        : Vector2{ cardX + cardW * 0.5f, cardY + cardH };

    Color lineCol = Fade(Color{ 168, 85, 247, 255 }, alpha * 0.45f);
    DrawLineEx(shopScreen, anchorCard, 1.5f, lineCol);
    DrawCircleV(shopScreen, 3.5f, Fade(Color{ 216, 180, 254, 255 }, alpha * 0.7f));
    DrawCircleLines(static_cast<int>(shopScreen.x), static_cast<int>(shopScreen.y), 6.0f, lineCol);

    // 2. Card Background Frame
    Rectangle cardRect = { cardX, cardY, cardW, cardH };
    lastCardRect = cardRect;
    DrawRectangleRec(cardRect, Fade(Color{ 14, 15, 20, 248 }, alpha));
    DrawRectangleLinesEx(cardRect, 1.5f, Fade(Color{ 147, 51, 234, 255 }, alpha * 0.8f));

    // Corner decorative accents
    const float cLen = 8.0f;
    Color cornerCol = Fade(Color{ 216, 180, 254, 255 }, alpha * 0.9f);
    DrawLineEx({ cardX, cardY }, { cardX + cLen, cardY }, 2.0f, cornerCol);
    DrawLineEx({ cardX, cardY }, { cardX, cardY + cLen }, 2.0f, cornerCol);
    DrawLineEx({ cardX + cardW, cardY }, { cardX + cardW - cLen, cardY }, 2.0f, cornerCol);
    DrawLineEx({ cardX + cardW, cardY }, { cardX + cardW, cardY + cLen }, 2.0f, cornerCol);
    DrawLineEx({ cardX, cardY + cardH }, { cardX + cLen, cardY + cardH }, 2.0f, cornerCol);
    DrawLineEx({ cardX, cardY + cardH }, { cardX, cardY + cardH - cLen }, 2.0f, cornerCol);
    DrawLineEx({ cardX + cardW, cardY + cardH }, { cardX + cardW - cLen, cardY + cardH }, 2.0f, cornerCol);
    DrawLineEx({ cardX + cardW, cardY + cardH }, { cardX + cardW, cardY + cardH - cLen }, 2.0f, cornerCol);

    float curY = cardY + 8.0f;

    // 3. Header: Title, Scrap, and Close Button
    {
        float hLeft = cardX + 12.0f;
        DrawText("CASINO TERMINAL", static_cast<int>(hLeft), static_cast<int>(curY), 13, Fade(Color{ 216, 180, 254, 255 }, alpha));
        DrawText("EUROPEAN ROULETTE TABLE", static_cast<int>(hLeft), static_cast<int>(curY + 14), 10, Fade(Colors::Zinc400, alpha));

        // Scrap balance
        std::string scrapStr = std::to_string(scrapCount) + " SCRAP";
        int scrapW = MeasureText(scrapStr.c_str(), 12);
        float sX = cardX + cardW - scrapW - 38.0f;
        DrawText(scrapStr.c_str(), static_cast<int>(sX), static_cast<int>(curY + 4), 12, Fade(Colors::Amber400, alpha));

        // Close button [X]
        Rectangle closeBtn = { cardX + cardW - 26.0f, curY + 2.0f, 18.0f, 18.0f };
        bool overClose = CheckCollisionPointRec(mousePos, closeBtn);
        DrawRectangleRec(closeBtn, Fade(overClose ? Colors::Zinc700 : Colors::Zinc800, alpha));
        DrawRectangleLinesEx(closeBtn, 1.0f, Fade(overClose ? Colors::Zinc400 : Colors::Zinc600, alpha));
        DrawText("X", static_cast<int>(closeBtn.x + 5), static_cast<int>(closeBtn.y + 3), 11, Fade(overClose ? Colors::Zinc200 : Colors::Zinc400, alpha));
        if (overClose && mousePressed) {
            requestClose = true;
            actionTaken = true;
        }

        curY += 32.0f;
    }

    // Divider
    DrawLineEx({ cardX + 8.0f, curY }, { cardX + cardW - 8.0f, curY }, 1.0f, Fade(Colors::Zinc800, alpha));
    curY += 6.0f;

    bool isSpinning = (shop.roulette.spinState == core::RouletteSpinState::Spinning);

    // 4. Bet Amount Selector / Custom Amount Input
    {
        float labelX = cardX + 12.0f;
        DrawText("BET AMOUNT:", static_cast<int>(labelX), static_cast<int>(curY + 5), 11, Fade(Colors::Zinc400, alpha));
        float boxX = labelX + 80.0f;

        // Editable Bet Amount Box
        Rectangle inputRect = { boxX, curY + 1.0f, 65.0f, 22.0f };
        bool overInput = CheckCollisionPointRec(mousePos, inputRect) && !isSpinning;
        if (overInput && mousePressed) {
            isEditingBet = true;
            betInputBuffer = std::to_string(customBetAmount);
        } else if (mousePressed && !overInput) {
            isEditingBet = false;
        }

        if (isEditingBet) {
            int key = GetCharPressed();
            while (key > 0) {
                if (key >= '0' && key <= '9') {
                    if (betInputBuffer == "0") betInputBuffer.clear();
                    if (betInputBuffer.length() < 7) {
                        betInputBuffer.push_back(static_cast<char>(key));
                    }
                }
                key = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE)) {
                if (!betInputBuffer.empty()) {
                    betInputBuffer.pop_back();
                }
            }
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
                isEditingBet = false;
            }

            if (!betInputBuffer.empty()) {
                try {
                    uint64_t parsed = std::stoull(betInputBuffer);
                    customBetAmount = std::clamp(parsed, static_cast<uint64_t>(1), std::max(static_cast<uint64_t>(1), scrapCount));
                    if (shop.roulette.activeBet.type != core::RouletteBetType::None) {
                        shop.roulette.activeBet.amount = customBetAmount;
                    }
                } catch (...) {}
            }
        }

        DrawRectangleRec(inputRect, Fade(isEditingBet ? Color{ 30, 27, 75, 255 } : (overInput ? Colors::Zinc800 : Colors::Zinc900), alpha));
        DrawRectangleLinesEx(inputRect, 1.0f, Fade(isEditingBet ? Color{ 192, 132, 252, 255 } : (overInput ? Colors::Zinc500 : Colors::Zinc700), alpha));

        std::string dispStr = isEditingBet ? (betInputBuffer + ((static_cast<int>(pulseTimer * 3.0f) % 2 == 0) ? "|" : "")) : std::to_string(customBetAmount);
        int dW = MeasureText(dispStr.c_str(), 11);
        DrawText(dispStr.c_str(), static_cast<int>(inputRect.x + (inputRect.width - dW) * 0.5f), static_cast<int>(inputRect.y + 5), 11, Fade(isEditingBet ? WHITE : Colors::Amber300, alpha));

        // Quick Stepper Buttons: [-10], [-1], [+1], [+10], [+50], [1/2], [2x], [MAX]
        float btnX = inputRect.x + inputRect.width + 8.0f;
        auto drawStepper = [&](const char* lbl, int64_t delta, float mult, bool isMax) {
            float bW = isMax ? 36.0f : 28.0f;
            Rectangle sRect = { btnX, curY + 1.0f, bW, 22.0f };
            bool sHover = CheckCollisionPointRec(mousePos, sRect) && !isSpinning;
            DrawRectangleRec(sRect, Fade(sHover ? Colors::Zinc700 : Colors::Zinc850, alpha));
            DrawRectangleLinesEx(sRect, 1.0f, Fade(sHover ? Colors::Zinc500 : Colors::Zinc700, alpha));
            int lW = MeasureText(lbl, 10);
            DrawText(lbl, static_cast<int>(sRect.x + (bW - lW) * 0.5f), static_cast<int>(sRect.y + 5), 10, Fade(sHover ? WHITE : Colors::Zinc300, alpha));

            if (sHover && mousePressed) {
                if (isMax) {
                    customBetAmount = scrapCount;
                } else if (mult > 0.0f) {
                    customBetAmount = static_cast<uint64_t>(std::clamp(static_cast<double>(customBetAmount) * mult, 1.0, static_cast<double>(scrapCount)));
                } else {
                    int64_t nextVal = static_cast<int64_t>(customBetAmount) + delta;
                    customBetAmount = static_cast<uint64_t>(std::clamp(nextVal, static_cast<int64_t>(1), static_cast<int64_t>(scrapCount)));
                }
                betInputBuffer = std::to_string(customBetAmount);
                if (shop.roulette.activeBet.type != core::RouletteBetType::None) {
                    shop.roulette.activeBet.amount = customBetAmount;
                }
                actionTaken = true;
            }
            btnX += bW + 4.0f;
        };

        drawStepper("-10", -10, 0.0f, false);
        drawStepper("-1",  -1,  0.0f, false);
        drawStepper("+1",  +1,  0.0f, false);
        drawStepper("+10", +10, 0.0f, false);
        drawStepper("+50", +50, 0.0f, false);
        drawStepper("1/2",   0, 0.5f, false);
        drawStepper("2x",    0, 2.0f, false);
        drawStepper("MAX",   0, 0.0f, true);

        curY += 28.0f;
    }

    // Divider
    DrawLineEx({ cardX + 8.0f, curY }, { cardX + cardW - 8.0f, curY }, 1.0f, Fade(Colors::Zinc800, alpha));
    curY += 6.0f;

    // Helper lambda for bet table buttons
    auto drawBetCell = [&](
        Rectangle rect,
        const char* title,
        core::RouletteBetType type,
        int number,
        Color idleBg,
        Color activeBorder,
        int fontSize
    ) {
        bool isActive = (shop.roulette.activeBet.type == type && (type != core::RouletteBetType::SingleNumber || shop.roulette.activeBet.selectedNumber == number));
        bool isHover = CheckCollisionPointRec(mousePos, rect) && !isSpinning;

        Color bg = idleBg;
        if (isActive) {
            bg = Color{ static_cast<unsigned char>(std::min(255, idleBg.r + 45)),
                        static_cast<unsigned char>(std::min(255, idleBg.g + 45)),
                        static_cast<unsigned char>(std::min(255, idleBg.b + 45)), 255 };
        } else if (isHover) {
            bg = Color{ static_cast<unsigned char>(std::min(255, idleBg.r + 25)),
                        static_cast<unsigned char>(std::min(255, idleBg.g + 25)),
                        static_cast<unsigned char>(std::min(255, idleBg.b + 25)), 255 };
        }

        Color border = isActive ? activeBorder : (isHover ? Colors::Zinc500 : Colors::Zinc700);

        DrawRectangleRec(rect, Fade(bg, alpha));
        DrawRectangleLinesEx(rect, isActive ? 1.5f : 1.0f, Fade(border, alpha));

        int tW = MeasureText(title, fontSize);
        float textX = rect.x + (rect.width - tW) * 0.5f;
        float textY = rect.y + (rect.height - fontSize) * 0.5f;
        DrawText(title, static_cast<int>(textX), static_cast<int>(textY), fontSize, Fade(WHITE, alpha));

        // If this bet is currently active, draw chip badge
        if (isActive && shop.roulette.activeBet.amount > 0) {
            std::string bStr = std::to_string(shop.roulette.activeBet.amount);
            int bW = MeasureText(bStr.c_str(), 9);
            Rectangle badge = { rect.x + rect.width - bW - 6.0f, rect.y - 3.0f, static_cast<float>(bW + 4), 11.0f };
            DrawRectangleRec(badge, Fade(Colors::Amber500, alpha));
            DrawText(bStr.c_str(), static_cast<int>(badge.x + 2), static_cast<int>(badge.y + 1), 9, Colors::Zinc950);
        }

        if (isHover && mousePressed) {
            if (isActive) {
                // Clicking same bet increments by customBetAmount
                uint64_t newAmount = shop.roulette.activeBet.amount + customBetAmount;
                shop.roulette.activeBet.amount = std::min(newAmount, scrapCount);
            } else {
                shop.roulette.activeBet.type = type;
                shop.roulette.activeBet.selectedNumber = number;
                shop.roulette.activeBet.amount = std::min(customBetAmount, scrapCount);
            }
            actionTaken = true;
        }
    };

    // 5. Classic European Roulette Table Grid
    {
        const float gridStartX = cardX + 12.0f;
        const float zeroW = 32.0f;
        const float cellH = 20.0f;
        const float gridH = cellH * 3.0f + 2.0f * 2.0f; // 64px for 3 number rows
        const float numColStartX = gridStartX + zeroW + 3.0f;
        const float totalColsW = (cardW - 24.0f) - zeroW - 3.0f; // ~421px
        const float colW = (totalColsW - 11.0f * 2.0f) / 12.0f;  // ~33px per column

        // A. Green 0 Button (spans all 3 rows on the left)
        Rectangle zeroRect = { gridStartX, curY, zeroW, gridH };
        drawBetCell(
            zeroRect,
            "0",
            core::RouletteBetType::SingleNumber,
            0,
            Color{ 20, 83, 45, 255 }, // Dark emerald
            Color{ 74, 222, 128, 255 }, // Bright green border
            12
        );

        // B. 36 Numbers Grid (3 rows x 12 columns)
        // Row 0 (top):    3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36
        // Row 1 (middle): 2, 5, 8, 11, 14, 17, 20, 23, 26, 29, 32, 35
        // Row 2 (bottom): 1, 4, 7, 10, 13, 16, 19, 22, 25, 28, 31, 34
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 12; ++c) {
                int num = (3 - r) + c * 3;
                float cellX = numColStartX + c * (colW + 2.0f);
                float cellY = curY + r * (cellH + 2.0f);
                Rectangle nRect = { cellX, cellY, colW, cellH };

                bool isRed = core::isRouletteRed(num);
                Color cellBg = isRed ? Color{ 127, 29, 29, 255 } : Color{ 22, 22, 28, 255 };
                Color activeCol = isRed ? Color{ 248, 113, 113, 255 } : Color{ 216, 180, 254, 255 };

                std::string numStr = std::to_string(num);
                drawBetCell(
                    nRect,
                    numStr.c_str(),
                    core::RouletteBetType::SingleNumber,
                    num,
                    cellBg,
                    activeCol,
                    10
                );
            }
        }

        curY += gridH + 3.0f;

        // C. Dozens Row (1st 12, 2nd 12, 3rd 12) directly under the 12 number columns
        {
            const float dozW = (totalColsW - 2.0f * 3.0f) / 3.0f;
            Rectangle d1 = { numColStartX, curY, dozW, 18.0f };
            Rectangle d2 = { numColStartX + dozW + 3.0f, curY, dozW, 18.0f };
            Rectangle d3 = { numColStartX + (dozW + 3.0f) * 2.0f, curY, dozW, 18.0f };

            drawBetCell(d1, "1st 12 (1-12) [3x]", core::RouletteBetType::Dozen1, 0, Colors::Zinc900, Colors::Amber400, 10);
            drawBetCell(d2, "2nd 12 (13-24) [3x]", core::RouletteBetType::Dozen2, 0, Colors::Zinc900, Colors::Amber400, 10);
            drawBetCell(d3, "3rd 12 (25-36) [3x]", core::RouletteBetType::Dozen3, 0, Colors::Zinc900, Colors::Amber400, 10);

            curY += 21.0f;
        }

        // D. Outside Bets Row: 1-18, EVEN, RED, BLACK, ODD, 19-36
        {
            const float outW = (totalColsW - 5.0f * 3.0f) / 6.0f;
            float oX = numColStartX;

            Rectangle rLow = { oX, curY, outW, 20.0f };
            drawBetCell(rLow, "1-18", core::RouletteBetType::Low, 0, Colors::Zinc900, Colors::Amber400, 10);
            oX += outW + 3.0f;

            Rectangle rEven = { oX, curY, outW, 20.0f };
            drawBetCell(rEven, "EVEN", core::RouletteBetType::Even, 0, Colors::Zinc900, Colors::Amber400, 10);
            oX += outW + 3.0f;

            Rectangle rRed = { oX, curY, outW, 20.0f };
            drawBetCell(rRed, "RED", core::RouletteBetType::Red, 0, Color{ 127, 29, 29, 255 }, Color{ 239, 68, 68, 255 }, 10);
            oX += outW + 3.0f;

            Rectangle rBlack = { oX, curY, outW, 20.0f };
            drawBetCell(rBlack, "BLACK", core::RouletteBetType::Black, 0, Color{ 18, 18, 24, 255 }, Color{ 216, 180, 254, 255 }, 10);
            oX += outW + 3.0f;

            Rectangle rOdd = { oX, curY, outW, 20.0f };
            drawBetCell(rOdd, "ODD", core::RouletteBetType::Odd, 0, Colors::Zinc900, Colors::Amber400, 10);
            oX += outW + 3.0f;

            Rectangle rHigh = { oX, curY, outW, 20.0f };
            drawBetCell(rHigh, "19-36", core::RouletteBetType::High, 0, Colors::Zinc900, Colors::Amber400, 10);

            curY += 25.0f;
        }
    }

    // 6. Active Bet Summary & Payout Calculation Box
    {
        Rectangle sumBox = { cardX + 12.0f, curY, cardW - 24.0f, 22.0f };
        DrawRectangleRec(sumBox, Fade(Color{ 20, 20, 28, 255 }, alpha));
        DrawRectangleLinesEx(sumBox, 1.0f, Fade(Colors::Zinc800, alpha));

        std::string summaryText;
        if (shop.roulette.activeBet.type == core::RouletteBetType::None || shop.roulette.activeBet.amount == 0) {
            summaryText = "SELECT A NUMBER OR OUTSIDE BET ON THE TABLE";
            DrawText(summaryText.c_str(), static_cast<int>(sumBox.x + 10), static_cast<int>(sumBox.y + 5), 10, Fade(Colors::Zinc500, alpha));
        } else {
            std::string typeName;
            uint64_t mult = 2;
            switch (shop.roulette.activeBet.type) {
                case core::RouletteBetType::Red: typeName = "RED (2x)"; mult = 2; break;
                case core::RouletteBetType::Black: typeName = "BLACK (2x)"; mult = 2; break;
                case core::RouletteBetType::Even: typeName = "EVEN (2x)"; mult = 2; break;
                case core::RouletteBetType::Odd: typeName = "ODD (2x)"; mult = 2; break;
                case core::RouletteBetType::Low: typeName = "1-18 (2x)"; mult = 2; break;
                case core::RouletteBetType::High: typeName = "19-36 (2x)"; mult = 2; break;
                case core::RouletteBetType::Dozen1: typeName = "1st 12 (3x)"; mult = 3; break;
                case core::RouletteBetType::Dozen2: typeName = "2nd 12 (3x)"; mult = 3; break;
                case core::RouletteBetType::Dozen3: typeName = "3rd 12 (3x)"; mult = 3; break;
                case core::RouletteBetType::GreenZero: typeName = "GREEN 0 (36x)"; mult = 36; break;
                case core::RouletteBetType::SingleNumber:
                    typeName = "NUMBER #" + std::to_string(shop.roulette.activeBet.selectedNumber) + " (36x)";
                    mult = 36;
                    break;
                default: break;
            }
            summaryText = "BET: " + std::to_string(shop.roulette.activeBet.amount) + " on " + typeName +
                          "  |  POTENTIAL WIN: " + std::to_string(shop.roulette.activeBet.amount * mult) + " SCRAP";
            DrawText(summaryText.c_str(), static_cast<int>(sumBox.x + 10), static_cast<int>(sumBox.y + 5), 10, Fade(Colors::Amber300, alpha));
        }

        curY += 26.0f;
    }

    // 7. Spin & Clear Controls
    {
        float btnAreaW = cardW - 24.0f;
        Rectangle clearBtn = { cardX + 12.0f, curY, 70.0f, 36.0f };
        bool canClear = (shop.roulette.activeBet.type != core::RouletteBetType::None && !isSpinning);
        bool clearHover = CheckCollisionPointRec(mousePos, clearBtn) && canClear;
        DrawRectangleRec(clearBtn, Fade(clearHover ? Colors::Zinc700 : Colors::Zinc800, alpha));
        DrawRectangleLinesEx(clearBtn, 1.0f, Fade(clearHover ? Colors::Zinc500 : Colors::Zinc700, alpha));
        DrawText("CLEAR", static_cast<int>(clearBtn.x + 16), static_cast<int>(clearBtn.y + 12), 11, Fade(canClear ? Colors::Zinc200 : Colors::Zinc600, alpha));
        if (clearHover && mousePressed) {
            shop.roulette.activeBet = {};
            actionTaken = true;
        }

        // Big SPIN WHEEL Button
        Rectangle spinBtn = { cardX + 12.0f + 76.0f, curY, btnAreaW - 76.0f, 36.0f };
        bool canSpin = (!isSpinning && shop.roulette.activeBet.amount > 0 && scrapCount >= shop.roulette.activeBet.amount);
        bool spinHover = CheckCollisionPointRec(mousePos, spinBtn) && canSpin;

        Color spinBg;
        Color spinBorder;
        std::string spinLabel;

        if (isSpinning) {
            int curPocket = core::getPocketUnderPointer(shop.angle);
            int curNum = core::getNumberForPocketIndex(curPocket);
            spinBg = Color{ 88, 28, 135, 255 };
            spinBorder = Color{ 192, 132, 252, 255 };
            spinLabel = "SPINNING... [ " + std::to_string(curNum) + " ]";
        } else if (shop.roulette.activeBet.amount == 0) {
            spinBg = Colors::Zinc800;
            spinBorder = Colors::Zinc700;
            spinLabel = "PLACE A BET TO SPIN";
        } else if (scrapCount < shop.roulette.activeBet.amount) {
            spinBg = Color{ 127, 29, 29, 255 };
            spinBorder = Color{ 239, 68, 68, 255 };
            spinLabel = "INSUFFICIENT SCRAP";
        } else {
            // Active ready state
            float pulse = (std::sin(pulseTimer * 5.0f) + 1.0f) * 0.5f;
            spinBg = spinHover ? Color{ 126, 34, 206, 255 } : Color{ 107, 33, 168, 255 };
            float t = pulse * 0.4f;
            spinBorder = Color{
                static_cast<unsigned char>(192 + (250 - 192) * t),
                static_cast<unsigned char>(132 + (204 - 132) * t),
                static_cast<unsigned char>(252 + (21 - 252) * t),
                255
            };
            spinLabel = "SPIN WHEEL (" + std::to_string(shop.roulette.activeBet.amount) + " SCRAP)";
        }

        DrawRectangleRec(spinBtn, Fade(spinBg, alpha));
        DrawRectangleLinesEx(spinBtn, 1.5f, Fade(spinBorder, alpha));
        int spW = MeasureText(spinLabel.c_str(), 12);
        DrawText(spinLabel.c_str(), static_cast<int>(spinBtn.x + (spinBtn.width - spW) * 0.5f), static_cast<int>(spinBtn.y + 11), 12, Fade(WHITE, alpha));

        if (spinHover && mousePressed) {
            // Deduct bet amount immediately
            scrapCount -= shop.roulette.activeBet.amount;
            shop.startSpin(-1, 5.0f);
            popup.active = false;
            actionTaken = true;
        }

        curY += 40.0f;
    }

    // 8. Outcome Banner
    {
        Rectangle bannerRect = { cardX + 12.0f, curY, cardW - 24.0f, 22.0f };
        if (isSpinning) {
            DrawRectangleRec(bannerRect, Fade(Color{ 30, 27, 75, 255 }, alpha));
            DrawRectangleLinesEx(bannerRect, 1.0f, Fade(Color{ 147, 51, 234, 255 }, alpha));
            DrawText("RNG SECTOR LOCKED: WHEEL IN MOTION", static_cast<int>(bannerRect.x + 80), static_cast<int>(bannerRect.y + 5), 10, Fade(Color{ 216, 180, 254, 255 }, alpha));
        } else if (!shop.roulette.resultMessage.empty()) {
            Color bBg = shop.roulette.lastWon ? Color{ 20, 83, 45, 255 } : Color{ 39, 39, 42, 255 };
            Color bBorder = shop.roulette.lastWon ? Color{ 74, 222, 128, 255 } : Colors::Zinc600;
            Color bTxt = shop.roulette.lastWon ? Color{ 187, 247, 208, 255 } : Colors::Zinc300;

            DrawRectangleRec(bannerRect, Fade(bBg, alpha));
            DrawRectangleLinesEx(bannerRect, 1.0f, Fade(bBorder, alpha));
            int msgW = MeasureText(shop.roulette.resultMessage.c_str(), 10);
            DrawText(shop.roulette.resultMessage.c_str(), static_cast<int>(bannerRect.x + (bannerRect.width - msgW) * 0.5f), static_cast<int>(bannerRect.y + 5), 10, Fade(bTxt, alpha));
        } else {
            DrawRectangleRec(bannerRect, Fade(Colors::Zinc950, alpha * 0.6f));
            DrawRectangleLinesEx(bannerRect, 1.0f, Fade(Colors::Zinc800, alpha));
            const char* tip = "[L-CLICK] Bet on Table  |  Type Amount  |  [ESC] Exit";
            int tipW = MeasureText(tip, 10);
            DrawText(tip, static_cast<int>(bannerRect.x + (bannerRect.width - tipW) * 0.5f), static_cast<int>(bannerRect.y + 5), 10, Fade(Colors::Zinc500, alpha));
        }
    }

    return actionTaken;
}

void RouletteUI::triggerPopup(int number, bool won, uint64_t payout, uint64_t betAmount) {
    popup.active = true;
    popup.timer = 0.0f;
    popup.duration = 3.5f;
    popup.winningNumber = number;
    popup.won = won;
    popup.payout = payout;
    popup.betAmount = betAmount;
}

void RouletteUI::updatePopup(float dt) {
    if (!popup.active) return;
    popup.timer += dt;
    if (popup.timer >= popup.duration) {
        popup.active = false;
        popup.timer = 0.0f;
    }
}

void RouletteUI::drawPopup(
    const render::CameraController& camera,
    int screenW,
    int screenH,
    Vector2 shipPos,
    float cardAlpha
) {
    if (!popup.active) return;

    float progress = std::clamp(popup.timer / popup.duration, 0.0f, 1.0f);
    float floatY = progress * 14.0f;

    // Fading popup: full opacity for first 70% of duration, then smooth fade-out over final 30%
    float alpha = 1.0f;
    if (progress > 0.70f) {
        alpha = (1.0f - progress) / 0.30f;
    }
    alpha = std::clamp(alpha, 0.0f, 1.0f);

    if (alpha <= 0.01f) return;

    const float popW = 380.0f;
    const float popH = 76.0f;

    Vector2 shipScreen = camera.getWorldToScreen(shipPos);
    float targetX = 0.0f;
    float targetY = 0.0f;

    if (cardAlpha > 0.2f && lastCardRect.width > 0.0f) {
        // Table card is open: anchor smoothly above the table card
        targetX = lastCardRect.x + (lastCardRect.width - popW) * 0.5f;
        if (lastCardRect.y - popH - 14.0f >= 60.0f) {
            targetY = lastCardRect.y - popH - 14.0f - floatY;
        } else {
            targetY = lastCardRect.y + lastCardRect.height + 14.0f + floatY;
        }
    } else {
        // Table card is closed: anchor directly above the ship in screen space
        targetX = shipScreen.x - popW * 0.5f;
        targetY = shipScreen.y - 75.0f - popH - floatY;
    }

    float px = std::clamp(targetX, 16.0f, static_cast<float>(screenW) - popW - 16.0f);
    float py = std::clamp(targetY, 16.0f, static_cast<float>(screenH) - popH - 16.0f);
    Rectangle popRect = { px, py, popW, popH };

    // 1. Soft drop shadow
    Rectangle shadowRect = { px + 4.0f, py + 5.0f, popW, popH };
    DrawRectangleRounded(shadowRect, 0.25f, 6, Fade(BLACK, alpha * 0.70f));

    // 2. Translucent Glass Panel Background
    Color bg = popup.won ? Color{ 22, 16, 8, 250 } : Color{ 26, 12, 12, 250 };
    DrawRectangleRounded(popRect, 0.25f, 6, Fade(bg, alpha));

    // 3. Glowing Border
    float pulse = (std::sin(popup.timer * 8.0f) + 1.0f) * 0.5f;
    Color borderCol = popup.won
        ? Color{
            static_cast<unsigned char>(251 + (255 - 251) * pulse),
            static_cast<unsigned char>(191 + (225 - 191) * pulse),
            static_cast<unsigned char>(36 + (100 - 36) * pulse),
            255
          }
        : Color{ 239, 68, 68, 255 };

    DrawRectangleRoundedLines(popRect, 0.25f, 6, Fade(borderCol, alpha * 0.95f));

    // Corner decorative accents
    const float cLen = 8.0f;
    Color cornerCol = Fade(popup.won ? Color{ 254, 240, 138, 255 } : Color{ 254, 202, 202, 255 }, alpha);
    DrawLineEx({ px, py }, { px + cLen, py }, 2.0f, cornerCol);
    DrawLineEx({ px, py }, { px, py + cLen }, 2.0f, cornerCol);
    DrawLineEx({ px + popW, py }, { px + popW - cLen, py }, 2.0f, cornerCol);
    DrawLineEx({ px + popW, py }, { px + popW, py + cLen }, 2.0f, cornerCol);
    DrawLineEx({ px, py + popH }, { px + cLen, py + popH }, 2.0f, cornerCol);
    DrawLineEx({ px, py + popH }, { px, py + popH - cLen }, 2.0f, cornerCol);
    DrawLineEx({ px + popW, py + popH }, { px + popW - cLen, py + popH }, 2.0f, cornerCol);
    DrawLineEx({ px + popW, py + popH }, { px + popW, py + popH - cLen }, 2.0f, cornerCol);

    // 4. Number Pocket Badge (Left side)
    float badgeSize = 56.0f;
    float badgeX = px + 12.0f;
    float badgeY = py + (popH - badgeSize) * 0.5f;
    Rectangle badgeRect = { badgeX, badgeY, badgeSize, badgeSize };

    Color pBg;
    Color pBorder;
    Color pTextCol;
    const char* pColorName = "";

    if (popup.winningNumber == 0) {
        pBg = Color{ 20, 83, 45, 255 };       // Dark Emerald
        pBorder = Color{ 74, 222, 128, 255 }; // Bright Green
        pTextCol = Color{ 240, 253, 244, 255 };
        pColorName = "GREEN";
    } else if (core::isRouletteRed(popup.winningNumber)) {
        pBg = Color{ 153, 27, 27, 255 };      // Dark Crimson
        pBorder = Color{ 248, 113, 113, 255 };// Bright Red
        pTextCol = WHITE;
        pColorName = "RED";
    } else {
        pBg = Color{ 24, 24, 32, 255 };       // Dark Slate
        pBorder = Color{ 192, 132, 252, 255 };// Violet
        pTextCol = Color{ 245, 245, 250, 255 };
        pColorName = "BLACK";
    }

    DrawRectangleRounded(badgeRect, 0.28f, 6, Fade(pBg, alpha));
    DrawRectangleRoundedLines(badgeRect, 0.28f, 6, Fade(pBorder, alpha));

    // Big bold winning number
    std::string numStr = std::to_string(popup.winningNumber);
    int nSize = (numStr.length() > 1) ? 22 : 24;
    int nW = MeasureText(numStr.c_str(), nSize);
    DrawText(numStr.c_str(), static_cast<int>(badgeX + (badgeSize - nW) * 0.5f), static_cast<int>(badgeY + 8), nSize, Fade(pTextCol, alpha));

    // Color label under the number
    int clW = MeasureText(pColorName, 9);
    DrawText(pColorName, static_cast<int>(badgeX + (badgeSize - clW) * 0.5f), static_cast<int>(badgeY + badgeSize - 15), 9, Fade(pBorder, alpha));

    // 5. Right Content Area: Result Title & Payout Display
    float textX = badgeX + badgeSize + 16.0f;

    if (popup.won) {
        // Winner Title
        bool isJackpot = (popup.betAmount > 0 && popup.payout >= popup.betAmount * 36);
        const char* title = isJackpot ? "* * * JACKPOT WINNER (36x) * * *" : "* WINNER! *";
        Color titleCol = isJackpot ? Color{ 250, 204, 21, 255 } : Color{ 253, 224, 71, 255 };
        DrawText(title, static_cast<int>(textX), static_cast<int>(py + 10), 13, Fade(titleCol, alpha));

        // Payout Amount
        std::string payStr = "+" + std::to_string(popup.payout) + " SCRAP";
        DrawText(payStr.c_str(), static_cast<int>(textX), static_cast<int>(py + 27), 21, Fade(Colors::Amber400, alpha));

        // Details Subtitle
        std::string sub = "LANDED ON NUMBER " + numStr + " [" + pColorName + "]";
        DrawText(sub.c_str(), static_cast<int>(textX), static_cast<int>(py + 52), 10, Fade(Colors::Zinc400, alpha));
    } else {
        // Loss Title
        DrawText("NO WIN THIS ROUND", static_cast<int>(textX), static_cast<int>(py + 10), 12, Fade(Color{ 248, 113, 113, 255 }, alpha));

        // Payout Display (0 SCRAP)
        std::string payStr = "PAYOUT: 0 SCRAP";
        DrawText(payStr.c_str(), static_cast<int>(textX), static_cast<int>(py + 27), 19, Fade(Colors::Zinc400, alpha));

        // Details Subtitle
        std::string sub = "LANDED ON NUMBER " + numStr + " [" + pColorName + "]";
        DrawText(sub.c_str(), static_cast<int>(textX), static_cast<int>(py + 51), 10, Fade(Colors::Zinc500, alpha));
    }
}

} // namespace minesweeper::ui
