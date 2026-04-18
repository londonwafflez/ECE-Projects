#pragma once

#include <cctype>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

enum class CryptoAlgorithm {
    kCaesar,
    kLfsr,
    kRsa,
    kUnknown,
};

inline std::string algorithmToString(CryptoAlgorithm algorithm) {
    switch (algorithm) {
        case CryptoAlgorithm::kCaesar:
            return "CAESAR";
        case CryptoAlgorithm::kLfsr:
            return "LFSR";
        case CryptoAlgorithm::kRsa:
            return "RSA";
        default:
            return "UNKNOWN";
    }
}

inline CryptoAlgorithm algorithmFromString(const std::string& value) {
    if (value == "CAESAR") return CryptoAlgorithm::kCaesar;
    if (value == "LFSR") return CryptoAlgorithm::kLfsr;
    if (value == "RSA") return CryptoAlgorithm::kRsa;
    return CryptoAlgorithm::kUnknown;
}

inline std::string hexEncode(const std::vector<uint8_t>& bytes) {
    static const char* kHex = "0123456789ABCDEF";
    std::string out;
    out.reserve(bytes.size() * 2);
    for (uint8_t byte : bytes) {
        out.push_back(kHex[(byte >> 4) & 0x0F]);
        out.push_back(kHex[byte & 0x0F]);
    }
    return out;
}

inline uint8_t hexValue(char c) {
    if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    if (c >= 'A' && c <= 'F') return static_cast<uint8_t>(10 + c - 'A');
    throw std::runtime_error("Invalid hex digit");
}

inline std::vector<uint8_t> hexDecode(const std::string& hex) {
    if (hex.size() % 2 != 0) {
        throw std::runtime_error("Hex string must have even length");
    }
    std::vector<uint8_t> out;
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        uint8_t high = hexValue(hex[i]);
        uint8_t low = hexValue(hex[i + 1]);
        out.push_back(static_cast<uint8_t>((high << 4) | low));
    }
    return out;
}

inline std::string csvEncode(const std::vector<uint32_t>& values) {
    std::ostringstream out;
    for (size_t i = 0; i < values.size(); ++i) {
        if (i != 0) out << ',';
        out << values[i];
    }
    return out.str();
}

inline std::vector<uint32_t> csvDecodeU32(const std::string& csv) {
    std::vector<uint32_t> out;
    if (csv.empty()) {
        return out;
    }
    std::stringstream ss(csv);
    std::string item;
    while (std::getline(ss, item, ',')) {
        if (item.empty()) {
            continue;
        }
        out.push_back(static_cast<uint32_t>(std::stoul(item)));
    }
    return out;
}

inline std::string escapeJson(const std::string& input) {
    std::string out;
    out.reserve(input.size() + 8);
    for (char c : input) {
        switch (c) {
            case '\\':
                out += "\\\\";
                break;
            case '"':
                out += "\\\"";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                out.push_back(c);
                break;
        }
    }
    return out;
}

inline std::string extractJsonString(const std::string& json, const std::string& key) {
    const std::string needle = "\"" + key + "\":\"";
    size_t start = json.find(needle);
    if (start == std::string::npos) {
        return "";
    }
    start += needle.size();
    std::string out;
    bool escaped = false;
    for (size_t i = start; i < json.size(); ++i) {
        char c = json[i];
        if (escaped) {
            switch (c) {
                case 'n': out.push_back('\n'); break;
                case 'r': out.push_back('\r'); break;
                case 't': out.push_back('\t'); break;
                default: out.push_back(c); break;
            }
            escaped = false;
            continue;
        }
        if (c == '\\') {
            escaped = true;
            continue;
        }
        if (c == '"') {
            return out;
        }
        out.push_back(c);
    }
    return "";
}

inline int extractJsonInt(const std::string& json, const std::string& key, int fallback = 0) {
    const std::string needle = "\"" + key + "\":";
    size_t start = json.find(needle);
    if (start == std::string::npos) {
        return fallback;
    }
    start += needle.size();
    size_t end = start;
    while (end < json.size() && (json[end] == '-' || std::isdigit(static_cast<unsigned char>(json[end])))) {
        ++end;
    }
    if (end == start) {
        return fallback;
    }
    return std::stoi(json.substr(start, end - start));
}

inline bool extractJsonBool(const std::string& json, const std::string& key, bool fallback = false) {
    const std::string needle = "\"" + key + "\":";
    size_t start = json.find(needle);
    if (start == std::string::npos) {
        return fallback;
    }
    start += needle.size();
    if (json.compare(start, 4, "true") == 0) return true;
    if (json.compare(start, 5, "false") == 0) return false;
    return fallback;
}
