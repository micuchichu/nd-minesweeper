#include "scrap_system.hpp"
#include "../ui/theme.hpp"
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <filesystem>

namespace minesweeper::render {

static float randomFloat(float min, float max) {
    return min + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * (max - min);
}

ScrapSystem::ScrapSystem() = default;

ScrapSystem::~ScrapSystem() {
    cleanup();
}

void ScrapSystem::init() {
    if (texture.id != 0) return;

    std::vector<std::string> searchPaths = {
        "assets/scrap.png",
        "x64/Release/assets/scrap.png",
        std::string(GetApplicationDirectory()) + "assets/scrap.png",
        std::string(GetApplicationDirectory()) + "../assets/scrap.png",
        std::string(GetApplicationDirectory()) + "../../assets/scrap.png"
    };

    for (const auto& p : searchPaths) {
        if (FileExists(p.c_str())) {
            texture = LoadTexture(p.c_str());
            if (texture.id != 0) {
                SetTextureFilter(texture, TEXTURE_FILTER_POINT);
                break;
            }
        }
    }

    if (texture.id == 0) {
        // Fallback procedural scrap icon (16x16 battery/scrap pixel art)
        Image img = GenImageColor(16, 16, BLANK);
        for (int y = 3; y < 13; ++y) {
            for (int x = 3; x < 13; ++x) {
                ImageDrawPixel(&img, x, y, ui::Colors::Zinc800);
            }
        }
        for (int y = 5; y < 11; ++y) {
            for (int x = 5; x < 11; ++x) {
                ImageDrawPixel(&img, x, y, ui::Colors::Amber500);
            }
        }
        ImageDrawPixel(&img, 7, 2, ui::Colors::Cyan400);
        ImageDrawPixel(&img, 8, 2, ui::Colors::Cyan400);
        texture = LoadTextureFromImage(img);
        UnloadImage(img);
        SetTextureFilter(texture, TEXTURE_FILTER_POINT);
    }
}

void ScrapSystem::cleanup() {
    if (texture.id != 0) {
        UnloadTexture(texture);
        texture = {};
    }
    clear();
}

void ScrapSystem::clear() {
    items.clear();
    floatingTexts.clear();
    sparks.clear();
    pendingCollected = 0;
}

bool ScrapSystem::isScrapCell(uint64_t seed, size_t cellIndex, size_t totalCells, int bombCount) {
    if (totalCells == 0 || bombCount <= 0) return false;

    double density = static_cast<double>(bombCount) / static_cast<double>(totalCells);
    // Baseline standard density is 15% (0.15) for a 5.0% base drop rate
    double dropRate = 0.05 * (density / 0.15);
    dropRate = std::clamp(dropRate, 0.0, 0.25); // Cap at 25% max for extreme density

    uint64_t threshold = static_cast<uint64_t>(dropRate * 1000000.0);
    if (threshold == 0) return false;

    // SplitMix64 hash
    uint64_t z = seed + static_cast<uint64_t>(cellIndex) * 0x9E3779B97F4A7C15ULL + 0x5C8A9DULL;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    z = z ^ (z >> 31);
    return (z % 1000000ULL) < threshold;
}

void ScrapSystem::spawn(Vector2 cellCenterPos) {
    ScrapItem item;
    item.basePos = cellCenterPos;
    item.currentPos = cellCenterPos;
    item.velocity = { randomFloat(-35.0f, 35.0f), randomFloat(-240.0f, -190.0f) };
    item.rotation = randomFloat(-15.0f, 15.0f);
    item.rotSpeed = randomFloat(-160.0f, 160.0f);
    item.bounceCount = 0;
    item.squashTimer = 0.0f;
    item.state = ScrapState::Bouncing;
    item.stateTimer = 0.0f;
    items.push_back(item);

    // Initial sparkles
    for (int i = 0; i < 5; ++i) {
        float ang = randomFloat(0.0f, 6.283185f);
        float spd = randomFloat(40.0f, 130.0f);
        sparks.push_back({
            cellCenterPos,
            { std::cos(ang) * spd, std::sin(ang) * spd },
            (i % 2 == 0) ? ui::Colors::Amber400 : ui::Colors::Cyan400,
            0.4f,
            0.4f,
            randomFloat(2.5f, 4.5f)
        });
    }
}

void ScrapSystem::update(float dt, Vector2 hudScrapScreenPos, const Camera2D& camera, Vector2 mouseWorldPos, bool mouseClicked) {
    // Update items
    for (size_t i = 0; i < items.size(); ++i) {
        ScrapItem& it = items[i];

        if (it.squashTimer > 0.0f) {
            it.squashTimer -= dt;
        }

        if (it.state == ScrapState::Bouncing) {
            it.velocity.y += 750.0f * dt;
            it.currentPos.x += it.velocity.x * dt;
            it.currentPos.y += it.velocity.y * dt;
            it.rotation += it.rotSpeed * dt;

            // Ground impact check
            if (it.currentPos.y >= it.basePos.y) {
                it.currentPos.y = it.basePos.y;
                it.velocity.y = -it.velocity.y * 0.50f;
                it.velocity.x *= 0.55f;
                it.rotSpeed *= 0.55f;
                it.squashTimer = 0.09f;
                it.bounceCount++;

                // Bounce sparks
                for (int s = 0; s < 3; ++s) {
                    float ang = randomFloat(3.5f, 5.9f); // upward burst
                    float spd = randomFloat(30.0f, 90.0f);
                    sparks.push_back({
                        { it.currentPos.x, it.basePos.y },
                        { std::cos(ang) * spd, std::sin(ang) * spd },
                        ui::Colors::Amber400,
                        0.35f,
                        0.35f,
                        randomFloat(2.0f, 3.5f)
                    });
                }

                if (std::abs(it.velocity.y) < 35.0f || it.bounceCount >= 3) {
                    it.state = ScrapState::Settled;
                    it.stateTimer = 0.0f;
                    it.velocity = { 0.0f, 0.0f };
                    it.currentPos.y = it.basePos.y;

                    // Spawn floating +1
                    floatingTexts.push_back({
                        { it.basePos.x, it.basePos.y - 12.0f },
                        "+1",
                        ui::Colors::Amber400,
                        0.85f,
                        0.85f
                    });
                }
            }
        }
        else if (it.state == ScrapState::Settled) {
            it.stateTimer += dt;
            // Gentle hovering
            it.currentPos.y = it.basePos.y + std::sin(it.stateTimer * 7.0f) * 2.2f;

            // Click-to-collect
            bool clickedOn = mouseClicked && (Vector2Distance(mouseWorldPos, it.currentPos) < 22.0f);

            // Auto-collect after settling briefly (~0.6s)
            if (clickedOn || it.stateTimer >= 0.60f) {
                it.state = ScrapState::FlyingToHUD;
                it.flyStartScreen = GetWorldToScreen2D(it.currentPos, camera);
                it.targetScreen = hudScrapScreenPos;
                it.flyProgress = 0.0f;
            }
        }
        else if (it.state == ScrapState::FlyingToHUD) {
            it.flyProgress += dt * 2.2f; // ~0.45s flight

            if (it.flyProgress >= 1.0f) {
                it.reachedHUD = true;
                pendingCollected++;
            }
        }
    }

    // Clean up collected items
    for (size_t i = 0; i < items.size();) {
        if (items[i].reachedHUD) {
            items[i] = items.back();
            items.pop_back();
        } else {
            ++i;
        }
    }

    // Update floating texts
    for (size_t i = 0; i < floatingTexts.size();) {
        floatingTexts[i].life -= dt;
        floatingTexts[i].worldPos.y -= 26.0f * dt;
        if (floatingTexts[i].life <= 0.0f) {
            floatingTexts[i] = floatingTexts.back();
            floatingTexts.pop_back();
        } else {
            ++i;
        }
    }

    // Update sparks
    for (size_t i = 0; i < sparks.size();) {
        ScrapSpark& s = sparks[i];
        s.life -= dt;
        s.vel.y += 200.0f * dt;
        s.pos.x += s.vel.x * dt;
        s.pos.y += s.vel.y * dt;
        if (s.life <= 0.0f) {
            sparks[i] = sparks.back();
            sparks.pop_back();
        } else {
            ++i;
        }
    }
}

void ScrapSystem::drawWorld(const Camera2D& camera) {
    (void)camera;

    // 1. Draw tile drop shadows & scrap sprites for bouncing / settled items
    float scrapBaseSize = 20.0f;

    for (const auto& it : items) {
        if (it.state == ScrapState::FlyingToHUD) continue;

        float h = std::max(0.0f, it.basePos.y - it.currentPos.y);

        // Ground shadow
        float shadowAlpha = std::clamp(0.40f - (h * 0.005f), 0.08f, 0.40f);
        float shadowW = std::max(5.0f, 15.0f - (h * 0.12f));
        float shadowH = shadowW * 0.45f;
        DrawEllipse(static_cast<int>(it.basePos.x), static_cast<int>(it.basePos.y + 6.0f), shadowW, shadowH, Fade(BLACK, shadowAlpha));

        // Squash and stretch
        float scaleX = 1.0f;
        float scaleY = 1.0f;
        if (it.squashTimer > 0.0f) {
            scaleX = 1.35f;
            scaleY = 0.70f;
        } else if (std::abs(it.velocity.y) > 40.0f) {
            float stretch = std::min(0.30f, std::abs(it.velocity.y) / 750.0f);
            scaleY = 1.0f + stretch;
            scaleX = 1.0f / std::sqrt(scaleY);
        }

        float drawW = scrapBaseSize * scaleX;
        float drawH = scrapBaseSize * scaleY;

        Rectangle src = { 0.0f, 0.0f, static_cast<float>(texture.width), static_cast<float>(texture.height) };
        Rectangle dst = { it.currentPos.x, it.currentPos.y, drawW, drawH };
        Vector2 origin = { drawW * 0.5f, drawH * 0.5f };

        // Subtle yellow/amber halo glow when settled
        if (it.state == ScrapState::Settled) {
            float pulse = 0.5f + 0.5f * std::sin(it.stateTimer * 9.0f);
            DrawCircle(static_cast<int>(it.currentPos.x), static_cast<int>(it.currentPos.y), 13.0f + pulse * 4.0f, Fade(ui::Colors::Amber400, 0.12f * pulse));
            DrawCircleLines(static_cast<int>(it.currentPos.x), static_cast<int>(it.currentPos.y), 14.0f + pulse * 5.0f, Fade(ui::Colors::Amber400, 0.25f * pulse));
        }

        DrawTexturePro(texture, src, dst, origin, it.rotation, WHITE);
    }

    // 2. Draw sparks in world space
    for (const auto& s : sparks) {
        float alpha = std::clamp(s.life / s.maxLife, 0.0f, 1.0f);
        Color c = Fade(s.color, alpha);
        DrawRectanglePro({ s.pos.x, s.pos.y, s.size, s.size }, { s.size * 0.5f, s.size * 0.5f }, 45.0f, c);
    }

    // 3. Draw floating "+1" text
    for (const auto& ft : floatingTexts) {
        float alpha = std::clamp(ft.life / ft.maxLife, 0.0f, 1.0f);
        int textW = MeasureText(ft.text.c_str(), 16);
        int posX = static_cast<int>(ft.worldPos.x - textW * 0.5f);
        int posY = static_cast<int>(ft.worldPos.y);

        // Dark dropshadow outline
        DrawText(ft.text.c_str(), posX + 1, posY + 1, 16, Fade(BLACK, alpha * 0.9f));
        DrawText(ft.text.c_str(), posX, posY, 16, Fade(ft.color, alpha));
    }
}

void ScrapSystem::drawScreen(float guiScale) {
    if (texture.id == 0) return;

    for (const auto& it : items) {
        if (it.state != ScrapState::FlyingToHUD) continue;

        float t = std::clamp(it.flyProgress, 0.0f, 1.0f);
        // Ease-in quad/cubic: slow at start, accelerating to HUD
        float ease = t * t * (3.0f - 2.0f * t);

        // Target position in UI camera screen space
        Vector2 start = { it.flyStartScreen.x / guiScale, it.flyStartScreen.y / guiScale };
        Vector2 target = (it.targetScreen.x > 0.0f) ? it.targetScreen : Vector2{ 140.0f, 31.0f };
        // Arch trajectory with upward arc midpoint
        Vector2 mid = { (start.x + target.x) * 0.5f, std::min(start.y, target.y) - 60.0f };

        // Quadratic Bezier: (1-t)^2 * P0 + 2*(1-t)*t * P1 + t^2 * P2
        float inv = 1.0f - ease;
        Vector2 cur = {
            inv * inv * start.x + 2.0f * inv * ease * mid.x + ease * ease * target.x,
            inv * inv * start.y + 2.0f * inv * ease * mid.y + ease * ease * target.y
        };

        float drawSize = 22.0f * (1.1f - ease * 0.25f);
        Rectangle src = { 0.0f, 0.0f, static_cast<float>(texture.width), static_cast<float>(texture.height) };
        Rectangle dst = { cur.x, cur.y, drawSize, drawSize };
        Vector2 origin = { drawSize * 0.5f, drawSize * 0.5f };

        // Gold trail glow
        DrawCircle(static_cast<int>(cur.x), static_cast<int>(cur.y), 9.0f, Fade(ui::Colors::Amber400, 0.25f * (1.0f - ease)));
        DrawTexturePro(texture, src, dst, origin, t * 360.0f, WHITE);
    }
}

int ScrapSystem::collectPending() {
    int count = pendingCollected;
    pendingCollected = 0;
    return count;
}

int ScrapSystem::collectAll() {
    int total = pendingCollected + static_cast<int>(items.size());
    items.clear();
    pendingCollected = 0;
    return total;
}

} // namespace minesweeper::render
