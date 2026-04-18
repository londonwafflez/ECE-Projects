#pragma once

#include <cstdint>
#include <vector>

#include "string_compat.hpp"

struct CaesarVector {
    String input;
    int shift;
    String encrypted;
};

struct LfsrVector {
    String input;
    uint8_t seed;
    std::vector<uint8_t> encrypted;
};

struct RsaVector {
    String input;
    uint32_t e;
    uint32_t d;
    uint32_t n;
    std::vector<uint32_t> encrypted;
};

inline const std::vector<CaesarVector> kCaesarVectors = {
    {"abc xyz", 3, "def abc"},
    {"attack at dawn", 5, "fyyfhp fy ifbs"},
    {"hello, world!", -3, "ebiil, tloia!"},
};

inline const std::vector<LfsrVector> kLfsrVectors = {
    {"abc", 0xA7, {0xC6, 0x96, 0xED}},
    {"hello", 0xA7, {0xCF, 0x91, 0xE2, 0x25, 0x82}},
};

inline const std::vector<RsaVector> kRsaVectors = {
    {"hi", 3, 235, 391, {348, 265}},
    {"OK", 17, 2753, 3233, {1307, 597}},
};
