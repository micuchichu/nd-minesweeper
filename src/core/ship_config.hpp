#pragma once

#include "raylib.h"
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

enum class JsonType {
    Null,
    Bool,
    Number,
    String,
    Array,
    Object
};

struct JsonValue {
    JsonType type = JsonType::Null;
    bool boolVal = false;
    double numVal = 0.0;
    std::string strVal;
    std::vector<JsonValue> arrVal;
    std::unordered_map<std::string, JsonValue> objVal;

    bool isNull() const { return type == JsonType::Null; }
    bool isBool() const { return type == JsonType::Bool; }
    bool isNumber() const { return type == JsonType::Number; }
    bool isString() const { return type == JsonType::String; }
    bool isArray() const { return type == JsonType::Array; }
    bool isObject() const { return type == JsonType::Object; }

    double asNumber(double def = 0.0) const { return isNumber() ? numVal : def; }
    float asFloat(float def = 0.0f) const { return isNumber() ? static_cast<float>(numVal) : def; }
    int asInt(int def = 0) const { return isNumber() ? static_cast<int>(numVal) : def; }
    std::string asString(const std::string& def = "") const { return isString() ? strVal : def; }
    bool asBool(bool def = false) const { return isBool() ? boolVal : def; }

    bool contains(const std::string& key) const {
        if (!isObject()) return false;
        return objVal.find(key) != objVal.end();
    }

    const JsonValue& operator[](const std::string& key) const {
        static const JsonValue nullVal;
        if (!isObject()) return nullVal;
        auto it = objVal.find(key);
        return (it != objVal.end()) ? it->second : nullVal;
    }

    const JsonValue& operator[](size_t idx) const {
        static const JsonValue nullVal;
        if (!isArray() || idx >= arrVal.size()) return nullVal;
        return arrVal[idx];
    }
};

class SimpleJsonParser {
public:
    static bool parse(const std::string& text, JsonValue& outRoot) {
        SimpleJsonParser parser(text);
        return parser.parseValue(outRoot);
    }

private:
    std::string_view src;
    size_t pos = 0;

    explicit SimpleJsonParser(const std::string& text) : src(text), pos(0) {}

