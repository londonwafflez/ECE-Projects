#pragma once

#include <cstdint>
#include <vector>

#include "string_compat.hpp"

// ================================
// Student crypto contract
// ================================
//
// These are the ONLY functions students should edit/implement.
// The rest of the firmware, networking, and protocol code should be treated as read-only.
//
// Function rules:
// 1. Use the provided String type for text inputs/outputs.
// 2. Use std::vector when you need a growable array of bytes or RSA ciphertext
//    blocks. In practice you should mainly need push_back(...), size(), and
//    vec[i].
// 3. Keep your code portable by sticking to basic String operations that also work
//    in the local C++ tests (for example: length(), operator[], +=, and c_str()).
// 4. Caesar cipher:
//    - Shift lowercase letters 'a' through 'z'.
//    - Preserve all other characters unchanged.
//    - Shifts may be larger than 26 or negative.
// 5. LFSR:
//    - Use an 8-bit LFSR.
//    - Taps are Q1 and Q0, i.e. bits 1 and 0 of the CURRENT state.
//    - stepLFSR(state) computes the NEXT state.
//    - Shift direction is right.
//    - New bit inserted into bit 7 is (bit1 XOR bit0).
//    - During encryption/decryption, each keystream byte is generated from 8 output bits.
//    - The output bit each step is the current least-significant bit BEFORE the shift.
//    - Pack those 8 output bits into a byte least-significant-bit first.
//    - Encrypt/decrypt by XORing each plaintext/ciphertext byte with the next keystream byte.
// 6. RSA:
//    - Encrypt/decrypt each byte independently.
//    - encryptRSA returns one ciphertext block per plaintext byte.
//    - decryptRSA returns one plaintext byte per ciphertext block.
//    - Assume the provided modulus n is > 255.
//
// Public vector examples:
// - stepLFSR(0xA7) == 0x53
// - encryptCaesar("abc xyz", 3) == "def abc"
// - encryptLFSR("abc", 0xA7) == {0xC6, 0x96, 0xED}
// - encryptRSA("hi", 3, 391) == {348, 265}

uint32_t modExp(uint32_t base, uint32_t exp, uint32_t mod);
uint8_t stepLFSR(uint8_t state);

String encryptCaesar(const String& plaintext, int shift);
String decryptCaesar(const String& ciphertext, int shift);

std::vector<uint8_t> encryptLFSR(const String& plaintext, uint8_t seed);
String decryptLFSR(const std::vector<uint8_t>& ciphertext, uint8_t seed);

std::vector<uint32_t> encryptRSA(const String& plaintext, uint32_t e, uint32_t n);
String decryptRSA(const std::vector<uint32_t>& ciphertext, uint32_t d, uint32_t n);
