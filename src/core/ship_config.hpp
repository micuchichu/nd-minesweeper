#pragma once

#include "raylib.h"
#include "json_parser.hpp"
#include "ship.hpp"
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cmath>

namespace minesweeper::core {

inline Color parseColorValue(const JsonValue& val, Color defColor = { 255, 179, 0, 255 }) {
    if (val.isArray()) {
        unsigned char r = static_cast<unsigned char>(std::clamp(val[0].asInt(defColor.r), 0, 255));
        unsigned char g = static_cast<unsigned char>(std::clamp(val[1].asInt(defColor.g), 0, 255));
        unsigned char b = static_cast<unsigned char>(std::clamp(val[2].asInt(defColor.b), 0, 255));
        unsigned char a = (val.arrVal.size() >= 4) ? static_cast<unsigned char>(std::clamp(val[3].asInt(255), 0, 255)) : 255;
        return Color{ r, g, b, a };
    }
    if (val.isObject()) {
        unsigned char r = static_cast<unsigned char>(std::clamp(val["r"].asInt(defColor.r), 0, 255));
        unsigned char g = static_cast<unsigned char>(std::clamp(val["g"].asInt(defColor.g), 0, 255));
        unsigned char b = static_cast<unsigned char>(std::clamp(val["b"].asInt(defColor.b), 0, 255));
        unsigned char a = val.contains("a") ? static_cast<unsigned char>(std::clamp(val["a"].asInt(255), 0, 255)) : 255;
        return Color{ r, g, b, a };
    }
    if (val.isString()) {
        std::string s = val.asString();
        if (!s.empty() && s[0] == '#') s = s.substr(1);
        if (s.length() == 6 || s.length() == 8) {
            unsigned int hexVal = 0;
            std::stringstream ss;
            ss << std::hex << s;
            ss >> hexVal;
            if (s.length() == 6) {
                return Color{
                    static_cast<unsigned char>((hexVal >> 16) & 0xFF),
                    static_cast<unsigned char>((hexVal >> 8) & 0xFF),
                    static_cast<unsigned char>(hexVal & 0xFF),
                    255
                };
            } else {
                return Color{
                    static_cast<unsigned char>((hexVal >> 24) & 0xFF),
                    static_cast<unsigned char>((hexVal >> 16) & 0xFF),
                    static_cast<unsigned char>((hexVal >> 8) & 0xFF),
                    static_cast<unsigned char>(hexVal & 0xFF)
                };
            }
        }
    }
    return defColor;
}

// ============================================================================
// Ship Configuration Data
// ============================================================================
struct ShipConfig {
    std::string name = "SHOP";
    float scale = 1.8f;
    float mass = 8.0f;
    float speed = 140.0f;
    float range = 160.0f;
    float collisionRadius = 26.0f;
    float capsuleLength = 0.0f; // Length of focal line segment for capsule collisions (0 for circular ships)
    Color thrusterColor = { 255, 179, 0, 255 }; // Unified thruster color
    bool bumpable = false; // Pushable, but not bumpable for shop freighters
    int shopTier = 1;      // Shop tier (e.g. 1 for small shop, 2 for big shop)
    int itemCapacity = 2;  // Item capacity (e.g. 2 for mini shop, 4 for big shop)
    std::vector<ShipThruster> thrusters;

