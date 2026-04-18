#include "student_crypto.hpp"

#include <cstddef>
#include <iostream>
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

int main() {
    cout << encryptCaesar("abc xyz", 3) << endl;
    cout << decryptCaesar(encryptCaesar("abc xyz", 3), 3) << endl;
    return 0;
}

uint32_t modExp(uint32_t base, uint32_t exp, uint32_t mod) {
    // TODO: Replace this with fast modular exponentiation.
    
    (void)base;
    (void)exp;
    return mod == 0 ? 0u : 1u % mod;
}

uint8_t stepLFSR(uint8_t state) {
    // TODO: Replace this with the correct 8-bit LFSR step.
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
    return String (encrypted.begin(), encrypted.end());
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
    return String (decrypted.begin(), decrypted.end());
}

std::vector<uint8_t> encryptLFSR(const String& plaintext, uint8_t seed) {
    // TODO: Build and return the ciphertext byte list.
    std::vector<uint8_t> out;
    out.reserve(plaintext.length()); // Gives you enough space to store the output
    for (size_t i = 0; i < plaintext.length(); ++i) {
        out.push_back(static_cast<uint8_t>(static_cast<unsigned char>(plaintext[i])));
    }
    (void)seed;
    return out;
}

String decryptLFSR(const std::vector<uint8_t>& ciphertext, uint8_t seed) {
    // TODO: Use the ciphertext byte list to recover the plaintext.
    String out;
    out.reserve(ciphertext.size()); // Gives you enough space to store the output
    for (uint8_t byte : ciphertext) {
        out += static_cast<char>(byte);
    }
    (void)seed;
    return out;
}

std::vector<uint32_t> encryptRSA(const String& plaintext, uint32_t e, uint32_t n) {
    // TODO: Build and return the RSA ciphertext block list.
    std::vector<uint32_t> out;
    out.reserve(plaintext.length()); // Gives you enough space to store the output
    for (size_t i = 0; i < plaintext.length(); ++i) {
        out.push_back(static_cast<uint32_t>(static_cast<unsigned char>(plaintext[i])));
    }
    (void)e;
    (void)n;
    return out;
}

String decryptRSA(const std::vector<uint32_t>& ciphertext, uint32_t d, uint32_t n) {
    // TODO: Use the ciphertext block list to recover the plaintext.
    String out;
    out.reserve(ciphertext.size()); // Gives you enough space to store the output
    for (uint32_t block : ciphertext) {
        out += static_cast<char>(block);
    }
    (void)d;
    (void)n;
    return out;
}

// END STUDENT CODE
