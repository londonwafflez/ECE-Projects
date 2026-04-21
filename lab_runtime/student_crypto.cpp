#include "student_crypto.hpp"

#include <cstddef>
#include <iostream>
#include <cmath>
#include <inttypes.h>
using namespace std;

namespace {
    int normalizeShift(int shift) {
        int result = shift % 26;
        if (result < 0) {
            result += 26;
        }
        return result;
    }
}  // namespace
// BEGIN STUDENT CODE

// int main() {
    // printf("Modexp: %d", modExp(79,17,3233));

    // cout << encryptCaesar("abc xy35./z", -28) << endl;
    // cout << decryptCaesar(encryptCaesar("abc xy35./z", -5), -5) << endl;
    // for (uint32_t i : encryptRSA("hi", 3, 391)) {printf("%d, ", i);}; 
    // for (uint8_t i : encryptLFSR("abc", 0xA7)) {printf("%X, ", i);}; 
    // cout << endl;
    // for (uint8_t i : decryptLFSR(vector<uint8_t>{0xC6, 0x96, 0xED}, 0xA7)) {printf("%c, ", i);}; 
    // cout << endl;
    // cout << ("Decrypt RSA: " + decryptRSA(vector<uint32_t>{348, 265}, 3, 391)) << endl; 
    // printf("Step LFSR %x\n", stepLFSR(0xA7)); // 0x53
    // 1010 0111 -> 0101 0011
    //   A7      -> 53
    // d3 = 1101 0011
    // if (0x11 & 16) cout << endl;
    
//     return 0;
// }

uint32_t modExp(uint32_t base, uint32_t exp, uint32_t mod) {
    uint64_t out = 1;
    if (exp & 1) out = base;
    cout << out << endl;
    for (int i = 1; i < 32; i++) {
        if (exp & static_cast<uint32_t>(pow(2, i))) {
            for (int j = 0; j < pow(2, i); j++) {
                out = base % mod * out %  mod;
            }
        }
    }
    return out;
}

uint8_t stepLFSR(uint8_t state) {
    int tap = (state ^ state >> 1) & 0x01;
    state = state >> 1;
    if (tap) state += 0x80;
    return state;
}

String encryptCaesar(const String& plaintext, int shift) {
    vector<char> encrypted;
    char newC; 

    shift %= 26; 
    for (char c : plaintext) {
        if (c < 0x61 || c > 0x7A) {
            encrypted.push_back(c);
            continue;
        }
        newC = c + shift;
        if (newC < 0x61) newC += 0x1A;
        if (newC > 0x7A) newC -= 0x1A;
        encrypted.push_back(newC);
    }
    return String(encrypted.begin(), encrypted.end());
}

String decryptCaesar(const String& ciphertext, int shift) {
    vector<char> decrypted;
    char newC; 

    shift %= 26; 
    for (char c : ciphertext) {
        if (c < 0x61 || c > 0x7A) {
            decrypted.push_back(c);
            continue;
        }
        newC = c - shift;
        if (newC < 0x61) newC += 0x1A;
        if (newC > 0x7A) newC -= 0x1A;
        decrypted.push_back(newC);
    }
    return String(decrypted.begin(), decrypted.end());
}

std::vector<uint8_t> encryptLFSR(const String& plaintext, uint8_t seed) {
    std::vector<uint8_t> out;
    out.reserve(plaintext.length()); // Gives you enough space to store the output
    for (size_t i = 0; i < plaintext.length(); i++) {
        out.push_back(static_cast<uint8_t>(static_cast<unsigned char>(plaintext[i])) ^ seed);
        for (int i = 0; i < 8; i++) {
            seed = stepLFSR(seed);
        }
    }
    return out;
}

String decryptLFSR(const std::vector<uint8_t>& ciphertext, uint8_t seed) {
    // TODO: Use the ciphertext byte list to recover the plaintext.
    String out;
    out.reserve(ciphertext.size()); // Gives you enough space to store the output
    for (uint8_t byte : ciphertext) {
        out += static_cast<char>(byte ^ seed);
        for (int i = 0; i < 8; i++) {
            seed = stepLFSR(seed);
        }
    }
    return out;
}

std::vector<uint32_t> encryptRSA(const String& plaintext, uint32_t e, uint32_t n) {
    std::vector<uint32_t> out;
    out.reserve(plaintext.length()); // Gives you enough space to store the output
    for (size_t i = 0; i < plaintext.length(); i++) {
        out.push_back(modExp(plaintext[i], e, n));
    }
    return out;
}

String decryptRSA(const std::vector<uint32_t>& ciphertext, uint32_t d, uint32_t n) {
    String out;
    out.reserve(ciphertext.size()); // Gives you enough space to store the output
    for (uint32_t block : ciphertext) {
        printf("out: %x, block: %d\n", modExp(block, d, n), block);
        // printf("out: %s", static_cast<char>(modExp(block, d, n)));
        out += static_cast<char>(modExp(block, d, n));
    }
    return out;
}

// END STUDENT CODE
