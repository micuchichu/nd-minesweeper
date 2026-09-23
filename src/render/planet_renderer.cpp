#include "planet_renderer.hpp"
#include "../core/campaign.hpp"
#include "../ui/theme.hpp"
#include <cmath>
#include <algorithm>
#include <map>

#if defined(_WIN32)
#ifdef GetMouseRay
#undef GetMouseRay
#endif
extern "C" RLAPI Ray GetMouseRay(Vector2 mousePosition, Camera camera);
static inline Ray castCameraRay(Vector2 pos, Camera cam) {
    return GetMouseRay(pos, cam);
}
#else
static inline Ray castCameraRay(Vector2 pos, Camera cam) {
    return GetScreenToWorldRay(pos, cam);
}
#endif

namespace minesweeper::render {

PlanetRenderer::PlanetRenderer() {
    init();
}

void PlanetRenderer::init() {
    if (initialized) return;

    camera.position = { 0.12f, 0.0f, cameraDistance };
    camera.target = { 0.12f, 0.0f, 0.0f };
    camera.up = { 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // Initialize with default planet config if not yet set
    auto defaultPlanets = core::CampaignManager::getDefaultPlanetConfigs();
    if (!defaultPlanets.empty()) {
        currentPlanetConfig = defaultPlanets[0];
    }

    generateGeodesicHexGrid(&currentPlanetConfig);
    generateStars(1920, 1080);

    // Initial focus on Campaign Sector 01 (Outpost Alpha)
    int startSec = getSectorIdxForFortress(0);
    if (startSec >= 0) {
        focusSector(startSec);
    }

    initialized = true;
}

void PlanetRenderer::generateGeodesicHexGrid(const core::PlanetConfig* cfg) {
    if (cfg) {
        currentPlanetConfig = *cfg;
    }
    sectors.clear();

    // 1. Construct Base Icosahedron (12 vertices, 20 faces)
    float t = (1.0f + std::sqrt(5.0f)) / 2.0f;
    std::vector<Vector3> icoVerts = {
        Vector3Normalize({ -1,  t,  0 }),
        Vector3Normalize({  1,  t,  0 }),
        Vector3Normalize({ -1, -t,  0 }),
        Vector3Normalize({  1, -t,  0 }),
        Vector3Normalize({  0, -1,  t }),
        Vector3Normalize({  0,  1,  t }),
        Vector3Normalize({  0, -1, -t }),
        Vector3Normalize({  0,  1, -t }),
        Vector3Normalize({  t,  0, -1 }),
        Vector3Normalize({  t,  0,  1 }),
        Vector3Normalize({ -t,  0, -1 }),
        Vector3Normalize({ -t,  0,  1 })
    };

    struct Tri { int a, b, c; };
    std::vector<Tri> icoFaces = {
        {0, 11, 5}, {0, 5, 1}, {0, 1, 7}, {0, 7, 10}, {0, 10, 11},
        {1, 5, 9}, {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
        {3, 9, 4}, {3, 4, 2}, {3, 2, 6}, {3, 6, 8}, {3, 8, 9},
        {4, 9, 5}, {2, 4, 11}, {6, 2, 10}, {8, 6, 7}, {9, 8, 1}
    };

    // 2. Subdivide Edges (Frequency F = 2, resulting in 42 vertices and 80 triangles)
    std::vector<Vector3> geoVerts = icoVerts;
    std::map<std::pair<int, int>, int> midMap;

    auto getMidpoint = [&](int i1, int i2) -> int {
        int smallIdx = std::min(i1, i2);
        int largeIdx = std::max(i1, i2);
        auto key = std::make_pair(smallIdx, largeIdx);
        auto it = midMap.find(key);
        if (it != midMap.end()) return it->second;

        Vector3 mid = Vector3Normalize(Vector3Scale(Vector3Add(geoVerts[i1], geoVerts[i2]), 0.5f));
        int newIdx = static_cast<int>(geoVerts.size());
        geoVerts.push_back(mid);
        midMap[key] = newIdx;
        return newIdx;
    };

    std::vector<Tri> geoFaces;
    for (const auto& f : icoFaces) {
        int mAB = getMidpoint(f.a, f.b);
        int mBC = getMidpoint(f.b, f.c);
        int mCA = getMidpoint(f.c, f.a);

        geoFaces.push_back({ f.a, mAB, mCA });
        geoFaces.push_back({ f.b, mBC, mAB });
        geoFaces.push_back({ f.c, mCA, mBC });
        geoFaces.push_back({ mAB, mBC, mCA });
    }

    // 3. Construct Dual Polyhedron (Geodesic Hexagons covering entire sphere)
    // Precalculate face centroids on unit sphere
    std::vector<Vector3> faceCentroids(geoFaces.size());
    for (size_t f = 0; f < geoFaces.size(); ++f) {
        Vector3 sum = Vector3Add(Vector3Add(geoVerts[geoFaces[f].a], geoVerts[geoFaces[f].b]), geoVerts[geoFaces[f].c]);
        faceCentroids[f] = Vector3Normalize(sum);
    }

    // Palette of surface tones from data-driven planet configuration
    std::vector<Color> tones = currentPlanetConfig.visual.surfaceTones;
    if (tones.empty()) {
        tones = {
            Color{ 78, 36, 44, 255 },  // Volcanic basalt crust
            Color{ 58, 26, 34, 255 },  // Deep charcoal obsidian
            Color{ 96, 44, 54, 255 },  // Warm basalt ridge
            Color{ 68, 30, 38, 255 },  // Crater rim rock
            Color{ 112, 50, 60, 255 }  // Elevated volcanic plateau
        };
    }

    bool hasRifts = currentPlanetConfig.visual.hasMagmaRifts;
    float riftChance = currentPlanetConfig.visual.magmaRiftChance;
    Color riftCol = currentPlanetConfig.visual.magmaRiftColor;

    uint32_t rng = 883311;
    auto nextRnd = [&rng]() -> uint32_t {
        rng = rng * 1664525u + 1013904223u;
        return rng;
    };

    for (size_t v = 0; v < geoVerts.size(); ++v) {
        HexSector sec;
        sec.id = static_cast<int>(v);
        sec.center = geoVerts[v];

        // Gather all adjacent face centroids
        std::vector<Vector3> adjFaces;
        for (size_t f = 0; f < geoFaces.size(); ++f) {
            if (geoFaces[f].a == static_cast<int>(v) ||
                geoFaces[f].b == static_cast<int>(v) ||
                geoFaces[f].c == static_cast<int>(v)) {
                adjFaces.push_back(faceCentroids[f]);
            }
        }

        if (adjFaces.size() < 3) continue;

        // Sort adjacent centroids CCW around sec.center
        Vector3 normal = sec.center;
        Vector3 upRef = (std::abs(normal.y) > 0.9f) ? Vector3{ 1, 0, 0 } : Vector3{ 0, 1, 0 };
        Vector3 tangentU = Vector3Normalize(Vector3CrossProduct(normal, upRef));
        Vector3 tangentV = Vector3CrossProduct(normal, tangentU);

        std::vector<std::pair<float, Vector3>> sorted;
        for (const auto& cPos : adjFaces) {
            Vector3 diff = Vector3Subtract(cPos, normal);
            float uVal = Vector3DotProduct(diff, tangentU);
            float vVal = Vector3DotProduct(diff, tangentV);
            float angle = std::atan2(vVal, uVal);
            sorted.push_back({ angle, cPos });
        }

        std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
            return a.first < b.first;
        });

        for (const auto& item : sorted) {
            sec.corners.push_back(item.second);
        }

        // Deterministic terrain coloring
        uint32_t rVal = nextRnd();
        sec.baseColor = tones[rVal % tones.size()];
        sec.isMagmaRift = hasRifts && ((rVal % 100) < static_cast<uint32_t>(std::clamp(riftChance * 100.0f, 0.0f, 100.0f)));
        if (sec.isMagmaRift) {
            sec.baseColor = riftCol;
        }

        // Territory metadata
        int sectorNum = (static_cast<int>(v) * 7) % 97 + 3;
        sec.codename = "SECTOR-" + std::to_string(sectorNum);
        sec.name = "Territory " + std::to_string(sectorNum);
        sec.subtitle = currentPlanetConfig.visual.geologicalStatus.empty() ? "Unclaimed Planetary Buffer" : currentPlanetConfig.visual.geologicalStatus;
        sec.threatLevel = (rVal % 3) + 1;

        sectors.push_back(sec);
    }

    // 4. Assign the 4 Campaign Fortress Sectors to prominent, well-distributed hexagons
    // Find best candidates based on position:
    // Sector 1 (Outpost Alpha): Front-facing equator (High Z)
    // Sector 2 (Foundry Wastes): East equatorial (High X)
    // Sector 3 (Chemical Basin): North-East (High Y, moderate X)
    // Sector 4 (Core Vault): South-West (Low X, Low Y)
    int s1 = -1, s2 = -1, s3 = -1, s4 = -1;
    float bestZ = -999.0f, bestX = -999.0f, bestY = -999.0f, bestLow = 999.0f;

    for (size_t i = 0; i < sectors.size(); ++i) {
        const auto& c = sectors[i].center;
        if (c.z > bestZ) { bestZ = c.z; s1 = static_cast<int>(i); }
    }
    for (size_t i = 0; i < sectors.size(); ++i) {
        if (static_cast<int>(i) == s1) continue;
        const auto& c = sectors[i].center;
        if (c.x > bestX && c.z > -0.2f) { bestX = c.x; s2 = static_cast<int>(i); }
    }
    for (size_t i = 0; i < sectors.size(); ++i) {
        if (static_cast<int>(i) == s1 || static_cast<int>(i) == s2) continue;
        const auto& c = sectors[i].center;
        if (c.y > bestY && c.x > 0.0f) { bestY = c.y; s3 = static_cast<int>(i); }
    }
    for (size_t i = 0; i < sectors.size(); ++i) {
        if (static_cast<int>(i) == s1 || static_cast<int>(i) == s2 || static_cast<int>(i) == s3) continue;
        const auto& c = sectors[i].center;
        float score = c.x + c.y;
        if (score < bestLow) { bestLow = score; s4 = static_cast<int>(i); }
    }

    int fortressIndices[4] = { s1, s2, s3, s4 };
    const char* fNames[4] = { "Sector 01: Outpost Alpha", "Sector 02: Foundry Wastes", "Sector 03: Chemical Basin", "Sector 04: The Core Vault" };
    const char* fCodes[4] = { "ALPHA-01", "FOUNDRY-02", "CHEM-03", "CORE-04" };
    const char* fSubs[4] = { "Perimeter Landing Zone", "Abandoned Smelting Foundry", "Volatile Refinery Basin", "Deep Subterranean Hub" };

    for (int k = 0; k < 4; ++k) {
        int idx = fortressIndices[k];
        if (idx >= 0 && idx < static_cast<int>(sectors.size())) {
            sectors[idx].isFortress = true;
            sectors[idx].fortressIdx = k;
            sectors[idx].name = fNames[k];
            sectors[idx].codename = fCodes[k];
            sectors[idx].subtitle = fSubs[k];
            sectors[idx].threatLevel = k + 1;
            sectors[idx].baseColor = tones[tones.size() / 2];
        }
    }
}

void PlanetRenderer::applyPlanetConfig(const core::PlanetConfig& cfg) {
    currentPlanetConfig = cfg;
    generateGeodesicHexGrid(&currentPlanetConfig);
    int startSec = getSectorIdxForFortress(0);
    if (startSec >= 0) {
        focusSector(startSec);
    }
}

void PlanetRenderer::generateStars(int screenW, int screenH) {
    (void)screenW; (void)screenH;
    stars.clear();
    uint32_t seed = 998811;
    auto rnd = [&seed]() -> float {
        seed = seed * 1664525u + 1013904223u;
        return static_cast<float>(seed & 0xFFFF) / 65535.0f;
    };

    for (int i = 0; i < 180; ++i) {
        Star3D s;
        float z = rnd() * 2.0f - 1.0f;
        float phi = rnd() * 2.0f * PI;
        float rad = std::sqrt(std::max(0.0f, 1.0f - z * z));
        float x = rad * std::cos(phi);
        float y = z;
        float zCoord = rad * std::sin(phi);

        float starDist = 70.0f + rnd() * 25.0f;
        s.worldPos = { x * starDist, y * starDist, zCoord * starDist };
        s.size = 1.0f + rnd() * 1.8f;
        s.brightness = 0.35f + rnd() * 0.65f;
        s.twinkleSpeed = 1.0f + rnd() * 3.0f;
        s.twinkleOffset = rnd() * 6.28f;
        stars.push_back(s);
    }
}

Vector3 PlanetRenderer::rotateVector(Vector3 v) const {
    // Transform vector into camera view coordinates (for testing and orientation query)
    float cosP = std::cos(pitch);
    float sinP = std::sin(pitch);
    float cosY = std::cos(yaw);
    float sinY = std::sin(yaw);

    Vector3 camDir = { cosP * sinY, sinP, cosP * cosY }; // Normalized vector from center to camera
    Vector3 fwd = Vector3Negate(camDir);
    Vector3 upWorld = { 0.0f, 1.0f, 0.0f };
    Vector3 right = Vector3Normalize(Vector3CrossProduct(fwd, upWorld));
    Vector3 up = Vector3Normalize(Vector3CrossProduct(right, fwd));

    float x = Vector3DotProduct(v, right);
    float y = Vector3DotProduct(v, up);
    float z = Vector3DotProduct(v, camDir);
    return { x, y, z };
}

int PlanetRenderer::getSectorIdxForFortress(int fortressIdx) const {
    for (size_t i = 0; i < sectors.size(); ++i) {
        if (sectors[i].isFortress && sectors[i].fortressIdx == fortressIdx) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void PlanetRenderer::syncCampaignSectors(const core::CampaignManager& campaign) {
    for (size_t k = 0; k < campaign.sectors.size(); ++k) {
        int idx = getSectorIdxForFortress(static_cast<int>(k));
        if (idx >= 0 && idx < static_cast<int>(sectors.size())) {
            const auto& cSec = campaign.sectors[k];
            sectors[idx].name = cSec.name;
            sectors[idx].codename = cSec.codename;
            sectors[idx].subtitle = cSec.subtitle;
            sectors[idx].threatLevel = cSec.threatLevel;
        }
    }
}

void PlanetRenderer::focusSector(int sectorIdx) {
    if (sectorIdx < 0 || sectorIdx >= static_cast<int>(sectors.size())) return;
    const auto& sec = sectors[sectorIdx];

    // Align camera orbit ray directly with sec.center
    float lat = std::asin(std::clamp(sec.center.y, -1.0f, 1.0f));
    float lon = std::atan2(sec.center.x, sec.center.z);

    targetPitch = lat;
    targetYaw = lon;
    isTransitioning = true;
    idleTimer = 0.0f;
}

void PlanetRenderer::update(float dt) {
    // Smooth camera distance
    cameraDistance += (targetCameraDistance - cameraDistance) * std::min(1.0f, dt * 8.0f);

    if (!isDragging) {
        idleTimer += dt;

        if (isTransitioning) {
            float diffYaw = targetYaw - yaw;
            while (diffYaw < -PI) diffYaw += 2.0f * PI;
            while (diffYaw > PI) diffYaw -= 2.0f * PI;

            yaw += diffYaw * std::min(1.0f, dt * 5.5f);
            pitch += (targetPitch - pitch) * std::min(1.0f, dt * 5.5f);

            if (std::abs(diffYaw) < 0.005f && std::abs(targetPitch - pitch) < 0.005f) {
                yaw = targetYaw;
                pitch = targetPitch;
                isTransitioning = false;
            }
        } else if (idleTimer > 3.0f) {
            // Idle ambient camera orbit around the globe
            yaw += 0.035f * dt;
            targetYaw = yaw;
        }
    } else {
        idleTimer = 0.0f;
        isTransitioning = false;
    }

    // Compute orbiting camera position and look vectors
    float cosP = std::cos(pitch);
    float sinP = std::sin(pitch);
    float cosY = std::cos(yaw);
    float sinY = std::sin(yaw);

    Vector3 camDir = { cosP * sinY, sinP, cosP * cosY }; // Direction from planet origin to camera
    Vector3 unshiftedPos = Vector3Scale(camDir, cameraDistance);
    Vector3 fwd = Vector3Normalize(Vector3Negate(camDir));
    Vector3 upWorld = { 0.0f, 1.0f, 0.0f };
    Vector3 right = Vector3Normalize(Vector3CrossProduct(fwd, upWorld));
    Vector3 up = Vector3Normalize(Vector3CrossProduct(right, fwd));

    // Lateral right-vector offset frames the planet in the gap between left and right UI panels
    float lateralOffset = 0.14f;
    camera.position = Vector3Add(unshiftedPos, Vector3Scale(right, lateralOffset));
    camera.target = Vector3Scale(right, lateralOffset);
    camera.up = up;
}

int PlanetRenderer::handleInput(Rectangle viewport, int& outHoveredSector) {
    Vector2 mousePos = GetMousePosition();
    outHoveredSector = -1;

    bool insideViewport = CheckCollisionPointRec(mousePos, viewport);

    // Zoom via mouse wheel
    if (insideViewport) {
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) {
            targetCameraDistance = std::clamp(targetCameraDistance - wheel * 0.35f, 2.8f, 5.2f);
        }
    }

    // Direction vector from origin to camera
    Vector3 camDir = Vector3Normalize(Vector3Subtract(camera.position, camera.target));

    if (insideViewport) {
        // 1. Precise 3D ray collision on the planet sphere
        Ray mouseRay = castCameraRay(mousePos, camera);
        RayCollision hit = GetRayCollisionSphere(mouseRay, { 0.0f, 0.0f, 0.0f }, PLANET_RADIUS);

        if (hit.hit) {
            // Mathematical spherical Voronoi query:
            // Every point on the sphere surface belongs to the hex sector whose center has maximum dot product with hit normal
            Vector3 hitNorm = Vector3Normalize(hit.point);
            float bestDot = -2.0f;
            int bestIdx = -1;
            for (size_t i = 0; i < sectors.size(); ++i) {
                float dot = Vector3DotProduct(sectors[i].center, hitNorm);
                if (dot > bestDot) {
                    bestDot = dot;
                    bestIdx = static_cast<int>(i);
                }
            }
            if (bestIdx >= 0) {
                if (Vector3DotProduct(sectors[bestIdx].center, camDir) > -0.05f) {
                    outHoveredSector = bestIdx;
                }
            }
        }

        // 2. Generous screen-space fallback for clicks near the planet limb or on floating sector badges
        if (outHoveredSector < 0) {
            float closestDist = 52.0f;
            for (size_t i = 0; i < sectors.size(); ++i) {
                float dot = Vector3DotProduct(sectors[i].center, camDir);
                if (dot > 0.05f) { // Front-facing
                    Vector3 pWorld = Vector3Scale(sectors[i].center, PLANET_RADIUS);
                    Vector2 sPos = GetWorldToScreen(pWorld, camera);
                    float dist = Vector2Distance(mousePos, sPos);
                    if (dist < closestDist) {
                        closestDist = dist;
                        outHoveredSector = static_cast<int>(i);
                    }
                }
            }
        }
    }

    int clickedSector = -1;

    // Mouse button pressed: begin drag interaction or prepare click
    if (insideViewport && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        isDragging = true;
        hasDragged = false;
        dragStartPos = mousePos;
        dragStartYaw = yaw;
        dragStartPitch = pitch;
        potentialClickSector = outHoveredSector;
    }

    if (isDragging) {
        float moveDist = Vector2Distance(mousePos, dragStartPos);
        if (moveDist > 4.0f) {
            hasDragged = true;
        }

        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            if (hasDragged) {
                float dx = mousePos.x - dragStartPos.x;
                float dy = mousePos.y - dragStartPos.y;
                yaw = dragStartYaw - dx * 0.007f;
                pitch = std::clamp(dragStartPitch + dy * 0.007f, -1.25f, 1.25f);
                targetYaw = yaw;
                targetPitch = pitch;
            }
        }

        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) || !IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            if (!hasDragged) {
                // Short click without dragging -> select sector!
                int target = (outHoveredSector >= 0) ? outHoveredSector : potentialClickSector;
                if (target >= 0) {
                    clickedSector = target;
                    focusSector(clickedSector);
                }
            }
            isDragging = false;
            hasDragged = false;
            potentialClickSector = -1;
        }
    }

    return clickedSector;
}