    void skipWhitespace() {
        while (pos < src.size()) {
            char c = src[pos];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                ++pos;
            } else if (c == '/' && pos + 1 < src.size() && src[pos + 1] == '/') {
                pos += 2;
                while (pos < src.size() && src[pos] != '\n' && src[pos] != '\r') {
                    ++pos;
                }
            } else if (c == '/' && pos + 1 < src.size() && src[pos + 1] == '*') {
                pos += 2;
                while (pos + 1 < src.size() && !(src[pos] == '*' && src[pos + 1] == '/')) {
                    ++pos;
                }
                if (pos + 1 < src.size()) pos += 2;
            } else {
                break;
            }
        }
    }

    bool parseValue(JsonValue& out) {
        skipWhitespace();
        if (pos >= src.size()) return false;

        char c = src[pos];
        if (c == '{') return parseObject(out);
        if (c == '[') return parseArray(out);
        if (c == '"') {
            out.type = JsonType::String;
            return parseString(out.strVal);
        }
        if (c == 't' || c == 'f') return parseBool(out);
        if (c == 'n') return parseNull(out);
        if (c == '-' || (c >= '0' && c <= '9')) return parseNumber(out);

        return false;
    }

    bool parseObject(JsonValue& out) {
        out.type = JsonType::Object;
        out.objVal.clear();
        ++pos; // skip '{'

        while (pos < src.size()) {
            skipWhitespace();
            if (pos >= src.size()) return false;
            if (src[pos] == '}') {
                ++pos;
                return true;
            }

            if (src[pos] != '"') return false;
            std::string key;
            if (!parseString(key)) return false;

            skipWhitespace();
            if (pos >= src.size() || src[pos] != ':') return false;
            ++pos; // skip ':'

            JsonValue val;
            if (!parseValue(val)) return false;
            out.objVal[key] = std::move(val);

            skipWhitespace();
            if (pos >= src.size()) return false;
            if (src[pos] == ',') {
                ++pos;
                skipWhitespace();
                if (pos < src.size() && src[pos] == '}') {
                    ++pos; // trailing comma allowed
                    return true;
                }
            } else if (src[pos] == '}') {
                ++pos;
                return true;
            } else {
                return false;
            }
        }
        return false;
    }

    bool parseArray(JsonValue& out) {
        out.type = JsonType::Array;
        out.arrVal.clear();
        ++pos; // skip '['

        while (pos < src.size()) {
            skipWhitespace();
            if (pos >= src.size()) return false;
            if (src[pos] == ']') {
                ++pos;
                return true;
            }

            JsonValue elem;
            if (!parseValue(elem)) return false;
            out.arrVal.push_back(std::move(elem));

            skipWhitespace();
            if (pos >= src.size()) return false;
            if (src[pos] == ',') {
                ++pos;
                skipWhitespace();
                if (pos < src.size() && src[pos] == ']') {
                    ++pos; // trailing comma allowed
                    return true;
                }
            } else if (src[pos] == ']') {
                ++pos;
                return true;
            } else {
                return false;
            }
        }
        return false;
    }

    bool parseString(std::string& outStr) {
        outStr.clear();
        if (pos >= src.size() || src[pos] != '"') return false;
        ++pos; // skip opening quote

        while (pos < src.size()) {
            char c = src[pos++];
            if (c == '"') {
                return true;
            }
            if (c == '\\') {
                if (pos >= src.size()) return false;
                char esc = src[pos++];
                switch (esc) {
                    case '"':  outStr.push_back('"'); break;
                    case '\\': outStr.push_back('\\'); break;
                    case '/':  outStr.push_back('/'); break;
                    case 'b':  outStr.push_back('\b'); break;
                    case 'f':  outStr.push_back('\f'); break;
                    case 'n':  outStr.push_back('\n'); break;
                    case 'r':  outStr.push_back('\r'); break;
                    case 't':  outStr.push_back('\t'); break;
                    default:   outStr.push_back(esc); break;
                }
            } else {
                outStr.push_back(c);
            }
        }
        return false;
    }

    bool parseNumber(JsonValue& out) {
        size_t start = pos;
        if (pos < src.size() && (src[pos] == '-' || src[pos] == '+')) ++pos;
        while (pos < src.size() && (src[pos] >= '0' && src[pos] <= '9')) ++pos;
        if (pos < src.size() && src[pos] == '.') {
            ++pos;
            while (pos < src.size() && (src[pos] >= '0' && src[pos] <= '9')) ++pos;
        }
        if (pos < src.size() && (src[pos] == 'e' || src[pos] == 'E')) {
            ++pos;
            if (pos < src.size() && (src[pos] == '+' || src[pos] == '-')) ++pos;
            while (pos < src.size() && (src[pos] >= '0' && src[pos] <= '9')) ++pos;
        }

        std::string numStr(src.substr(start, pos - start));
        try {
            out.numVal = std::stod(numStr);
            out.type = JsonType::Number;
            return true;
        } catch (...) {
            return false;
        }
    }

    bool parseBool(JsonValue& out) {
        if (src.substr(pos, 4) == "true") {
            pos += 4;
            out.type = JsonType::Bool;
            out.boolVal = true;
            return true;
        }
        if (src.substr(pos, 5) == "false") {
            pos += 5;
            out.type = JsonType::Bool;
            out.boolVal = false;
            return true;
        }
        return false;
    }

    bool parseNull(JsonValue& out) {
        if (src.substr(pos, 4) == "null") {
            pos += 4;
            out.type = JsonType::Null;
            return true;
        }
        return false;
    }
};

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
    std::vector<ShipThruster> thrusters;

    static ShipConfig createDefault(int texW, int texH, const std::string& defaultName = "SHOP") {
        ShipConfig cfg;
        cfg.name = defaultName;
        cfg.scale = 1.8f;
        cfg.thrusterColor = { 255, 179, 0, 255 };
        cfg.bumpable = false;

        if (texW == 64 && texH == 32) {
            cfg.mass = 8.0f;
            cfg.speed = 140.0f;
            cfg.range = 160.0f;
            cfg.collisionRadius = 26.0f;
            cfg.capsuleLength = 0.0f;
            cfg.thrusters.push_back({ { -25.0f, -11.0f }, { -1.0f, 0.0f }, 3.6f, 6.0f, cfg.thrusterColor, cfg.thrusterColor });
            cfg.thrusters.push_back({ { -27.0f,   0.0f }, { -1.0f, 0.0f }, 4.4f, 8.0f, cfg.thrusterColor, cfg.thrusterColor });
            cfg.thrusters.push_back({ { -25.0f,  11.0f }, { -1.0f, 0.0f }, 3.6f, 6.0f, cfg.thrusterColor, cfg.thrusterColor });
        } else if (texW == 128 && texH == 48) {
            cfg.mass = 12.0f;
            cfg.speed = 130.0f;
            cfg.range = 160.0f;
            cfg.collisionRadius = 28.0f;
            cfg.capsuleLength = 130.0f;
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
