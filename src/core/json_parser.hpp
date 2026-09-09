#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include <cctype>

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

} // namespace minesweeper::core