void PlanetRenderer::drawGlobe(const core::CampaignManager& campaign, int selectedSectorIdx, int hoveredSectorIdx) {
    syncCampaignSectors(campaign);

    // 1. Draw 3D Celestial Starfield (projected to screen based on camera rotation)
    float time = static_cast<float>(GetTime());
    Vector3 fwd = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    for (const auto& s : stars) {
        Vector3 toStar = Vector3Subtract(s.worldPos, camera.position);
        if (Vector3DotProduct(toStar, fwd) > 0.0f) {
            Vector2 sPos = GetWorldToScreen(s.worldPos, camera);
            float tw = 0.70f + 0.30f * std::sin(time * s.twinkleSpeed + s.twinkleOffset);
            Color starCol = Fade(WHITE, s.brightness * tw);
            DrawCircleV(sPos, s.size, starCol);
        }
    }

    // 2. Begin 3D Rendering Pass
    BeginMode3D(camera);

    // Faint equatorial orbital guide ring in space (stationary in world space)
    DrawCircle3D({ 0, 0, 0 }, PLANET_RADIUS * 1.45f, { 0, 1, 0 }, 0.0f, Fade(currentPlanetConfig.visual.atmosphereColor, 0.18f));

    // Directional light vector in world space (upper right forward)
    Vector3 lightDir = Vector3Normalize({ 0.35f, 0.70f, 0.60f });
    Vector3 camDir = Vector3Normalize(camera.position);

    // 3. Render Low-Poly Faceted Hexagonal Sectors in stationary world space
    for (size_t i = 0; i < sectors.size(); ++i) {
        const auto& sec = sectors[i];

        // Backface culling: skip sectors on the back side of the planet relative to camera
        if (Vector3DotProduct(sec.center, camDir) < -0.20f) continue;

        size_t nCorners = sec.corners.size();
        if (nCorners < 3) continue;

        Vector3 centerWorld = Vector3Scale(sec.center, PLANET_RADIUS);

        // Determine base color & status
        Color baseCol = sec.baseColor;
        if (sec.isFortress) {
            const auto* cSec = campaign.getSectorByIndex(sec.fortressIdx);
            if (cSec && cSec->isCleared) {
                baseCol = Color{ 24, 76, 44, 255 };  // Secured green
            } else if (cSec && cSec->isUnlocked) {
                float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(GetTime()) * 4.5f);
                baseCol = Color{
                    static_cast<unsigned char>(160 + 60 * pulse),
                    static_cast<unsigned char>(110 + 40 * pulse),
                    10,
                    255
                }; // Contested pulsing amber (#FFB000)
            } else {
                baseCol = Color{ 42, 42, 48, 255 };  // Locked dark charcoal (#2A2A30)
            }
        }

        // Draw flat-shaded triangular facets from center to consecutive corners
        for (size_t k = 0; k < nCorners; ++k) {
            size_t nextK = (k + 1) % nCorners;
            Vector3 vA = Vector3Scale(sec.corners[k], PLANET_RADIUS);
            Vector3 vB = Vector3Scale(sec.corners[nextK], PLANET_RADIUS);

            // Compute face normal for flat low-poly shading
            Vector3 edge1 = Vector3Subtract(vA, centerWorld);
            Vector3 edge2 = Vector3Subtract(vB, centerWorld);
            Vector3 faceNorm = Vector3Normalize(Vector3CrossProduct(edge1, edge2));

            // Ensure normal points outward
            if (Vector3DotProduct(faceNorm, sec.center) < 0.0f) {
                faceNorm = Vector3Negate(faceNorm);
            }

            // Directional diffuse lighting with ambient base
            float diff = std::max(0.0f, Vector3DotProduct(faceNorm, lightDir));
            float shade = 0.45f + 0.55f * diff;

            Color facetCol = {
                static_cast<unsigned char>(std::clamp(baseCol.r * shade, 0.0f, 255.0f)),
                static_cast<unsigned char>(std::clamp(baseCol.g * shade, 0.0f, 255.0f)),
                static_cast<unsigned char>(std::clamp(baseCol.b * shade, 0.0f, 255.0f)),
                255
            };

            DrawTriangle3D(centerWorld, vA, vB, facetCol);
            DrawTriangle3D(centerWorld, vB, vA, facetCol);
        }

        // Low-poly wireframe edge lines along hexagon perimeter
        bool isSel = (static_cast<int>(i) == selectedSectorIdx);
        bool isHov = (static_cast<int>(i) == hoveredSectorIdx);

        Color edgeCol = Fade(currentPlanetConfig.visual.wireframeColor, 0.60f);
        if (isSel) {
            edgeCol = Color{ 254, 202, 202, 255 }; // Glowing white-pink
        } else if (isHov) {
            edgeCol = WHITE;
        } else if (sec.isFortress) {
            const auto* cSec = campaign.getSectorByIndex(sec.fortressIdx);
            if (cSec && cSec->isCleared) edgeCol = ui::Colors::Green400;
            else if (cSec && cSec->isUnlocked) {
                float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(GetTime()) * 5.0f);
                edgeCol = Fade(Color{ 255, 176, 0, 255 }, 0.70f + 0.30f * pulse);
            }
            else edgeCol = Color{ 42, 42, 48, 255 }; // Locked dark charcoal
        }

        for (size_t k = 0; k < nCorners; ++k) {
            size_t nextK = (k + 1) % nCorners;
            Vector3 vA = Vector3Scale(sec.corners[k], PLANET_RADIUS * 1.002f);
            Vector3 vB = Vector3Scale(sec.corners[nextK], PLANET_RADIUS * 1.002f);
            DrawLine3D(vA, vB, edgeCol);
        }
    }

    // 4. Draw Transit Arcs Connecting the Campaign Fortresses in World Space
    int numArcs = static_cast<int>(campaign.sectors.size()) - 1;
    for (int k = 0; k < numArcs; ++k) {
        int idxA = getSectorIdxForFortress(k);
        int idxB = getSectorIdxForFortress(k + 1);
        if (idxA < 0 || idxB < 0) continue;

        const auto* cSecA = campaign.getSectorByIndex(k);
        const auto* cSecB = campaign.getSectorByIndex(k + 1);
        bool isCleared = cSecA && cSecA->isCleared;
        bool isUnlocked = cSecB && cSecB->isUnlocked;

        Color arcCol = isCleared ? ui::Colors::Green400 : (isUnlocked ? ui::Colors::Amber500 : Fade(ui::Colors::Zinc600, 0.35f));

        Vector3 pA = sectors[idxA].center;
        Vector3 pB = sectors[idxB].center;

        float dot = std::clamp(Vector3DotProduct(pA, pB), -1.0f, 1.0f);
        float omega = std::acos(dot);
        float sinOmega = std::sin(omega);

        constexpr int ARC_SEGMENTS = 16;
        Vector3 prevP = Vector3Scale(pA, PLANET_RADIUS * 1.015f);

        for (int step = 1; step <= ARC_SEGMENTS; ++step) {
            float frac = static_cast<float>(step) / static_cast<float>(ARC_SEGMENTS);
            Vector3 slerpNorm;
            if (sinOmega > 1e-4f) {
                float wA = std::sin((1.0f - frac) * omega) / sinOmega;
                float wB = std::sin(frac * omega) / sinOmega;
                slerpNorm = Vector3Normalize(Vector3Add(Vector3Scale(pA, wA), Vector3Scale(pB, wB)));
            } else {
                slerpNorm = pA;
            }
            Vector3 curP = Vector3Scale(slerpNorm, PLANET_RADIUS * 1.015f);

            if (Vector3DotProduct(prevP, camDir) > 0.0f && Vector3DotProduct(curP, camDir) > 0.0f) {
                DrawLine3D(prevP, curP, arcCol);
            }
            prevP = curP;
        }
    }

    EndMode3D();
}