    static ShipConfig createDefault(int texW, int texH, const std::string& defaultName = "SHOP") {
        ShipConfig cfg;
        cfg.name = defaultName;
        cfg.scale = 1.8f;
        cfg.thrusterColor = { 255, 179, 0, 255 };
        cfg.bumpable = false;
        cfg.shopTier = 1;
        cfg.itemCapacity = 2;

        if (texW == 64 && texH == 32) {
            cfg.mass = 8.0f;
            cfg.speed = 140.0f;
            cfg.range = 160.0f;
            cfg.collisionRadius = 26.0f;
            cfg.capsuleLength = 0.0f;
            cfg.shopTier = 1;
            cfg.itemCapacity = 2;
            cfg.thrusters.push_back({ { -25.0f, -11.0f }, { -1.0f, 0.0f }, 3.6f, 6.0f, cfg.thrusterColor, cfg.thrusterColor });
            cfg.thrusters.push_back({ { -27.0f,   0.0f }, { -1.0f, 0.0f }, 4.4f, 8.0f, cfg.thrusterColor, cfg.thrusterColor });
            cfg.thrusters.push_back({ { -25.0f,  11.0f }, { -1.0f, 0.0f }, 3.6f, 6.0f, cfg.thrusterColor, cfg.thrusterColor });
        } else if (texW == 128 && texH == 48) {
            cfg.mass = 12.0f;
            cfg.speed = 130.0f;
            cfg.range = 160.0f;
            cfg.collisionRadius = 28.0f;
            cfg.capsuleLength = 130.0f;
            cfg.shopTier = 2;
            cfg.itemCapacity = 4;
            cfg.thrusters.push_back({ { -48.0f, -14.0f }, { -1.0f, 0.0f }, 4.8f, 7.0f, cfg.thrusterColor, cfg.thrusterColor });
            cfg.thrusters.push_back({ { -50.0f,   0.0f }, { -1.0f, 0.0f }, 6.4f, 10.0f, cfg.thrusterColor, cfg.thrusterColor });
            cfg.thrusters.push_back({ { -48.0f,  14.0f }, { -1.0f, 0.0f }, 4.8f, 7.0f, cfg.thrusterColor, cfg.thrusterColor });
        } else if (texW > 0 && texH > 0) {
            float w = static_cast<float>(texW);
            float h = static_cast<float>(texH);
            float halfW = w * 0.5f;
            float halfH = h * 0.5f;
            cfg.mass = std::max(6.0f, (w * h) / 300.0f);
            cfg.speed = 140.0f;
            cfg.range = 160.0f;
            cfg.collisionRadius = std::max(20.0f, std::min(halfW, halfH) * 1.3f);
            cfg.capsuleLength = (w > h * 1.8f) ? std::max(0.0f, (w - h) * cfg.scale) : 0.0f;
            cfg.thrusters.push_back({ { -halfW * 0.82f, -halfH * 0.60f }, { -1.0f, 0.0f }, 3.6f, 6.0f, cfg.thrusterColor, cfg.thrusterColor });
            cfg.thrusters.push_back({ { -halfW * 0.86f,   0.0f         }, { -1.0f, 0.0f }, 4.8f, 8.0f, cfg.thrusterColor, cfg.thrusterColor });
            cfg.thrusters.push_back({ { -halfW * 0.82f,  halfH * 0.60f }, { -1.0f, 0.0f }, 3.6f, 6.0f, cfg.thrusterColor, cfg.thrusterColor });
        } else {
            cfg.mass = 8.0f;
            cfg.speed = 140.0f;
            cfg.range = 160.0f;
            cfg.collisionRadius = 26.0f;
            cfg.capsuleLength = 0.0f;
            cfg.thrusters.push_back({ { -25.0f, 0.0f }, { -1.0f, 0.0f }, 3.0f, 6.0f, cfg.thrusterColor, cfg.thrusterColor });
        }

        // Apply unified inner core color
        for (auto& th : cfg.thrusters) {
            th.outerColor = cfg.thrusterColor;
            th.innerColor = Color{
                static_cast<unsigned char>(std::min(255, cfg.thrusterColor.r + 50)),
                static_cast<unsigned char>(std::min(255, cfg.thrusterColor.g + 50)),
                static_cast<unsigned char>(std::min(255, cfg.thrusterColor.b + 50)),
                255
            };
        }
        return cfg;
    }

    static bool parse(const std::string& jsonText, ShipConfig& outConfig) {
        JsonValue root;
        if (!SimpleJsonParser::parse(jsonText, root) || !root.isObject()) {
            return false;
        }

        if (root.contains("name")) outConfig.name = root["name"].asString(outConfig.name);
        if (root.contains("scale")) outConfig.scale = root["scale"].asFloat(outConfig.scale);
        else if (root.contains("shipScale")) outConfig.scale = root["shipScale"].asFloat(outConfig.scale);

        if (root.contains("mass")) outConfig.mass = root["mass"].asFloat(outConfig.mass);
        if (root.contains("speed")) outConfig.speed = root["speed"].asFloat(outConfig.speed);
        else if (root.contains("maxSpeed")) outConfig.speed = root["maxSpeed"].asFloat(outConfig.speed);

        if (root.contains("range")) outConfig.range = root["range"].asFloat(outConfig.range);
        if (root.contains("collisionRadius")) outConfig.collisionRadius = root["collisionRadius"].asFloat(outConfig.collisionRadius);
        else if (root.contains("collision_radius")) outConfig.collisionRadius = root["collision_radius"].asFloat(outConfig.collisionRadius);
        else if (root.contains("radius")) outConfig.collisionRadius = root["radius"].asFloat(outConfig.collisionRadius);

        if (root.contains("capsuleLength")) outConfig.capsuleLength = root["capsuleLength"].asFloat(outConfig.capsuleLength);
        else if (root.contains("capsule_length")) outConfig.capsuleLength = root["capsule_length"].asFloat(outConfig.capsuleLength);
        else if (root.contains("capsule")) outConfig.capsuleLength = root["capsule"].asFloat(outConfig.capsuleLength);

        if (root.contains("thrusterColor")) {
            outConfig.thrusterColor = parseColorValue(root["thrusterColor"], outConfig.thrusterColor);
        } else if (root.contains("thruster_color")) {
            outConfig.thrusterColor = parseColorValue(root["thruster_color"], outConfig.thrusterColor);
        } else if (root.contains("color")) {
            outConfig.thrusterColor = parseColorValue(root["color"], outConfig.thrusterColor);
        }

        if (root.contains("bumpable")) {
            outConfig.bumpable = root["bumpable"].asBool(outConfig.bumpable);
        } else if (root.contains("isBumpable")) {
            outConfig.bumpable = root["isBumpable"].asBool(outConfig.bumpable);
        } else if (root.contains("canBump")) {
            outConfig.bumpable = root["canBump"].asBool(outConfig.bumpable);
        }

        if (root.contains("tier")) {
            outConfig.shopTier = root["tier"].asInt(outConfig.shopTier);
        } else if (root.contains("shopTier")) {
            outConfig.shopTier = root["shopTier"].asInt(outConfig.shopTier);
        }

        if (root.contains("itemCapacity")) {
            outConfig.itemCapacity = root["itemCapacity"].asInt(outConfig.itemCapacity);
        } else if (root.contains("capacity")) {
            outConfig.itemCapacity = root["capacity"].asInt(outConfig.itemCapacity);
        }

        if (root.contains("thrusters") && root["thrusters"].isArray()) {
            outConfig.thrusters.clear();
            const auto& thList = root["thrusters"].arrVal;
            for (const auto& item : thList) {
                if (!item.isObject()) continue;

                Vector2 offset = { 0.0f, 0.0f };
                if (item.contains("x") && item.contains("y")) {
                    offset.x = item["x"].asFloat(0.0f);
                    offset.y = item["y"].asFloat(0.0f);
                } else if (item.contains("offset") && item["offset"].isArray()) {
                    offset.x = item["offset"][0].asFloat(0.0f);
                    offset.y = item["offset"][1].asFloat(0.0f);
                } else if (item.contains("position") && item["position"].isArray()) {
                    offset.x = item["position"][0].asFloat(0.0f);
                    offset.y = item["position"][1].asFloat(0.0f);
                }

                Vector2 direction = { -1.0f, 0.0f };
                if (item.contains("direction") && item["direction"].isArray()) {
                    direction.x = item["direction"][0].asFloat(-1.0f);
                    direction.y = item["direction"][1].asFloat(0.0f);
                } else if (item.contains("dir") && item["dir"].isArray()) {
                    direction.x = item["dir"][0].asFloat(-1.0f);
                    direction.y = item["dir"][1].asFloat(0.0f);
                }

                float nozzleSize = 3.0f;
                if (item.contains("size")) nozzleSize = item["size"].asFloat(3.0f);
                else if (item.contains("nozzleWidth")) nozzleSize = item["nozzleWidth"].asFloat(3.0f);
                else if (item.contains("width")) nozzleSize = item["width"].asFloat(3.0f);
                else if (item.contains("radius")) nozzleSize = item["radius"].asFloat(3.0f);

                float flameLength = 6.0f;
                if (item.contains("flameLength")) flameLength = item["flameLength"].asFloat(6.0f);
                else if (item.contains("length")) flameLength = item["length"].asFloat(6.0f);

                Color outerCol = outConfig.thrusterColor;
                Color innerCol = Color{
                    static_cast<unsigned char>(std::min(255, outerCol.r + 50)),
                    static_cast<unsigned char>(std::min(255, outerCol.g + 50)),
                    static_cast<unsigned char>(std::min(255, outerCol.b + 50)),
                    255
                };

                outConfig.thrusters.push_back({ offset, direction, nozzleSize, flameLength, outerCol, innerCol });
            }
        }

        return true;
    }

    static bool loadFromFile(const std::string& filepath, ShipConfig& outConfig) {
        std::ifstream file(filepath);
        if (!file.is_open()) return false;
        std::stringstream buffer;
        buffer << file.rdbuf();
        return parse(buffer.str(), outConfig);
    }
};

} // namespace minesweeper::core