void PlanetRenderer::drawSectorOverlays(const core::CampaignManager& campaign, int selectedSectorIdx, int hoveredSectorIdx) {
    float time = static_cast<float>(GetTime());
    Vector3 camDir = Vector3Normalize(camera.position);

    for (size_t i = 0; i < sectors.size(); ++i) {
        const auto& sec = sectors[i];
        if (Vector3DotProduct(sec.center, camDir) < 0.14f) continue; // Behind planet horizon

        Vector3 pWorld = Vector3Scale(sec.center, PLANET_RADIUS * 1.01f);
        Vector2 sPos = GetWorldToScreen(pWorld, camera);

        bool isSel = (static_cast<int>(i) == selectedSectorIdx);
        bool isHov = (static_cast<int>(i) == hoveredSectorIdx);

        // Only draw prominent icons / glows for selected, hovered, or campaign fortress sectors
        if (!isSel && !isHov && !sec.isFortress) continue;

        size_t nCorners = sec.corners.size();
        std::vector<Vector2> screenCorners(nCorners);
        for (size_t k = 0; k < nCorners; ++k) {
            screenCorners[k] = GetWorldToScreen(Vector3Scale(sec.corners[k], PLANET_RADIUS * 1.01f), camera);
        }

        // Draw glowing polygon fill for selected or hovered sector
        if (isSel) {
            float pulse = 0.25f + 0.12f * std::sin(time * 5.0f);
            Color fillCol = Fade(Color{ 244, 63, 94, 255 }, pulse);
            for (size_t k = 0; k < nCorners; ++k) {
                size_t nextK = (k + 1) % nCorners;
                DrawTriangle(sPos, screenCorners[k], screenCorners[nextK], fillCol);
            }

            // Outer glowing aura
            Color auraCol = Fade(Color{ 244, 63, 94, 255 }, 0.40f + 0.20f * std::sin(time * 5.0f));
            for (size_t k = 0; k < nCorners; ++k) {
                size_t nextK = (k + 1) % nCorners;
                DrawLineEx(screenCorners[k], screenCorners[nextK], 5.0f, auraCol);
            }
            for (size_t k = 0; k < nCorners; ++k) {
                size_t nextK = (k + 1) % nCorners;
                DrawLineEx(screenCorners[k], screenCorners[nextK], 3.0f, WHITE);
            }
        } else if (isHov) {
            Color fillCol = Fade(WHITE, 0.18f);
            for (size_t k = 0; k < nCorners; ++k) {
                size_t nextK = (k + 1) % nCorners;
                DrawTriangle(sPos, screenCorners[k], screenCorners[nextK], fillCol);
                DrawLineEx(screenCorners[k], screenCorners[nextK], 2.5f, WHITE);
            }
        }

        // Tactical Outpost / Lock / Hazard Icons
        if (sec.isFortress) {
            const auto* cSec = campaign.getSectorByIndex(sec.fortressIdx);
            bool isSecured = cSec && cSec->isCleared;
            bool isHostile = cSec ? (cSec->isUnlocked && !cSec->isCleared) : (sec.fortressIdx == 0);

            float iconSize = 8.5f;
            Vector2 pRoof = { sPos.x, sPos.y - iconSize * 0.7f };
            Vector2 pLeft = { sPos.x - iconSize * 0.7f, sPos.y };
            Vector2 pRight = { sPos.x + iconSize * 0.7f, sPos.y };
            Vector2 pBotLeft = { sPos.x - iconSize * 0.7f, sPos.y + iconSize * 0.6f };
            Vector2 pBotRight = { sPos.x + iconSize * 0.7f, sPos.y + iconSize * 0.6f };

            Color iconCol = isSel ? WHITE : (isSecured ? ui::Colors::Green400 : (isHostile ? Color{ 255, 176, 0, 255 } : Color{ 42, 42, 48, 255 }));

            if (isSecured || isHostile || isSel) {
                // Outpost house/base icon
                DrawLineEx(pLeft, pRoof, 1.8f, iconCol);
                DrawLineEx(pRoof, pRight, 1.8f, iconCol);
                DrawLineEx(pRight, pBotRight, 1.8f, iconCol);
                DrawLineEx(pBotRight, pBotLeft, 1.8f, iconCol);
                DrawLineEx(pBotLeft, pLeft, 1.8f, iconCol);
            } else {
                // Padlock icon for locked dark charcoal fortress
                DrawRectangle(static_cast<int>(sPos.x - 5), static_cast<int>(sPos.y - 3), 10, 8, Color{ 42, 42, 48, 255 });
                DrawRectangleLines(static_cast<int>(sPos.x - 5), static_cast<int>(sPos.y - 3), 10, 8, ui::Colors::Zinc400);
                DrawCircleLines(static_cast<int>(sPos.x), static_cast<int>(sPos.y - 4), 3, ui::Colors::Zinc400);
            }
        }

        // Floating Tactical Callout Badge
        if (isSel || isHov) {
            std::string labelText = sec.codename;
            int textW = MeasureText(labelText.c_str(), 12);
            float badgeW = static_cast<float>(textW) + 14.0f;
            float badgeH = 20.0f;
            float badgeX = sPos.x - badgeW * 0.5f;
            float badgeY = sPos.y + 22.0f;

            Color badgeCol = isSel ? Color{ 244, 63, 94, 255 } : WHITE;

            DrawRectangleRec({ badgeX, badgeY, badgeW, badgeH }, ui::Colors::Zinc950);
            DrawRectangleLinesEx({ badgeX, badgeY, badgeW, badgeH }, 1.0f, badgeCol);
            DrawText(labelText.c_str(), static_cast<int>(badgeX + 7.0f), static_cast<int>(badgeY + 4.0f), 12, badgeCol);
        }
    }
}

} // namespace minesweeper::render
